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
