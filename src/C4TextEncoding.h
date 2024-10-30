/*
 * LegacyClonk
 *
 * Copyright (c) 2024, The LegacyClonk Team and contributors
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

#include <cstring>
#include <string>
#include <string_view>

#ifdef _WIN32
#include "StdStringEncodingConverter.h"
#elif defined(HAVE_ICONV)
#include <functional>
#include <mutex>
#include <utility>

#include <iconv.h>
#endif

enum class C4TextEncoding
{
	Ansi,
	Clonk = Ansi,
#ifdef _WIN32
	System = Ansi,
#endif
	Utf8,
#ifdef _WIN32
	Utf16,
#else
	System = Utf8,
#endif
};


class C4TextEncodingConverter
{
#ifdef HAVE_ICONV
private:
	class IconvPtr
	{
	public:
		IconvPtr() = default;
		IconvPtr(iconv_t ptr) noexcept : ptr{ptr} {}

		IconvPtr(const IconvPtr &ptr) = delete;

		IconvPtr &operator=(const IconvPtr &ptr) = delete;
		IconvPtr(IconvPtr &&ptr) noexcept : ptr{std::exchange(ptr.ptr, Invalid())} {}
		IconvPtr &operator=(IconvPtr &&ptr) noexcept
		{
			reset();
			ptr.ptr = std::exchange(ptr.ptr, Invalid());
			return *this;
		}

		~IconvPtr() noexcept
		{
			reset();
		}

	public:
		void reset()
		{
			if (*this)
			{
				iconv_close(std::exchange(ptr, Invalid()));
			}
		}

		iconv_t get() const noexcept
		{
			return ptr;
		}

		explicit operator bool() const noexcept
		{
			return ptr != Invalid();
		}

		const iconv_t *operator &() const noexcept
		{
			return &ptr;
		}

		static iconv_t Invalid() noexcept
		{
			return reinterpret_cast<iconv_t>(-1);
		}

	private:
		iconv_t ptr{Invalid()};
	};
#endif

private:
	template<typename CharType, C4TextEncoding Encoding>
	static consteval bool IsCompatible()
	{
		switch (Encoding)
		{
		case C4TextEncoding::Ansi:
			return std::is_same_v<CharType, char>;

		case C4TextEncoding::Utf8:
			return std::is_same_v<CharType, char> || std::is_same_v<CharType, char8_t>;

#ifdef _WIN32
		case C4TextEncoding::Utf16:
			return std::is_same_v<CharType, wchar_t>;
#endif
		}

		return false;
	}

	template<typename OutputString, typename InputCharType, typename InputCharTraits>
	static OutputString CopyThrough(const std::basic_string_view<InputCharType, InputCharTraits> input)
	{
		using OutputCharType = typename OutputString::value_type;
		if constexpr (std::is_same_v<InputCharType, OutputCharType>)
		{
			return OutputString{input};
		}
		else if constexpr (sizeof(InputCharType) == sizeof(OutputCharType))
		{
			// Easy conversion between u8string and string
			OutputString result;
			result.resize_and_overwrite(input.size(), [&input](OutputCharType *const ptr, const std::size_t size)
			{
				std::memcpy(ptr, input.data(), size * sizeof(OutputCharType));
				return size;
			});

			return result;
		}
	}

#ifdef _WIN32
	static consteval std::uint32_t ToCodePage(const C4TextEncoding encoding)
	{
		switch (encoding)
		{
		case C4TextEncoding::Ansi:
			return CP_ACP;

		case C4TextEncoding::Utf8:
			return CP_UTF8;

		default:
			throw "Invalid encoding";
		}
	}
#elif defined(HAVE_ICONV)
	static consteval IconvPtr C4TextEncodingConverter::*GetConverter(const C4TextEncoding from, const C4TextEncoding to)
	{
		if (from == C4TextEncoding::Clonk && to == C4TextEncoding::System)
		{
			return &C4TextEncodingConverter::clonkToSystem;
		}
		else if (from == C4TextEncoding::System && to == C4TextEncoding::Clonk)
		{
			return &C4TextEncodingConverter::systemToClonk;
		}
		else if (from == C4TextEncoding::Clonk && to == C4TextEncoding::Utf8)
		{
			return &C4TextEncodingConverter::clonkToUtf8;
		}
		else if (from == C4TextEncoding::Utf8 && to == C4TextEncoding::Clonk)
		{
			return &C4TextEncodingConverter::utf8ToClonk;
		}
		else
		{
			throw "Invalid encoding";
		}
	}
#endif

public:
	C4TextEncodingConverter() = default;

public:
	template<
			C4TextEncoding From,
			C4TextEncoding To,
			typename _OutputCharType = void,
			typename _OutputCharTraits = void,
			typename _OutputAlloc = void,
			typename InputCharType,
			typename InputCharTraits
			>
	auto Convert(const std::basic_string<InputCharType, InputCharTraits> &input)
	{
		return Convert<From, To, _OutputCharType, _OutputCharTraits, _OutputAlloc>(static_cast<std::basic_string_view<InputCharType, InputCharTraits>>(input));
	}

	template<
			C4TextEncoding From,
			C4TextEncoding To,
			typename _OutputCharType = void,
			typename _OutputCharTraits = void,
			typename _OutputAlloc = void,
			typename InputCharType,
			typename InputCharTraits
			>
	auto Convert(std::basic_string_view<InputCharType, InputCharTraits> input)
	{
		using OutputCharType = std::conditional_t<
			std::is_same_v<_OutputCharType, void>,
			std::conditional_t<
				To == C4TextEncoding::Utf8,
				char8_t,
#ifdef _WIN32
				std::conditional_t<
					To == C4TextEncoding::Utf16,
					wchar_t,
					char>
				>,
#else
				char>,
#endif
			_OutputCharType>;

		using OutputCharTraits = std::conditional_t<
			std::is_same_v<_OutputCharTraits, void>,
			std::char_traits<OutputCharType>,
			_OutputCharTraits>;

		using OutputAlloc = std::conditional_t<
			std::is_same_v<_OutputAlloc, void>,
			std::allocator<OutputCharType>,
			_OutputAlloc>;

		static_assert(std::is_integral_v<InputCharType> && std::is_integral_v<OutputCharType>);
		static_assert(IsCompatible<InputCharType, From>() && IsCompatible<OutputCharType, To>());

		using OutputString = std::basic_string<OutputCharType, OutputCharTraits, OutputAlloc>;

#ifndef _WIN32
		static_assert(!std::is_same_v<InputCharType, wchar_t> && !std::is_same_v<OutputCharType, wchar_t>, "wchar_t is not supported on this platform");
#endif

		if constexpr (From == To)
		{
			return CopyThrough<OutputString>(input);
		}

#ifdef _WIN32
		if constexpr (std::is_same_v<InputCharType, wchar_t>)
		{
			OutputString result;
			result.resize_and_overwrite(StdStringEncodingConverter::WideCharToMultiByte(ToCodePage(To), input, {}), [&input](OutputCharType *const ptr, const std::size_t size)
			{
				return StdStringEncodingConverter::WideCharToMultiByte(ToCodePage(To), input, {reinterpret_cast<char *>(ptr), size});
			});

			return result;
		}
		else if constexpr (std::is_same_v<OutputCharType, wchar_t>)
		{
			OutputString result;
			const std::span<const char> inputChar{reinterpret_cast<const char *>(input.data()), input.size()};

			result.resize_and_overwrite(StdStringEncodingConverter::MultiByteToWideChar(ToCodePage(From), inputChar, {}), [&inputChar](OutputCharType *const ptr, const std::size_t size)
			{
				return StdStringEncodingConverter::MultiByteToWideChar(ToCodePage(From), inputChar, {ptr, size});
			});

			return result;
		}
		else
		{
			using TempAlloc = typename std::allocator_traits<OutputAlloc>::template rebind_alloc<wchar_t>;
			auto temp = Convert<From, C4TextEncoding::Utf16, wchar_t, void, TempAlloc>(input);
			return Convert<C4TextEncoding::Utf16, To, OutputCharType, OutputCharTraits, OutputAlloc>(temp);
		}
#elif defined(HAVE_ICONV)
		IconvPtr &converterPtr{std::invoke(GetConverter(From, To), this)};

		const std::lock_guard lock{iconvMutex};

		if (!converterPtr)
		{
			return CopyThrough<OutputString>(input);
		}

		const iconv_t converter{converterPtr.get()};

		OutputString result;

		// Reset converter
		iconv(converter, nullptr, nullptr, nullptr, nullptr);

		result.resize(input.size());
		const char *inbuf{reinterpret_cast<const char *>(input.data())};
		std::size_t inlen{input.size()};
		char *outbuf{reinterpret_cast<char *>(result.data())};
		std::size_t outlen{result.size()};

		while (inlen > 0)
		{
			if (iconv(converter, const_cast<ICONV_CONST char **>(&inbuf), &inlen, &outbuf, &outlen) == static_cast<std::size_t>(-1))
			{
				switch (errno)
				{
				// There is not sufficient room at *outbuf.
				case E2BIG:
				{
					const std::ptrdiff_t done{outbuf - reinterpret_cast<char *>(result.data())};
					result.resize(result.size() + inlen * 2);
					outbuf = reinterpret_cast<char *>(result.data()) + done;
					outlen += inlen * 2;
					break;
				}
				// An invalid multibyte sequence has been encountered in the input.
				case EILSEQ:
					++inbuf;
					--inlen;
					break;
				// An incomplete multibyte sequence has been encountered in the input.
				case EINVAL:
				default:
					break;
				}
			}
		}

		if (outlen)
		{
			result.resize(result.size() - outlen);
		}

		return result;
#else
		static_assert(!std::is_same_v<std::type_identity_t<OutputCharType>, OutputCharType>, "Unsupported conversion");
#endif
	}

	template<typename CharType = char8_t>
	std::basic_string<CharType> ClonkToUtf8(const std::string_view input)
	{
		return Convert<C4TextEncoding::Clonk, C4TextEncoding::Utf8, CharType>(input);
	}

	std::string Utf8ToClonk(const std::u8string_view input)
	{
		return Convert<C4TextEncoding::Utf8, C4TextEncoding::Clonk>(input);
	}

	std::string Utf8ToClonk(const std::string_view input)
	{
		return Convert<C4TextEncoding::Utf8, C4TextEncoding::Clonk>(input);
	}

	std::string ClonkToSystem(const std::string_view input)
	{
		return Convert<C4TextEncoding::Clonk, C4TextEncoding::System, char>(input);
	}

	std::string SystemToClonk(const std::string_view input)
	{
		return Convert<C4TextEncoding::System, C4TextEncoding::Clonk>(input);
	}


#ifdef HAVE_ICONV
	void CreateConverters(const char *charsetCodeName);
#endif

#ifdef HAVE_ICONV
private:
	std::mutex iconvMutex;
	IconvPtr clonkToSystem;
	IconvPtr systemToClonk;
	IconvPtr clonkToUtf8;
	IconvPtr utf8ToClonk;
#endif
};

extern C4TextEncodingConverter TextEncodingConverter;
