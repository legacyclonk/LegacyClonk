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

// SHA-1 calculation using Microsoft CryptoAPI

#include <Standard.h>
#include <StdSha1.h>

#include <wincrypt.h>

#include <stdexcept>
#include <string>

struct StdSha1::Impl
{
	bool isHashValid;
	HCRYPTPROV hCryptProv;
	HCRYPTHASH hHash;

	Impl()
	{
		AcquireContext();

		try
		{
			CreateHash();
		}
		catch (const std::runtime_error &)
		{
			ReleaseContext();
			throw;
		}
	}

	~Impl()
	{
		try
		{
			if (isHashValid) DestroyHash();
		}
		catch (const std::runtime_error &)
		{
		}
		ReleaseContext();
	}

	void AcquireContext()
	{
		ThrowIfFailed(
			CryptAcquireContext(&hCryptProv, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT),
			"CryptAcquireContext failed");
	}

	void ReleaseContext() noexcept
	{
		CryptReleaseContext(hCryptProv, 0);
	}

	void CreateHash()
	{
		ThrowIfFailed(CryptCreateHash(hCryptProv, CALG_SHA1, 0, 0, &hHash),
			"CryptCreateHash failed");
		isHashValid = true;
	}

	void DestroyHash()
	{
		isHashValid = false;
		ThrowIfFailed(CryptDestroyHash(hHash), "CryptDestroyHash failed");
	}

	void Update(const void *const buffer, const size_t len)
	{
		ThrowIfFailed(CryptHashData(hHash, static_cast<const BYTE *>(buffer), checked_cast<DWORD>(len), 0),
			"CryptHashData failed");
	}

	void GetHash(void *const result)
	{
		DWORD digestLen = DigestLength;
		ThrowIfFailed(
			CryptGetHashParam(hHash, HP_HASHVAL, static_cast<BYTE *>(result), &digestLen, 0),
			"CryptGetHashParam failed");
	}

	void Reset()
	{
		DestroyHash();
		CreateHash();
	}

	void ThrowIfFailed(const BOOL succeeded, const char *const msg)
	{
		if (!succeeded)
		{
			const auto errorNumber = GetLastError();
			throw std::runtime_error(std::string() +
				msg + " (error " + std::to_string(errorNumber) + ")");
		}
	}
};

StdSha1::StdSha1() : impl(new Impl()) {}
StdSha1::~StdSha1() = default;

void StdSha1::Update(const void *const buffer, const size_t len) { impl->Update(buffer, len); }
void StdSha1::GetHash(void *const result) { impl->GetHash(result); }
void StdSha1::Reset() { impl->Reset(); }
