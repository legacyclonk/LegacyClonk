/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2018, The OpenClonk Team and contributors
 * Copyright (c) 2017-2022, The LegacyClonk Team and contributors
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

#include "C4EnumInfo.h"
#include "Standard.h"
#include "StdCompiler.h"

#include <charconv>
#include <chrono>
#include <limits>
#include <memory>
#include <optional>
#include <type_traits>

// * Wrappers for C4Compiler-types

// Whole-line string, automatic size deduction (C4Compiler-String)
#define toC4CStr(szString) mkStringAdaptMA(szString)
#define toC4CStrBuf(rBuf) mkParAdapt(rBuf, StdCompiler::RCT_All)

// * Null Adaptor
// Doesn't compile anything
struct StdNullAdapt
{
	inline void CompileFunc(StdCompiler *pComp) const {}
};

// * Defaulting Adaptor
// Sets default if CompileFunc fails with a Exception of type NotFoundException
template <class T, class D>
struct StdDefaultAdapt
{
	T &rValue; const D &rDefault;

	StdDefaultAdapt(T &rValue, const D &rDefault) : rValue(rValue), rDefault(rDefault) {}

	inline void CompileFunc(StdCompiler *pComp) const
	{
		try
		{
			pComp->Value(rValue);
		}
		catch (const StdCompiler::NotFoundException &)
		{
			rValue = rDefault;
		}
	}
};

template <class T, class D>
inline StdDefaultAdapt<T, D> mkDefaultAdapt(T &&rValue, const D &rDefault) { return StdDefaultAdapt<T, D>(rValue, rDefault); }

// * Naming Adaptor
// Embeds a value into a named section, failsafe
// (use for values that do defaulting themselves - e.g. objects using naming)
template <class T>
struct StdNamingAdapt
{
	T &rValue; const char *szName;

	StdNamingAdapt(T &rValue, const char *szName) : rValue(rValue), szName(szName) {}

	inline void CompileFunc(StdCompiler *pComp) const
	{
		auto name = pComp->Name(szName);
		try
		{
			pComp->Value(rValue);
		}
		catch (const StdCompiler::Exception &)
		{
			name.Abort();
			throw;
		}
	}

	template <class D> inline bool operator==(const D &nValue) const { return rValue == nValue; }
	template <class D> inline StdNamingAdapt &operator=(const D &nValue) { rValue = nValue; return *this; }
};

template <class T>
inline StdNamingAdapt<T> mkNamingAdapt(T &&rValue, const char *szName) { return StdNamingAdapt<T>(rValue, szName); }

// * Naming Adaptor (defaulting)
// Embeds a value into a named section, sets default on fail
template <class T, class D>
struct StdNamingDefaultAdapt
{
	T &rValue; const char *szName; const D &rDefault; bool fPrefillDefault; bool fStoreDefault;

	StdNamingDefaultAdapt(T &rValue, const char *szName, const D &rDefault, bool fPrefillDefault, bool fStoreDefault) : rValue(rValue), szName(szName), rDefault(rDefault), fPrefillDefault(fPrefillDefault), fStoreDefault(fStoreDefault) {}

	inline void CompileFunc(StdCompiler *pComp) const
	{
		// Default check
		if (pComp->hasNaming() && pComp->isDecompiler() && rValue == rDefault && !fStoreDefault)
		{
			if (pComp->Default(szName)) return;
		}
		auto name = pComp->Name(szName);
		try
		{
			// Search named section, set default if not found
			if (name)
			{
				if (fPrefillDefault && pComp->isCompiler()) rValue = rDefault; // default prefill if desired
				pComp->Value(mkDefaultAdapt(rValue, rDefault));
			}
			else
				rValue = rDefault;
		}
		catch (const StdCompiler::Exception &)
		{
			name.Abort();
			throw;
		}
	}
};

template <class T, class D>
inline StdNamingDefaultAdapt<T, D> mkNamingAdapt(T &&rValue, const char *szName, const D &rDefault, bool fPrefillDefault = false, bool fStoreDefault = false) { return StdNamingDefaultAdapt<T, D>(rValue, szName, rDefault, fPrefillDefault, fStoreDefault); }

// * Decompiling Adaptor
// Allows to use const objects if the compiler won't change the targets
template <class T>
struct StdDecompileAdapt
{
	const T &rValue;

	explicit StdDecompileAdapt(const T &rValue) : rValue(rValue) {}

	inline void CompileFunc(StdCompiler *pComp) const
	{
		assert(pComp->isDecompiler());
		pComp->Value(const_cast<T &>(rValue));
	}
};

template <class T>
inline StdDecompileAdapt<T> mkDecompileAdapt(const T &rValue) { return StdDecompileAdapt<T>(rValue); }

// * Runtime value Adaptor
// Allows the C4ValueSetCompiler to set the value
template <class T>
struct StdRuntimeValueAdapt
{
	T &rValue;

