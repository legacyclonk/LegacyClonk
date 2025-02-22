/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2001, Sven2
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

// C4Aul script engine CP conversion

#include <C4Include.h>
#include <C4Aul.h>

#include <C4Config.h>
#include <C4Def.h>
#include <C4Log.h>
#include <C4Components.h>

#include <format>

namespace C4AulAST
{
Statement::~Statement() {}
Expression::~Expression() {}
NoRef::~NoRef() {}
BinaryExpression::~BinaryExpression() {}
NilTestExpression::~NilTestExpression() {}

std::string Statement::ToTree() const
{
	return ToTreeInternal() | std::views::join_with('\n') | std::ranges::to<std::string>();
}

void Script::CompileFunc(StdCompiler *const comp)
{
	Statement::CompileFunc(comp);

	comp->Value(mkNamingAdapt(mkSTLContainerAdapt(statements), "Statements"));
}

template<std::ranges::range Range>
static auto IndentElementsOf(Range &&range)
{
	return std::forward<Range>(range) | std::views::transform([](std::string str)
	{
		return std::string{"\t"} + std::move(str);
	});
}

std::generator<std::string> Script::ToTreeInternal() const
{
	co_yield "(script";

	for (const auto &statement : statements)
	{
		co_yield std::ranges::elements_of(IndentElementsOf(statement->ToTreeInternal()));
	}

	co_yield ")";
}

void Expression::CompileFunc(StdCompiler *const comp)
{
	Statement::CompileFunc(comp);
	comp->Value(mkNamingAdapt(mkCastIntAdapt(type), "Type"));
}

void NoRef::CompileFunc(StdCompiler *const comp)
{
	comp->Value(mkNamingAdapt(noRef, "NoRef"));
}

void Nil::CompileFunc(StdCompiler *const comp)
{
	Expression::CompileFunc(comp);
	comp->Value(mkNamingAdapt(preferStack, "PreferStack"));
}

std::string Nil::GetLiteralString() const
{
	return "(nil)";
}

void IntLiteral::CompileFunc(StdCompiler *const comp)
{
	Expression::CompileFunc(comp);
	comp->Value(mkNamingAdapt(value, "Value"));
}

std::string IntLiteral::GetLiteralString() const
{
	return std::format("(int.const {})", value);
}

void BoolLiteral::CompileFunc(StdCompiler *const comp)
{
	Expression::CompileFunc(comp);
	comp->Value(mkNamingAdapt(value, "Value"));
}

std::string BoolLiteral::GetLiteralString() const
{
	return std::format("(bool.const {})", value);
}

void C4IDLiteral::CompileFunc(StdCompiler *const comp)
{
	Expression::CompileFunc(comp);
	comp->Value(mkNamingAdapt(mkC4IDAdapt(value), "Value"));
}

std::string C4IDLiteral::GetLiteralString() const
{
	return std::format("(id.const {})", value);
}

void StringLiteral::CompileFunc(StdCompiler *const comp)
{
	Expression::CompileFunc(comp);
	comp->Value(mkNamingAdapt(value, "Value"));
}

std::string StringLiteral::GetLiteralString() const
{
	return std::format("(string.const {:?})", value);
}

void ArrayLiteral::CompileFunc(StdCompiler *comp)
{
	Expression::CompileFunc(comp);
	comp->Value(mkNamingAdapt(mkSTLContainerAdapt(expressions), "Expressions"));
}

std::generator<std::string> ArrayLiteral::ToTreeInternal() const
{
	co_yield "(array.const";

	for (const auto &expression : expressions)
	{
		co_yield std::ranges::elements_of(IndentElementsOf(expression->ToTreeInternal()));
	}

	co_yield ")";
}

void MapLiteral::KeyValuePair::CompileFunc(StdCompiler *const comp)
{
	comp->Value(mkNamingAdapt(Key, "Key"));
	comp->Value(mkNamingAdapt(Value, "Value"));
}

std::generator<std::string> MapLiteral::KeyValuePair::ToTreeInternal() const
{
	co_yield "(key";
	co_yield std::ranges::elements_of(IndentElementsOf(Key->ToTreeInternal()));
	co_yield ")";
	co_yield "(value";
	co_yield std::ranges::elements_of(IndentElementsOf(Value->ToTreeInternal()));
	co_yield ")";
}

void MapLiteral::CompileFunc(StdCompiler *comp)
{
	Expression::CompileFunc(comp);
	comp->Value(mkNamingAdapt(mkSTLContainerAdapt(expressions), "Expressions"));
}

std::generator<std::string> MapLiteral::ToTreeInternal() const
{
	co_yield "map.const (";

	for (const auto &expression : expressions)
	{
		co_yield std::ranges::elements_of(IndentElementsOf(expression.ToTreeInternal()));
	}

	co_yield ")";
}

void GlobalConstant::CompileFunc(StdCompiler *const comp)
{
	Expression::CompileFunc(comp);
	comp->Value(mkNamingAdapt(identifier, "Identifier"));
	comp->Value(mkNamingAdapt(value, "Value"));
}

std::generator<std::string> GlobalConstant::ToTreeInternal() const
{
	if (auto *const constantLiteral = dynamic_cast<ConstantLiteral *>(value.get()))
	{
		co_yield std::format("(global.const.get {:?} {})", identifier, constantLiteral->GetLiteralString());;
	}
	else
	{
		co_yield std::format("(global.const.get {:?}", identifier);
		co_yield std::ranges::elements_of(IndentElementsOf(value->ToTreeInternal()));
		co_yield ")";
	}
}

void ParN::CompileFunc(StdCompiler *const comp)
{
	Expression::CompileFunc(comp);
	NoRef::CompileFunc(comp);
	comp->Value(mkNamingAdapt(n, "N"));
}

std::generator<std::string> ParN::ToTreeInternal() const
{
	co_yield std::format("(par.get{} {})", IsNoRef() ? "" : ".ref", n);
}

void VarN::CompileFunc(StdCompiler *const comp)
{
	Expression::CompileFunc(comp);
	NoRef::CompileFunc(comp);
	comp->Value(mkNamingAdapt(identifier, "Identifier"));
}

std::generator<std::string> VarN::ToTreeInternal() const
{
	co_yield std::format("(var.get{} {:?})", IsNoRef() ? "" : ".ref", identifier);
}

void Par::CompileFunc(StdCompiler *const comp)
{
	Expression::CompileFunc(comp);
	NoRef::CompileFunc(comp);
	comp->Value(mkNamingAdapt(expression, "Expression"));
}

std::generator<std::string> Par::ToTreeInternal() const
{
	co_yield std::format("(par.get.expr{}", IsNoRef() ? "" : ".ref");
	co_yield std::ranges::elements_of(IndentElementsOf(expression->ToTreeInternal()));
	co_yield ")";
}

void Var::CompileFunc(StdCompiler *const comp)
{
	Expression::CompileFunc(comp);
	NoRef::CompileFunc(comp);
	comp->Value(mkNamingAdapt(expression, "Expression"));
}

std::generator<std::string> Var::ToTreeInternal() const
{
	co_yield std::format("(var.get.expr{}", IsNoRef() ? "" : ".ref");
	co_yield std::ranges::elements_of(IndentElementsOf(expression->ToTreeInternal()));
	co_yield ")";
}

void LocalN::CompileFunc(StdCompiler *const comp)
{
	Expression::CompileFunc(comp);
	NoRef::CompileFunc(comp);
	comp->Value(mkNamingAdapt(identifier, "Identifier"));
}

std::generator<std::string> LocalN::ToTreeInternal() const
{
	co_yield std::format("(local.get{} {:?})", IsNoRef() ? "" : ".ref", identifier);
}

void GlobalN::CompileFunc(StdCompiler *const comp)
{
	Expression::CompileFunc(comp);
	NoRef::CompileFunc(comp);
	comp->Value(mkNamingAdapt(identifier, "Identifier"));
}

std::generator<std::string> GlobalN::ToTreeInternal() const
{
	co_yield std::format("(global.get{} {:?})", IsNoRef() ? "" : ".ref", identifier);
}

void Declaration::CompileFunc(StdCompiler *const comp)
{
	Statement::CompileFunc(comp);

	comp->Value(mkNamingAdapt(type, "Type"));
	comp->Value(mkNamingAdapt(identifier, "Identifier"));
	comp->Value(mkNamingAdapt(StdPtrAdapt{value, true}, "Value"));
}

std::generator<std::string> Declaration::ToTreeInternal() const
{
	const std::string typeName{[](const Type type)
	{
		switch (type)
		{
		case Type::Local:
			return "local";

		case Type::Static:
			return "global";

		case Type::StaticConst:
			return "global.const";

		case Type::Var:
			return "var";
		}
	}(type)};

	if (value)
	{
		if (auto *const constantLiteral = dynamic_cast<ConstantLiteral *>(value.get()))
		{
			co_yield std::format("({} {:?} {})", typeName, identifier, constantLiteral->GetLiteralString());;
		}
		else
		{
			co_yield std::format("({} {:?}", typeName, identifier);
			co_yield std::ranges::elements_of(IndentElementsOf(value->ToTreeInternal()));
			co_yield ")";
		}
	}
	else
	{
		co_yield std::format("({} {:?})", typeName, identifier);
	}
}

void Declarations::CompileFunc(StdCompiler *const comp)
{
	Statement::CompileFunc(comp);

	comp->Value(mkNamingAdapt(mkSTLContainerAdapt(declarations), "Declarations"));
}

std::generator<std::string> Declarations::ToTreeInternal() const
{
	co_yield "(declare";

	for (const auto &declaration : declarations)
	{
		co_yield std::ranges::elements_of(IndentElementsOf(declaration->ToTreeInternal()));
	}

	co_yield ")";
}

static auto mkOperatorAdapt(std::size_t &opID)
{
	struct Adaptor
	{
		std::size_t &OpID;

		void CompileFunc(StdCompiler *comp)
		{
			if (comp->isCompiler())
			{
				std::string identifier;
				comp->Value(identifier);

				for (std::size_t i{0}; C4ScriptOpMap[i].Identifier; ++i)
				{
					if (C4ScriptOpMap[i].Identifier == identifier)
					{
						OpID = i;
						return;
					}
				}

				comp->excCorrupt("Invalid operator ID");
			}
			else
			{
				std::string identifier{C4ScriptOpMap[OpID].Identifier};
				comp->Value(identifier);
			}
		}
	};

	return Adaptor{.OpID = opID};
}

void UnaryOperator::CompileFunc(StdCompiler *comp)
{
	Expression::CompileFunc(comp);
	comp->Value(mkNamingAdapt(mkOperatorAdapt(opID), "Operator"));
	comp->Value(mkNamingAdapt(side, "Expression"));
}

std::generator<std::string> UnaryOperator::ToTreeInternal() const
{
	co_yield std::format("(op.unary {:?}", C4ScriptOpMap[opID].Identifier);
	co_yield std::ranges::elements_of(IndentElementsOf(side->ToTreeInternal()));
	co_yield ")";
}

void BinaryOperator::CompileFunc(StdCompiler *comp)
{
	Expression::CompileFunc(comp);
	comp->Value(mkNamingAdapt(mkOperatorAdapt(opID), "Operator"));
	comp->Value(mkNamingAdapt(lhs, "LHS"));
	comp->Value(mkNamingAdapt(rhs, "RHS"));
}

std::generator<std::string> BinaryOperator::ToTreeInternal() const
{
	co_yield std::format("(op.binary {:?}", C4ScriptOpMap[opID].Identifier);
	co_yield std::ranges::elements_of(IndentElementsOf(lhs->ToTreeInternal()));
	co_yield std::ranges::elements_of(IndentElementsOf(rhs->ToTreeInternal()));
	co_yield ")";
}

void Prototype::Parameter::CompileFunc(StdCompiler *const comp)
{
	comp->Value(mkNamingAdapt(mkCastIntAdapt(Type), "Type"));
	comp->Value(mkNamingAdapt(Name, "Name")),
	comp->Value(mkNamingAdapt(IsRef, "IsRef"));
}

void Prototype::CompileFunc(StdCompiler *const comp)
{
	Statement::CompileFunc(comp);

	comp->Value(mkNamingAdapt(mkCastIntAdapt(returnType), "ReturnType"));
	comp->Value(mkNamingAdapt(returnRef,  "ReturnRef"));
	comp->Value(mkNamingAdapt(mkCastIntAdapt(access),     "Access"));
	comp->Value(mkNamingAdapt(name,       "Name"));
	comp->Value(mkNamingAdapt(mkSTLContainerAdapt(parameters), "Parameters"));
}

std::generator<std::string> Prototype::ToTreeInternal() const
{
	co_return;
}

void Function::CompileFunc(StdCompiler *const comp)
{
	Statement::CompileFunc(comp);

	comp->Value(mkNamingAdapt(description, "Description"));
	comp->Value(mkNamingAdapt(prototype, "Prototype"));
	comp->Value(mkNamingAdapt(body, "Body"));
}

std::generator<std::string> Function::ToTreeInternal() const
{
	co_return;
}

void Block::CompileFunc(StdCompiler *const comp)
{
	Statement::CompileFunc(comp);

	comp->Value(mkNamingAdapt(mkSTLContainerAdapt(statements), "Statements"));
}

std::generator<std::string> Block::ToTreeInternal() const
{
	co_yield "(block";
	for (const auto &statement : statements)
	{
		co_yield std::ranges::elements_of(IndentElementsOf(statement->ToTreeInternal()));
	}
	co_yield ")";
}

void Include::CompileFunc(StdCompiler *const comp)
{
	Statement::CompileFunc(comp);

	comp->Value(mkNamingAdapt(mkC4IDAdapt(id), "ID"));
	comp->Value(mkNamingAdapt(nowarn, "NoWarn"));
}

std::generator<std::string> Include::ToTreeInternal() const
{
	co_yield std::format("(include {}{})", C4IdText(id), nowarn ? " nowarn" : "");
}

void Append::CompileFunc(StdCompiler *const comp)
{
	Statement::CompileFunc(comp);

	comp->Value(mkNamingAdapt(mkC4IDAdapt(id), "ID"));
	comp->Value(mkNamingAdapt(nowarn, "NoWarn"));
}

std::generator<std::string> Append::ToTreeInternal() const
{
	co_yield std::format("(appendto {}{})", id == -1 ? "*" : C4IdText(id), nowarn ? " nowarn" : "");
}

void Strict::CompileFunc(StdCompiler *const comp)
{
	Statement::CompileFunc(comp);

	comp->Value(mkNamingAdapt(strict, "Strict"));
}

std::generator<std::string> Strict::ToTreeInternal() const
{
	co_yield std::format("(strict {})", std::to_underlying(strict));
}

void Return::CompileFunc(StdCompiler *const comp)
{
	Statement::CompileFunc(comp);
	comp->Value(mkNamingAdapt(mkSTLContainerAdapt(expressions), "Expression"));
	comp->Value(mkNamingAdapt(preferStack, "PreferStack"));
}

std::generator<std::string> Return::ToTreeInternal() const
{
	std::string header{"(return"};
	if (preferStack)
	{
		header += " preferstack";
	}

	co_yield std::move(header);

	if (expressions.empty())
	{
		co_yield "\t(nil)";
	}
	else
	{
		for (const auto &expression : expressions)
		{
			co_yield std::ranges::elements_of(IndentElementsOf(expression->ToTreeInternal()));
		}
	}

	co_yield ")";
}

void ReturnAsParam::CompileFunc(StdCompiler *const comp)
{
	Expression::CompileFunc(comp);
	comp->Value(mkNamingAdapt(StdPtrAdapt{expression, true}, "Expression"));
	comp->Value(mkNamingAdapt(preferStack, "PreferStack"));
}

std::generator<std::string> ReturnAsParam::ToTreeInternal() const
{
	std::string header{"(returnparam"};
	if (preferStack)
	{
		header += " preferstack";
	}

	co_yield std::move(header);

	if (expression)
	{
		co_yield std::ranges::elements_of(IndentElementsOf(expression->ToTreeInternal()));
	}
	else
	{
		co_yield "\t(nil)";
	}
	co_yield ")";
}

void FunctionCall::CompileFunc(StdCompiler *const comp)
{
	Expression::CompileFunc(comp);
	comp->Value(mkNamingAdapt(identifier, "Identifier"));
	comp->Value(mkNamingAdapt(mkSTLContainerAdapt(arguments), "Arguments"));
}

std::generator<std::string> FunctionCall::ToTreeInternal() const
{
	co_yield std::format("(call {:?}", identifier);
	for (const auto &argument : arguments)
	{
		co_yield std::ranges::elements_of(IndentElementsOf(argument->ToTreeInternal()));
	}
	co_yield ")";
}

void ArrayAccess::CompileFunc(StdCompiler *const comp)
{
	Expression::CompileFunc(comp);
	NilTestExpression::CompileFunc(comp);
	NoRef::CompileFunc(comp);

	comp->Value(mkNamingAdapt(lhs, "LHS"));
	comp->Value(mkNamingAdapt(rhs, "RHS"));
}

std::generator<std::string> ArrayAccess::ToTreeInternal() const
{
	co_yield std::format("(array.access{}", IsNoRef() ? "" : ".ref");
	co_yield std::ranges::elements_of(IndentElementsOf(lhs->ToTreeInternal()));
	co_yield std::ranges::elements_of(IndentElementsOf(rhs->ToTreeInternal()));
	co_yield ")";
}

void ArrayAccess::OnSetNoRef()
{
	lhs = C4AulAST::NoRef::SetNoRef(std::move(lhs));
}

void ArrayAppend::CompileFunc(StdCompiler *const comp)
{
	Expression::CompileFunc(comp);
	NilTestExpression::CompileFunc(comp);

	comp->Value(mkNamingAdapt(array, "Array"));
}

std::generator<std::string> ArrayAppend::ToTreeInternal() const
{
	co_yield std::format("(array.append");
	co_yield std::ranges::elements_of(IndentElementsOf(array->ToTreeInternal()));
	co_yield ")";
}

void PropertyAccess::OnSetNoRef()
{
	object = NoRef::SetNoRef(std::move(object));
}

void PropertyAccess::CompileFunc(StdCompiler *const comp)
{
	Expression::CompileFunc(comp);
	NilTestExpression::CompileFunc(comp);
	NoRef::CompileFunc(comp);

	comp->Value(mkNamingAdapt(object, "Object"));
	comp->Value(mkNamingAdapt(property, "Property"));
}

std::generator<std::string> PropertyAccess::ToTreeInternal() const
{
	co_yield std::format("(property.access{} {:?}", IsNoRef() ? "" : ".ref", property);
	co_yield std::ranges::elements_of(IndentElementsOf(object->ToTreeInternal()));
	co_yield ")";
}

void IndirectCall::CompileFunc(StdCompiler *const comp)
{
	Expression::CompileFunc(comp);
	NilTestExpression::CompileFunc(comp);

	comp->Value(mkNamingAdapt(globalCall, "GlobalCall"));

	if (!globalCall)
	{
		comp->Value(mkNamingAdapt(callee, "Callee"));
	}
	comp->Value(mkNamingAdapt(failSafe, "FailSafe"));
	comp->Value(mkNamingAdapt(mkC4IDAdapt(namespaceId), "NamespaceId"));
	comp->Value(mkNamingAdapt(identifier, "Identifier"));
	comp->Value(mkNamingAdapt(mkSTLContainerAdapt(arguments), "Arguments"));
}

std::generator<std::string> IndirectCall::ToTreeInternal() const
{
	std::string header{std::format("(icall {:?}", identifier)};
	if (namespaceId)
	{
		header += std::format(" {}", C4IdText(namespaceId));
	}

	if (globalCall)
	{
		header += " global";
	}

	if (failSafe)
	{
		header += " failsafe" ;
	}

	co_yield std::move(header);

	if (!globalCall)
	{
		co_yield std::ranges::elements_of(IndentElementsOf(callee->ToTreeInternal()));
	}

	for (const auto &argument : arguments)
	{
		co_yield std::ranges::elements_of(IndentElementsOf(argument->ToTreeInternal()));
	}

	co_yield ")";
}

void If::CompileFunc(StdCompiler *const comp)
{
	Statement::CompileFunc(comp);

	comp->Value(mkNamingAdapt(condition, "Condition"));
	comp->Value(mkNamingAdapt(StdPtrAdapt{then, true}, "Then"));
	comp->Value(mkNamingAdapt(StdPtrAdapt{other, true}, "Else"));
}

template<std::derived_from<Statement> T>
static std::generator<std::string> StatementWithName(const std::string_view name, const std::unique_ptr<T> &statement)
{
	co_yield std::format("({}", name);
	co_yield std::ranges::elements_of(IndentElementsOf(statement->ToTreeInternal()));
	co_yield ")";
}

template<std::derived_from<Statement> T>
static std::generator<std::string> StatementIfPresent(const std::string_view name, const std::unique_ptr<T> &statement)
{
	if (statement)
	{
		co_yield std::format("({}", name);
		co_yield std::ranges::elements_of(IndentElementsOf(statement->ToTreeInternal()));
		co_yield ")";
	}
}

std::generator<std::string> If::ToTreeInternal() const
{
	co_yield "(if";
	co_yield std::ranges::elements_of(IndentElementsOf([this]() -> std::generator<std::string>
	{
		co_yield std::ranges::elements_of(StatementIfPresent("condition", condition));
		co_yield std::ranges::elements_of(StatementIfPresent("then", then));
		co_yield std::ranges::elements_of(StatementIfPresent("else", other));
	}()));
	co_yield ")";
}

void ExprIf::CompileFunc(StdCompiler *comp)
{
	Expression::CompileFunc(comp);

	comp->Value(mkNamingAdapt(condition, "Condition"));
	comp->Value(mkNamingAdapt(StdPtrAdapt{then, true}, "Then"));
	comp->Value(mkNamingAdapt(StdPtrAdapt{other, true}, "Else"));
}

std::generator<std::string> ExprIf::ToTreeInternal() const
{
	co_yield "(if";
	co_yield std::ranges::elements_of(IndentElementsOf([this]() -> std::generator<std::string>
	{
		co_yield std::ranges::elements_of(StatementIfPresent("condition", condition));
		co_yield std::ranges::elements_of(StatementIfPresent("then", then));
		co_yield std::ranges::elements_of(StatementIfPresent("else", other));
	}()));
	co_yield ")";
}

void While::CompileFunc(StdCompiler *const comp)
{
	Statement::CompileFunc(comp);
	comp->Value(mkNamingAdapt(condition, "Condition"));
	comp->Value(mkNamingAdapt(StdPtrAdapt{body, true}, "Body"));
}

std::generator<std::string> While::ToTreeInternal() const
{
	co_yield "(while";
	co_yield std::ranges::elements_of(IndentElementsOf([this]() -> std::generator<std::string>
	{
		co_yield std::ranges::elements_of(StatementWithName("condition", condition));
		co_yield std::ranges::elements_of(StatementIfPresent("body", body));
	}()));
	co_yield ")";
}

void For::CompileFunc(StdCompiler *const comp)
{
	Statement::CompileFunc(comp);
	comp->Value(mkNamingAdapt(StdPtrAdapt{init, true}, "Init"));
	comp->Value(mkNamingAdapt(StdPtrAdapt{condition, true}, "Condition"));
	comp->Value(mkNamingAdapt(StdPtrAdapt{after, true}, "After"));
	comp->Value(mkNamingAdapt(StdPtrAdapt{body, true}, "Body"));
}

std::generator<std::string> For::ToTreeInternal() const
{
	co_yield "(for";
	co_yield std::ranges::elements_of(IndentElementsOf([this]() -> std::generator<std::string>
	{
		co_yield std::ranges::elements_of(StatementIfPresent("init", init));
		co_yield std::ranges::elements_of(StatementIfPresent("condition", condition));
		co_yield std::ranges::elements_of(StatementIfPresent("after", after));
		co_yield std::ranges::elements_of(StatementIfPresent("body", body));
	}()));
	co_yield ")";
}

void ForEach::CompileFunc(StdCompiler *const comp)
{
	Statement::CompileFunc(comp);
	comp->Value(mkNamingAdapt(init, "Init"));
	comp->Value(mkNamingAdapt(iterable, "Iterable"));
	comp->Value(mkNamingAdapt(StdPtrAdapt{body, true}, "Body"));
}

std::generator<std::string> ForEach::ToTreeInternal() const
{
	co_yield "(foreach";
	co_yield std::ranges::elements_of(IndentElementsOf([this]() -> std::generator<std::string>
	{
		co_yield std::ranges::elements_of(StatementWithName("init", init));
		co_yield std::ranges::elements_of(StatementWithName("iterable", iterable));
		co_yield std::ranges::elements_of(StatementIfPresent("body", body));
	}()));
	co_yield ")";
}

void Inherited::CompileFunc(StdCompiler *const comp)
{
	FunctionCall::CompileFunc(comp);
	comp->Value(mkNamingAdapt(failSafe, "FailSafe"));

	if (failSafe)
	{
		comp->Value(mkNamingAdapt(found, "Found"));
	}
}

std::generator<std::string> Inherited::ToTreeInternal() const
{
	std::string header{std::format("(inherited {:?}", identifier)};
	if (failSafe)
	{
		header += " failsafe";
		header += found ? " found" : " notfound";
	}

	co_yield std::move(header);

	for (const auto &argument : arguments)
	{
		co_yield std::ranges::elements_of(IndentElementsOf(argument->ToTreeInternal()));
	}

	co_yield ")";
}

std::generator<std::string> Break::ToTreeInternal() const
{
	co_yield "(break)";
}

std::generator<std::string> Continue::ToTreeInternal() const
{
	co_yield "(continue)";
}

std::generator<std::string> This::ToTreeInternal() const
{
	co_yield "(this)";
}

std::generator<std::string> Nop::ToTreeInternal() const
{
	co_yield "(nop)";
}

std::generator<std::string> Error::ToTreeInternal() const
{
	co_yield "(error)";
}

}

