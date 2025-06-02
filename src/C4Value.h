/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2017-2021, The LegacyClonk Team and contributors
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

#include "C4Id.h"
#include "C4AulScriptStrict.h"
#include "C4ValueBase.h"

#include <concepts>
#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>

// class declarations
class C4Value;
class C4Object;
class C4String;
class C4ValueArray;
class C4ValueHash;
class C4ValueContainer;

constexpr auto C4V_Last = static_cast<std::underlying_type_t<C4V_Type>>(C4V_pC4Value);

const char *GetC4VName(const C4V_Type Type);
char GetC4VID(const C4V_Type Type);
C4V_Type GetC4VFromID(char C4VID);

union C4V_Data
{
	C4ValueInt Int;
	C4ID ID;
	C4Object *Obj;
	C4String *Str;
	C4Value *Ref;
	C4ValueContainer *Container;
	C4ValueArray *Array;
	C4ValueHash *Map;
	std::intptr_t Raw;
	// cheat a little - assume that all members have the same length
	explicit operator bool() const noexcept { return Ref; }
	bool operator==(C4V_Data b) const noexcept { return Ref == b.Ref; }
	C4V_Data &operator=(C4Value *p) { Ref = p; return *this; }
};
// converter function, used in converter table
struct C4VCnvFn
{
	bool(*Function)(C4Value *, C4V_Type, bool); // function to be called; returns whether possible
	bool Warn;
};

template <typename T> struct C4ValueConv;

class C4Value
{
public:
	constexpr C4Value() noexcept : Type(C4V_Any), NextRef(nullptr), FirstRef(nullptr) { Data.Raw = 0; }

	C4Value(const C4Value &nValue, C4ValueHash *owningMap = nullptr) : Data(nValue.Data), Type(nValue.Type), NextRef(nullptr), FirstRef(nullptr), OwningMap(owningMap)
	{
		AddDataRef();
	}

	C4Value(C4V_Data nData, C4V_Type nType) : Data(nData), Type(nData || nType == C4V_Int || nType == C4V_Bool ? nType : C4V_Any), NextRef(nullptr), FirstRef(nullptr)
	{
		AddDataRef();
	}

	template<typename T> requires (!std::same_as<T, C4ID>)
	explicit C4Value(T nData, C4V_Type nType) : Type(nData || nType == C4V_Int || nType == C4V_Bool ? nType : C4V_Any), NextRef(nullptr), FirstRef(nullptr)
	{
		Data.Raw = nData;
		AddDataRef();
	}

	explicit constexpr C4Value(C4ID id) noexcept : Type(id ? C4V_C4ID : C4V_Any), NextRef(nullptr), FirstRef(nullptr)
	{
		Data.Raw = id;
	}

	explicit constexpr C4Value(C4ValueInt value) noexcept : Type(C4V_Int), NextRef(nullptr), FirstRef(nullptr)
	{
		Data.Raw = value;
	}

	explicit constexpr C4Value(bool value) noexcept : Type(C4V_Bool), NextRef(nullptr), FirstRef(nullptr)
	{
		Data.Raw = value;
	}

	explicit C4Value(C4Object *pObj) : Type(pObj ? C4V_C4Object : C4V_Any), NextRef(nullptr), FirstRef(nullptr)
	{
		Data.Obj = pObj; AddDataRef();
	}

	explicit C4Value(std::string_view str) : C4Value{MakeC4String(str)} {}

	explicit C4Value(C4String *pStr) : Type(pStr ? C4V_String : C4V_Any), NextRef(nullptr), FirstRef(nullptr)
	{
		Data.Str = pStr; AddDataRef();
	}

	explicit C4Value(C4ValueArray *pArray) : Type(pArray ? C4V_Array : C4V_Any), NextRef(nullptr), FirstRef(nullptr)
	{
		Data.Array = pArray; AddDataRef();
	}

	explicit C4Value(C4ValueHash *pMap) : Type(pMap ? C4V_Map : C4V_Any), NextRef(nullptr), FirstRef(nullptr)
	{
		Data.Map = pMap; AddDataRef();
	}

	explicit C4Value(C4Value *pVal) : Type(pVal ? C4V_pC4Value : C4V_Any), NextRef(nullptr), FirstRef(nullptr)
	{
		Data.Ref = pVal; AddDataRef();
	}

