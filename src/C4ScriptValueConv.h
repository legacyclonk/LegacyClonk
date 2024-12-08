#pragma once

#include <C4Def.h>
#include <C4EnumeratedObjectPtr.h>
#include <C4Player.h>
#include <C4PlayerInfo.h>
#include <C4Value.h>
#include <C4Wrappers.h>

template <> struct C4ValueConv<C4Value>
{
	constexpr static C4V_Type Type() { return C4V_Any; }
	inline static C4Value FromC4V(C4Value &v) { return v; }
	inline static C4Value _FromC4V(const C4Value &v) { return v; }
	inline static C4Value ToC4V(C4Value v) { return v; }
};

template <> struct C4ValueConv<std::nullptr_t>
{
	constexpr static C4V_Type Type() { return C4V_Any; }
	inline static std::nullptr_t FromC4V(C4Value &) { return nullptr; }
	inline static std::nullptr_t _FromC4V(const C4Value &) { return nullptr; }
	inline static C4Value ToC4V(std::nullptr_t) { return C4VNull; }
};

template <> struct C4ValueConv<std::make_unsigned_t<C4ValueInt>>
{
	using C4ValueIntUnsigned = std::make_unsigned_t<C4ValueInt>;
	constexpr static C4V_Type Type() { return C4V_Int; }
	inline static C4ValueIntUnsigned FromC4V(C4Value &v) { return static_cast<C4ValueIntUnsigned>(v.getInt()); }
	inline static C4ValueIntUnsigned _FromC4V(const C4Value &v) { return static_cast<C4ValueIntUnsigned>(v._getInt()); }
	inline static C4Value ToC4V(C4ValueIntUnsigned v) { return C4VInt(static_cast<C4ValueInt>(v)); }
};

template <> struct C4ValueConv<C4EnumeratedObjectPtr>
{
	constexpr static C4V_Type Type() { return C4V_C4Object; }
	inline static C4Value ToC4V(C4EnumeratedObjectPtr v) { return C4VObj(v.Object()); }
};

template <typename T> struct C4ValueConv<std::optional<T>>
{
	constexpr static C4V_Type Type() { return C4ValueConv<T>::Type(); }
	inline static std::optional<T> FromC4V(C4Value &v)
	{
		if (v.GetType() != C4V_Any) return {C4ValueConv<T>::FromC4V(v)};
		return {};
	}
	inline static std::optional<T> _FromC4V(const C4Value &v)
	{
		if (v.GetType() != C4V_Any) return {C4ValueConv<T>::_FromC4V(v)};
		return {};
	}
	inline static C4Value ToC4V(const std::optional<T>& v)
	{
		if (v) return C4ValueConv<T>::ToC4V(*v);
		return C4VNull;
	}
};

// convert id to definition with C4ID2Def
template <> struct C4ValueConv<C4Def *>
{
	constexpr static C4V_Type Type() { return C4V_C4ID; }
	inline static C4Def *FromC4V(C4Value &v)
	{
		return C4Id2Def(v.getC4ID());
	}
	inline static C4Def *_FromC4V(const C4Value &v)
	{
		return C4Id2Def(v._getC4ID());
	}
};

// convert int to player by player number lookup
template <> struct C4ValueConv<C4Player *>
{
	constexpr static C4V_Type Type() { return C4V_Int; }
	inline static C4Player *FromC4V(C4Value &v)
	{
		return Game.Players.Get(v.getInt());
	}
	inline static C4Player *_FromC4V(const C4Value &v)
	{
		return Game.Players.Get(v._getInt());
	}
};

// convert int to player info by player id lookup
template <> struct C4ValueConv<C4PlayerInfo *>
{
	constexpr static C4V_Type Type() { return C4V_Int; }
	inline static C4PlayerInfo *FromC4V(C4Value &v)
	{
		return Game.PlayerInfos.GetPlayerInfoByID(v.getInt());
	}
	inline static C4PlayerInfo *_FromC4V(const C4Value &v)
	{
		return Game.PlayerInfos.GetPlayerInfoByID(v._getInt());
	}
};
