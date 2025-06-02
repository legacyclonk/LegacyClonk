#pragma once

#include "C4ValueBase.h"
#include "C4Value.h"

#include <concepts>
#include <compare>
#include <cstddef>
#include <stdexcept>
#include <string_view>
#include <variant>

#ifdef Nil
#undef Nil
#endif

struct C4ValueConstexpr {
	struct AllowRuntime_s {};
	static constexpr AllowRuntime_s AllowRuntime{};

	struct Nil
	{
		constexpr std::strong_ordering operator<=>(const Nil &) const noexcept = default;
	};

private:
	using VariantType = std::variant<Nil, C4ValueInt, bool, C4ID, std::string_view>;

	template <typename T>
	struct tag {};

	template <typename T, typename V>
	struct get_index_s;

	template <typename T, typename... Ts>
	struct get_index_s<T, std::variant<Ts...>> : std::integral_constant<size_t, std::variant<tag<Ts>...>(tag<T>{}).index()> {};

	template <typename T>
	static constexpr auto get_index = get_index_s<T, VariantType>::value;

	VariantType value;

	friend ::std::hash<C4ValueConstexpr>;
	friend C4Value;

public:
	consteval C4ValueConstexpr() noexcept = default;
	template <typename TBool> requires (std::same_as<std::remove_cvref_t<TBool>, bool>)
	consteval C4ValueConstexpr(TBool value) noexcept : value{value} {}
	consteval C4ValueConstexpr(C4ValueInt value) noexcept : value{value} {}
	consteval C4ValueConstexpr(C4ID value) noexcept : value{value} {}
	consteval C4ValueConstexpr(const char *value) noexcept : C4ValueConstexpr{std::string_view{value}} {}
	consteval C4ValueConstexpr(std::string_view value) noexcept : value{value} {}

	template <typename T> requires std::is_constructible_v<VariantType, T>
	constexpr C4ValueConstexpr(T value, AllowRuntime_s) noexcept : value{value} {}

	constexpr C4V_Type GetType() const
	{
		switch (value.index())
		{
			case get_index<Nil>:
				return C4V_Any;
			case get_index<bool>:
				return C4V_Bool;
			case get_index<C4ValueInt>:
				return C4V_Int;
			case get_index<C4ID>:
				return C4V_C4ID;
			case get_index<std::string_view>:
				return C4V_String;
			case std::variant_npos:
				throw std::bad_variant_access{};
		}

		throw std::runtime_error{"Invalid value type index (Should not happen!)"};
	}

	constexpr auto operator<=>(const C4ValueConstexpr &other) const noexcept = default;
	std::strong_ordering operator<=>(const C4Value &other) const;

	constexpr operator C4Value() const
	{
		return std::visit(StdOverloadedCallable
		{
			[](C4ValueConstexpr::Nil) { return C4Value{}; },
			[](auto value) { return C4Value{value}; },
		}, value);
	}
};

namespace std
{
	template <>
	struct hash<C4ValueConstexpr>
	{
		std::size_t operator()(const C4ValueConstexpr &value);
	};
}