	explicit StdRuntimeValueAdapt(T &rValue) : rValue(rValue) {}

	inline void CompileFunc(StdCompiler *pComp) const
	{
		pComp->setRuntimeWritesAllowed(+1);
		pComp->Value(rValue);
		pComp->setRuntimeWritesAllowed(-1);
	}

	template <class D> inline bool operator==(const D &nValue) const { return rValue == nValue; }
	template <class D> inline StdRuntimeValueAdapt<T> &operator=(const D &nValue) { rValue = nValue; return *this; }
};

template <class T>
inline StdRuntimeValueAdapt<T> mkRuntimeValueAdapt(T &&rValue) { return StdRuntimeValueAdapt<T>(rValue); }

// * String adaptor
struct StdStringAdapt
{
	char *szString; int iMaxLength; StdCompiler::RawCompileType eRawType;

	StdStringAdapt(char *szString, int iMaxLength, StdCompiler::RawCompileType eRawType = StdCompiler::RCT_Escaped)
		: szString(szString), iMaxLength(iMaxLength), eRawType(eRawType) {}

	inline void CompileFunc(StdCompiler *pComp) const
	{
		pComp->String(szString, iMaxLength, eRawType);
	}

	inline bool operator==(const char *szDefault) const { return SEqual(szString, szDefault); }
	inline StdStringAdapt &operator=(const char *szDefault) { SCopy(szDefault, szString, iMaxLength); return *this; }
};

inline StdStringAdapt mkStringAdapt(char *szString, int iMaxLength, StdCompiler::RawCompileType eRawType = StdCompiler::RCT_Escaped)
{
	return StdStringAdapt(szString, iMaxLength, eRawType);
}

#define mkStringAdaptM(szString) mkStringAdapt(szString, (sizeof(szString) / sizeof(*szString)) - 1)
#define mkStringAdaptMA(szString) mkStringAdapt(szString, (sizeof(szString) / sizeof(*szString)) - 1, StdCompiler::RCT_All)
#define mkStringAdaptMI(szString) mkStringAdapt(szString, (sizeof(szString) / sizeof(*szString)) - 1, StdCompiler::RCT_Idtf)
#define mkStringAdaptMIE(szString) mkStringAdapt(szString, (sizeof(szString) / sizeof(*szString)) - 1, StdCompiler::RCT_IdtfAllowEmpty)

// * std::string adaptor
struct StdStdStringAdapt
{
	std::string &string; StdCompiler::RawCompileType eRawType;
	StdStdStringAdapt(std::string &string, StdCompiler::RawCompileType eRawType = StdCompiler::RCT_Escaped)
		: string(string), eRawType(eRawType) { }
	inline void CompileFunc(StdCompiler *pComp) const
	{
		pComp->String(string, eRawType);
	}
	inline bool operator == (const char *szDefault) const { return string == szDefault; }
	inline StdStdStringAdapt &operator = (const char *szDefault) { string = szDefault; return *this; }
};
inline StdStdStringAdapt mkStringAdapt(std::string &string, StdCompiler::RawCompileType eRawType = StdCompiler::RCT_Escaped)
{ return StdStdStringAdapt(string, eRawType); }
inline StdStdStringAdapt mkStringAdaptA(std::string &string)
{ return StdStdStringAdapt(string, StdCompiler::RCT_All); }

// * Raw adaptor
struct StdRawAdapt
{
	void *pData; size_t iSize; StdCompiler::RawCompileType eRawType;

	StdRawAdapt(void *pData, size_t iSize, StdCompiler::RawCompileType eRawType = StdCompiler::RCT_Escaped)
		: pData(pData), iSize(iSize), eRawType(eRawType) {}

	inline void CompileFunc(StdCompiler *pComp) const
	{
		pComp->Raw(pData, iSize, eRawType);
	}

	inline bool operator==(const void *pDefault) const { return !memcmp(pDefault, pData, iSize); }
	inline StdRawAdapt &operator=(const void *pDefault) { memcpy(pData, pDefault, iSize); return *this; }
};

// * Integer Adaptor
// Stores Integer-like datatypes (Enumerations)
template <class T, class int_t = int32_t>
struct StdIntAdapt
{
	T &rValue;

	explicit StdIntAdapt(T &rValue) : rValue(rValue) {}

	inline void CompileFunc(StdCompiler *pComp) const
	{
		// Cast
		int_t iVal = int_t(rValue);
		pComp->Value(iVal);
		rValue = T(iVal);
	}

	// Operators for default checking/setting
	template <class D> inline bool operator==(const D &nValue) const { return rValue == nValue; }
	template <class D> inline StdIntAdapt &operator=(const D &nValue) { rValue = nValue; return *this; }
};

template <class T> inline StdIntAdapt<T> mkIntAdapt(T &rValue) { return StdIntAdapt<T>(rValue); }
template <class int_t, class T> StdIntAdapt<T, int_t> mkIntAdaptT(T &rValue) { return StdIntAdapt<T, int_t>(rValue); }