	static C4Value *OfMap(C4ValueHash *map)
	{
		auto ret = new C4Value;
		ret->OwningMap = map;
		return ret;
	}

	C4Value &operator=(const C4Value &nValue);

	constexpr ~C4Value()
	{
		// resolve all C4Values referencing this Value
		while (FirstRef)
			FirstRef->Set(*this);

		if consteval
		{
			if (!IsNil(false) && Type != C4V_Bool && Type != C4V_Int && Type != C4V_C4ID)
			{
				throw std::runtime_error{"Only nil, bool, int and ID are supported for constexpr destruction currently"};
			}
		}
		else
		{
			// delete contents
			DelDataRef(Data, Type, GetNextRef(), GetBaseContainer());
		}
	}

	// explicit conversion from int, bool and id
	std::optional<StdStrBuf> toString() const;

	// Checked getters
	C4ValueInt getInt()      { return ConvertTo(C4V_Int)      ? Data.Int   : 0; }
	C4ValueInt getIntOrID()  { Deref(); if (Type == C4V_Int || Type == C4V_Bool) return Data.Int; else if (Type == C4V_C4ID) return static_cast<C4ValueInt>(Data.ID); else return 0; }
	bool getBool()           { return ConvertTo(C4V_Bool)     ? !!Data.Int : false; }
	C4ID getC4ID()           { return ConvertTo(C4V_C4ID)     ? Data.ID : C4ID_None; }
	C4Object *getObj()       { return ConvertTo(C4V_C4Object) ? Data.Obj   : nullptr; }
	C4String *getStr()       { return ConvertTo(C4V_String)   ? Data.Str   : nullptr; }
	C4ValueArray *getArray() { return ConvertTo(C4V_Array)    ? Data.Array : nullptr; }
	C4ValueHash *getMap()    { return ConvertTo(C4V_Map)      ? Data.Map   : nullptr; }
	C4Value *getRef()        { return ConvertTo(C4V_pC4Value) ? Data.Ref   : nullptr; }

	// Unchecked getters
	constexpr C4ValueInt _getInt()      const noexcept { return Data.Int; }
	constexpr bool _getBool()           const noexcept { return !!Data.Int; }
	constexpr C4ID _getC4ID()           const noexcept { return Data.ID; }
	constexpr C4Object *_getObj()       const noexcept { return Data.Obj; }
	constexpr C4String *_getStr()       const noexcept { return Data.Str; }
	constexpr C4ValueArray *_getArray() const noexcept { return Data.Array; }
	constexpr C4ValueHash *_getMap()    const noexcept { return Data.Map; }
	constexpr C4Value *_getRef()        const noexcept { return Data.Ref; }
	constexpr std::intptr_t _getRaw()   const noexcept { return Data.Raw; }

	// Template versions
	template <typename T> inline T Get() { return C4ValueConv<T>::FromC4V(*this); }

	// only false for C4Script values nil, 0, false and NONE
	// this is the same as bool conversion in C4Script
	explicit operator bool() const { return static_cast<bool>(GetRefVal().GetData()); }

	void Set(const C4Value &nValue) { if (this != &nValue) Set(nValue.Data, nValue.Type); }

	void SetInt(C4ValueInt i) { C4V_Data d; d.Raw = 0; d.Int = i; Set(d, C4V_Int); }

	void SetBool(bool b) { C4V_Data d; d.Raw = 0; d.Int = b; Set(d, C4V_Bool); }

	void SetC4ID(C4ID id) { C4V_Data d; d.Raw = 0; d.ID = id; Set(d, C4V_C4ID); }

	void SetObject(C4Object *Obj) { C4V_Data d; d.Obj = Obj; Set(d, C4V_C4Object); }

	void SetString(C4String *Str) { C4V_Data d; d.Str = Str; Set(d, C4V_String); }

	void SetArray(C4ValueArray *Array) { C4V_Data d; d.Array = Array; Set(d, C4V_Array); }

	void SetMap(C4ValueHash *Map) { C4V_Data d; d.Map = Map; Set(d, C4V_Map); }

	void SetRef(C4Value *nValue) { C4V_Data d; d.Ref = nValue; Set(d, C4V_pC4Value); }

	void Set0();

	bool Equals(const C4Value &other, C4AulScriptStrict strict) const;

