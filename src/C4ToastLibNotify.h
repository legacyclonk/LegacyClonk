/*
 * LegacyClonk
 *
 * Copyright (c) 2021, The LegacyClonk Team and contributors
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

#include "C4Toast.h"

#include <string>
#include <vector>

#include <libnotify/notify.h>

class C4ToastSystemLibNotify : public C4ToastSystem
{
public:
	C4ToastSystemLibNotify();
	~C4ToastSystemLibNotify();
};

class C4ToastImplLibNotify: public C4ToastImpl
{
private:
	struct UserData
	{
		C4ToastImplLibNotify *const toast;
		const size_t index;
	};

public:
	C4ToastImplLibNotify();
	~C4ToastImplLibNotify();

public:
	void AddAction(std::string_view action) override;
	void SetExpiration(std::uint32_t expiration) override;
	void SetText(std::string_view text) override;
	void SetTitle(std::string_view title) override;

	void Show() override;
	void Hide() override;

private:
	static void Activated(NotifyNotification *, char *, gpointer userData);
	static void OnAction(NotifyNotification *, char *, gpointer userData);
	static void Dismissed(NotifyNotification *, gpointer userData);

private:
	NotifyNotification *notification;
	std::vector<std::string> actions;
	std::string title;
	std::string text;
	gulong signalClose;
};
