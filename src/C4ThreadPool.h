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

#include <bit>
#include <coroutine>
#include <cstdint>
#include <memory>
#include <utility>

#ifdef _WIN32
#include <coroutine>
#else
#include <functional>
#include <mutex>
#include <queue>
#include <semaphore>
#include <thread>
#include <vector>
#endif

class C4ThreadPool
{
#ifdef _WIN32
private:
	class CallbackEnvironment
	{
	public:
		CallbackEnvironment() noexcept;
		~CallbackEnvironment() noexcept;

		CallbackEnvironment(const CallbackEnvironment &) = delete;
		CallbackEnvironment &operator=(const CallbackEnvironment &) = delete;

		CallbackEnvironment(CallbackEnvironment &&) = delete;
		CallbackEnvironment &operator=(CallbackEnvironment &&) = delete;

	public:
		bool HasPool() const noexcept { return callbackEnvironment.Pool; }

		operator TP_CALLBACK_ENVIRON &() noexcept { return callbackEnvironment; }
		operator const TP_CALLBACK_ENVIRON &() const noexcept { return callbackEnvironment; }

		TP_CALLBACK_ENVIRON *operator&() noexcept { return &callbackEnvironment; }
		const TP_CALLBACK_ENVIRON *operator&() const noexcept { return &callbackEnvironment; }

	private:
		TP_CALLBACK_ENVIRON callbackEnvironment;
	};

	template<typename T>
	struct ThreadPoolTraitsBase
	{
		using type = T;
		static constexpr type invalid()
		{
			return nullptr;
		}
	};

	struct ThreadPoolTraits : ThreadPoolTraitsBase<PTP_POOL>
	{
		static void close(const type handle);
	};

	struct ThreadPoolCleanupTraits : ThreadPoolTraitsBase<PTP_CLEANUP_GROUP>
	{
		static void close(const type handle);
	};
#else
private:
	using Callback = std::function<void()>;
#endif

public:
	C4ThreadPool() = default;
	C4ThreadPool(std::uint32_t minimum, std::uint32_t maximum);
	~C4ThreadPool();

	C4ThreadPool(const C4ThreadPool &) = delete;
	C4ThreadPool &operator=(const C4ThreadPool &) = delete;

	C4ThreadPool(C4ThreadPool &&) = delete;
	C4ThreadPool &operator=(C4ThreadPool &&) = delete;

public:
#ifdef _WIN32
	template<typename T>
	void SubmitCallback(T &&callback)
	{
		if constexpr (sizeof(void *) == sizeof(void(*)()) && std::is_convertible_v<T, void(*)()>)
		{
			SubmitCallback([](const PTP_CALLBACK_INSTANCE, void *const data)
			{
				std::bit_cast<void(*)()>(data)();
			}, std::bit_cast<void *>(static_cast<void(*)()>(callback)));
		}
		else
		{
			using Type = std::remove_reference_t<T>;
			auto callbackPtr = std::make_unique<Type>(std::move(callback));
			SubmitCallback([](const PTP_CALLBACK_INSTANCE, void *const data)
			{
				const std::unique_ptr<Type> callback{reinterpret_cast<Type *>(data)};
				(*callback)();
			}, const_cast<void *>(static_cast<const void *>(callbackPtr.get())));

			callbackPtr.release();
		}
	}

	void SubmitCallback(const std::coroutine_handle<> handle)
	{
		SubmitCallback([](const PTP_CALLBACK_INSTANCE, void *const data)
		{
			std::coroutine_handle<>::from_address(data).resume();
		}, handle.address());
	}

	void SubmitCallback(PTP_SIMPLE_CALLBACK callback, void *data);

	template<typename Func>
	decltype(auto) NativeThreadPoolOperation(Func &&operation)
	{
		return operation(pool.get(), GetCallbackEnvironment());
	}
#else
	template<typename T>
	void SubmitCallback(T &&callback)
	{
		{
			const std::lock_guard lock{callbackMutex};
			callbacks.push(std::move(callback));
		}

		availableCallbacks.release();
	}
#endif

	auto operator co_await() & noexcept
	{
		struct Awaiter
		{
			C4ThreadPool &ThreadPool;

			constexpr bool await_ready() const noexcept
			{
				return false;
			}

			void await_suspend(const std::coroutine_handle<> handle) const noexcept
			{
				ThreadPool.SubmitCallback(handle);
			}

			constexpr void await_resume() const noexcept
			{
			}
		};

		return Awaiter{*this};
	}

public:
	static std::shared_ptr<C4ThreadPool> Global();

private:
#ifdef _WIN32
	PTP_CALLBACK_ENVIRON GetCallbackEnvironment() noexcept
	{
		return callbackEnvironment.HasPool() ? &callbackEnvironment : nullptr;
	}
#else
	void ThreadProc();
#endif

private:
	static inline std::shared_ptr<C4ThreadPool> globalPool{};

#ifdef _WIN32
	CallbackEnvironment callbackEnvironment;
	winrt::handle_type<ThreadPoolTraits> pool;
	winrt::handle_type<ThreadPoolCleanupTraits> cleanupGroup;
#else
	std::vector<std::thread> threads;
	std::atomic_bool quit{false};
	std::queue<Callback> callbacks;
	std::counting_semaphore<> availableCallbacks{0};
	std::mutex callbackMutex;
#endif
};