C4AulError::C4AulError() {}

void C4AulError::show() const
{
	// simply log error message
	if (!message.empty())
		DebugLog(isWarning ? spdlog::level::warn : spdlog::level::err, message);
}

C4AulFunc::C4AulFunc(C4AulScript *pOwner, const char *pName, bool bAtEnd) :
	MapNext(nullptr),
	LinkedTo(nullptr),
	OverloadedBy(nullptr),
	NextSNFunc(nullptr)
{
	// reg2list (at end or at the beginning)
	Owner = pOwner;
	if (bAtEnd)
	{
		if (Prev = Owner->FuncL)
		{
			Prev->Next = this;
			Owner->FuncL = this;
		}
		else
		{
			Owner->Func0 = this;
			Owner->FuncL = this;
		}
		Next = nullptr;
	}
	else
	{
		if (Next = Owner->Func0)
		{
			Next->Prev = this;
			Owner->Func0 = this;
		}
		else
		{
			Owner->Func0 = this;
			Owner->FuncL = this;
		}
		Prev = nullptr;
	}

	// store name
	SCopy(pName, Name, C4AUL_MAX_Identifier);
	// add to global lookuptable with this name
	Owner->Engine->FuncLookUp.Add(this, bAtEnd);
}

C4AulFunc::~C4AulFunc()
{
	// if it's a global: remove the global link!
	if (LinkedTo && Owner)
		if (LinkedTo->Owner == Owner->Engine)
			delete LinkedTo;
	// unlink func
	if (LinkedTo)
	{
		// find prev
		C4AulFunc *pAkt = this;
		while (pAkt->LinkedTo != this) pAkt = pAkt->LinkedTo;
		if (pAkt == LinkedTo)
			pAkt->LinkedTo = nullptr;
		else
			pAkt->LinkedTo = LinkedTo;
		LinkedTo = nullptr;
	}
	// remove from list
	if (Prev) Prev->Next = Next;
	if (Next) Next->Prev = Prev;
	if (Owner)
	{
		if (Owner->Func0 == this) Owner->Func0 = Next;
		if (Owner->FuncL == this) Owner->FuncL = Prev;
		Owner->Engine->FuncLookUp.Remove(this);
	}
}