	bool operator==(const C4Value &Value2) const;

	// Change and set Type to int in case it was any before (avoids GuessType())
	C4Value &operator+=(C4ValueInt by) { GetData().Int += by;                        GetRefVal().Type = C4V_Int; return *this; }
	C4Value &operator++()              { GetData().Int++;                            GetRefVal().Type = C4V_Int; return *this; }
	C4Value operator++(int)            { C4Value alt = GetRefVal(); GetData().Int++; GetRefVal().Type = C4V_Int; return alt; }
	C4Value &operator--()              { GetData().Int--;                            GetRefVal().Type = C4V_Int; return *this; }
	C4Value &operator-=(C4ValueInt by) { GetData().Int -= by;                        GetRefVal().Type = C4V_Int; return *this; }
	C4Value operator--(int)            { C4Value alt = GetRefVal(); GetData().Int--; GetRefVal().Type = C4V_Int; return alt; }

	void Move(C4Value *nValue);

	C4Value GetRef() { return C4Value(this); }
	void Deref() { Set(GetRefVal()); }
	bool IsRef() { return Type == C4V_pC4Value; }
	constexpr bool IsNil(bool deref = true) const
	{
		const C4Value &val = deref ? GetRefVal() : *this;
		return val.Type == C4V_Any && val.Data.Raw == 0;
	}

	// get data of referenced value
	C4V_Data GetData() const { return GetRefVal().Data; }
	C4V_Data &GetData() { return GetRefVal().Data; }

	// get type of referenced value
	C4V_Type GetType() const { return GetRefVal().Type; }

	// return referenced value
	const C4Value &GetRefVal() const;
	C4Value &GetRefVal();

	// Get the Value at the index. May Throw C4AulExecError
	void GetContainerElement(C4Value *index, C4Value &to, struct C4AulContext *pctx = nullptr, bool noref = false);
	// Set the length of the array. Throws C4AulExecError if not an array
	void SetArrayLength(int32_t size, C4AulContext *cthr);

	const char *GetTypeName() const { return GetC4VName(GetType()); }
	const char *GetTypeInfo();

	void DenumeratePointer();

	std::string GetDataString() const;

	inline bool ConvertTo(C4V_Type vtToType, bool fStrict = true) // convert to dest type
	{
		C4VCnvFn Fn = C4ScriptCnvMap[Type][vtToType];
		if (Fn.Function)
			return (*Fn.Function)(this, vtToType, fStrict);
		return true;
	}

	void HintType(C4V_Type type);

	// Compilation
	void CompileFunc(StdCompiler *pComp);

protected:
	// data
	C4V_Data Data;

	// reference-list
	union
	{
		C4Value *NextRef;
		C4ValueContainer *BaseContainer;
	};
	C4Value *FirstRef;

	C4ValueHash *OwningMap = nullptr;

	// data type
	C4V_Type Type : 8;
	bool HasBaseContainer = false;

	C4Value *GetNextRef() { if (HasBaseContainer) return nullptr; else return NextRef; }
	C4ValueContainer *GetBaseContainer() { if (HasBaseContainer) return BaseContainer; else return nullptr; }

	void Set(C4V_Data nData, C4V_Type nType);

	void AddRef(C4Value *pRef);
	void DelRef(const C4Value *pRef, C4Value *pNextRef, C4ValueContainer *pBaseContainer);

	void AddDataRef();
	void DelDataRef(C4V_Data Data, C4V_Type Type, C4Value *pNextRef, C4ValueContainer *pBaseContainer);

	void CheckRemoveFromMap();

	// guess type from data (if type == c4v_any)
	C4V_Type GuessType();

	static C4String *MakeC4String(std::string_view value);
	static C4VCnvFn C4ScriptCnvMap[C4V_Last + 1][C4V_Last + 1];
	static bool FnCnvInt2Id(C4Value *Val, C4V_Type toType, bool fStrict);
	static bool FnCnvGuess(C4Value *Val, C4V_Type toType, bool fStrict);

	friend class C4Object;
	friend class C4AulDefFunc;
};

