#pragma once

#include <array>
#include <span>
#include <type_traits>

#include <C4Game.h>
#include <C4Object.h>
#include <C4ScriptArgumentAdapters.h>
#include <C4Value.h>
#include <C4Wrappers.h>

namespace C4ScriptHelpers {
	inline void Warn(C4Object *const obj, const std::string_view message)
	{
		C4AulExecError{obj, message}.show();
	}

	template<typename... Args>
	inline void StrictError(C4AulContext *const context, C4AulScriptStrict errorSince, const std::format_string<Args...> message, Args &&... args)
	{
		const auto strictness = context->Caller ? context->Caller->Func->Owner->Strict : C4AulScriptStrict::NONSTRICT;

		const std::string result{std::format(message, std::forward<Args>(args)...)};
		if (strictness < errorSince)
		{
			Warn(context->Obj, result);
		}
		else
		{
			throw C4AulExecError{context->Obj, result};
		}
	}

	inline const static char *FnStringPar(const C4String &string)
	{
		return string.Data.getData();
	}

	inline const static char *FnStringPar(const C4String *const pString)
	{
		return pString ? FnStringPar(*pString) : "";
	}

	inline C4String *String(const char *str)
	{
		return str ? new C4String((str), &Game.ScriptEngine.Strings) : nullptr;
	}

	inline C4String *String(StdStrBuf &&str)
	{
		return str ? new C4String(std::forward<StdStrBuf>(str), &Game.ScriptEngine.Strings) : nullptr;
	}

	inline void MakePositionRelative(C4Object *relativeToObj, C4ValueInt &x, C4ValueInt &y)
	{
		assert(relativeToObj);

		x += relativeToObj->x;
		y += relativeToObj->y;
	}

	inline void MakePositionRelative(C4AulContext *ctx, C4ValueInt &x, C4ValueInt &y, bool extraCondition = true)
	{
		if (const auto obj = ctx->Obj; obj && extraCondition)
		{
			MakePositionRelative(obj, x, y);
		}
	}

	uint32_t StringBitEval(const char *str)
	{
		uint32_t rval = 0;
		for (int cpos = 0; str && str[cpos]; cpos++)
			if ((str[cpos] != '_') && (str[cpos] != ' '))
				rval += 1 << cpos;
		return rval;
	}
}

namespace C4ScriptHelpers {
	template<std::size_t ParCount>
	class C4AulEngineFuncHelper : public C4AulFunc
	{
		const std::array<C4V_Type, ParCount> parTypes;
		const bool pub;

	public:
		template<typename... ParTypes>
		C4AulEngineFuncHelper(C4AulScript *owner, const char *name, bool pub, ParTypes... parTypes) : C4AulFunc{owner, name}, parTypes{parTypes...}, pub{pub} {}
		virtual const C4V_Type *GetParType() noexcept override { return parTypes.data(); }
		virtual int GetParCount() noexcept override { return ParCount; }
		virtual bool GetPublic() noexcept override { return pub; }
	};

	template<typename Func, typename Ret, typename... Pars>
	class C4AulEngineFunc : public C4AulEngineFuncHelper<sizeof...(Pars)>
	{
		constexpr static auto ParCount = sizeof...(Pars);
		constexpr auto static isVoid = std::is_same_v<Ret, void>;
		Func func;

	public:
		C4AulEngineFunc(C4AulScript *owner, const char *name, Func func, bool pub) : C4AulEngineFuncHelper<ParCount>{owner, name, pub, ArgumentConverter<Pars>::Type...}, func{func} {}

		virtual C4V_Type GetRetType() noexcept override
		{
			if constexpr (isVoid) { return C4V_Any; }
			else return C4ValueConv<Ret>::Type();
		}

		C4Value Exec(C4AulContext *context, const C4Value pars[], bool = false) override
		{
			return ExecHelper(context, pars, std::make_index_sequence<ParCount>());
		}

	private:
		template<std::size_t... indices>
		C4Value ExecHelper(C4AulContext *context, const C4Value pars[], std::index_sequence<indices...>) const
		{
			if constexpr (sizeof...(Pars) != 0 && (Condition<Pars>::HasCondition || ...))
			{
				C4Value failValue;
				if (!([=, &failValue](const C4Value& par)
				{
					(void)failValue;
					if constexpr (Condition<Pars>::HasCondition)
					{
						if (!Condition<Pars>::Ok(context, par))
						{
							failValue = C4ValueConv<typename Condition<Pars>::FailType>::ToC4V(Condition<Pars>::FailValue);
							return false;
						}
					}
					return true;
				}(pars[indices]) && ...))
				{
					return failValue;
				}
			}

			const auto callHelper = [this, context, pars]
			{
				if constexpr (std::is_invocable_r_v<Ret, Func, C4AulContext*, Pars...>)
				{
					return std::invoke_r<Ret>(func, context, ArgumentConverter<Pars>::Convert(context, pars[indices])...);
				}
				else
				{
					return std::invoke_r<Ret>(func, ArgumentConverter<Pars>::Convert(context, pars[indices])...);
				}
			};

			if constexpr (isVoid)
			{
				callHelper();
				return C4VNull;
			}
			else
			{
				return C4ValueConv<Ret>::ToC4V(callHelper());
			}
		}
	};

