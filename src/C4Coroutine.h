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

#pragma once

#include "C4Attributes.h"

#ifdef _WIN32
#include "C4WinRT.h"
#endif

#include "StdSync.h"

#include <atomic>
#include <bit>
#include <concepts>
#include <coroutine>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>

namespace C4Task
{

class CancelledException : public std::exception
#ifdef _WIN32
	, public winrt::hresult_canceled
#endif
{
public:
	CancelledException() = default;

#ifdef _WIN32
	CancelledException(const winrt::hresult_canceled &e) : hresult_canceled{e} {}
	CancelledException(winrt::hresult_canceled &&e) : hresult_canceled{std::move(e)} {}
#endif

public:
	const char *what() const noexcept override { return "Cancelled"; }
};

template<typename U>
struct ValueWrapper
{
	[[NO_UNIQUE_ADDRESS]] U Value;
	U &&GetValue() { return static_cast<U &&>(Value); }
};

template<>
struct ValueWrapper<void>
{
	void GetValue() const {}
};

template<typename T, bool IsNoExcept>
union Optional
{
public:
	Optional() noexcept {}
	~Optional() noexcept {}

	ValueWrapper<T> Wrapper;
	std::exception_ptr Exception;
};

template<typename T>
union Optional<T, true>
{
public:
	Optional() noexcept {}
	~Optional() noexcept {}

	ValueWrapper<T> Wrapper;
};

template<typename T, bool IsNoExcept = false>
class ResultBase
{
protected:
	decltype(auto) GetValue()
	{
		return result.Wrapper.GetValue();
	}

	[[noreturn]] void ReThrowException() const
	{
		std::rethrow_exception(result.Exception);
	}

	template<typename U = std::type_identity_t<T>>
	void CreateValue(U &&value) noexcept(noexcept(std::construct_at(std::addressof(result.Wrapper), std::forward<U>(value)))) requires (!std::same_as<U, void>)
	{
		std::construct_at(std::addressof(result.Wrapper), std::forward<U>(value));
	}

	void CreateValue() noexcept requires std::same_as<T, void>
	{
		std::construct_at(std::addressof(result.Wrapper));
	}

	void CreateException(std::exception_ptr &&ptr) requires (!IsNoExcept)
	{
		std::construct_at(std::addressof(this->result.Exception), std::move(ptr));
	}

	void DestroyWrapper() noexcept
	{
		std::destroy_at(std::addressof(result.Wrapper));
	}

	void DestroyException() noexcept requires (!IsNoExcept)
	{
		std::destroy_at(std::addressof(result.Exception));
	}

protected:
	Optional<T, IsNoExcept> result;
};

template<>
class ResultBase<void, true>
{
protected:
	constexpr void GetValue() const noexcept {}
	constexpr void CreateValue() const noexcept {}
	constexpr void DestroyWrapper() const noexcept {}
};

template<typename U, bool IsNoExcept_>
struct EnumBaseType
{
	using Type = int;
};

template<typename U>
struct EnumBaseType<U, true>
{
	using Type = std::conditional_t<
		sizeof(U) == 1, std::uint8_t, std::conditional_t<
			sizeof(U) == 2, std::uint16_t, int
			>
		>;
};

template<>
struct EnumBaseType<void, true>
{
	using Type = std::uint8_t;
};

template<typename T, bool IsNoExcept = false>
class Result : protected ResultBase<T, IsNoExcept>
{
private:
	enum ResultState : typename EnumBaseType<T, IsNoExcept>::Type
	{
		NotPresent,
		Present,
		Exception
	};

public:
	Result() = default;
	~Result()
	{
		switch (resultState)
		{
		case ResultState::Present:
			ResultBase<T, IsNoExcept>::DestroyWrapper();
			break;

		case ResultState::Exception:
			if constexpr (!IsNoExcept)
			{
				ResultBase<T, IsNoExcept>::DestroyException();
			}
			else
			{
				std::unreachable();
			}
			break;

		default:
			break;
		}
	}