void C4AulFunc::DestroyLinked()
{
	// delete all functions linked to this one.
	while (LinkedTo)
		delete LinkedTo;
}

C4AulFunc *C4AulFunc::GetLocalSFunc(const char *szIdtf)
{
	// owner is engine, i.e. this is a global func?
	if (Owner == Owner->Engine && LinkedTo)
	{
		// then search linked scope first
		if (C4AulFunc *pFn = LinkedTo->Owner->GetSFunc(szIdtf)) return pFn;
	}
	// search local owner list
	return Owner->GetSFunc(szIdtf);
}

C4AulFunc *C4AulFunc::FindSameNameFunc(C4Def *pScope)
{
	// Note: NextSNFunc forms a ring, not a list
	// find function
	C4AulFunc *pFunc = this, *pResult = nullptr;
	do
	{
		// definition matches? This is the one
		if (pFunc->Owner->Def == pScope)
		{
			pResult = pFunc; break;
		}
		// global function? Set func, but continue searching
		// (may be overloaded by a local fn)
		if (pFunc->Owner == Owner->Engine)
			pResult = pFunc;
	} while ((pFunc = pFunc->NextSNFunc) && pFunc != this);
	return pResult;
}

std::string C4AulScriptFunc::GetFullName()
{
	// "lost" function?
	std::string owner;
	if (!Owner)
	{
		owner = "(unknown) ";
	}
	else if (Owner->Def)
	{
		owner = std::format("{}::", C4IdText(Owner->Def->id));
	}
	else if (Owner->Engine == Owner)
	{
		owner = "global ";
	}
	else
	{
		owner = "game ";
	}

	owner += Name;
	return owner;
}

