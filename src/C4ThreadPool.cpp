/*
 * LegacyClonk
 *
 * Copyright (c) 2023, The LegacyClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */

#include "C4ThreadPool.h"

#ifdef _WIN32
#include <format>
#include <limits>

#include <threadpoolapiset.h>
#endif

#ifdef _WIN32

C4ThreadPool::CallbackEnvironment::CallbackEnvironment() noexcept
{
	TpInitializeCallbackEnviron(&callbackEnvironment);
}

C4ThreadPool::CallbackEnvironment::~CallbackEnvironment() noexcept
{
	TpDestroyCallbackEnviron(&callbackEnvironment);
}

void C4ThreadPool::ThreadPoolTraits::close(const type handle)
{
	CloseThreadpool(handle);
}

void C4ThreadPool::ThreadPoolCleanupTraits::close(const type handle)
{
	CloseThreadpoolCleanupGroupMembers(handle, false, nullptr);
	CloseThreadpoolCleanupGroup(handle);
}

void C4ThreadPool::ThreadPoolIoTraits::close(const type handle)
{
	CloseThreadpoolIo(handle);
}

C4ThreadPool::Io::Awaiter::Awaiter(Io &io, IoFunction &&function, const std::uint64_t offset) noexcept
	: io{io}, ioFunction{std::move(function)}
{
	overlapped.Offset = static_cast<std::uint32_t>(offset);
	overlapped.OffsetHigh = static_cast<std::uint32_t>(offset >> 32);
}

C4ThreadPool::Io::Awaiter::~Awaiter() noexcept
{
	if (state.exchange(State::Cancelled, std::memory_order_acq_rel) == State::Started)
	{
		io.Cancel();
		io.PopAwaiter(&overlapped);
		CancelIoEx(io.fileHandle, &overlapped);
	}
}

bool C4ThreadPool::Io::Awaiter::await_suspend(const std::coroutine_handle<> handle)
{
	SetCancellablePromise(handle);
	coroutineHandle.store(handle, std::memory_order_relaxed);
	io.SetAwaiter(&overlapped, this);

	io.Start();
	if (ioFunction(io.fileHandle, &overlapped))
	{
		state.store(State::Result, std::memory_order_relaxed);
		io.Cancel();
		io.PopAwaiter(&overlapped);

		DWORD transferred;
		winrt::check_bool(GetOverlappedResult(io.fileHandle, &overlapped, &transferred, false));

		result.numberOfBytesTransferred = transferred;
		return false;
	}
	else if (const DWORD error{GetLastError()}; error != ERROR_IO_PENDING)
	{
		state.store(State::Error, std::memory_order_relaxed);
		io.Cancel();
		io.PopAwaiter(&overlapped);

		result.error = error;
		return false;
	}

	State expected{State::NotStarted};
	if (!state.compare_exchange_strong(expected, State::Started, std::memory_order_release))
	{
		// The callback has already been called.
		return false;
	}

	return true;
}

std::uint64_t C4ThreadPool::Io::Awaiter::await_resume() const
{
	switch (state.load(std::memory_order_acquire))
	{
	case State::Result:
		return result.numberOfBytesTransferred;

	case State::Error:
		winrt::throw_hresult(HRESULT_FROM_WIN32(result.error));

	case State::Cancelled:
		throw C4Task::CancelledException{};

	default:
		std::terminate();
	}
}

void C4ThreadPool::Io::Awaiter::SetupCancellation(C4Task::CancellablePromise *const promise)
{
	promise->SetCancellationCallback(
		[](void *const argument)
		{
			auto *const awaiter = reinterpret_cast<Awaiter *>(argument);

			if (awaiter->state.exchange(State::Cancelled, std::memory_order_acq_rel) == State::Started)
			{
				awaiter->io.Cancel();
				CancelIoEx(awaiter->io.fileHandle, &awaiter->overlapped);
			}
		},
		this
	);
}

