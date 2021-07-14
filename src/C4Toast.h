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
	virtual void Activated() {}
	virtual void Dismissed() {}
	virtual void Failed() { }
	virtual void OnAction(std::string_view action) { (void) action; }
};

class C4ToastImpl
{
protected:
	virtual void AddAction(std::string_view action) { (void) action; }
	virtual void SetEventHandler(C4ToastEventHandler *const eventHandler);
	virtual void SetExpiration(std::uint32_t expiration) { (void) expiration; }
	virtual void SetText(std::string_view text) { (void) text; }
	virtual void SetTitle(std::string_view title) { (void) title; }

	virtual void Show() {}
	virtual void Hide() {}

protected:
	C4ToastEventHandler *eventHandler;

	friend class C4Toast;
};

class C4Toast
{
public:
	C4Toast();

	static void ShowToast(std::string_view title, std::string_view text, C4ToastEventHandler *const eventHandler = nullptr, std::uint32_t expiration = 0, std::initializer_list<std::string_view> actions = {});

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