	template <auto val, typename... IgnorePars>
	auto Constant(IgnorePars...)
	{
		return val;
	}

	template <typename Ret, typename... Pars>
	static void AddFunc(C4AulScript *owner, const char *name, Ret (&func)(C4AulContext *context, Pars...), bool pub = true)
	{
		new C4AulEngineFunc<Ret (&)(C4AulContext *context, Pars...), Ret, Pars...>{owner, name, func, pub};
	}

	template <typename Ret, typename... Pars>
	static void AddFunc(C4AulScript *owner, const char *name, Ret (&func)(Pars...), bool pub = true)
	{
		new C4AulEngineFunc<Ret (&)(Pars...), Ret, Pars...>{owner, name, func, pub};
	}

	template <typename Class, typename Member>
	static void AddFunc(C4AulScript *owner, const char *name, Member Class::*member, bool pub = true)
	{
		new C4AulEngineFunc<Member Class::*, Member, Class &>{owner, name, member, pub};
	}

	template <typename Class, typename Ret, typename... Pars>
	static void AddFunc(C4AulScript *owner, const char *name, Ret (Class::*member)(Pars...), bool pub = true)
	{
		new C4AulEngineFunc<Ret (Class::*)(Pars...), Ret, Class &, Pars...>{owner, name, member, pub};
	}

	template <typename Class, typename Ret, typename... Pars>
	static void AddFunc(C4AulScript *owner, const char *name, Ret (Class::*member)(Pars...) const, bool pub = true)
	{
		new C4AulEngineFunc<Ret (Class::*)(Pars...) const, Ret, Class &, Pars...>{owner, name, member, pub};
	}

	template <typename MemberPtr, typename Class, typename Ret, typename... Pars>
	static void AddInstanceMemberFunc(C4AulScript *owner, const char *name, Class &instance, MemberPtr member, bool pub = true)
	{
		struct Callable {
			Ret operator()(Pars... pars) const
			{
				return std::invoke(member, instance, pars...);
			}

			Class &instance;
			MemberPtr member;
		};

		new C4AulEngineFunc<Callable, Ret, Pars...>{owner, name, {instance, member}, pub};
	}

	template <typename Class, typename Ret, typename... Pars>
	static void AddFunc(C4AulScript *owner, const char *name, std::derived_from<Class> auto &instance, Ret (Class:: *member)(Pars...), bool pub = true)
	{
		return AddInstanceMemberFunc<Ret (Class:: *)(Pars...), Class, Ret, Pars...>(owner, name, instance, member, pub);
	}

	template <typename Class, typename Ret, typename... Pars>
	static void AddFunc(C4AulScript *owner, const char *name, std::derived_from<Class> auto &instance, Ret (Class:: *member)(Pars...) const, bool pub = true)
	{
		return AddInstanceMemberFunc<Ret (Class:: *)(Pars...) const, Class, Ret, Pars...>(owner, name, instance, member, pub);
	}

	template <typename Class, typename Ret> requires (!std::is_member_function_pointer_v<Ret Class:: *>)
	static void AddFunc(C4AulScript *owner, const char *name, Class& instance, Ret Class:: *member, bool pub = true)
	{
		return AddInstanceMemberFunc<Ret Class:: *, Class, Ret>(owner, name, instance, member, pub);
	}

	template<C4V_Type fromType, C4V_Type toType>
	class C4AulDefCastFunc : public C4AulEngineFuncHelper<1>
	{
	public:
		C4AulDefCastFunc(C4AulScript *owner, const char *name) :
		C4AulEngineFuncHelper<1>(owner, name, false, fromType) {}

		C4Value Exec(C4AulContext *, const C4Value pars[], bool = false) override
		{
			return C4Value{pars->GetData(), toType};
		}

		C4V_Type GetRetType() noexcept override { return toType; }
	};

	template<std::size_t ParCount, C4V_Type RetType>
	class C4AulEngineFuncParArray : public C4AulEngineFuncHelper<ParCount>
	{
		using Func = C4Value(&)(C4AulContext *context, std::span<const C4Value> pars);
		Func func;

	public:
		template<typename... ParTypes>
		C4AulEngineFuncParArray(C4AulScript *owner, const char *name, Func func, ParTypes... parTypes) : C4AulEngineFuncHelper<ParCount>{owner, name, true, parTypes...}, func{func} {}
		C4V_Type GetRetType() noexcept override { return RetType; }
		C4Value Exec(C4AulContext *context, const C4Value pars[], bool = false) override
		{
			return func(context, std::span<const C4Value>{pars, ParCount});
		}
	};
}
