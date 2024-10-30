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

#include "C4TextEncoding.h"
#include "C4ToastLibNotify.h"

#include "C4Language.h"
#include "C4Log.h"
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

std::unique_ptr<C4Toast> C4ToastSystemLibNotify::CreateToast()
{
	return std::make_unique<C4ToastLibNotify>();
}

C4ToastLibNotify::C4ToastLibNotify() : C4Toast{}, notification{notify_notification_new("", "", "")}
{
	notify_notification_add_action(notification, "default", "Default", &C4ToastLibNotify::Activated, this, nullptr);
}

C4ToastLibNotify::~C4ToastLibNotify()
{
	g_object_unref(G_OBJECT(notification));
}

void C4ToastLibNotify::AddAction(std::string_view action)
{
	const size_t index{actions.size()};
	actions.emplace_back(action);

	notify_notification_add_action(
		notification,
		std::format("action-{}", index).c_str(),
		TextEncodingConverter.ClonkToUtf8<char>(action).c_str(),
		&C4ToastLibNotify::OnAction,
		new UserData{this, index},
		operator delete
	);
}

void C4ToastLibNotify::SetExpiration(std::uint32_t expiration)
{
	notify_notification_set_timeout(notification, expiration * 1000u);
}

void C4ToastLibNotify::SetText(std::string_view text)
{
	this->text = text;
}

void C4ToastLibNotify::SetTitle(std::string_view title)
{
	this->title = title;
}

void C4ToastLibNotify::Show()
{
	signalClose = g_signal_connect(notification, "closed", G_CALLBACK(&C4ToastLibNotify::Dismissed), this);
	notify_notification_update(notification, TextEncodingConverter.ClonkToUtf8<char>(title).c_str(), TextEncodingConverter.ClonkToUtf8<char>(text).c_str(), nullptr);

	if (GError *error{nullptr}; !notify_notification_show(notification, &error) && eventHandler)
	{
		if (error)
		{
			LogNTr(spdlog::level::err, "C4ToastLibNotify: Failed to show notification: {}", TextEncodingConverter.Utf8ToClonk(title).c_str());
		}
		eventHandler->Failed();
	}
}

void C4ToastLibNotify::Hide()
{
	g_signal_handler_disconnect(notification, signalClose);
	if (GError *error{nullptr}; !notify_notification_close(notification, &error) && error)
	{
		LogNTr(spdlog::level::err, "C4ToastLibNotify: Failed to hide notification: {}", error->message);
	}
}

void C4ToastLibNotify::Activated(NotifyNotification *, char *, gpointer userData)
{
	if (auto *const toast = reinterpret_cast<C4ToastLibNotify *>(userData); toast->eventHandler)
	{
		toast->eventHandler->Activated();
	}
}

void C4ToastLibNotify::OnAction(NotifyNotification *, char *, gpointer userData)
{
	if (auto [toast, index] = *reinterpret_cast<UserData *>(userData); toast->eventHandler)
	{
		toast->eventHandler->OnAction(toast->actions.at(index));
	}
}

void C4ToastLibNotify::Dismissed(NotifyNotification *, gpointer userData)
{
	if (auto *const toast = reinterpret_cast<C4ToastLibNotify *>(userData); toast->eventHandler)
	{
		toast->eventHandler->Dismissed();
	}
}