C4AulScript::C4AulScript()
{
	// init defaults
	Default();
}

void C4AulScript::Default()
{
	// not compiled
	State = ASS_NONE;
	Script.Clear();
	Code = CPos = nullptr;
	CodeSize = CodeBufSize = 0;
	IncludesResolved = false;

	// defaults
	idDef = C4ID_None;
	Strict = C4AulScriptStrict::NONSTRICT;
	Preparsing = Resolving = false;
	Temporary = false;
	LocalNamed.Reset();

	// prepare lists
	Child0 = ChildL = Prev = Next = nullptr;
	Owner = Engine = nullptr;
	Func0 = FuncL = nullptr;
	// prepare include list
	Includes.clear();
	Appends.clear();
}

C4AulScript::~C4AulScript()
{
	// clear
	Clear();
	// unreg
	Unreg();
}

void C4AulScript::Unreg()
{
	// remove from list
	if (Prev) Prev->Next = Next; else if (Owner) Owner->Child0 = Next;
	if (Next) Next->Prev = Prev; else if (Owner) Owner->ChildL = Prev;
	Prev = Next = Owner = nullptr;
}

void C4AulScript::Clear()
{
	// remove includes
	Includes.clear();
	Appends.clear();
	// delete child scripts + funcs
	while (Child0)
		if (Child0->Delete()) delete Child0; else Child0->Unreg();
	while (Func0) delete Func0;
	// delete script+code
	Script.Clear();
	delete[] Code; Code = nullptr;
	CodeSize = CodeBufSize = 0;
	// reset flags
	State = ASS_NONE;
}