// * Casting Adaptor
// Does a reinterprete_cast
template <class T, class to_t>
struct StdCastAdapt
{
	T &rValue;

	explicit StdCastAdapt(T &rValue) : rValue(rValue) {}

	inline void CompileFunc(StdCompiler *pComp) const
	{
		// Cast
		assert(sizeof(to_t) == sizeof(T));
		to_t vVal = *reinterpret_cast<to_t *>(&rValue);
		pComp->Value(vVal);
		rValue = *reinterpret_cast<T *>(&vVal);
	}

	// Operators for default checking/setting
	template <class D> inline bool operator==(const D &nValue) const { return rValue == nValue; }
	template <class D> inline StdCastAdapt &operator=(const D &nValue) { rValue = nValue; return *this; }
};

template <class to_t, class T> StdCastAdapt<T, to_t> mkCastAdapt(T &rValue) { return StdCastAdapt<T, to_t>(rValue); }
template <class T> StdCastAdapt<T, int32_t> mkCastIntAdapt(T &rValue) { return StdCastAdapt<T, int32_t>(rValue); }

// Helper: Identity function class
template <class T>
struct _IdFuncClass
{
	T &operator()(T &rValue) const { return rValue; }
};

// * Array Adaptor
// Stores a separated list
template <class T, class M = _IdFuncClass<T>>
struct StdArrayAdapt
{
	StdArrayAdapt(T *pArray, std::size_t size, M map = M())
		: pArray(pArray), size(size), map(map) {}

	T *pArray; std::size_t size; M map;

	inline void CompileFunc(StdCompiler *pComp) const
	{
		for (std::size_t i{0}; i < size; i++)
		{
			if (i) pComp->Separator(StdCompiler::SEP_SEP);
			pComp->Value(map(pArray[i]));
		}
	}

	// Operators for default checking/setting
	inline bool operator==(const T &rDefault) const
	{
		for (std::size_t i{0}; i < size; i++)
			if (pArray[i] != rDefault)
				return false;
		return true;
	}

	inline StdArrayAdapt &operator=(const T &rDefault)
	{
		for (std::size_t i{0}; i < size; i++)
			pArray[i] = rDefault;
		return *this;
	}

	inline bool operator==(const T *pDefaults) const
	{
		for (std::size_t i{0}; i < size; i++)
			if (pArray[i] != pDefaults[i])
				return false;
		return true;
	}

	inline StdArrayAdapt &operator=(const T *pDefaults)
	{
		for (std::size_t i{0}; i < size; i++)
			pArray[i] = pDefaults[i];
		return *this;
	}
};

template <class T>
inline StdArrayAdapt<T> mkArrayAdaptS(T *array, std::size_t size) { return StdArrayAdapt<T>(array, size); }

template <class T, std::size_t N>
inline StdArrayAdapt<T> mkArrayAdapt(T (&array)[N]) { return StdArrayAdapt<T>(array, N); }

template <class T, class M>
inline StdArrayAdapt<T, M> mkArrayAdaptMapS(T *array, std::size_t size, M map) { return StdArrayAdapt<T, M>(array, size, map); }

template <class T, class M, std::size_t N>
inline StdArrayAdapt<T, M> mkArrayAdaptMap(T (&array)[N], M map) { return mkArrayAdaptMapS<T, M>(array, N, map); }

// * Array Adaptor (defaulting)
// Stores a separated list, sets defaults if a value or separator is missing.
template <class T, class D, class M = _IdFuncClass<T>>
struct StdArrayDefaultAdapt
{
	StdArrayDefaultAdapt(T *pArray, size_t iSize, const D &rDefault, const M &map = M())
		: pArray(pArray), iSize(iSize), rDefault(rDefault), map(map) {}

	T *pArray; size_t iSize; const D &rDefault; const M map;

	inline void CompileFunc(StdCompiler *pComp) const
	{
		size_t i, iWrite = iSize;
		bool fCompiler = pComp->isCompiler();
		// Decompiling: Omit defaults
		if (!fCompiler && pComp->hasNaming())
			while (iWrite > 0 && pArray[iWrite - 1] == rDefault)
				iWrite--;
		// Read/write values
		for (i = 0; i < iWrite; i++)
		{
			// Separator?
			if (i) if (!pComp->Separator(StdCompiler::SEP_SEP)) break;
			// Expect a value. Default if not found.
			pComp->Value(mkDefaultAdapt(map(pArray[i]), rDefault));
		}
		// Fill rest of array
		if (fCompiler)
			for (; i < iSize; i++)
				pArray[i] = rDefault;
	}

	// Additional defaulting (whole array)
	inline bool operator==(const T *pDefaults) const
	{
		for (size_t i = 0; i < iSize; i++)
			if (pArray[i] != pDefaults[i])
				return false;
		return true;
	}

	inline StdArrayDefaultAdapt &operator=(const T *pDefaults)
	{
		for (size_t i = 0; i < iSize; i++)
			pArray[i] = pDefaults[i];
		return *this;
	}
};