	Result(const Result &) = delete;
	Result &operator=(const Result &) = delete;

public:
	decltype(auto) GetResult()
	{
		switch (resultState)
		{
		case ResultState::Present:
			return ResultBase<T, IsNoExcept>::GetValue();

		case ResultState::Exception:
			if constexpr (!IsNoExcept)
			{
				ResultBase<T, IsNoExcept>::ReThrowException();
			}
			else
			{
				std::unreachable();
			}

		default:
			std::terminate();
		}
	}

	template<typename U = std::type_identity_t<T>>
	void SetResult(U &&result) noexcept(noexcept(ResultBase<T, IsNoExcept>::CreateValue(std::forward<U>(result)))) requires (!std::same_as<U, void>)
	{
		if (resultState == ResultState::NotPresent)
		{
			ResultBase<T, IsNoExcept>::CreateValue(std::forward<U>(result));
			resultState = ResultState::Present;
		}
	}

	void SetResult() noexcept requires std::same_as<T, void>
	{
		if (resultState == ResultState::NotPresent)
		{
			ResultBase<T, IsNoExcept>::CreateValue();
			resultState = ResultState::Present;
		}
	}

	void SetException(std::exception_ptr &&exceptionPtr) noexcept requires (!IsNoExcept)
	{
		if (resultState == ResultState::NotPresent)
		{
			ResultBase<T, IsNoExcept>::CreateException(std::move(exceptionPtr));
			resultState = ResultState::Exception;
		}
	}

private:
	ResultState resultState{ResultState::NotPresent};
};

struct CancellationTokenMarker {};

inline CancellationTokenMarker GetCancellationToken()
{
	return {};
}

struct PromiseAwaiterMarker {};

inline PromiseAwaiterMarker GetPromise()
{
	return {};
}

class CancellablePromise
#ifdef _WIN32
	: public winrt::cancellable_promise
#endif
{
public:
	class CancellationToken
	{
	public:
		constexpr CancellationToken(const CancellablePromise *const promise) : promise{promise} {}

	public:
		constexpr bool await_ready() const noexcept { return true; }
		constexpr void await_suspend(const std::coroutine_handle<>) const noexcept {}
		constexpr CancellationToken await_resume() const noexcept { return *this; }

		bool operator()() const noexcept
		{
			return promise->IsCancelled();
		}

	private:
		const CancellablePromise *promise;
	};

private:
	struct PromiseAwaiter
	{
		CancellablePromise &promise;

		constexpr bool await_ready() const noexcept
		{
			return true;
		}

		constexpr void await_suspend(const std::coroutine_handle<>) const noexcept {}

		CancellablePromise &await_resume() const noexcept
		{
			return promise;
		}
	};

public:
#ifdef _WIN32
	CancellablePromise()
	{
		enable_cancellation_propagation(true);
	}
#endif

	void Cancel()
	{
#ifdef _WIN32
		cancelled.store(true, std::memory_order_release);
		cancel();
#else

		const auto callback = cancellationCallback.exchange(CancelledSentinel, std::memory_order_acq_rel);

		if (callback && callback != CancelledSentinel)
		{
			const struct Cleanup
			{
				CancellablePromise &Promise;

				~Cleanup()
				{
					Promise.cancellationCallback.store(CancelledSentinel, std::memory_order_release);
				}
			} cleanup{*this};

			callback(cancellationArgument);
		}
#endif
	}

	void SetCancellationCallback(void(*const callback)(void *), void *const argument) noexcept
	{
#ifdef _WIN32
		set_canceller(callback, argument);
#else
		cancellationArgument = argument;
		cancellationCallback.store(callback, std::memory_order_release);
#endif
	}

	void ResetCancellationCallback() noexcept
	{
#ifdef _WIN32
		revoke_canceller();
#else
		auto callback = cancellationCallback.load(std::memory_order_acquire);

		do
		{
			if (!callback || callback == CancelledSentinel)
			{
				return;
			}
		}
		while (!cancellationCallback.compare_exchange_weak(callback, nullptr, std::memory_order_release));
#endif
	}

	bool IsCancelled() const noexcept
	{
#ifdef _WIN32
		return cancelled.load(std::memory_order_acquire);
#else
		const auto callback = cancellationCallback.load(std::memory_order_acquire);
		return callback == CancelledSentinel;
#endif
	}

	CancellationToken await_transform(const CancellationTokenMarker) noexcept
	{
		return {this};
	}

	PromiseAwaiter await_transform(const PromiseAwaiterMarker) noexcept
	{
		return {*this};
	}

#ifdef _WIN32
private:
	std::atomic_bool cancelled{false};
#else
public:
	static inline const auto CancelledSentinel = std::bit_cast<void(*)(void *)>(reinterpret_cast<void *>(0x02));

protected:
	std::atomic<void(*)(void *)> cancellationCallback{nullptr};
	void *cancellationArgument{nullptr};
#endif
};

template<typename Class>
class CancellableAwaiter
#ifdef _WIN32
	: public winrt::cancellable_awaiter<Class>
#endif
{
public:
	CancellableAwaiter() noexcept = default;

#ifndef _WIN32
	~CancellableAwaiter() noexcept
	{
		if (promise)
		{
			promise->ResetCancellationCallback();
		}
	}
#endif

	CancellableAwaiter(const CancellableAwaiter &) noexcept = delete;
	CancellableAwaiter &operator=(const CancellableAwaiter &) noexcept = delete;

#ifndef _WIN32
	CancellableAwaiter(CancellableAwaiter &&other) noexcept : promise{std::exchange(other.promise, nullptr)} {}

	CancellableAwaiter &operator=(CancellableAwaiter &&other) noexcept
	{
		promise = std::exchange(other.promise, nullptr);
		return *this;
	}
#endif

public:
	template<typename T>
	void SetCancellablePromise(const std::coroutine_handle<T> handle)
	{
#ifdef _WIN32
		winrt::cancellable_awaiter<Class>::set_cancellable_promise_from_handle(handle);
#else
		if constexpr (std::derived_from<T, CancellablePromise>)
		{
			promise = &handle.promise();
			static_cast<Class *>(this)->SetupCancellation(promise);
		}
#endif
	}

#ifdef _WIN32
	void enable_cancellation(winrt::cancellable_promise *const promise)
	{
		static_cast<Class *>(this)->SetupCancellation(static_cast<CancellablePromise *>(promise));
	}
#endif

private:
#ifndef _WIN32
	CancellablePromise *promise{nullptr};
#endif
};

struct PromiseTraitsDefault
{
	static constexpr bool TerminateOnException{false};
};

struct PromiseTraitsTerminateOnException : PromiseTraitsDefault
{
	static constexpr bool TerminateOnException{true};
};

template<typename T, typename PromiseTraits>
struct Promise : CancellablePromise
{
	struct Deleter
	{
		void operator()(Promise *const promise) noexcept;
	};