void C4AulScript::Reg2List(C4AulScriptEngine *pEngine, C4AulScript *pOwner)
{
	// already regged? (def reloaded)
	if (Owner) return;
	// reg to list
	Engine = pEngine;
	if (Owner = pOwner)
	{
		if (Prev = Owner->ChildL)
			Prev->Next = this;
		else
			Owner->Child0 = this;
		Owner->ChildL = this;
	}
	else
		Prev = nullptr;
	Next = nullptr;
}

C4AulFunc *C4AulScript::GetOverloadedFunc(C4AulFunc *ByFunc)
{
	assert(ByFunc);
	// search local list
	C4AulFunc *f = ByFunc;
	if (f) f = f->Prev; else f = FuncL;
	while (f)
	{
		if (SEqual(ByFunc->Name, f->Name)) break;
		f = f->Prev;
	}
#ifndef NDEBUG
	C4AulFunc *f2 = Engine ? Engine->GetFunc(ByFunc->Name, this, ByFunc) : nullptr;
	assert(f == f2);
#endif
	// nothing found? then search owner, if existent
	if (!f && Owner)
	{
		if (f = Owner->GetFuncRecursive(ByFunc->Name))
			// just found the global link?
			if (ByFunc && f->LinkedTo == ByFunc)
				f = Owner->GetOverloadedFunc(f);
	}
	// return found fn
	return f;
}

