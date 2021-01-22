/*
 * LegacyClonk
 *
 * Copyright (c) 2020, The LegacyClonk Team and contributors
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

#include <memory>
#include <string_view>

class C4Toast;

class C4ToastSystem
{
public:
	virtual ~C4ToastSystem() = default;

public:
	static C4ToastSystem *NewInstance();
};

class C4ToastEventHandler
{
public:
	virtual ~C4ToastEventHandler() = default;

public:
	virtual bool Activated() { return false; }
	virtual bool Dismissed() { return false; }
	virtual bool Failed() { return false; }
	virtual bool OnAction(std::string_view action) { (void) action; return false; }
};

class C4ToastImpl
{
protected:
	virtual void AddAction(std::string_view action) = 0;
	virtual void SetEventHandler(C4ToastEventHandler *const eventHandler);
	virtual void SetExpiration(std::uint32_t expiration) = 0;
	virtual void SetText(std::string_view text) = 0;
	virtual void SetTitle(std::string_view title) = 0;

	virtual void Show() = 0;
	virtual void Hide() = 0;

protected:
	C4ToastEventHandler *eventHandler;

	friend class C4Toast;
};

class C4Toast
{
public:
	C4Toast();

public:
	virtual void AddAction(std::string_view action) { return impl->AddAction(action); }
	virtual void SetEventHandler(C4ToastEventHandler *const eventHandler) { return impl->SetEventHandler(eventHandler); }
	virtual void SetExpiration(std::uint32_t expiration) { return impl->SetExpiration(expiration); }
	virtual void SetText(std::string_view text) { return impl->SetText(text); }
	virtual void SetTitle(std::string_view title) { return impl->SetTitle(title); }

	virtual void Show() { return impl->Show(); }
	virtual void Hide() { return impl->Hide(); }

private:
	const std::unique_ptr<C4ToastImpl> impl;
};
