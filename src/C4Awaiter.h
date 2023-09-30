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
		class ResumeOnSignal : public C4Task::CancellableAwaiter<ResumeOnSignal>
		{
		public:
			ResumeOnSignal(const pollfd fd, const std::uint32_t timeout) : fds{{fd, {.fd = -1}}}, timeout{timeout} {}

		public:
			bool await_ready() noexcept
			{
				return StdSync::Poll(std::span{fds.begin(), 1}, 0) == 1;
			}

			template<typename T>
			void await_suspend(const std::coroutine_handle<T> handle)
			{
				const std::size_t size{fds.size()};
				SetCancellablePromise(handle);

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
							if (cancellationEvent && fds[fds.size() - 1].revents & POLLIN)
							{
								throw C4Task::CancelledException{};
							}

							return;
						}
					}
					catch (...)
					{
						exception = std::current_exception();
					}
				});
			}

			short await_resume() const
			{
				if (exception)
				{
					std::rethrow_exception(exception);
				}

				return fds[0].revents;
			}

			void SetupCancellation(C4Task::CancellablePromise *const promise)
			{
				cancellationEvent.emplace();
				fds[1].fd = cancellationEvent->GetFD();

				promise->SetCancellationCallback([](void *const argument)
				{
					reinterpret_cast<CStdEvent *>(argument)->Set();
				}, &*cancellationEvent);
			}

		private:
			std::array<pollfd, 2> fds;
			std::uint32_t timeout;
			std::exception_ptr exception;
			std::optional<CStdEvent> cancellationEvent;
		};

		class ResumeOnSignals : public C4Task::CancellableAwaiter<ResumeOnSignals>
		{
		public:
			template<std::ranges::range T>
			ResumeOnSignals(T &&range, const std::uint32_t timeout) : fds{std::ranges::begin(range), std::ranges::end(range)}, timeout{timeout} {}

		public:
			bool await_ready() noexcept
			{
				return StdSync::Poll(fds, 0) > 0;
			}

			template<typename T>
			void await_suspend(const std::coroutine_handle<T> handle)
			{
				const std::size_t size{fds.size()};

				SetCancellablePromise(handle);

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
							if (cancellationEvent && fds[fds.size() - 1].revents & POLLIN)
							{
								throw C4Task::CancelledException{};
							}

							return;
						}
					}
					catch (...)
					{
						exception = std::current_exception();
					}
				});
			}

			std::vector<pollfd> await_resume()
			{
				if (exception)
				{
					std::rethrow_exception(exception);
				}

				if (cancellationEvent)
				{
					fds.resize(fds.size() - 1);
				}

				std::erase_if(fds, [](const auto &fd) { return fd.revents == 0; });

				return std::move(fds);
			}

			void SetupCancellation(C4Task::CancellablePromise *const promise)
			{
				cancellationEvent.emplace();
				fds.push_back({.fd = cancellationEvent->GetFD(), .events = POLLIN});

				promise->SetCancellationCallback([](void *const argument)
				{
					reinterpret_cast<CStdEvent *>(argument)->Set();
				}, &*cancellationEvent);
			}

		private:
			std::vector<pollfd> fds;
			std::uint32_t timeout;
			std::exception_ptr exception;
			std::optional<CStdEvent> cancellationEvent;
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
	[[nodiscard]] inline Awaiter::ResumeOnSignal ResumeOnSignal(const pollfd fd, const std::uint32_t timeout = StdSync::Infinite) noexcept
	{
		return {fd, timeout};
	}

	template<std::ranges::range T>
	[[nodiscard]] inline Awaiter::ResumeOnSignals ResumeOnSignals(T &&fds, const std::uint32_t timeout = StdSync::Infinite) noexcept
	{
		return {std::forward<T>(fds), timeout};
	}
#endif
}
