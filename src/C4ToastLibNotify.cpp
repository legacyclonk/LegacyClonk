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

#include "C4ToastLibNotify.h"
#include "C4Version.h"

#include "StdBuf.h"

C4ToastSystemLibNotify::C4ToastSystemLibNotify()
{
	if (!notify_init(C4ENGINENAME))
	{
		throw std::runtime_error{"notify_init failed"};
	}
}

C4ToastSystemLibNotify::~C4ToastSystemLibNotify()
{
	notify_uninit();
}

C4ToastImplLibNotify::C4ToastImplLibNotify() : C4ToastImpl{}, notification{notify_notification_new("", "", "")}
{
	notify_notification_add_action(notification, "default", "Default", &C4ToastImplLibNotify::Activated, this, nullptr);
}

C4ToastImplLibNotify::~C4ToastImplLibNotify()
{
	g_object_unref(G_OBJECT(notification));
}

void C4ToastImplLibNotify::AddAction(std::string_view action)
{
	const size_t index{actions.size()};
	actions.emplace_back(action);

	notify_notification_add_action(
				notification,
				FormatString("action-%lu", index).getData(),
				action.data(),
				&C4ToastImplLibNotify::OnAction,
				new UserData{this, index},
				operator delete);
}

void C4ToastImplLibNotify::SetExpiration(std::uint32_t expiration)
{
	notify_notification_set_timeout(notification, expiration * 1000u);
}

void C4ToastImplLibNotify::SetText(std::string_view text)
{
	this->text = text;
}

void C4ToastImplLibNotify::SetTitle(std::string_view title)
{
	this->title = title;
}

void C4ToastImplLibNotify::Show()
{
	signalClose = g_signal_connect(notification, "closed", G_CALLBACK(&C4ToastImplLibNotify::Dismissed), this);
	notify_notification_update(notification, title.c_str(), text.c_str(), nullptr);

	if (GError *error{nullptr}; !notify_notification_show(notification, &error) && eventHandler)
	{
		if (error)
		{
			LogF("C4ToastImplLibNotify: Failed to show notification: %s", error->message);
		}
		eventHandler->Failed();
	}
}

void C4ToastImplLibNotify::Hide()
{
	g_signal_handler_disconnect(notification, signalClose);
	if (GError *error{nullptr}; !notify_notification_close(notification, &error) && error)
	{
		LogF("C4ToastImplLibNotify: Failed to hide notification: %s", error->message);
	}
}

void C4ToastImplLibNotify::Activated(NotifyNotification *, char *, gpointer userData)
{
	if (auto *const toast = reinterpret_cast<C4ToastImplLibNotify *>(userData); toast->eventHandler)
	{
		toast->eventHandler->Activated();
	}
}

void C4ToastImplLibNotify::OnAction(NotifyNotification *, char *, gpointer userData)
{
	if (auto [toast, index] = *reinterpret_cast<UserData *>(userData); toast->eventHandler)
	{
		toast->eventHandler->OnAction(toast->actions.at(index));
	}
}

void C4ToastImplLibNotify::Dismissed(NotifyNotification *, gpointer userData)
{
	if (auto *const toast = reinterpret_cast<C4ToastImplLibNotify *>(userData); toast->eventHandler)
	{
		toast->eventHandler->Dismissed();
	}
}