C4AulFunc *C4AulScript::GetFuncRecursive(const char *pIdtf)
{
	// search local list
	C4AulFunc *f = GetFunc(pIdtf);
	if (f) return f;
	// nothing found? then search owner, if existent
	else if (Owner) return Owner->GetFuncRecursive(pIdtf);
	return nullptr;
}

C4AulFunc *C4AulScript::GetFunc(const char *pIdtf)
{
	return Engine ? Engine->GetFunc(pIdtf, this, nullptr) : nullptr;
}

C4AulScriptFunc *C4AulScript::GetSFuncWarn(const char *pIdtf, C4AulAccess AccNeeded, const char *WarnStr)
{
	// no identifier
	if (!pIdtf || !pIdtf[0]) return nullptr;
	// get func?
	C4AulScriptFunc *pFn = GetSFunc(pIdtf, AccNeeded, true);
	if (!pFn)
		Warn(std::format("Error getting {} function '{}'", WarnStr, pIdtf), nullptr);
	return pFn;
}

C4AulScriptFunc *C4AulScript::GetSFunc(const char *pIdtf, C4AulAccess AccNeeded, bool fFailsafe)
{
	// failsafe call
	if (*pIdtf == '~') { fFailsafe = true; pIdtf++; }

	// get function reference from table
	C4AulScriptFunc *pFn = GetSFunc(pIdtf);

	// undefined function
	if (!pFn)
	{
		// not failsafe?
		if (!fFailsafe)
		{
			// show error
			C4AulParseError err(this, "Undefined function: ", pIdtf);
			err.show();
		}
		return nullptr;
	}

	// check access
	if (pFn->Access < AccNeeded)
	{
		// no access? show error
		C4AulParseError err(this, "insufficient access level");
		err.show();
		// don't even break in strict execution, because the caller might be non-strict
	}

	// return found function
	return pFn;
}

