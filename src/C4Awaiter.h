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

#include "C4Coroutine.h"
#include "C4ThreadPool.h"
#include "StdSync.h"

#ifdef _WIN32
#include "C4WinRT.h"

#include <chrono>
#else
#include <array>
#include <cstring>
#include <limits>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <vector>
#endif

namespace C4Awaiter
{
	namespace Awaiter
	{
		struct ResumeInMainThread
		{
			bool await_ready() const noexcept;
			void await_suspend(const std::coroutine_handle<> handle) const noexcept;
			constexpr void await_resume() const noexcept {}
		};

		struct ResumeInGlobalThreadPool
		{
			constexpr bool await_ready() const noexcept { return false; }

			void await_suspend(const std::coroutine_handle<> handle) const noexcept
			{
				C4ThreadPool::Global->SubmitCallback(handle);
			}

			constexpr void await_resume() const noexcept {}
		};

#ifndef _WIN32
		template<bool Single>
		class ResumeOnSignals : public C4Task::CancellableAwaiter<ResumeOnSignals<Single>>
		{
		private:
			enum class State
			{
				Idle,
				Pending,
				Cancelling,
				Cancelled
			};

			using PollFdContainer = std::conditional_t<Single, std::array<pollfd, 2>, std::vector<pollfd>>;

		public:
			template<std::ranges::range T>
			ResumeOnSignals(T &&range, const std::uint32_t timeout) requires (!Single) : fds{std::ranges::begin(range), std::ranges::end(range)}, timeout{timeout}
			{
				fds.push_back(pollfd{.fd = cancellationEvent.GetFD(), .events = POLLIN});
			}

			ResumeOnSignals(const pollfd fd, const std::uint32_t timeout) requires Single
				: fds{{fd, pollfd{.fd = cancellationEvent.GetFD(), .events = POLLIN}}},
				  timeout{timeout}
			{
			}

		public:
			bool await_ready() noexcept
			{
				return StdSync::Poll(std::span{fds.data(), fds.size() - 1}, 0) > 0;
			}

			template<typename T>
			void await_suspend(const std::coroutine_handle<T> handle)
			{
				const std::size_t size{fds.size()};

				C4Task::CancellableAwaiter<ResumeOnSignals<Single>>::SetCancellablePromise(handle);

				State expected{State::Idle};
				if (!state.compare_exchange_strong(expected, State::Pending, std::memory_order_acq_rel))
				{
					handle.resume();
				}

				C4ThreadPool::Global->SubmitCallback([handle, size, this]
				{
					const struct Cleanup
					{
						~Cleanup()
						{
							handle.resume();
						}

						std::coroutine_handle<> handle;
					} cleanup{handle};

					try
					{
						switch (StdSync::Poll(fds, timeout))
						{
						case -1:
								throw std::runtime_error{std::string{"poll failed: "} + std::strerror(errno)};

						case 0:
							return;

						default:
							return;
						}
					}
					catch (...)
					{
						exception = std::current_exception();
					}
				});
			}

			std::vector<pollfd> await_resume() requires (!Single)
			{
				CheckCancellationAndException();

				fds.resize(fds.size() - 1);

				std::erase_if(fds, [](const auto &fd) { return fd.revents == 0; });

				return std::move(fds);
			}

			pollfd await_resume() requires Single
			{
				CheckCancellationAndException();

				return fds[0];
			}

			void SetupCancellation(C4Task::CancellablePromise *const promise)
			{
				promise->SetCancellationCallback([](void *const argument)
				{
					auto &awaiter = *reinterpret_cast<ResumeOnSignals *>(argument);
					State expected{State::Pending};
					if (awaiter.state.compare_exchange_strong(expected, State::Cancelling, std::memory_order_acq_rel))
					{
						awaiter.cancellationEvent.Set();
						awaiter.state.store(State::Cancelled, std::memory_order_release);
					}
				}, this);
			}

		private:
			void CheckCancellationAndException()
			{
				State expected{State::Pending};

				while (!state.compare_exchange_weak(expected, State::Idle, std::memory_order_acq_rel))
				{
					if (expected == State::Cancelling)
					{
						expected = State::Cancelled;
					}
				}

				if (expected == State::Cancelled)
				{
					throw C4Task::CancelledException{};
				}

				if (exception)
				{
					std::rethrow_exception(exception);
				}
			}

		private:
			CStdEvent cancellationEvent;
			PollFdContainer fds;
			std::uint32_t timeout;
			std::atomic<State> state{State::Idle};
			std::exception_ptr exception;
		};
#endif
	}

	[[nodiscard]] inline constexpr Awaiter::ResumeInMainThread ResumeInMainThread() noexcept
	{
		return {};
	}

	[[nodiscard]] inline constexpr Awaiter::ResumeInGlobalThreadPool ResumeInGlobalThreadPool() noexcept
	{
		return {};
	}

#ifdef _WIN32
	[[nodiscard]] inline auto ResumeOnSignal(const HANDLE handle, const std::uint32_t timeout = StdSync::Infinite)
	{
		if (timeout == StdSync::Infinite)
		{
			return winrt::resume_on_signal(handle);
		}
		else
		{
			return winrt::resume_on_signal(handle, std::chrono::duration_cast<winrt::Windows::Foundation::TimeSpan>(std::chrono::milliseconds{timeout}));
		}
	}
#else
	[[nodiscard]] inline Awaiter::ResumeOnSignals<true> ResumeOnSignal(const pollfd fd, const std::uint32_t timeout = StdSync::Infinite) noexcept
	{
		return {fd, timeout};
	}

	template<std::ranges::range T>
	[[nodiscard]] inline Awaiter::ResumeOnSignals<false> ResumeOnSignals(T &&fds, const std::uint32_t timeout = StdSync::Infinite) noexcept
	{
		return {std::forward<T>(fds), timeout};
	}
#endif
}
