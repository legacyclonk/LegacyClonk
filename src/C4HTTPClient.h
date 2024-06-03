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
#include "C4CurlSystem.h"
#include "C4Network2Address.h"
#include "StdBuf.h"
#include "StdSync.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <span>
#include <stdexcept>
#include <string>
#include <unordered_map>

using CURLM = struct Curl_multi;
using CURL = struct Curl_easy;
using curl_socket_t = SOCKET;
using CURLU = struct Curl_URL;
using CURLS = struct Curl_share;

class C4HTTPClient
{
private:
	struct CURLSDeleter
	{
		void operator()(CURLS *share);
	};

public:
	using Headers = std::unordered_map<std::string_view, std::string_view>;
	using ProgressCallback = std::function<bool(std::int64_t, std::int64_t)>;

	class Exception : public C4CurlSystem::Exception
	{
		using C4CurlSystem::Exception::Exception;
	};

	class Uri
	{
	public:
		enum class Part
		{
			Uri,
			Scheme,
			User,
			Password,
			Options,
			Host,
			Port,
			Path,
			Query,
			Fragment,
			ZoneId
		};

		class String
		{
		public:
			String() = default;
			~String();

			String(const String &) = delete;
			String &operator=(const String &) = delete;
			String(String &&other) noexcept;
			String &operator=(String &&other) noexcept;

		public:
			char **operator &() noexcept { return &ptr; }
			operator std::string() && { return ptr; }
			operator StdStrBuf() && { return StdStrBuf{ptr}; }

		private:
			char *ptr{nullptr};
		};

	private:
		struct CURLUDeleter
		{
			void operator()(CURLU *const uri);
		};

	public:
		Uri(const std::string &serverAddress, std::uint16_t port = 0);

	private:
		Uri(std::unique_ptr<CURLU, CURLUDeleter> uri, std::uint16_t port = 0);

	public:
		[[nodiscard]] String GetPart(Part part) const;

		auto get() const { return uri.get(); }

	public:
		[[nodiscard]] static Uri ParseOldStyle(const std::string &serverAddress, std::uint16_t port = 0);

	private:
		void SetPort(std::uint16_t port);

	private:
		std::unique_ptr<CURLU, CURLUDeleter> uri;
	};

	struct Request
	{
		C4HTTPClient::Uri Uri;
		bool Binary{false};
		std::span<const std::byte> Data{};
	};

	struct Result
	{
		StdBuf Buffer;
		C4NetIO::addr_t ServerAddress;
	};

public:
	C4HTTPClient();

public:
	C4Task::Hot<Result> GetAsync(Request request, ProgressCallback &&progressCallback = {}, Headers headers = {});
	C4Task::Hot<Result> PostAsync(Request request, ProgressCallback &&progressCallback = {}, Headers headers = {});

private:
	C4Task::Hot<Result> RequestAsync(Request request, ProgressCallback progressCallback, Headers headers, bool post);
	C4CurlSystem::EasyHandle PrepareRequest(const Request &request, Headers headers, bool post);

	static std::size_t WriteFunction(char *ptr, std::size_t, std::size_t nmemb, void *userData);
	static int XferInfoFunction(void *userData, std::int64_t downloadTotal, std::int64_t downloadNow, std::int64_t, std::int64_t);
	static void LockFunction(CURL *, int data, int access, void *userData);
	static void UnlockFunction(CURL *, int data, void *userData);

private:
	std::array<std::pair<std::shared_mutex, bool>, 9> shareMutexes{};
	std::unique_ptr<CURLS, CURLSDeleter> shareHandle;
};
