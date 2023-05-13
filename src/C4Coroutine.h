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
{
public:
	CancelledException() = default;

public:
	const char *what() const noexcept override { return "Cancelled"; }
};

template<typename U>
struct ValueWrapper
{
	[[no_unique_address]] U Value;
	U GetValue() const { return static_cast<U>(Value); }
};

template<>
struct ValueWrapper<void>
{
	void GetValue() const {}
};

template<typename T>
class Result
{
private:
	enum ResultState
	{
		NotPresent,
		Present,
		Exception
	};

	union Optional
	{
	public:
		Optional() noexcept {}
		~Optional() noexcept {}

		ValueWrapper<T> Wrapper;
		std::exception_ptr Exception;
	};

	static_assert(std::is_default_constructible_v<Optional>);

public:
	Result() = default;
	~Result()
	{
		switch (resultState)
		{
		case ResultState::Present:
			std::destroy_at(std::addressof(result.Wrapper));
			break;

		case ResultState::Exception:
			std::destroy_at(std::addressof(result.Exception));
			break;

		default:
			break;
		}
	}

	Result(const Result &) = delete;
	Result &operator=(const Result &) = delete;

public:
	decltype(auto) GetResult() const
	{
		switch (resultState)
		{
		case ResultState::Present:
			return result.Wrapper.GetValue();

		case ResultState::Exception:
			std::rethrow_exception(result.Exception);

		default:
			std::terminate();
		}
	}

	template<typename U = std::type_identity_t<T>>
	void SetResult(U &&result) noexcept(noexcept(std::construct_at(std::addressof(this->result.Wrapper), std::forward<T>(result)))) requires (!std::same_as<U, void>)
	{
		if (resultState == ResultState::NotPresent)
		{
			std::construct_at(std::addressof(this->result.Wrapper), std::forward<T>(result));
			resultState = ResultState::Present;
		}
	}

	void SetResult() noexcept requires std::same_as<T, void>
	{
		if (resultState == ResultState::NotPresent)
		{
			std::construct_at(std::addressof(this->result.Wrapper));
			resultState = ResultState::Present;
		}
	}

	void SetException(std::exception_ptr &&exceptionPtr) noexcept
	{
		if (resultState == ResultState::NotPresent)
		{
			std::construct_at(std::addressof(result.Exception), std::move(exceptionPtr));

			resultState = ResultState::Exception;
		}
	}

private:
	ResultState resultState{ResultState::NotPresent};
	Optional result;
};

class CancellablePromise
{
public:
	void Cancel()
	{
		const auto callback = cancellationCallback.exchange(CancellingSentinel, std::memory_order_acq_rel);

		if (callback && callback != CancellingSentinel && callback != CancelledSentinel)
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
	}

	bool IsCancelled() const noexcept
	{
		return cancellationCallback.load(std::memory_order_acquire) == CancelledSentinel;
	}

	void SetCancellationCallback(void(*const callback)(void *), void *const argument) noexcept
	{
		cancellationArgument = argument;
		cancellationCallback.store(callback, std::memory_order_release);
	}

	void ResetCancellationCallback() noexcept
	{
		cancellationArgument = nullptr;
		cancellationCallback.store(nullptr, std::memory_order_release);
	}

public:
	static inline const auto CancellingSentinel = std::bit_cast<void(*)(void *)>(reinterpret_cast<void *>(0x01));
	static inline const auto CancelledSentinel = std::bit_cast<void(*)(void *)>(reinterpret_cast<void *>(0x02));

protected:
	std::atomic<void(*)(void *)> cancellationCallback{nullptr};
	void *cancellationArgument{nullptr};
};

class CancellableAwaiter
{
public:
	CancellableAwaiter() noexcept = default;
	~CancellableAwaiter() noexcept
	{
		if (promise)
		{
			promise->ResetCancellationCallback();
		}
	}

	CancellableAwaiter(const CancellableAwaiter &) noexcept = delete;
	CancellableAwaiter &operator=(const CancellableAwaiter &) noexcept = delete;

	CancellableAwaiter(CancellableAwaiter &&other) noexcept : promise{std::exchange(other.promise, nullptr)} {}

	CancellableAwaiter &operator=(CancellableAwaiter &&other) noexcept
	{
		promise = std::exchange(other.promise, nullptr);
		return *this;
	}

public:
	void SetPromise(CancellablePromise *const promise)
	{
		this->promise = promise;
	}

protected:
	CancellablePromise *promise{nullptr};
};

template<typename T>
struct Promise : CancellablePromise
{
	struct Deleter
	{
		void operator()(Promise *const promise) noexcept;
	};

	using PromisePtr = std::unique_ptr<Promise, Deleter>;
	using ResumeFunction = void(*)(void *);