// converter
inline constexpr C4Value C4VInt(C4ValueInt iVal) noexcept { return C4Value{iVal}; }
inline constexpr C4Value C4VBool(bool fVal) noexcept { return C4Value{fVal}; }
inline constexpr C4Value C4VID(C4ID idVal) noexcept { return C4Value{idVal}; }
inline C4Value C4VObj(C4Object *pObj) { return C4Value(pObj); }
inline C4Value C4VString(C4String *pStr) { return C4Value(pStr); }
inline C4Value C4VArray(C4ValueArray *pArray) { return C4Value(pArray); }
inline C4Value C4VMap(C4ValueHash *pMap) { return C4Value(pMap); }
inline C4Value C4VRef(C4Value *pVal) { return pVal->GetRef(); }

C4Value C4VString(StdStrBuf &&strString);
C4Value C4VString(const char *strString);

// converter templates
template <> struct C4ValueConv<C4ValueInt>
{
	inline static constexpr C4V_Type Type() noexcept { return C4V_Int; }
	inline static C4ValueInt FromC4V(C4Value &v) { return v.getInt(); }
	inline static constexpr C4ValueInt _FromC4V(const C4Value &v) noexcept { return v._getInt(); }
	inline static constexpr C4Value ToC4V(int32_t v) noexcept { return C4VInt(v); }
};

template <> struct C4ValueConv<bool>
{
	inline static constexpr C4V_Type Type() noexcept { return C4V_Bool; }
	inline static bool FromC4V(C4Value &v) { return v.getBool(); }
	inline static constexpr bool _FromC4V(const C4Value &v) noexcept { return v._getBool(); }
	inline static constexpr C4Value ToC4V(bool v) noexcept { return C4VBool(v); }
};

template <> struct C4ValueConv<C4ID>
{
	inline static constexpr C4V_Type Type() noexcept { return C4V_C4ID; }
	inline static C4ID FromC4V(C4Value &v) { return v.getC4ID(); }
	inline static constexpr C4ID _FromC4V(const C4Value &v) noexcept { return v._getC4ID(); }
	inline static constexpr C4Value ToC4V(C4ID v) noexcept { return C4VID(v); }
};

template <> struct C4ValueConv<C4Object *>
{
	inline static constexpr C4V_Type Type() noexcept { return C4V_C4Object; }
	inline static C4Object *FromC4V(C4Value &v) { return v.getObj(); }
	inline static constexpr C4Object *_FromC4V(const C4Value &v) noexcept { return v._getObj(); }
	inline static C4Value ToC4V(C4Object *v) { return C4VObj(v); }
};

template <> struct C4ValueConv<C4String *>
{
	inline static constexpr C4V_Type Type() noexcept { return C4V_String; }
	inline static C4String *FromC4V(C4Value &v) { return v.getStr(); }
	inline static constexpr C4String *_FromC4V(const C4Value &v) noexcept { return v._getStr(); }
	inline static C4Value ToC4V(C4String *v) { return C4VString(v); }
};

template <> struct C4ValueConv<C4ValueArray *>
{
	inline static constexpr C4V_Type Type() noexcept { return C4V_Array; }
	inline static C4ValueArray *FromC4V(C4Value &v) { return v.getArray(); }
	inline static constexpr C4ValueArray *_FromC4V(const C4Value &v) noexcept { return v._getArray(); }
	inline static C4Value ToC4V(C4ValueArray *v) { return C4VArray(v); }
};

template <> struct C4ValueConv<C4ValueHash *>
{
	inline static constexpr C4V_Type Type() noexcept { return C4V_Map; }
	inline static C4ValueHash *FromC4V(C4Value &v) { return v.getMap(); }
	inline static constexpr C4ValueHash *_FromC4V(const C4Value &v) noexcept { return v._getMap(); }
	inline static C4Value ToC4V(C4ValueHash *v) { return C4VMap(v); }
};

template <> struct C4ValueConv<C4Value *>
{
	inline static constexpr C4V_Type Type() noexcept { return C4V_pC4Value; }
	inline static C4Value *FromC4V(C4Value &v) { return v.getRef(); }
	inline static constexpr C4Value *_FromC4V(const C4Value &v) noexcept { return v._getRef(); }
	inline static C4Value ToC4V(C4Value *v) { return C4VRef(v); }
};

namespace std
{
	template<>
	struct hash<C4Value>
	{
		std::size_t operator()(const C4Value &value) const;
	};
}

inline constexpr const C4Value C4VNull{};
inline constexpr const C4Value C4VFalse{false};
inline constexpr const C4Value C4VTrue{true};
