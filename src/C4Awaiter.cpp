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

#include "C4Application.h"
#include "C4Awaiter.h"

bool C4Awaiter::Awaiter::ResumeInMainThread::await_ready() const noexcept
{
    return Application.IsMainThread();
}

void C4Awaiter::Awaiter::ResumeInMainThread::await_suspend(const std::coroutine_handle<> handle) const noexcept
{
    Application.InteractiveThread.ExecuteInMainThread(handle);
}

#ifndef _WIN32
bool C4Awaiter::Awaiter::ResumeOnSignal::await_ready() noexcept
{
	return StdSync::Poll(std::span{fds.begin(), 1}, 0) == 1;
}

void C4Awaiter::Awaiter::ResumeOnSignal::await_suspend(std::coroutine_handle<> handle)
{
	if (promise)
	{
		cancellationEvent.emplace();
		fds[1].fd = cancellationEvent->GetFDs()[0];

		promise->SetCancellationCallback([](void *const argument)
		{
			reinterpret_cast<CStdEvent *>(argument)->Set();
		}, &*cancellationEvent);
	}

	C4ThreadPool::Global->SubmitCallback([handle, this]
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
				if (fds[1].revents & POLLIN)
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

short C4Awaiter::Awaiter::ResumeOnSignal::await_resume() const
{
	if (exception)
	{
		std::rethrow_exception(exception);
	}

	return fds[0].revents;
}

bool C4Awaiter::Awaiter::ResumeOnSignals::await_ready() noexcept
{
	return StdSync::Poll(fds, 0) > 0;
}

void C4Awaiter::Awaiter::ResumeOnSignals::await_suspend(std::coroutine_handle<> handle)
{
	const std::size_t size{fds.size()};

	if (promise)
	{
		cancellationEvent.emplace();
		fds.push_back({.fd = cancellationEvent->GetFDs()[0], .events = POLLIN});

		promise->SetCancellationCallback([](void *const argument)
		{
			reinterpret_cast<CStdEvent *>(argument)->Set();
		}, &*cancellationEvent);
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

std::vector<pollfd> C4Awaiter::Awaiter::ResumeOnSignals::await_resume()
{
	if (exception)
	{
		std::rethrow_exception(exception);
	}

	if (cancellationEvent)
	{
		fds.resize(fds.size() - 1);
	}

	return std::move(fds);
}
#endif