template <class T, class D>
inline StdArrayDefaultAdapt<T, D> mkArrayAdaptS(T *array, std::size_t size, const D &default_) { return StdArrayDefaultAdapt<T, D>(array, size, default_); }

template <class T, class D, std::size_t N>
inline StdArrayDefaultAdapt<T, D> mkArrayAdapt(T (&array)[N], const D &default_) { return mkArrayAdaptS<T, D>(array, N, default_); }

template <class T, class D, class M>
inline StdArrayDefaultAdapt<T, D, M> mkArrayAdaptMapS(T *array, std::size_t size, const D &default_, M map) { return StdArrayDefaultAdapt<T, D, M>(array, size, default_, map); }

template <class T, class D, class M, std::size_t N>
inline StdArrayDefaultAdapt<T, D, M> mkArrayAdaptMap(T (&array)[N], const D &default_, M map) { return mkArrayAdaptMapS<T, D, M>(array, N, default_, map); }

// * Insertion Adaptor
// Compile a value before / after another
template <class T, class I>
struct StdInsertAdapt
{
	StdInsertAdapt(T &rObj, I &rIns, bool fBefore = true)
		: rObj(rObj), rIns(rIns), fBefore(fBefore) {}
	T &rObj; I &rIns; bool fBefore;
	void CompileFunc(StdCompiler *pComp) const
	{
		if (fBefore) pComp->Value(rIns);
		pComp->Value(rObj);
		if (!fBefore) pComp->Value(rIns);
	}
};

template <class T, class I>
inline StdInsertAdapt<T, I> mkInsertAdapt(T &&rObj, I &&rIns, bool fBefore = true) { return StdInsertAdapt<T, I>(rObj, rIns, fBefore); }

// * Parameter Adaptor
// Specify a second parameter for the CompileFunc
template <class T, class P>
struct StdParameterAdapt
{
	StdParameterAdapt(T &rObj, const P &rPar) : rObj(rObj), Par(rPar) {}

	T &rObj; const P Par;

	void CompileFunc(StdCompiler *pComp) const
	{
		::CompileFunc(rObj, pComp, Par);
	}

	// Operators for default checking/setting
	template <class D> inline bool operator==(const D &nValue) const { return rObj == nValue; }
	template <class D> inline StdParameterAdapt &operator=(const D &nValue) { rObj = nValue; return *this; }

	// getting value
	inline T &GetObj() { return rObj; }
};

template <class T, class P>
inline StdParameterAdapt<T, P> mkParAdapt(T &rObj, const P &rPar) { return StdParameterAdapt<T, P>(rObj, rPar); }

// * Parameter Adaptor 2
// Specify a second and a third parameter for the CompileFunc
template <class T, class P1, class P2>
struct StdParameter2Adapt
{
	StdParameter2Adapt(T &rObj, const P1 &rPar1, const P2 &rPar2) : rObj(rObj), rPar1(rPar1), rPar2(rPar2) {}

	T &rObj; const P1 &rPar1; const P2 &rPar2;

	void CompileFunc(StdCompiler *pComp) const
	{
		::CompileFunc(rObj, pComp, rPar1, rPar2);
	}

	// Operators for default checking/setting
	template <class D> inline bool operator==(const D &nValue) const { return rObj == nValue; }
	template <class D> inline StdParameter2Adapt &operator=(const D &nValue) { rObj = nValue; return *this; }
};

template <class T, class P1, class P2>
inline StdParameter2Adapt<T, P1, P2> mkParAdapt(T &rObj, const P1 &rPar1, const P2 &rPar2) { return StdParameter2Adapt<T, P1, P2>(rObj, rPar1, rPar2); }

// * Store pointer (contents)
// Defaults to null
template <class PointerWrapper>
struct StdPtrAdapt
{
	using T = typename PointerWrapper::element_type;

	StdPtrAdapt(PointerWrapper &rpObj, bool fAllowNull = true, const char *szNaming = "Data")
		: rpObj(rpObj), fAllowNull(fAllowNull), szNaming(szNaming) {}

	PointerWrapper &rpObj; bool fAllowNull; const char *szNaming;

	void CompileFunc(StdCompiler *pComp) const
	{
		bool fCompiler = pComp->isCompiler(),
			fNaming = pComp->hasNaming();
		std::optional<StdCompiler::NameGuard> name;
		// Compiling? Clear object before
		if (fCompiler) { rpObj.reset(); }
		// Null checks - different with naming support.
		if (fAllowNull)
			if (fNaming)
			{
				// Null check: just omit when writing
				if (!fCompiler && !rpObj) return;
				name.emplace(pComp->Name(szNaming));
				// Set up naming
				if (!*name) { assert(fCompiler); return; }
			}
			else
			{
				bool fNull = !!rpObj;
				pComp->Value(fNull);
				// Null? Nothing further to do
				if (fNull) return;
			}
		else if (!fCompiler)
			assert(rpObj);
		// Compile value
		if (fCompiler)
		{
			T *rpnObj;
			CompileNewFunc(rpnObj, pComp);
			rpObj.reset(rpnObj);
		}
		else
			pComp->Value(mkDecompileAdapt(*rpObj));
	}

