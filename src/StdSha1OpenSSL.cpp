/*
 * LegacyClonk
 *
 * Copyright (c) 2017-2019, The LegacyClonk Team and contributors
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
#include <StdHelpers.h>
#include <StdSha1.h>

#include <openssl/err.h>
#include <openssl/evp.h>

#include <stdexcept>

struct StdSha1::Impl
{
	inline static C4DeleterFunctionUniquePtr<EVP_MD_free> sha1Digest{EVP_MD_fetch(nullptr, "SHA1", nullptr)};

	bool isCtxValid;
	C4DeleterFunctionUniquePtr<EVP_MD_CTX_free> ctx{EVP_MD_CTX_new()};

	Impl()
	{
		Init();
	}

	~Impl()
	{
		ClearNoExcept();
	}

	void Init()
	{
		ThrowIfFailed("EVP_MD_fetch for SHA1", sha1Digest != nullptr);
		ThrowIfFailed("EVP_DigestInit_ex", EVP_DigestInit_ex(ctx.get(), sha1Digest.get(), nullptr));
		isCtxValid = true;
	}

	void Clear()
	{
		if (isCtxValid)
		{
			ThrowIfFailed("EVP_MD_CTX_reset", EVP_MD_CTX_reset(ctx.get()));
			isCtxValid = false;
		}
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
		ThrowIfFailed("EVP_DigestUpdate", EVP_DigestUpdate(ctx.get(), buffer, len));
	}

	void GetHash(void *const result)
	{
		ThrowIfFailed("EVP_DigestFinal_ex", EVP_DigestFinal_ex(ctx.get(), static_cast<unsigned char *>(result), nullptr));
	}

	void Reset()
	{
		Clear();
		Init();
	}

	void ThrowIfFailed(const std::string_view function, const int result) const
	{
		ThrowIfFailed(function, result == 1);
	}

	void ThrowIfFailed(const std::string_view function, const bool success) const
	{
		if (success)
		{
			return;
		}

		std::string message{function};
		message += " failed: ";

		C4DeleterFunctionUniquePtr<BIO_free> bio{BIO_new(BIO_s_mem())};
		if (!bio)
		{
			throw std::runtime_error{message + "BIO_new failed too; Further error information is unavailable"};
		}

		ERR_print_errors(bio.get());

		char* data;
		std::size_t len = BIO_get_mem_data(bio.get(), &data);
		message += std::string_view{data, len};

		throw std::runtime_error{message};
	}
};

StdSha1::StdSha1() : impl(new Impl()) {}
StdSha1::~StdSha1() = default;

void StdSha1::Update(const void *const buffer, const std::size_t len) { impl->Update(buffer, len); }
void StdSha1::GetHash(void *const result) { impl->GetHash(result); }
void StdSha1::Reset() { impl->Reset(); }