C4AulScriptFunc *C4AulScript::GetSFunc(const char *pIdtf)
{
	// get func by name; return script func
	if (!pIdtf) return nullptr;
	if (!pIdtf[0]) return nullptr;
	if (pIdtf[0] == '~') pIdtf++;
	C4AulFunc *f = GetFunc(pIdtf);
	if (!f) return nullptr;
	return f->SFunc();

}

C4AulScriptFunc *C4AulScript::GetSFunc(int iIndex, const char *szPattern, C4AulAccess AccNeeded)
{
	C4AulFunc *f;
	C4AulScriptFunc *sf;
	// loop through script funcs
	f = FuncL;
	while (f)
	{
		if (sf = f->SFunc(); sf)
			if (!szPattern || SEqual2(sf->Name, szPattern))
			{
				if (!iIndex)
				{
					if (sf->Access < AccNeeded)
					{
						C4AulParseError err(this, "insufficient access level");
						err.show();
					}
					return sf;
				}
				--iIndex;
			}
		f = f->Prev;
	}

	// indexed script func not found
	return nullptr;
}

C4AulAccess C4AulScript::GetAllowedAccess(C4AulFunc *func, C4AulScript *caller)
{
	C4AulScriptFunc *sfunc = func->SFunc();

	if (
		!sfunc ||
		sfunc->pOrgScript == caller ||
		std::find(sfunc->pOrgScript->Includes.begin(), sfunc->pOrgScript->Includes.end(), caller->idDef) != sfunc->pOrgScript->Includes.end() ||
		std::find(caller->Includes.begin(), caller->Includes.end(), sfunc->pOrgScript->idDef) != caller->Includes.end() ||
		std::find(sfunc->pOrgScript->Appends.begin(), sfunc->pOrgScript->Appends.end(), caller->idDef) != sfunc->pOrgScript->Appends.end() ||
		std::find(caller->Appends.begin(), caller->Appends.end(), sfunc->pOrgScript->idDef) != caller->Appends.end()
	)
	{
		return AA_PRIVATE;
	}
	else if (sfunc->pOrgScript->Strict >= C4AulScriptStrict::STRICT3 &&
		caller->Strict >= C4AulScriptStrict::STRICT3)
	{
		return sfunc->Access;
	}

	return AA_PRIVATE;
}

void C4AulScriptFunc::CopyBody(C4AulScriptFunc &FromFunc)
{
	// copy some members, that are set before linking
	Access = FromFunc.Access;
	Desc.Copy(FromFunc.Desc);
	DescText.Copy(FromFunc.DescText);
	DescLong.Copy(FromFunc.DescLong);
	Condition = FromFunc.Condition;
	idImage = FromFunc.idImage;
	iImagePhase = FromFunc.iImagePhase;
	ControlMethod = FromFunc.ControlMethod;
	Script = FromFunc.Script;
	VarNamed = FromFunc.VarNamed;
	ParNamed = FromFunc.ParNamed;
	bNewFormat = FromFunc.bNewFormat;
	bReturnRef = FromFunc.bReturnRef;
	pOrgScript = FromFunc.pOrgScript;
	for (int i = 0; i < C4AUL_MAX_Par; i++)
		ParType[i] = FromFunc.ParType[i];
	// must reset field here
	NextSNFunc = nullptr;
}

// C4AulScriptEngine

C4AulScriptEngine::C4AulScriptEngine() :
	warnCnt(0), errCnt(0), nonStrictCnt(0), lineCnt(0)
{
	// /me r b engine
	Engine = this;
	ScriptName = C4CFN_System;
	Strict = C4AulScriptStrict::MAXSTRICT;

	Global.Reset();

	GlobalNamedNames.Reset();
	GlobalNamed.Reset();
	GlobalNamed.SetNameList(&GlobalNamedNames);
	GlobalConstNames.Reset();
	GlobalConsts.Reset();
	GlobalConsts.SetNameList(&GlobalConstNames);
}

C4AulScriptEngine::~C4AulScriptEngine() { Clear(); }

void C4AulScriptEngine::Clear()
{
	// clear inherited
	C4AulScript::Clear();
	// clear own stuff
	// reset values
	warnCnt = errCnt = nonStrictCnt = lineCnt = 0;
	// resetting name lists will reset all data lists, too
	// except not...
	Global.Reset();
	GlobalNamedNames.Reset();
	GlobalConstNames.Reset();
	GlobalConsts.Reset();
	GlobalConsts.SetNameList(&GlobalConstNames);
	GlobalNamed.Reset();
	GlobalNamed.SetNameList(&GlobalNamedNames);
}