void C4ThreadPool::Io::Awaiter::Callback(const PTP_CALLBACK_INSTANCE instance, const ULONG result, const ULONG numberOfBytesTransferred)
{
	State expected{State::Started};
	State desired;

	switch (result)
	{
	case ERROR_SUCCESS:
		this->result.numberOfBytesTransferred = numberOfBytesTransferred;
		desired = State::Result;
		break;

	case ERROR_OPERATION_ABORTED:
		this->result.error = result;
		desired = State::Cancelled;
		break;

	default:
		this->result.error = result;
		desired = State::Error;
		break;
	}

	if (state.compare_exchange_strong(expected, desired, std::memory_order_acq_rel, std::memory_order_acquire))
	{
		CallbackMayRunLong(instance);

		const auto handle = coroutineHandle.load(std::memory_order_relaxed);
		handle.resume();
	}
}

C4ThreadPool::Io::Io(const HANDLE fileHandle, const PTP_CALLBACK_ENVIRON environment)
	: fileHandle{fileHandle}, io{winrt::check_pointer(CreateThreadpoolIo(fileHandle, &Io::Callback, this, environment))}
{
}

C4ThreadPool::Io::~Io() noexcept
{
	WaitForThreadpoolIoCallbacks(io.get(), true);
}

void C4ThreadPool::Io::Start()
{
	StartThreadpoolIo(io.get());
}

void C4ThreadPool::Io::Cancel()
{
	CancelThreadpoolIo(io.get());
}

void C4ThreadPool::Io::SetAwaiter(OVERLAPPED *const overlapped, Awaiter *const handle)
{
	const std::lock_guard lock{awaiterMapMutex};

	awaiterMap.insert_or_assign(overlapped, handle);
}

C4ThreadPool::Io::Awaiter *C4ThreadPool::Io::PopAwaiter(OVERLAPPED *const overlapped)
{
	const std::lock_guard lock{awaiterMapMutex};

	const auto node = awaiterMap.extract(overlapped);
	return node.empty() ? nullptr : node.mapped();
}

void C4ThreadPool::Io::Callback(const PTP_CALLBACK_INSTANCE instance, void *const context, void *const overlapped, const ULONG result, const ULONG_PTR numberOfBytesTransferred, const PTP_IO poolIo)
{
	auto &io = *reinterpret_cast<Io *>(context);
	Awaiter *const awaiter{io.PopAwaiter(reinterpret_cast<OVERLAPPED *>(overlapped))};

	if (awaiter)
	{
		awaiter->Callback(instance, result, numberOfBytesTransferred);
	}
}

C4ThreadPool::C4ThreadPool(const std::uint32_t minimum, const std::uint32_t maximum)
{
	MapHResultError([minimum, maximum, this]
	{
		pool.attach(winrt::check_pointer(CreateThreadpool(nullptr)));

		SetThreadpoolCallbackPool(&callbackEnvironment, pool.get());

		SetThreadpoolThreadMaximum(pool.get(), static_cast<DWORD>(minimum));
		winrt::check_bool(SetThreadpoolThreadMinimum(pool.get(), static_cast<DWORD>(minimum)));

		cleanupGroup.attach(winrt::check_pointer(CreateThreadpoolCleanupGroup()));

		SetThreadpoolCallbackCleanupGroup(&callbackEnvironment, cleanupGroup.get(), nullptr);
	});
}

C4ThreadPool::~C4ThreadPool()
{
}

void C4ThreadPool::SubmitCallback(const PTP_SIMPLE_CALLBACK callback, void *const data)
{
	if (!TrySubmitThreadpoolCallback(callback, data, GetCallbackEnvironment()))
	{
		MapHResultError([] { winrt::throw_last_error(); });
	}
}

#else

C4ThreadPool::C4ThreadPool(const std::uint32_t minimum, const std::uint32_t maximum)
{
	threads.reserve(maximum);

	for (std::size_t i{0}; i < maximum; ++i)
	{
		threads.emplace_back(std::thread{&C4ThreadPool::ThreadProc, this});
	}
}

C4ThreadPool::~C4ThreadPool()
{
	quit.store(true, std::memory_order_release);
	availableCallbacks.release(threads.size());

	for (auto &thread : threads)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}
}

void C4ThreadPool::ThreadProc()
{
	for (;;)
	{
		availableCallbacks.acquire();
		if (quit.load(std::memory_order_acquire))
		{
			return;
		}

		Callback callback;
		{
			std::lock_guard lock{callbackMutex};
			callback = std::move(callbacks.front());
			callbacks.pop();
		}

		callback();
	}
}

#endif
