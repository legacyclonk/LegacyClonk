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

#include "C4Application.h"
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

void C4HTTPClient::CURLSDeleter::operator()(CURLS *const share)
{
	curl_share_cleanup(share);
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

C4HTTPClient::C4HTTPClient()
	: shareHandle{ThrowIfFailed(curl_share_init(), "curl_share_init failed")}
{
	curl_share_setopt(shareHandle.get(), CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE);
	curl_share_setopt(shareHandle.get(), CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION);
	curl_share_setopt(shareHandle.get(), CURLSHOPT_LOCKFUNC, &C4HTTPClient::LockFunction);
	curl_share_setopt(shareHandle.get(), CURLSHOPT_UNLOCKFUNC, &C4HTTPClient::UnlockFunction);
	curl_share_setopt(shareHandle.get(), CURLSHOPT_USERDATA, &shareMutexes);
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
		curl_easy_setopt(curl.get(), CURLOPT_NOPROGRESS, 0L);
		curl_easy_setopt(curl.get(), CURLOPT_XFERINFOFUNCTION, &C4HTTPClient::XferInfoFunction);
		curl_easy_setopt(curl.get(), CURLOPT_XFERINFODATA, &progressCallback);
	}

	try
	{
		auto awaitResult = co_await Application.CurlSystem->WaitForEasyAsync(std::move(curl));
		co_return {std::move(result), std::move(awaitResult)};
	}
	catch (const C4CurlSystem::Exception &e)
	{
		throw Exception{e.what()};
	}
}

C4CurlSystem::EasyHandle C4HTTPClient::PrepareRequest(const Request &request, Headers headers, const bool post)
{
	C4CurlSystem::EasyHandle curl{ThrowIfFailed(curl_easy_init(), "curl_easy_init failed")};

	curl_easy_setopt(curl.get(), CURLOPT_CURLU, request.Uri.get());
	curl_easy_setopt(curl.get(), CURLOPT_ACCEPT_ENCODING, "gzip");
	curl_easy_setopt(curl.get(), CURLOPT_USERAGENT, C4ENGINENAME "/" C4VERSION );
	curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, C4HTTPQueryTimeout);
	curl_easy_setopt(curl.get(), CURLOPT_FAILONERROR, 1L);
	curl_easy_setopt(curl.get(), CURLOPT_COOKIEFILE, "");
	curl_easy_setopt(curl.get(), CURLOPT_SHARE, shareHandle.get());

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

void C4HTTPClient::LockFunction(CURL *, const int data, const int access, void *const userData)
{
	static_assert(sizeof(curl_lock_data) == sizeof(int) && alignof(curl_lock_data) == alignof(int));
	static_assert(sizeof(curl_lock_access) == sizeof(int) && alignof(curl_lock_access) == alignof(int));
	static_assert(std::tuple_size_v<decltype(C4HTTPClient::shareMutexes)> == CURL_LOCK_DATA_LAST + 1, "Invalid mutex array size");

	auto &[mutex, exclusive] = (*reinterpret_cast<decltype(C4HTTPClient::shareMutexes) *>(userData))[data];

	switch (static_cast<curl_lock_access>(access))
	{
	case CURL_LOCK_ACCESS_SHARED:
		mutex.lock_shared();
		exclusive = false;
		break;

	case CURL_LOCK_ACCESS_SINGLE:
		mutex.lock();
		exclusive = true;
		break;

	default:
		assert(false);
		std::unreachable();
	}
}

void C4HTTPClient::UnlockFunction(CURL *, const int data, void *const userData)
{
	auto &[mutex, exclusive] = (*reinterpret_cast<decltype(C4HTTPClient::shareMutexes) *>(userData))[data];

	if (exclusive)
	{
		mutex.unlock();
	}
	else
	{
		mutex.unlock_shared();
	}
}