	struct FinalSuspendAwaiter : std::suspend_always
	{
		Promise &promise;

		std::coroutine_handle<> await_suspend(const std::coroutine_handle<>) const noexcept
		{
			const auto waiter = promise.waiting.exchange(CompletedSentinel, std::memory_order_acq_rel);
			if (waiter == AbandonedSentinel)
			{
				promise.GetHandle().destroy();
			}

			else if (waiter != StartedSentinel)
			{
				if (promise.resumer)
				{
					promise.resumer(waiter);
				}
				else
				{
					return std::coroutine_handle<>::from_address(waiter);
				}
			}

			return std::noop_coroutine();
		}
	};

	using PromisePtr = std::unique_ptr<Promise, Deleter>;
	using ResumeFunction = void(*)(void *);

	auto get_return_object() noexcept { return this; }

	constexpr std::suspend_always initial_suspend() const noexcept
	{
		return {};
	}

	FinalSuspendAwaiter final_suspend() noexcept
	{
		return {.promise = *this};
	}

	void unhandled_exception() const noexcept
	{
		std::exception_ptr ptr{std::current_exception()};

#ifndef _WIN32
		if constexpr (PromiseTraits::TerminateOnException)
		{
#endif
			try
			{
				std::rethrow_exception(ptr);
			}
			catch (const C4Task::CancelledException &)
			{
				SetException(ptr);
			}
#ifdef _WIN32
			catch (const winrt::hresult_canceled &e)
			{
				SetException(std::make_exception_ptr(CancelledException{e}));
			}
#endif
			catch (...)
			{
				if constexpr (PromiseTraits::TerminateOnException)
				{
					std::terminate();
				}
				else
				{
					SetException(ptr);
				}
			}
#ifndef _WIN32
		}
		else
		{
			SetException(ptr);
		}
#endif
	}