	// Operators for default checking/setting
	inline bool operator==(const T &nValue) const { return rpObj && *rpObj == nValue; }
	inline StdPtrAdapt &operator=(const T &nValue) { rpObj.reset(new T(nValue)); return *this; }
	inline bool operator==(const T *pValue) const { return rpObj.get() == pValue; }
	inline StdPtrAdapt &operator=(const T *pValue) { rpObj.reset(pValue); return *this; }
};

template <typename T>
void CompileFunc(std::unique_ptr<T> &smartPtr, StdCompiler *comp)
{
	comp->Value(StdPtrAdapt{smartPtr, false});
}

// only for use with StdPlainPtrAdapt
// does not delete on destruction; only on reset
template <typename T>
class PlainPtrRef
{
	T *&ptr;

public:
	using element_type = T;

	explicit PlainPtrRef(T *&ptr) noexcept : ptr{ptr} {}

	T *get() const noexcept { return ptr; }
	void reset(T *ptr_ = nullptr) { delete ptr; ptr = ptr_; }
	explicit operator bool() const noexcept { return ptr; }

	T &operator*() const noexcept { return *ptr; }
};

template <typename T>
class StdPlainPtrAdapt : public StdPtrAdapt<PlainPtrRef<T>>
{
	PlainPtrRef<T> ptr;

public:
	StdPlainPtrAdapt(T *&rpObj, bool fAllowNull = true, const char *szNaming = "Data")
	: ptr{rpObj}, StdPtrAdapt<PlainPtrRef<T>>{ptr, fAllowNull, szNaming} {}
};

template <class T>
inline StdPlainPtrAdapt<T> mkPtrAdapt(T *&rpObj, bool fAllowNull = true) { return StdPlainPtrAdapt<T>(rpObj, fAllowNull); }

template <class T>
inline StdPlainPtrAdapt<T> mkNamingPtrAdapt(T *&rpObj, const char *szNaming) { return StdPlainPtrAdapt<T>(rpObj, true, szNaming); }

template <class T>
inline StdPlainPtrAdapt<T> mkPtrAdaptNoNull(T *&rpObj) { return mkPtrAdapt<T>(rpObj, false); }

// * Adaptor for STL containers
// Writes a comma-separated list for compilers that support it. Otherwise, the size is calculated and safed.
// The defaulting uses the standard STL operators (full match)
template <class C>
struct StdSTLContainerAdapt
{
	StdSTLContainerAdapt(C &rStruct, StdCompiler::Sep eSep = StdCompiler::SEP_SEP)
		: rStruct(rStruct), eSep(eSep) {}

	C &rStruct; const StdCompiler::Sep eSep;

	template<typename... Parameters>
	inline void CompileFunc(StdCompiler *pComp, Parameters &&...parameters) const
	{
		typedef typename C::value_type T;
		// Get compiler specs
		bool fCompiler = pComp->isCompiler();
		bool fNaming = pComp->hasNaming();
		// Decompiling?
		if (!fCompiler)
		{
			// Write size (binary only)
			if (!fNaming)
			{
				auto iSize = checked_cast<int32_t>(rStruct.size());
				pComp->Value(iSize);
			}
			auto first = true;
			// Write all entries
			for (auto it : rStruct)
			{
				if (!first)
				{
					pComp->Separator(eSep);
				}
				else
				{
					first = false;
				}
				if constexpr (sizeof...(parameters) > 0)
				{
					pComp->Value(mkParAdapt(it, std::forward<Parameters>(parameters)...));
				}
				else
				{
					pComp->Value(it);
				}
			}
		}
		else
		{
			// Compiling: Empty previous
			rStruct.clear();
			// Read size (binary only)
			uint32_t iSize;
			if (!fNaming) pComp->Value(iSize);
			// Read new
			do
			{
				// No entries left to read?
				if (!fNaming && !iSize--)
					break;
				// Read entries
				try
				{
					T val;
					if constexpr (sizeof...(parameters) > 0)
					{
						pComp->Value(mkParAdapt(val, std::forward<Parameters>(parameters)...));
					}
					else
					{
						pComp->Value(val);
					}
					rStruct.emplace_back(val);
				}
				catch (const StdCompiler::NotFoundException &)
				{
					// No value found: Stop reading loop
					break;
				}
			} while (pComp->Separator(eSep));
		}
	}

	// Operators for default checking/setting
	inline bool operator==(const C &nValue) const { return rStruct == nValue; }
	inline StdSTLContainerAdapt &operator=(const C &nValue) { rStruct = nValue; return *this; }
};

