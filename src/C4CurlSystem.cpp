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

#include "C4CurlSystem.h"
#include "C4Log.h"
#include "StdApp.h"
#include "StdResStr2.h"

#include <format>
#include <utility>

#define CURL_STRICTER
#include <curl/curl.h>

C4CurlSystem::C4CurlSystem()
{
	if (const auto ret = curl_global_init(CURL_GLOBAL_ALL); ret != CURLE_OK)
	{
		std::string message{std::vformat(LoadResStr("IDS_ERR_CURLGLOBALINIT"), std::make_format_args(curl_easy_strerror(ret)))};
		LogF("%s", message.c_str());
		throw CStdApp::StartupException{std::move(message)};
	}
}

C4CurlSystem::~C4CurlSystem()
{
	curl_global_cleanup();
}