	template<typename Awaiter>
	Awaiter &&await_transform(Awaiter &&awaiter)
	{
		if (IsCancelled())
		{
			throw CancelledException{};
		}

		return std::forward<Awaiter>(awaiter);
	}

	using CancellablePromise::await_transform;

	template<typename U = T> requires(!std::same_as<T, void>)
	void SetResult(U &&result) const noexcept(noexcept(this->result.SetResult(std::forward<U>(result))))
	{
		this->result.SetResult(std::forward<U>(result));
	}

	void SetResult() const noexcept
	{
		this->result.SetResult();
	}

	void SetException(std::exception_ptr e) const noexcept
	{
		this->result.SetException(std::move(e));
	}

	void Start() noexcept
	{
		waiting.store(StartedSentinel, std::memory_order_release);
		GetHandle().resume();
	}

	void Abandon() noexcept
	{
		const auto handle = waiting.exchange(AbandonedSentinel, std::memory_order_acq_rel);
		if (handle != StartedSentinel)
		{
			GetHandle().destroy();
		}
	}

	bool AwaitReady() const noexcept
	{
		return waiting.load(std::memory_order_acquire) != StartedSentinel;
	}

	bool AwaitSuspend(void *const address, const ResumeFunction resumer = {}) noexcept
	{
		this->resumer = resumer;
		return waiting.exchange(address, std::memory_order_acq_rel) == StartedSentinel;
	}

	bool ColdAwaitSuspend(void *const address, const ResumeFunction resumer = {}) noexcept
	{
		Start();
		return AwaitSuspend(address, resumer);
	}

	decltype(auto) AwaitResume()
	{
		return result.GetResult();
	}

private:
	auto GetHandle() noexcept
	{
		return std::coroutine_handle<Promise>::from_promise(*this);
	}

public:
	static inline const auto StartedSentinel = reinterpret_cast<void *>(0x00);
	static inline const auto CompletedSentinel = reinterpret_cast<void *>(0x01);
	static inline const auto AbandonedSentinel = reinterpret_cast<void *>(0x02);
	static inline const auto ColdSentinel = reinterpret_cast<void *>(0x03);

private:
	std::atomic<void *> waiting{ColdSentinel};
	ResumeFunction resumer;
	mutable Result<T> result;
};

template<typename T, typename PromiseTraits>
void Promise<T, PromiseTraits>::Deleter::operator()(Promise *const promise) noexcept
{
	if (promise)
	{
		promise->Abandon();
	}
}

struct TaskTraitsDefault
{
	static constexpr bool IsColdStart{false};
	static constexpr bool CancelAndWaitOnDestruction{false};
};

struct TaskTraitsCold : public TaskTraitsDefault
{
	static constexpr bool IsColdStart{true};
};

struct TaskTraitsHotWaitOnDestruction : TaskTraitsDefault
{
	static constexpr bool CancelAndWaitOnDestruction{true};
};

struct TaskTraitsColdWaitOnDestruction : TaskTraitsCold
{
	static constexpr bool CancelAndWaitOnDestruction{true};
};

template<typename T, typename TaskTraits, typename PromiseTraits = PromiseTraitsDefault>
class Task
{
private:
	struct Awaiter : public CancellableAwaiter<Awaiter>
	{
		typename Promise<T, PromiseTraits>::PromisePtr promisePtr;

		constexpr bool await_ready() const
		{
			return false;
		}

		template<typename U>
		bool await_suspend(const std::coroutine_handle<U> handle)
		{
			CancellableAwaiter<Awaiter>::SetCancellablePromise(handle);

			if constexpr (TaskTraits::IsColdStart)
			{
				return promisePtr->ColdAwaitSuspend(handle.address());
			}
			else
			{
				return promisePtr->AwaitSuspend(handle.address());
			}
		}

		decltype(auto) await_resume()
		{
			return promisePtr->AwaitResume();
		}