template <class C>
inline StdSTLContainerAdapt<C> mkSTLContainerAdapt(C &rTarget, StdCompiler::Sep eSep = StdCompiler::SEP_SEP) { return StdSTLContainerAdapt<C>(rTarget, eSep); }

// * Adaptor for maps following the std::map and std::unordered_map interfaces
// Writes the size of the map followed by a semicolon separated list of key=value pairs
template <class Map>
struct StdSTLMapAdapt
{
	Map &map;

	StdSTLMapAdapt(Map &map) : map(map) {}

	void CompileFunc(StdCompiler *pComp) const
	{
		auto count = checked_cast<int32_t>(map.size());
		pComp->Value(count);
		if (pComp->isCompiler())
		{
			map.clear();
			for (std::int32_t i = 0; i < count; ++i)
			{
				pComp->Separator(StdCompiler::SEP_SEP2);
				typename Map::key_type key;
				pComp->Value(key);
				pComp->Separator(StdCompiler::SEP_SET);
				typename Map::mapped_type value;
				pComp->Value(value);
				map[key] = value;
			}
		}
		else
		{
			for (auto &it : map)
			{
				auto key = it.first;
				pComp->Separator(StdCompiler::SEP_SEP2);
				pComp->Value(key);
				pComp->Separator(StdCompiler::SEP_SET);
				pComp->Value(it.second);
			}
		}
	}

	StdSTLMapAdapt<Map> &operator=(const Map &otherMap)
	{
		map = otherMap;
		return *this;
	}

	bool operator==(const Map &otherMap) const
	{
		return map == otherMap;
	}
};

template <class Map>
inline StdSTLMapAdapt<Map> mkSTLMapAdapt(Map &map) { return StdSTLMapAdapt<Map>(map); }

// Write an integer that is supposed to be small most of the time. The adaptor writes it in
// 7-bit-pieces, bit 8 being a continuation marker: If it's set, more data is following, if not,
// all following bits are 0.
// Code lengths for uint32_t:
// 0x00000000 (0)         - 0x0000007F (127)        : 1 byte
// 0x00000080 (128)       - 0x00003FFF (16383)      : 2 byte
// 0x00004000 (16384)     - 0x001FFFFF (2097151)    : 3 byte
// 0x00200000 (2097152)   - 0x0FFFFFFF (268435456)  : 4 byte
// 0x10000000 (268435456) - 0xFFFFFFFF (4294967295) : 5 byte
// So this sort of packing is always useful when the integer in question is almost impossible to
// grow bigger than 2,097,151.
template <class T>
struct StdIntPackAdapt
{
	StdIntPackAdapt(T &rVal) : rVal(rVal) {}

	T &rVal;

	inline T clearUpper(T x) const
	{
		const int CLEARBITS = 8 * sizeof(T) - 7;
		return (x << CLEARBITS) >> CLEARBITS;
	}

	void CompileFunc(StdCompiler *pComp) const
	{
		// simply write for textual compilers
		if (pComp->hasNaming())
		{
			pComp->Value(rVal);
			return;
		}
		T val; uint8_t tmp;
		// writing?
		if (!pComp->isCompiler())
		{
			val = rVal;
			for (;;)
			{
				tmp = uint8_t(clearUpper(val));
				// last byte?
				if (clearUpper(val) == val)
					break;
				// write byte
				tmp ^= 0x80; pComp->Value(tmp);
				// advance
				val >>= 7;
			}
			// write last byte
			pComp->Value(tmp);
		}
		// reading?
		else
		{
			// read first byte
			pComp->Value(tmp);
			val = clearUpper(T(tmp));
			// read remaining bytes
			int i = 7; T data = val;
			while (uint8_t(data) != tmp)
			{
				// read byte
				pComp->Value(tmp);
				// add to value
				data = clearUpper(T(tmp));
				val = (data << i) | (val & ((1 << i) - 1));
				// next byte
				i += 7;
			}
			// write
			rVal = val;
		}
	}

	template <class D> inline bool operator==(const D &nValue) const { return rVal == nValue; }
	template <class D> inline StdIntPackAdapt &operator=(const D &nValue) { rVal = nValue; return *this; }
};

template <class T>
StdIntPackAdapt<T> mkIntPackAdapt(T &rVal) { return StdIntPackAdapt<T>(rVal); }

namespace detail
{
	template<class T>
	using underlying_or_same = typename std::conditional_t<std::is_enum_v<T>, std::underlying_type<T>, std::type_identity<T>>::type;
}

// whether named enum values can or must have the prefix
enum class C4EnumAdaptPrefixMode
{
	None = 0x1,
	Force = 0x2,
	Allow = None | Force
};

template <>
struct C4BitfieldOperators<C4EnumAdaptPrefixMode> : std::true_type {};

