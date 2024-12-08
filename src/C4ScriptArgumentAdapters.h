#pragma once

#include <concepts>

#include <C4Aul.h>
#include <C4Object.h>
#include <C4ScriptValueConv.h>
#include <C4Value.h>

namespace C4ScriptHelpers {


	template <typename T>
	struct WrappedOrT_s { using type = T; };

	template <typename T> requires requires { typename T::Wrapped; }
	struct WrappedOrT_s<T> { using type = typename T::Wrapped; };

	template <typename T>
	using WrappedOrT = typename WrappedOrT_s<T>::type;

	// Simple C4Object * wrapper, which is used by script function adaptors to automatically fill in the context object instead of nil
	class C4ObjectOrThis
	{
		C4Object *obj;

	public:
		using Wrapped = C4Object *;

		constexpr C4ObjectOrThis(C4Object *obj) noexcept : obj{obj} {}
		constexpr C4Object &operator*() const noexcept { return *obj; }
		constexpr C4Object *operator->() const noexcept { return obj; }
		constexpr operator C4Object *() const noexcept { return obj; }
	};

	// Simple wrapper for integral types, which is used to automatically fill in a default value instead of nil or T{} if alsoForZero = true
	template<typename T, T defaultValue, bool alsoForZero = false>
	class Default
	{
		T value;

	public:
		using Wrapped = WrappedOrT<T>;

		constexpr Default(T value) noexcept : value{value} {}
		constexpr operator T &() noexcept { return value; }
		constexpr operator const T &() const noexcept { return value; }
		constexpr operator Wrapped() const requires (!std::same_as<Wrapped, T>) { return value; }
	};


	template<typename U, typename T>
	concept equals_comparable_with = requires (const T& t, const U& u)
	{
		{t == u} -> std::same_as<bool>;
	};


	// Wrapper type, which is used to automatically do nullptr/nil checks on script function arguments before calling the actual function
	// Also rejects zero values if nonZero = true.
	// nullptr is used instead of C4VNull, because C4Value is not usable as non-type template parameter
	template<typename T, auto failVal = nullptr, bool nonZero = false, bool allowNil = false>
	class Required
	{
		T value;

	public:
		using Wrapped = WrappedOrT<T>;

		constexpr Required(T value) noexcept : value{value} {}
		constexpr T &operator*() noexcept { return value; }
		constexpr const T &operator*() const noexcept { return value; }
		constexpr T operator->() const noexcept { return value; }

		template<equals_comparable_with<T> Other>
		constexpr bool operator==(const Other &other) const { return value == other; }

		constexpr operator T &() noexcept { return value; }
		constexpr operator const T &() const noexcept { return value; }
		constexpr operator Wrapped() const requires (!std::same_as<Wrapped, T>) { return value; }
	};

	template<typename T, auto failVal = nullptr>
	using RequiredNonZero = Required<T, failVal, true, false>;

	template<typename T>
	struct ArgumentConverter
	{
		static constexpr inline C4V_Type Type = C4ValueConv<T>::Type();
		static T Convert(C4AulContext *, const C4Value& value) { return C4ValueConv<T>::_FromC4V(value); }
	};

	template<>
	struct ArgumentConverter<C4ObjectOrThis>
	{
		static constexpr inline C4V_Type Type = C4V_C4Object;
		static C4ObjectOrThis Convert(C4AulContext *context, const C4Value& value)
		{
			const auto obj = value._getObj();
			return obj ? obj : context->Obj;
		}
	};

	template<typename T, T defaultVal>
	struct ArgumentConverter<Default<T, defaultVal, false>>
	{
		static constexpr inline C4V_Type Type = ArgumentConverter<T>::Type;
		static Default<T, defaultVal, false> Convert(C4AulContext *context, const C4Value& value)
		{
			if (value.GetType() != C4V_Any)
			{
				return ArgumentConverter<T>::Convert(context, value);
			}
			else
			{
				return defaultVal;
			}
		}
	};

	template<typename T, T defaultVal>
	struct ArgumentConverter<Default<T, defaultVal, true>> : ArgumentConverter<Default<T, defaultVal, false>>
	{
		static Default<T, defaultVal, true> Convert(C4AulContext *context, const C4Value& value)
		{
			const T result{ArgumentConverter<Default<T, defaultVal, false>>::Convert(context, value)};
			return (result != T{} ? result : defaultVal);

		}
	};

	template<typename T, auto failVal, bool nonZero, bool allowNil>
	struct ArgumentConverter<Required<T, failVal, nonZero, allowNil>>
	{
		static constexpr inline C4V_Type Type = ArgumentConverter<T>::Type;
		static Required<T, failVal, nonZero, allowNil> Convert(C4AulContext *context, const C4Value& value)
		{
			return ArgumentConverter<T>::Convert(context, value);
		}
	};

	template<typename T>
	struct ArgumentConverter<T &>
	{
		static constexpr inline C4V_Type Type = ArgumentConverter<T *>::Type;
		static T &Convert(C4AulContext *context, const C4Value& value)
		{
			return *ArgumentConverter<T *>::Convert(context, value);
		}
	};

	template<typename T>
	struct Condition
	{
		static constexpr inline bool HasCondition = false;
	};

	template<typename T, auto failVal, bool nonZero, bool allowNil>
	struct Condition<Required<T, failVal, nonZero, allowNil>>
	{
		static constexpr inline bool HasCondition = true;
		static constexpr inline auto FailValue = failVal;
		using FailType = decltype(failVal);

		static bool Ok(C4AulContext *context, const C4Value& value) noexcept
		{
			if constexpr (!allowNil)
			{
				if (value.GetType() == C4V_Any)
				{
					return false;
				}
			}
			if constexpr (nonZero)
			{
				if (ArgumentConverter<T>::Convert(context, value) == T{})
				{
					return false;
				}
			}
			return true;
		}
	};

	template<typename T>
	struct Condition<T &>
	{
		static constexpr inline bool HasCondition = true;
		static constexpr inline auto FailValue = nullptr;
		using FailType = std::nullptr_t;

		static bool Ok(C4AulContext *context, const C4Value& value) noexcept
		{
			return ArgumentConverter<T *>::Convert(context, value) != nullptr;
		}
	};

	template<auto failVal, bool nonZero, bool allowNil>
	struct Condition<Required<C4ObjectOrThis, failVal, nonZero, allowNil>>
	{
		static constexpr inline bool HasCondition = true;
		static constexpr inline auto FailValue = failVal;
		using FailType = decltype(failVal);

		static bool Ok(C4AulContext *context, const C4Value& value) noexcept
		{
			return value.GetType() != C4V_Any || context->Obj;
		}
	};
}
