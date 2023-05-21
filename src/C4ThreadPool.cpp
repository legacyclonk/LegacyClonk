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