// Enumeration: For text compilers, write a given name for a value.
// For everything else, just write an integer of given type.
template <class T, class Enum, std::size_t N = C4EnumInfo<Enum>::data.values.size()>
class C4EnumAdaptWithInfo
{
	static_assert(sizeof(T) >= sizeof(Enum), "The storage type size must be at least as big as the enum type");
	using Info = C4EnumInfoData<Enum, N>;
	using Underlying = std::common_type_t<detail::underlying_or_same<Enum>, detail::underlying_or_same<T>>;

public:
	constexpr C4EnumAdaptWithInfo(T &val, C4EnumAdaptPrefixMode prefixMode, const Info &info = C4EnumInfo<Enum>::data) noexcept : val{val}, info{info}, prefixMode{prefixMode} {}

	void CompileFunc(StdCompiler *pComp) const
	{
		// Write as int
		if (!pComp->isVerbose())
		{
			CompileAsInt(pComp);
			return;
		}
		// writing?
		if (!pComp->isCompiler())
		{
			for (const auto infoVal : info.scopedValues(C4EnumValueScope::Serialization))
			{
				if (infoVal.value == val)
				{
					if (prefixMode == C4EnumAdaptPrefixMode::Force)
					{
						std::string name{info.prefix};
						name.append(infoVal.name);
						pComp->String(name, StdCompiler::RCT_Idtf);
					}
					else
					{
						pComp->String(infoVal.name.data(), infoVal.name.size(), StdCompiler::RCT_Idtf);
					}
					return;
				}
			}
			// No name found?
			CompileAsInt(pComp);
		}
		// reading?
		else
		{
			try
			{
				CompileAsInt(pComp);
			}
			catch (const StdCompiler::NotFoundException &)
			{
				// Try to read as string
				std::string name;
				pComp->String(name, StdCompiler::RCT_Idtf);
				const std::string_view nameView{name};
				for (const auto infoVal : info.values)
				{
					if (((prefixMode & C4EnumAdaptPrefixMode::None) != C4EnumAdaptPrefixMode{} && infoVal.name == name)
						|| ((prefixMode & C4EnumAdaptPrefixMode::Force) != C4EnumAdaptPrefixMode{} &&
							nameView.starts_with(info.prefix) && nameView.substr(info.prefix.size()) == infoVal.name))
					{
						val = infoVal.value;
						return;
					}
				}
				pComp->Warn("Unknown value name: %s", name.c_str());
			}
		}
	}

	template <class D> inline bool operator==(const D &nValue) const { return val == nValue; }
	template <class D> inline auto &operator=(const D &nValue) { val = nValue; return *this; }

private:
	void CompileAsInt(StdCompiler *pComp) const
	{
		auto underlying = static_cast<Underlying>(val);
		pComp->Value(underlying);
		val = static_cast<T>(underlying);
	}

	T &val;
	const Info &info;
	C4EnumAdaptPrefixMode prefixMode;
};

template <class Enum, class T, std::size_t N = C4EnumInfo<Enum>::data.values.size()>
auto mkEnumAdapt(T &val, C4EnumAdaptPrefixMode prefixMode = C4EnumAdaptPrefixMode::None, const C4EnumInfoData<Enum, N> &info = C4EnumInfo<Enum>::data) { return C4EnumAdaptWithInfo<T, Enum, N>{val, prefixMode, info}; }


template <class Enum, std::size_t N = C4EnumInfo<Enum>::data.values.size()> requires std::is_enum_v<Enum>
auto mkEnumAdapt(Enum &val, C4EnumAdaptPrefixMode prefixMode = C4EnumAdaptPrefixMode::None, const C4EnumInfoData<Enum, N> &info = C4EnumInfo<Enum>::data) { return C4EnumAdaptWithInfo<Enum, Enum, N>{val, prefixMode, info}; }

// Convert a bitfield into/from something like "foo | bar | baz", where "foo", "bar" and "baz" are given constants.
template <class T, class Enum, std::size_t N = C4EnumInfo<Enum>::data.values.size()>
class C4BitfieldAdaptWithInfo
{
	using Info = C4BitfieldInfoData<Enum, N>;
	using Underlying = std::common_type_t<detail::underlying_or_same<Enum>, detail::underlying_or_same<T>>;

public:
	constexpr C4BitfieldAdaptWithInfo(T &val, C4EnumAdaptPrefixMode prefixMode, const Info &info = C4EnumInfo<Enum>::data) noexcept : val{val}, info{info}, prefixMode{prefixMode} {}