void C4AulScriptEngine::UnLink()
{
	// unlink scripts
	C4AulScript::UnLink();
	// clear string table ("hold" strings only)
	Strings.Clear();
	// Do not clear global variables and constants, because they are registered by the
	// preparser. Note that keeping those fields means that you cannot delete a global
	// variable or constant at runtime by removing it from the script.
}

void C4AulScriptEngine::RegisterGlobalConstant(const char *szName, const C4Value &rValue)
{
	// Register name and set value.
	// AddName returns the index of existing element if the name is assigned already.
	// That is OK, since it will only change the value of the global ("overload" it).
	// A warning would be nice here. However, this warning would show up whenever a script
	// containing globals constants is recompiled.
	GlobalConsts[GlobalConstNames.AddName(szName)] = rValue;
}

bool C4AulScriptEngine::GetGlobalConstant(const char *szName, C4Value *pTargetValue)
{
	// get index of global by name
	int32_t iConstIndex = GlobalConstNames.GetItemNr(szName);
	// not found?
	if (iConstIndex < 0) return false;
	// if it's found, assign the value if desired
	if (pTargetValue) *pTargetValue = GlobalConsts[iConstIndex];
	// constant exists
	return true;
}

bool C4AulScriptEngine::DenumerateVariablePointers()
{
	Global.DenumeratePointers();
	GlobalNamed.DenumeratePointers();
	// runtime data only: don't denumerate consts
	return true;
}

void C4AulScriptEngine::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(Global, "Globals", C4ValueList()));
	C4ValueMapData GlobalNamedDefault;
	GlobalNamedDefault.SetNameList(&GlobalNamedNames);
	pComp->Value(mkNamingAdapt(GlobalNamed, "GlobalNamed", GlobalNamedDefault));
}

// C4AulFuncMap

static const size_t CapacityInc = 1024;

C4AulFuncMap::C4AulFuncMap() : Capacity(CapacityInc), FuncCnt(0), Funcs(new C4AulFunc *[CapacityInc])
{
	memset(Funcs, 0, sizeof(C4AulFunc *) * Capacity);
}

C4AulFuncMap::~C4AulFuncMap()
{
	delete[] Funcs;
}

unsigned int C4AulFuncMap::Hash(const char *name)
{
	// Fowler/Noll/Vo hash
	unsigned int h = 2166136261u;
	while (*name)
		h = (h ^ * (name++)) * 16777619;
	return h;
}

C4AulFunc *C4AulFuncMap::GetFirstFunc(const char *Name)
{
	if (!Name) return nullptr;
	C4AulFunc *Func = Funcs[Hash(Name) % Capacity];
	while (Func && !SEqual(Name, Func->Name))
		Func = Func->MapNext;
	return Func;
}

C4AulFunc *C4AulFuncMap::GetNextSNFunc(const C4AulFunc *After)
{
	C4AulFunc *Func = After->MapNext;
	while (Func && !SEqual(After->Name, Func->Name))
		Func = Func->MapNext;
	return Func;
}

C4AulFunc *C4AulFuncMap::GetFunc(const char *Name, const C4AulScript *Owner, const C4AulFunc *After)
{
	if (!Name) return nullptr;
	C4AulFunc *Func = Funcs[Hash(Name) % Capacity];
	if (After)
	{
		while (Func && Func != After)
			Func = Func->MapNext;
		if (Func)
			Func = Func->MapNext;
	}
	while (Func && (Func->Owner != Owner || !SEqual(Name, Func->Name)))
		Func = Func->MapNext;
	return Func;
}

void C4AulFuncMap::Add(C4AulFunc *func, bool bAtStart)
{
	if (++FuncCnt > Capacity)
	{
		int NCapacity = Capacity + CapacityInc;
		C4AulFunc **NFuncs = new C4AulFunc *[NCapacity];
		memset(NFuncs, 0, sizeof(C4AulFunc *) * NCapacity);
		for (int i = 0; i < Capacity; ++i)
		{
			while (Funcs[i])
			{
				// Get a pointer to the bucket
				C4AulFunc **pNFunc = &(NFuncs[Hash(Funcs[i]->Name) % NCapacity]);
				// get a pointer to the end of the linked list
				while (*pNFunc) pNFunc = &((*pNFunc)->MapNext);
				// Move the func over
				*pNFunc = Funcs[i];
				// proceed with the next list member
				Funcs[i] = Funcs[i]->MapNext;
				// Terminate the linked list
				(*pNFunc)->MapNext = nullptr;
			}
		}
		Capacity = NCapacity;
		delete[] Funcs;
		Funcs = NFuncs;
	}
	// Get a pointer to the bucket
	C4AulFunc **pFunc = &(Funcs[Hash(func->Name) % Capacity]);
	if (bAtStart)
	{
		// move the current first to the second position
		func->MapNext = *pFunc;
	}
	else
	{
		// get a pointer to the end of the linked list
		while (*pFunc)
		{
			pFunc = &((*pFunc)->MapNext);
		}
	}
	// Add the func
	*pFunc = func;
}

void C4AulFuncMap::Remove(C4AulFunc *func)
{
	C4AulFunc **pFunc = &Funcs[Hash(func->Name) % Capacity];
	while (*pFunc != func)
	{
		pFunc = &((*pFunc)->MapNext);
		assert(*pFunc); // crash on remove of a not contained func
	}
	*pFunc = (*pFunc)->MapNext;
	--FuncCnt;
}
