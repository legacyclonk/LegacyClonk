/*
 * LegacyClonk
 * Copyright (c) 2013-2016, The OpenClonk Team and contributors
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

#include "C4Awaiter.h"
#include "C4HTTPClient.h"
#include "C4Version.h"

#include <format>

#define CURL_STRICTER
#include <curl/curl.h>

static constexpr long C4HTTPQueryTimeout{20}; // (s)

template<typename T, typename... Args> requires (sizeof...(Args) >= 1)
static decltype(auto) ThrowIfFailed(T &&result, Args &&...args)
{
	if (!result)
	{
		if constexpr (sizeof...(Args) == 1)
		{
			throw C4HTTPClient::Exception{std::forward<Args>(args)...};
		}
		else
		{
			throw C4HTTPClient::Exception{std::format(std::forward<Args>(args)...)};
		}
	}

	return std::forward<T>(result);
}

void C4HTTPClient::Uri::CURLUDeleter::operator()(CURLU * const uri)
{
	if (uri)
	{
		curl_url_cleanup(uri);
	}
}

C4HTTPClient::Uri::Uri(const std::string &serverAddress, const std::uint16_t port)
	: uri{ThrowIfFailed(curl_url(), "curl_url failed")}
{
	ThrowIfFailed(curl_url_set(uri.get(), CURLUPART_URL, serverAddress.c_str(), CURLU_DEFAULT_SCHEME) == CURLUE_OK, "malformed URL");

	if (port > 0)
	{
		char *portString;
		const auto errGetPort = curl_url_get(uri.get(), CURLUPART_PORT, &portString, 0);
		curl_free(portString);
		if (errGetPort == CURLUE_NO_PORT)
		{
			ThrowIfFailed(curl_url_set(uri.get(), CURLUPART_PORT, std::to_string(port).c_str(), 0) == CURLUE_OK, "curl_url_set PORT failed");
		}
		else
		{
			ThrowIfFailed(errGetPort == CURLUE_OK, "curl_url_get PORT failed");
		}
	}
}

std::string C4HTTPClient::Uri::GetServerAddress() const
{
	char *host{nullptr};
	ThrowIfFailed(curl_url_get(uri.get(), CURLUPART_HOST, &host, 0) == CURLUE_OK, "curl_url_get HOST failed");

	const std::unique_ptr<char, decltype([](char *const ptr) { curl_free(ptr); })> ptr{host};
	return host;
}

std::string C4HTTPClient::Uri::GetUriAsString() const
{
	char *string{nullptr};
	ThrowIfFailed(curl_url_get(uri.get(), CURLUPART_URL, &string, 0) == CURLUE_OK, "curl_url_get URL failed");

	const std::unique_ptr<char, decltype([](char *const ptr) { curl_free(ptr); })> ptr{string};
	return string;
}

void C4HTTPClient::CURLMultiDeleter::operator()(CURLM *const multi)
{
	if (multi)
	{
		curl_multi_cleanup(multi);
	}
}

void C4HTTPClient::CURLEasyDeleter::operator()(CURL *const easy)
{
	if (easy)
	{
		curl_easy_cleanup(easy);
	}
}

C4HTTPClient::CURLEasyAddedHandle::CURLEasyAddedHandle(C4HTTPClient &client, CURLEasy &&easy)
	: client{client}, easy{std::move(easy)}
{
	char *buffer;
	{
		const std::lock_guard lock{client.errorMessageMutex};

		const auto [it, success] = client.errorMessages.emplace(this->easy.get(), std::string{});
		ThrowIfFailed(success, "could not add error message buffer");

		it->second.resize(CURL_ERROR_SIZE);
		buffer = it->second.data();
	}

	curl_easy_setopt(this->easy.get(), CURLOPT_ERRORBUFFER, buffer);

	ThrowIfFailed(curl_multi_add_handle(client.multiHandle.get(), this->easy.get()) == CURLM_OK, "curl_multi_add_handle");

	const std::lock_guard lock{client.socketMapMutex};
	ThrowIfFailed(client.sockets.try_emplace(this->easy.get(), std::unordered_map<SOCKET, int>{}).second, "already added");
}

C4HTTPClient::CURLEasyAddedHandle::~CURLEasyAddedHandle()
{
	{
		const std::lock_guard lock{client.socketMapMutex};
		client.sockets.erase(easy.get());
	}

	{
		const std::lock_guard lock{client.errorMessageMutex};
		client.errorMessages.erase(easy.get());
	}

	curl_multi_remove_handle(client.multiHandle.get(), easy.get());
}

C4HTTPClient::C4HTTPClient()
	: multiHandle{ThrowIfFailed(curl_multi_init(), "curl_multi_init failed")}
{
	curl_multi_setopt(multiHandle.get(), CURLMOPT_SOCKETFUNCTION, &C4HTTPClient::SocketFunction);
	curl_multi_setopt(multiHandle.get(), CURLMOPT_SOCKETDATA, this);

	curl_multi_setopt(multiHandle.get(), CURLMOPT_TIMERFUNCTION, &C4HTTPClient::TimerFunction);
	curl_multi_setopt(multiHandle.get(), CURLMOPT_TIMERDATA, this);
}

C4Task::Hot<C4HTTPClient::Result> C4HTTPClient::GetAsync(Request request, ProgressCallback &&progressCallback, Headers headers)
{
	return RequestAsync(std::move(request), std::move(progressCallback), std::move(headers), false);
}

C4Task::Hot<C4HTTPClient::Result> C4HTTPClient::PostAsync(Request request, ProgressCallback &&progressCallback, Headers headers)
{
	return RequestAsync(std::move(request), std::move(progressCallback), std::move(headers), true);
}

C4Task::Hot<C4HTTPClient::Result> C4HTTPClient::RequestAsync(Request request, const ProgressCallback progressCallback, Headers headers, const bool post)
{
	auto curl = PrepareRequest(request, std::move(headers), post);

	StdBuf result;

	curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, &C4HTTPClient::WriteFunction);
	curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &result);

	if (progressCallback)
	{
		curl_easy_setopt(curl.get(), CURLOPT_XFERINFOFUNCTION, &C4HTTPClient::WriteFunction);
		curl_easy_setopt(curl.get(), CURLOPT_XFERINFODATA, &progressCallback);
	}

	const CURLEasyAddedHandle addedEasy{*this, std::move(curl)};
	int running{0};

	curl_multi_socket_action(multiHandle.get(), CURL_SOCKET_TIMEOUT, 0, &running);

	for (;;)
	{
#ifdef _WIN32
		if (co_await C4Awaiter::ResumeOnSignal(event.GetEvent(), timeout.load(std::memory_order_acquire)))
		{
			// copy map to prevent crashes
			const auto localSockets = GetSocketMapForEasy(addedEasy.get());

			for (const auto [socket, what] : localSockets)
			{
				if (WSANETWORKEVENTS networkEvents; !WSAEnumNetworkEvents(socket, event.GetEvent(), &networkEvents))
				{
					int eventBitmask{0};
					if (networkEvents.lNetworkEvents & (FD_READ | FD_ACCEPT | FD_CLOSE))
					{
						eventBitmask |= CURL_CSELECT_IN;
					}

					if (networkEvents.lNetworkEvents & (FD_WRITE | FD_CONNECT))
					{
						eventBitmask |= CURL_CSELECT_OUT;
					}

					curl_multi_socket_action(multiHandle.get(), socket, eventBitmask, &running);
				}
			}
		}
#else
		const auto events = co_await C4Awaiter::ResumeOnSignals(
			GetSocketMapForEasy(addedEasy.get()) | std::views::transform([](const auto &pair) { return pollfd{.fd = pair.first, .events = static_cast<short>(pair.second)}; }),
			timeout.load(std::memory_order_acquire)
			);

		if (!events.empty())
		{
			for (const auto event : events)
			{
				int eventBitmask{0};
				if (event.revents & POLLIN)
				{
					eventBitmask |= CURL_CSELECT_IN;
				}

				if (event.revents & POLLOUT)
				{
					eventBitmask |= CURL_CSELECT_OUT;
				}

				curl_multi_socket_action(multiHandle.get(), event.fd, eventBitmask, &running);
			}
		}
#endif
		else
		{
			curl_multi_socket_action(multiHandle.get(), CURL_SOCKET_TIMEOUT, 0, &running);
		}

		if (!ProcessMessages(addedEasy))
		{
			if (std::string errorMessage{RemoveEasy(addedEasy)}; errorMessage.empty())
			{
				char *ip;
				ThrowIfFailed(curl_easy_getinfo(addedEasy.get(), CURLINFO_PRIMARY_IP, &ip) == CURLE_OK, "curl_easy_getinfo(CURLINFO_PRIMARY_IP) failed");

				C4NetIO::addr_t serverAddress;
				serverAddress.SetHost(StdStrBuf{ip});
				co_return {std::move(result), serverAddress};
			}
			else
			{
				throw Exception{std::move(errorMessage)};
			}
		}
	}
}

C4HTTPClient::CURLEasy C4HTTPClient::PrepareRequest(const Request &request, Headers headers, const bool post)
{
	CURLEasy curl{ThrowIfFailed(curl_easy_init(), "curl_easy_init failed")};

	curl_easy_setopt(curl.get(), CURLOPT_CURLU, request.Uri.get());
	curl_easy_setopt(curl.get(), CURLOPT_ACCEPT_ENCODING, "gzip");
	curl_easy_setopt(curl.get(), CURLOPT_USERAGENT, C4ENGINENAME "/" C4VERSION );
	curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, C4HTTPQueryTimeout);
	curl_easy_setopt(curl.get(), CURLOPT_FAILONERROR, 1L);

	const char *const charset{GetCharsetCodeName(Config.General.LanguageCharset)};

	headers["Accept-Charset"] = charset;
	headers["Accept-Language"] = Config.General.LanguageEx;
	headers.erase("User-Agent");

	curl_slist *headerList{nullptr};

	if (post)
	{
		curl_easy_setopt(curl.get(), CURLOPT_POST, 1L);
		curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, request.Data.data());
		curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE, static_cast<long>(request.Data.size()));

		if (!headers.count("Content-Type"))
		{
			static constexpr auto defaultType = "text/plain; encoding=";
			headerList = curl_slist_append(headerList, std::format("Content-Type: {}{}", defaultType, charset).c_str());
		}

			   // Disable the Expect: 100-Continue header which curl automaticallY
			   // adds for POST requests.
		headerList = curl_slist_append(headerList, "Expect:");
	}

	for (const auto &[key, value] : headers)
	{
		std::string header{key};
		header += ": ";
		header += value;
		headerList = curl_slist_append(headerList, header.c_str());
	}

	curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headerList);

	return curl;
}

bool C4HTTPClient::ShouldContinue(const CURLEasyAddedHandle &easy)
{
	const std::lock_guard lock{socketMapMutex};
	return sockets.contains(easy.get());
}

std::string C4HTTPClient::RemoveEasy(const CURLEasyAddedHandle &easy)
{
	const std::scoped_lock lock{socketMapMutex, errorMessageMutex};
	sockets.erase(easy.get());

	std::string errorMessage{std::move(errorMessages.extract(easy.get()).mapped())};
	errorMessage.resize(std::strlen(errorMessage.c_str()));
	return errorMessage;
}

std::unordered_map<SOCKET, int> C4HTTPClient::GetSocketMapForEasy(CURL *const curl)
{
	const std::lock_guard lock{socketMapMutex};

	if (const auto it = sockets.find(curl); it != sockets.end())
	{
		return it->second;
	}
	else
	{
		return {};
	}
}

bool C4HTTPClient::ProcessMessages(const CURLEasyAddedHandle &easy)
{
	const std::unique_lock lock{messageProcessorMutex, std::try_to_lock};
	if (!ShouldContinue(easy))
	{
		return false;
	}

	if (!lock)
	{
		return true;
	}

	CURLMsg *message;
	bool shouldContinue{true};
	do
	{
		int messagesInQueue{0};
		message = curl_multi_info_read(multiHandle.get(), &messagesInQueue);

		if (message && message->msg == CURLMSG_DONE)
		{
			{
				const std::lock_guard errorMessageLock{errorMessageMutex};
				if (message->data.result != CURLE_OK)
				{
					if (const auto it = errorMessages.find(message->easy_handle); it != errorMessages.end() && it->second.empty())
					{
						it->second = curl_easy_strerror(message->data.result);
					}
				}

				if (message->easy_handle == easy.get())
				{
					shouldContinue = false;
				}
			}

			const std::lock_guard socketLock{socketMapMutex};
			sockets.erase(easy.get());
		}
	}
	while (message);

	return shouldContinue;
}

int C4HTTPClient::SocketFunction(CURL *const curl, const curl_socket_t s, const int what, void *const userData)
{
	auto &that = *reinterpret_cast<C4HTTPClient *>(userData);

	std::int32_t networkEvents;
#ifdef _WIN32
	static constexpr long NetworkEventsIn{FD_READ | FD_ACCEPT | FD_CLOSE};
	static constexpr long NetworkEventsOut{FD_WRITE | FD_CONNECT};
#else
	static constexpr std::int32_t NetworkEventsIn{POLLIN};
	static constexpr std::int32_t NetworkEventsOut{POLLOUT};
#endif

	switch (what)
	{
	case CURL_POLL_IN:
		networkEvents = NetworkEventsIn;
		break;

	case CURL_POLL_OUT:
		networkEvents = NetworkEventsOut;
		break;

	case CURL_POLL_INOUT:
		networkEvents = NetworkEventsIn | NetworkEventsOut;
		break;

	default:
		networkEvents = 0;
		break;
	}

#ifdef _WIN32
	if (WSAEventSelect(s, that.event.GetEvent(), networkEvents) == SOCKET_ERROR)
	{
		return CURL_SOCKOPT_ERROR;
	}
#endif

	const std::lock_guard lock{that.socketMapMutex};

	if (what == CURL_POLL_REMOVE)
	{
		if (const auto it = that.sockets.find(curl); it != that.sockets.end())
		{
			it->second.erase(s);
		}
	}
	else
	{
		that.sockets.find(curl)->second.insert_or_assign(s, networkEvents);
	}

	return 0;
}

int C4HTTPClient::TimerFunction(CURLM *, const long timeout, void *const userData)
{
	reinterpret_cast<C4HTTPClient *>(userData)->timeout.store(timeout >= 0 ? static_cast<int32_t>(timeout) : 1000, std::memory_order_release);
	return 0;
}

std::size_t C4HTTPClient::WriteFunction(char *const ptr, std::size_t, const std::size_t nmemb, void *const userData)
{
	auto &buf = *reinterpret_cast<StdBuf *>(userData);

	const std::size_t oldSize{buf.getSize()};
	buf.Append(ptr, nmemb);
	return buf.getSize() - oldSize;
}

int C4HTTPClient::XferInfoFunction(void *const userData, const std::int64_t downloadTotal, const std::int64_t downloadNow, std::int64_t, std::int64_t)
{
	const auto &callback = *reinterpret_cast<ProgressCallback *>(userData);
	if (callback && !callback(downloadTotal, downloadNow))
	{
		return 1;
	}

	return CURL_PROGRESSFUNC_CONTINUE;
}