	void CompileFunc(StdCompiler *pComp) const
	{
		if (!pComp->isVerbose())
		{
			pComp->Value(mkEnumAdapt(val, prefixMode, info));
			return;
		}

		Underlying tmp;
		C4EnumAdaptWithInfo<Underlying, Enum, N> enumAdapt{tmp, prefixMode, info};

		if (pComp->isCompiler())
		{
			Underlying uVal{0};
			do
			{
				pComp->Value(enumAdapt);
				uVal |= tmp;
			}
			while (pComp->Separator(StdCompiler::SEP_VLINE));

			val = static_cast<Enum>(uVal);
		}
		else
		{
			auto uVal = static_cast<Underlying>(val);
			bool first{true};

			for (const auto infoVal : info.scopedValues(C4EnumValueScope::Serialization))
			{
				tmp = static_cast<Underlying>(infoVal.value);
				// don’t write the zero-value if other bits are set
				if (val != 0 && tmp == 0)
				{
					continue;
				}
				if ((uVal & tmp) == tmp)
				{
					if (!first)
					{
						pComp->Separator(StdCompiler::SEP_VLINE);
					}
					first = false;
					pComp->Value(enumAdapt);
					uVal &= ~tmp;
				}
			}

			// Anything left is written as number
			if (uVal != 0)
			{
				if (!first)
				{
					pComp->Separator(StdCompiler::SEP_VLINE);
				}
				pComp->Value(uVal);
			}
		}
	}

	template <class D> inline bool operator==(const D &nValue) const { return val == nValue; }
	template <class D> inline auto &operator=(const D &nValue) { val = nValue; return *this; }

private:
	T &val;
	const Info &info;
	C4EnumAdaptPrefixMode prefixMode;
};

template <class Enum, size_t N = C4EnumInfo<Enum>::data.values.size()> requires std::is_enum_v<Enum>
auto mkBitfieldAdapt(Enum &val, C4EnumAdaptPrefixMode prefixMode = C4EnumAdaptPrefixMode::None, const C4BitfieldInfoData<Enum, N> &info = C4EnumInfo<Enum>::data) { return C4BitfieldAdaptWithInfo<Enum, Enum, N>{val, prefixMode, info}; }

template <class Enum, class T, size_t N = C4EnumInfo<Enum>::data.values.size()>
auto mkBitfieldAdapt(T &val, C4EnumAdaptPrefixMode prefixMode = C4EnumAdaptPrefixMode::None, const C4BitfieldInfoData<Enum, N> &info = C4EnumInfo<Enum>::data) { return C4BitfieldAdaptWithInfo<T, Enum, N>{val, prefixMode, info}; }

// * Name count adapter
// For compilers without name support, this just compiles the given value. Otherwise, the count
// of given namings is returned on compiling, and nothing is done for decompiling (The caller
// has to make sure that an appropriate number of namings will be created)
template <class int_t>
struct StdNamingCountAdapt
{
	int_t &iCount; const char *szName;
	StdNamingCountAdapt(int_t &iCount, const char *szName) : iCount(iCount), szName(szName) {}
	inline void CompileFunc(StdCompiler *pComp) const
	{
		if (pComp->hasNaming())
		{
			if (pComp->isCompiler())
				iCount = pComp->NameCount(szName);
		}
		else
			pComp->Value(mkIntPackAdapt(iCount));
	}
};

template <class int_t>
inline StdNamingCountAdapt<int_t> mkNamingCountAdapt(int_t &iCount, const char *szName) { return StdNamingCountAdapt<int_t>(iCount, szName); }

// * Hex adapter
// Writes raw binary data in hexadecimal
class StdHexAdapt
{
	void *pData; size_t iSize;

public:
	StdHexAdapt(void *pData, size_t iSize) : pData(pData), iSize(iSize) {}

	inline void CompileFunc(StdCompiler *pComp) const
	{
		if (!pComp->isVerbose())
			pComp->Raw(pData, iSize);
		char szData[2 + 1]; bool fCompiler = pComp->isCompiler();
		for (size_t i = 0; i < iSize; i++)
		{
			uint8_t *pByte = reinterpret_cast<uint8_t *>(pData) + i;
			if (!fCompiler)
			{
				*std::to_chars(szData, szData + 2, *pByte, 16).ptr = '\0';
			}
			pComp->String(szData, 2, StdCompiler::RCT_Idtf);
			if (fCompiler)
			{
				int b;
				if (std::from_chars(szData, szData + 2, b, 16).ec != std::errc{})
					pComp->excNotFound(i ? "hexadecimal data: bytes missing!" : "hexadecimal data missing!");
				*pByte = b;
			}
		}
	}
};

inline StdHexAdapt mkHexAdapt(void *pData, size_t iSize) { return StdHexAdapt(pData, iSize); }
template <class T>
inline StdHexAdapt mkHexAdapt(T &rData) { return StdHexAdapt(&rData, sizeof(rData)); }

template<typename Rep, typename Period>
inline void CompileFunc(std::chrono::duration<Rep, Period> &duration, StdCompiler *comp)
{
	if (comp->isCompiler())
	{
		Rep temp;
		comp->Value(temp);
		duration = std::chrono::duration<Rep, Period>{temp};
	}
	else
	{
		Rep temp{duration.count()};
		comp->Value(temp);
	}
}

template<typename T> requires std::is_scoped_enum_v<T>
inline void CompileFunc(T &value, StdCompiler *const comp)
{
	auto underlying = std::to_underlying(value);
	comp->Value(underlying);
	value = static_cast<T>(underlying);
}
