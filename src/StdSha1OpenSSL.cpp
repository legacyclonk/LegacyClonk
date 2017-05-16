/*
 * LegacyClonk
 *
 * Copyright (c) 2017, The LegacyClonk Team and contributors
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

// SHA-1 calculation using OpenSSL

#include <Standard.h>
#include <StdSha1.h>

#include <openssl/sha.h>

#include <stdexcept>

struct StdSha1::Impl
{
	bool isCtxValid;
	SHA_CTX ctx;

	Impl()
	{
		Init();
		isCtxValid = true;
	}

	~Impl()
	{
		ClearNoExcept();
	}

	void Init()
	{
		if (!SHA1_Init(&ctx)) throw std::runtime_error("SHA1_Init failed");
	}

	void Clear()
	{
		if (isCtxValid) GetHash(nullptr);
	}

	void ClearNoExcept() noexcept
	{
		try
		{
			Clear();
		}
		catch (const std::runtime_error &)
		{
		}
	}

	void Update(const void *const buffer, const std::size_t len)
	{
		if (!SHA1_Update(&ctx, buffer, len)) throw std::runtime_error("SHA1_Update");
	}

	void GetHash(void *const result)
	{
		const bool success = (SHA1_Final(static_cast<unsigned char *>(result), &ctx) == 1);
		isCtxValid = false;
		if (!success) throw std::runtime_error("SHA1_Final failed");
	}

	void Reset()
	{
		Clear();
		Init();
	}
};

StdSha1::StdSha1() : impl(new Impl()) {}
StdSha1::~StdSha1() = default;

void StdSha1::Update(const void *const buffer, const std::size_t len) { impl->Update(buffer, len); }
void StdSha1::GetHash(void *const result) { impl->GetHash(result); }
void StdSha1::Reset() { impl->Reset(); }