		void SetupCancellation(CancellablePromise *const promise)
		{
			promise->SetCancellationCallback([](void *const context)
			{
				reinterpret_cast<typename Promise<T, PromiseTraits>::PromisePtr::pointer>(context)->Cancel();
			}, this->promisePtr.get());
		}
	};

public:
	Task() = default;
	Task(Promise<T, PromiseTraits> *const promise) noexcept : promise{promise}
	{
		if constexpr (!TaskTraits::IsColdStart)
		{
			promise->Start();
		}
	}

	~Task()	noexcept
	{
		if constexpr (TaskTraits::CancelAndWaitOnDestruction)
		{
			if (promise)
			{
				std::move(*this).CancelAndWait();
			}
		}
	}

	Task(Task &&) = default;
	Task &operator=(Task &&) = default;

public:
	T Get() &&
	{
		if constexpr (TaskTraits::IsColdStart)
		{
			promise->Start();
		}

		if (!promise->AwaitReady())
		{
			constexpr auto signalEvent = [](void *const event)
			{
				reinterpret_cast<CStdEvent *>(event)->Set();
			};

			CStdEvent event;
			if (promise->AwaitSuspend(&event, signalEvent))
			{
				event.WaitFor(StdSync::Infinite);
			}
		}

		return promise->AwaitResume();
	}

	void Cancel() const
	{
		promise->Cancel();
	}

	bool IsCancelled() const noexcept
	{
		return promise->IsCancelled();
	}

	void CancelAndWait() && noexcept(PromiseTraitsTerminateOnException::TerminateOnException)
	{
		Cancel();

		try
		{
			(void) std::move(*this).Get();
		}
		catch (const CancelledException &)
		{
		}
	}

	void Start() const requires TaskTraits::IsColdStart
	{
		promise->Start();
	}

	explicit operator bool() const noexcept
	{
		return promise.get();
	}

	Awaiter operator co_await() && noexcept
	{
		return {.promisePtr = std::move(promise)};
	}

public:
	static constexpr inline bool IsColdStart{false};

protected:
	typename Promise<T, PromiseTraits>::PromisePtr promise;
};

template<typename T, typename PromiseTraits = PromiseTraitsDefault>
using Hot = Task<T, TaskTraitsDefault, PromiseTraits>;

template<typename T, typename PromiseTraits = PromiseTraitsDefault>
using Cold = Task<T, TaskTraitsCold, PromiseTraits>;

class OneShot
{
public:
	struct promise_type
	{
		constexpr OneShot get_return_object() const noexcept { return {}; }
		constexpr std::suspend_never initial_suspend() const noexcept { return {}; }
		constexpr std::suspend_never final_suspend() const noexcept { return {}; }
		constexpr void return_void() const noexcept {}
		void unhandled_exception() const noexcept { std::terminate(); }
	};
};

}

template<template<typename, typename, typename> typename Task, typename T, typename TaskTraits, typename PromiseTraits, typename... Args> requires std::derived_from<Task<T, TaskTraits, PromiseTraits>, C4Task::Task<T, TaskTraits, PromiseTraits>> && (!std::same_as<T, void>)
struct std::coroutine_traits<Task<T, TaskTraits, PromiseTraits>, Args...>
{
	struct promise_type : C4Task::Promise<T, PromiseTraits>
	{
		void return_value(T &&value) const noexcept(noexcept(C4Task::Promise<T, PromiseTraits>::SetResult(std::forward<T>(value))))
		{
			C4Task::Promise<T, PromiseTraits>::SetResult(std::forward<T>(value));
		}
	};
};

template<template<typename, typename, typename> typename Task, typename T, typename TaskTraits, typename PromiseTraits, typename... Args> requires std::derived_from<Task<T, TaskTraits, PromiseTraits>, C4Task::Task<T, TaskTraits, PromiseTraits>> && std::same_as<T, void>
struct std::coroutine_traits<Task<T, TaskTraits, PromiseTraits>, Args...>
{
	struct promise_type : C4Task::Promise<T, PromiseTraits>
	{
		void return_void() const noexcept(noexcept(C4Task::Promise<T, PromiseTraits>::SetResult()))
		{
			C4Task::Promise<T, PromiseTraits>::SetResult();
		}
	};
};