	auto get_return_object() noexcept { return this; }

	constexpr std::suspend_always initial_suspend() const noexcept
	{
		return {};
	}

	auto final_suspend() noexcept
	{
		struct Awaiter : std::suspend_always
		{
			Promise &promise;

			void await_suspend(const std::coroutine_handle<>) const noexcept
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
						std::coroutine_handle<>::from_address(waiter).resume();
					}
				}
			}
		};

		return Awaiter{.promise = *this};
	}

	void unhandled_exception() const noexcept
	{
		result.SetException(std::current_exception());
	}

	template<typename Awaiter>
	Awaiter &&await_transform(Awaiter &&awaiter)
	{
		if (IsCancelled())
		{
			throw CancelledException{};
		}

		if constexpr (std::derived_from<std::remove_reference_t<Awaiter>, CancellableAwaiter>)
		{
			awaiter.SetPromise(this);
		}

		return std::forward<Awaiter>(awaiter);
	}

	template<typename U = T> requires(!std::same_as<T, void>)
	void SetResult(U &&result) const noexcept
	{
		this->result.SetResult(std::forward<U>(result));
	}

	void SetResult() const noexcept
	{
		this->result.SetResult();
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

template<typename T>
void Promise<T>::Deleter::operator()(Promise<T> *const promise) noexcept
{
	if (promise)
	{
		promise->Abandon();
	}
}

template<template<typename> typename Class, typename T>
class TaskBase
{
public:
	TaskBase() = default;
	TaskBase(Promise<T> *const promise) noexcept : promise{promise}
	{
	}

	~TaskBase() noexcept = default;

	TaskBase(TaskBase &&) = default;
	TaskBase &operator=(TaskBase &&) = default;

public:
	T Get() && noexcept(noexcept(promise->AwaitResume()))
	{
		if constexpr (Class<T>::IsColdStart)
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

	auto operator co_await() && noexcept
	{
		return static_cast<Class<T> &&>(*this)->operator co_await();
	}

	explicit operator bool() const noexcept
	{
		return promise.get();
	}

public:
	static constexpr inline bool IsColdStart{false};

protected:
	typename Promise<T>::PromisePtr promise;
};

template<typename T>
class Cold : public TaskBase<Cold, T>
{
public:
	using TaskBase<Cold, T>::TaskBase;

public:
	void Start() noexcept
	{
		TaskBase<Cold, T>::promise->Start();
	}

	auto operator co_await() && noexcept
	{
		struct ColdAwaiter
		{
			typename Promise<T>::PromisePtr promise;

			constexpr bool await_ready() const
			{
				return false;
			}

			bool await_suspend(const std::coroutine_handle<> handle)
			{
				return promise->ColdAwaitSuspend(handle.address());
			}

			decltype(auto) await_resume()
			{
				return promise->AwaitResume();
			}
		};

		return ColdAwaiter{std::move(TaskBase<Cold, T>::promise)};
	}

public:
	static constexpr inline bool IsColdStart{true};
};

template<typename T>
class Hot : public TaskBase<Hot, T>
{
public:
	Hot() = default;
	Hot(Promise<T> *const promise) noexcept : TaskBase<Hot, T>{promise}
	{
		promise->Start();
	}

public:
	auto operator co_await() && noexcept
	{
		struct HotAwaiter
		{
			typename Promise<T>::PromisePtr promise;

			bool await_ready() const
			{
				return promise->AwaitReady();
			}

			bool await_suspend(const std::coroutine_handle<> handle)
			{
				return promise->AwaitSuspend(handle.address());
			}

			decltype(auto) await_resume()
			{
				return promise->AwaitResume();
			}
		};

		return HotAwaiter{std::move(TaskBase<Hot, T>::promise)};
	}
};

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

template<template<typename> typename Task, typename T, typename... Args> requires std::derived_from<Task<T>, C4Task::TaskBase<Task, T>> && (!std::same_as<T, void>)
struct std::coroutine_traits<Task<T>, Args...>
{
	struct promise_type : C4Task::Promise<T>
	{
		void return_value(T &&value) const noexcept(noexcept(C4Task::Promise<T>::SetResult(std::forward<T>(value))))
		{
			C4Task::Promise<T>::SetResult(std::forward<T>(value));
		}
	};
};

template<template<typename> typename Task, typename T, typename... Args> requires std::derived_from<Task<T>, C4Task::TaskBase<Task, T>> && std::same_as<T, void>
struct std::coroutine_traits<Task<T>, Args...>
{
	struct promise_type : C4Task::Promise<T>
	{
		void return_void() const noexcept(noexcept(C4Task::Promise<T>::SetResult()))
		{
			C4Task::Promise<T>::SetResult();
		}
	};
};
