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

#pragma once

#include "C4Coroutine.h"
#include "C4Network2.h"
#include "StdBuf.h"
#include "StdSync.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <span>
#include <stdexcept>
#include <string>
#include <unordered_map>

using CURLM = struct Curl_multi;
using CURL = struct Curl_easy;
using curl_socket_t = SOCKET;
using CURLU = struct Curl_URL;

class C4HTTPClient
{
public:
	using Headers = std::unordered_map<std::string_view, std::string_view>;
	using Notify = std::function<void(C4Network2HTTPClient *)>;
	using ProgressCallback = std::function<bool(std::int64_t, std::int64_t)>;

	class Exception : public std::runtime_error
	{
		using runtime_error::runtime_error;
	};

	class Uri
	{
	private:
		struct CURLUDeleter
		{
			void operator()(CURLU *const uri);
		};

	public:
		Uri(const std::string &serverAddress, std::uint16_t port = 0);

	public:
		std::string GetServerAddress() const;
		std::string GetUriAsString() const;

		auto get() const { return uri.get(); }

	private:
		std::unique_ptr<CURLU, CURLUDeleter> uri;
	};

	struct Request
	{
		C4HTTPClient::Uri Uri;
		bool Binary{false};
		std::span<const std::byte> Data;
	};

	struct Result
	{
		StdBuf Buffer;
		C4NetIO::addr_t ServerAddress;
	};

private:
	struct CURLMultiDeleter
	{
		void operator()(CURLM *multi);
	};

	struct CURLEasyDeleter
	{
		void operator()(CURL *easy);
	};

	using CURLMulti = std::unique_ptr<CURLM, CURLMultiDeleter>;
	using CURLEasy = std::unique_ptr<CURL, CURLEasyDeleter>;

	struct CURLEasyAddedHandle
	{
	public:
		CURLEasyAddedHandle(C4HTTPClient &client, CURLEasy &&easy);
		~CURLEasyAddedHandle();

	public:
		auto get() const { return easy.get(); }

	private:
		C4HTTPClient &client;
		CURLEasy easy;
	};

public:
	C4HTTPClient();

public:
	C4Task::Hot<Result> GetAsync(Request request, ProgressCallback &&progressCallback = {}, Headers headers = {});
	C4Task::Hot<Result> PostAsync(Request request, ProgressCallback &&progressCallback = {}, Headers headers = {});

private:
	C4Task::Hot<Result> RequestAsync(Request request, ProgressCallback progressCallback, Headers headers, bool post);
	CURLEasy PrepareRequest(const Request &request, Headers headers, bool post);

	bool ShouldContinue(const CURLEasyAddedHandle &easy);
	std::string RemoveEasy(const CURLEasyAddedHandle &easy);
	std::unordered_map<SOCKET, int> GetSocketMapForEasy(CURL *const curl);
	bool ProcessMessages(const CURLEasyAddedHandle &easy);

	static int SocketFunction(CURL *curl, curl_socket_t s, int what, void *userData);
	static int TimerFunction(CURLM *, long timeout, void *userData);
	static std::size_t WriteFunction(char *ptr, std::size_t, std::size_t nmemb, void *userData);
	static int XferInfoFunction(void *userData, std::int64_t downloadTotal, std::int64_t downloadNow, std::int64_t, std::int64_t);


private:
	CURLMulti multiHandle;
	std::atomic_uint32_t timeout{StdSync::Infinite};
	std::uint16_t defaultPort{0};

#ifdef _WIN32
	// event indicating network activity
	CStdEvent event;
#endif
	std::mutex socketMapMutex;
	std::unordered_map<CURL *, std::unordered_map<SOCKET, int>> sockets;

	std::mutex messageProcessorMutex;

	std::mutex errorMessageMutex;
	std::unordered_map<CURL *, std::string> errorMessages;
};
