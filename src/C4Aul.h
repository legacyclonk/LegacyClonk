/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2001, Sven2
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

// C4Aul script engine CP conversion
// (cut C4Aul of classes/structs and put everything in namespace C4Aul instead?)
// drop uncompiled scripts when not in developer mode
// -> build string table
// -> clear the string table in UnLink? ReLink won't happen to be called in player mode anyway

#pragma once

#include <C4AulScriptStrict.h>
#include <C4ValueList.h>
#include <C4ValueMap.h>
#include <C4Id.h>
#include "C4Log.h"
#include <C4Script.h>
#include <C4StringTable.h>
#include "StdAdaptors.h"

#include <cstdint>
#include <generator>
#include <list>
#include <stack>
#include <vector>

// class predefs
class C4AulError;
class C4AulFunc;
class C4AulScriptFunc;
class C4AulScript;
class C4AulScriptEngine;

struct C4AulContext;
struct C4AulBCC;

class C4Def;
class C4DefList;

// consts
#define C4AUL_MAX_String 1024 // max string length
#define C4AUL_MAX_Identifier 100 // max length of function identifiers
#define C4AUL_MAX_Par 10 // max number of parameters

#define C4AUL_ControlMethod_None 0
#define C4AUL_ControlMethod_Classic 1
#define C4AUL_ControlMethod_JumpAndRun 2
#define C4AUL_ControlMethod_All 3

// generic C4Aul error class
class C4AulError
{
protected:
	std::string message;
	bool isWarning{false};

public:
	C4AulError();
	C4AulError(const C4AulError &Error) : message{Error.message}, isWarning{Error.isWarning} {}
	virtual ~C4AulError() {}
	virtual void show() const; // present error message
};

// parse error
class C4AulParseError : public C4AulError
{
	C4AulParseError(std::string_view message, const char *identifier, bool warn);

public:
	C4AulParseError(C4AulScript *pScript, std::string_view msg, const char *pIdtf = nullptr, bool Warn = false);
	C4AulParseError(class C4AulParseState *state, std::string_view msg, const char *pIdtf = nullptr, bool Warn = false);
};

// execution error
class C4AulExecError : public C4AulError
{
	C4Object *cObj;

public:
	C4AulExecError(C4Object *pObj, std::string_view error);
	virtual void show() const override; // present error message
};

// function access
enum C4AulAccess
{
	AA_PRIVATE,
	AA_PROTECTED,
	AA_PUBLIC,
	AA_GLOBAL
};

struct C4AulParSet
{
	C4Value Par[C4AUL_MAX_Par];

	C4AulParSet() {}
	C4AulParSet(const C4Value &par0, const C4Value &par1 = C4Value(), const C4Value &par2 = C4Value(), const C4Value &par3 = C4Value(), const C4Value &par4 = C4Value(),
		const C4Value &par5 = C4Value(), const C4Value &par6 = C4Value(), const C4Value &par7 = C4Value(), const C4Value &par8 = C4Value(), const C4Value &par9 = C4Value())
	{
		Par[0].Set(par0); Par[1].Set(par1); Par[2].Set(par2); Par[3].Set(par3); Par[4].Set(par4);
		Par[5].Set(par5); Par[6].Set(par6); Par[7].Set(par7); Par[8].Set(par8); Par[9].Set(par9);
	}

	C4Value &operator[](int iIdx) { return Par[iIdx]; }
	const C4Value &operator[](int iIdx) const { return Par[iIdx]; }
};

#define Copy2ParSet8(Pars, Vars) Pars[0].Set(Vars##0); Pars[1].Set(Vars##1); Pars[2].Set(Vars##2); Pars[3].Set(Vars##3); Pars[4].Set(Vars##4); Pars[5].Set(Vars##5); Pars[6].Set(Vars##6); Pars[7].Set(Vars##7);
#define Copy2ParSet9(Pars, Vars) Pars[0].Set(Vars##0); Pars[1].Set(Vars##1); Pars[2].Set(Vars##2); Pars[3].Set(Vars##3); Pars[4].Set(Vars##4); Pars[5].Set(Vars##5); Pars[6].Set(Vars##6); Pars[7].Set(Vars##7); Pars[8].Set(Vars##8);

// byte code chunk type
// some special script functions defined hard-coded to reduce the exec context
enum C4AulBCCType
{
	AB_DEREF,        // deref the current value
	AB_MAPA_R,       // map access via .
	AB_MAPA_V,       // not creating a reference
	AB_ARRAYA_R,     // array access
	AB_ARRAYA_V,     // not creating a reference
	AB_ARRAY_APPEND, // always as a reference
	AB_VARN_R,       // a named var
	AB_VARN_V,
	AB_PARN_R,       // a named parameter
	AB_PARN_V,
	AB_LOCALN_R,     // a named local
	AB_LOCALN_V,
	AB_GLOBALN_R,    // a named global
	AB_GLOBALN_V,
	AB_VAR_R,        // Var statement
	AB_VAR_V,
	AB_PAR_R,        // Par statement
	AB_PAR_V,
	AB_FUNC,         // function

	// prefix
	AB_Inc1,   // ++
	AB_Dec1,   // --
	AB_BitNot, // ~
	AB_Not,    // !
	AB_Neg,    // -

	// postfix (whithout second statement)
	AB_Inc1_Postfix, // ++
	AB_Dec1_Postfix, // --

	// postfix
	AB_Pow,              // **
	AB_Div,              // /
	AB_Mul,              // *
	AB_Mod,              // %
	AB_Sub,              // -
	AB_Sum,              // +
	AB_LeftShift,        // <<
	AB_RightShift,       // >>
	AB_LessThan,         // <
	AB_LessThanEqual,    // <=
	AB_GreaterThan,      // >
	AB_GreaterThanEqual, // >=
	AB_Concat,           // ..
	AB_EqualIdent,       // old ==
	AB_Equal,            // new ==
	AB_NotEqualIdent,    // old !=
	AB_NotEqual,         // new !=
	AB_SEqual,           // S=, eq
	AB_SNEqual,          // ne
	AB_BitAnd,           // &
	AB_BitXOr,           // ^
	AB_BitOr,            // |
	AB_And,              // &&
	AB_Or,               // ||
	AB_NilCoalescing,    // ??
	AB_PowIt,            // **=
	AB_MulIt,            // *=
	AB_DivIt,            // /=
	AB_ModIt,            // %=
	AB_Inc,              // +=
	AB_Dec,              // -=
	AB_LeftShiftIt,      // <<=
	AB_RightShiftIt,     // >>=
	AB_ConcatIt,         // ..=
	AB_AndIt,            // &=
	AB_OrIt,             // |=
	AB_XOrIt,            // ^=
	AB_NilCoalescingIt,  // ??= only the jumping part
	AB_Set,              // =

	AB_CALLGLOBAL,       // global context call
	AB_CALL,             // direct object call
	AB_CALLFS,           // failsafe direct call
	AB_CALLNS,           // direct object call: namespace operator
	AB_STACK,            // push nulls / pop
	AB_NIL,              // constant: nil
	AB_INT,              // constant: int
	AB_BOOL,             // constant: bool
	AB_STRING,           // constant: string
	AB_C4ID,             // constant: C4ID
	AB_ARRAY,            // semi-constant: array
	AB_MAP,              // semi-constant: map
	AB_IVARN,            // initialization of named var
	AB_JUMP,             // jump
	AB_JUMPAND,          // jump if zero, else pop the stack
	AB_JUMPOR,           // jump if not zero, else pop the stack
	AB_JUMPNIL,          // jump if nil, otherwise just continue
	AB_JUMPNOTNIL,       // jump if not nil, otherwise just continue
	AB_CONDN,            // conditional jump (negated, pops stack)
	AB_FOREACH_NEXT,     // foreach: next element in array
	AB_FOREACH_MAP_NEXT, // foreach: next key-value pair in map
	AB_RETURN,           // return statement
	AB_ERR,              // parse error at this position
	AB_EOFN,             // end of function
	AB_EOF,              // end of file
};

// ** a definition of an operator
// there are two classes of operators, the postfix-operators (+,-,&,...) and the
// prefix-operators (mainly !,~,...).
struct C4ScriptOpDef
{
	unsigned short Priority;
	const char *Identifier;
	C4AulBCCType Code;
	bool Postfix;
	bool RightAssociative; // right or left-associative?
	bool NoSecondStatement; // no second statement expected (++/-- postfix)
	C4V_Type RetType; // type returned. ignored by C4V
	C4V_Type Type1;
	C4V_Type Type2;
};
extern C4ScriptOpDef C4ScriptOpMap[];

// byte code chunk
struct C4AulBCC
{
	C4AulBCCType bccType; // chunk type
	std::intptr_t bccX;
	const char *SPos;
};

class C4AulBCCGenerator;

namespace C4Aul
{
	const char *GetTTName(C4AulBCCType e);
	bool IsJumpType(C4AulBCCType type) noexcept;
	void AddBCC(std::intptr_t offset, C4AulBCCType type, std::intptr_t bccX, std::intptr_t &stack,
				C4AulScript &script, bool &jump);
}

namespace C4AulAST
{
enum class NodeType : std::uint16_t
{
	Script,
	Nil,
	IntLiteral,
	C4IDLiteral,
	BoolLiteral,
	StringLiteral,
	ArrayLiteral,
	MapLiteral,
	GlobalConstant,
	ParN,
	Par,
	VarN,
	Var,
	LocalN,
	GlobalN,
	Identifier,
	Declaration,
	Declarations,
	UnaryOperator,
	BinaryOperator,
	Prototype,
	Function,
	Block,
	Include,
	Append,
	Strict,
	Return,
	ReturnAsParam,
	FunctionCall,
	ArrayAccess,
	ArrayAppend,
	PropertyAccess,
	IndirectCall,
	If,
	ExprIf,
	While,
	For,
	ForEach,
	Break,
	Continue,
	Inherited,
	This,
	Nop,
	Error
};

inline void CompileFunc(NodeType &nodeType, StdCompiler *const comp)
{
	if (comp->isCompiler())
	{
		std::int16_t t;
		comp->Word(t);
		nodeType = static_cast<NodeType>(t);
	}
	else
	{
		auto t = static_cast<std::int16_t>(nodeType);
		comp->Word(t);
	}
}

class Statement
{
public:
	explicit Statement(const std::intptr_t position) : position{position} {}
	virtual ~Statement() = 0;

protected:
	explicit Statement() = default;

public:
	virtual void CompileFunc(StdCompiler *const comp)
	{
		if (comp->isDecompiler())
		{
			auto nodeType = GetNodeType();


			static constexpr StdEnumEntry<NodeType> NodeTypes[]
				{
#define NODE(x) {#x, NodeType::x}
				NODE(Script),
				NODE(Nil),
				NODE(IntLiteral),
				NODE(C4IDLiteral),
				NODE(BoolLiteral),
				NODE(StringLiteral),
				NODE(ArrayLiteral),
				NODE(MapLiteral),
				NODE(GlobalConstant),
				NODE(ParN),
				NODE(Par),
				NODE(VarN),
				NODE(Var),
				NODE(LocalN),
				NODE(GlobalN),
				NODE(Identifier),
				NODE(Declaration),
				NODE(Declarations),
				NODE(UnaryOperator),
				NODE(BinaryOperator),
				NODE(Prototype),
				NODE(Function),
				NODE(Block),
				NODE(Include),
				NODE(Append),
				NODE(Strict),
				NODE(Return),
				NODE(ReturnAsParam),
				NODE(FunctionCall),
				NODE(ArrayAccess),
				NODE(ArrayAppend),
				NODE(PropertyAccess),
				NODE(IndirectCall),
				NODE(If),
				NODE(ExprIf),
				NODE(While),
				NODE(For),
				NODE(ForEach),
				NODE(Break),
				NODE(Continue),
				NODE(Inherited),
				NODE(This),
				NODE(Nop),
				NODE(Error),
#undef NODE
				};

			comp->Value(mkNamingAdapt(mkEnumAdaptT<NodeType>(nodeType, NodeTypes), "NodeType"));
		}
		comp->Value(mkNamingAdapt(position, "Position"));
	}

	void Emit(C4AulBCCGenerator &generator) const;
	std::intptr_t GetPosition() const { return position; }

	std::string ToTree() const;
	virtual std::generator<std::string> ToTreeInternal() const = 0;
protected:
	virtual void EmitInternal(C4AulBCCGenerator &generator) const = 0;

protected:
	virtual NodeType GetNodeType() const = 0;

protected:
	std::intptr_t position;
};

class Script : public Statement
{
public:
	Script(std::vector<std::unique_ptr<Statement>> statements)
		: Statement{0}, statements{std::move(statements)} {}

public:
	explicit Script() : Script{{}} {}

public:

	const std::vector<std::unique_ptr<Statement>> &GetStatements() const { return statements; }

	void CompileFunc(StdCompiler *comp) override;
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::generator<std::string> ToTreeInternal() const override;

protected:
	NodeType GetNodeType() const override { return NodeType::Script; }

private:
	std::vector<std::unique_ptr<Statement>> statements;

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};

class Expression : public Statement
{
public:
	Expression(const std::intptr_t position, const C4V_Type type) : Statement{position}, type{type} {}
	virtual ~Expression() = 0;

public:
	C4V_Type GetType() const { return type; }

public:
	void CompileFunc(StdCompiler *comp) override;
	void Emit(C4AulBCCGenerator &generator) const;

protected:
	C4V_Type type;

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};

class NoRef
{
public:
	virtual ~NoRef() = 0;

public:
	void SetNoRef() noexcept { noRef = true; OnSetNoRef(); }
	bool IsNoRef() const noexcept { return noRef; }

	template<typename T>
	static std::unique_ptr<T> SetNoRef(std::unique_ptr<T> expression)
	{
		if constexpr (std::derived_from<NoRef, T>)
		{
			static_cast<NoRef *>(expression.get())->SetNoRef();
		}
		else if (auto noRef = dynamic_cast<NoRef *>(expression.get()))
		{
			noRef->SetNoRef();
		}

		return expression;
	}

	void CompileFunc(StdCompiler *comp);

protected:
	virtual void OnSetNoRef() {}

private:
	bool noRef{false};
};

class Literal : public Expression
{
public:
	using Expression::Expression;

protected:
	Literal() : Expression{{}, {}} {}
};

class ConstantLiteral : public Literal
{
public:
	using Literal::Literal;

public:
	virtual std::string GetLiteralString() const = 0;
	std::generator<std::string> ToTreeInternal() const override { co_yield GetLiteralString(); }
};

class Nil : public ConstantLiteral
{
public:
	Nil(const std::intptr_t position, const bool preferStack = false)
		: ConstantLiteral{position, C4V_Any}, preferStack{preferStack} {}

protected:
	explicit Nil() : ConstantLiteral{} {}

public:
	void CompileFunc(StdCompiler *comp) override;
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::string GetLiteralString() const override;

protected:
	NodeType GetNodeType() const override { return NodeType::Nil; }

protected:
	bool preferStack{false};

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;

};

class IntLiteral : public ConstantLiteral
{
public:
	IntLiteral(const std::intptr_t position, const C4ValueInt value) : ConstantLiteral{position, C4V_Int}, value{value} {}

protected:
	explicit IntLiteral() : ConstantLiteral{} {}

public:
	void CompileFunc(StdCompiler *comp) override;
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::string GetLiteralString() const override;

	void Negate() noexcept { value = -value; }

protected:
	NodeType GetNodeType() const override { return NodeType::IntLiteral; }

protected:
	C4ValueInt value;

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};


class BoolLiteral : public ConstantLiteral
{
public:
	BoolLiteral(const std::intptr_t position, const bool value) : ConstantLiteral{position, C4V_Bool}, value{value} {}

protected:
	explicit BoolLiteral() : ConstantLiteral{} {}

public:
	void CompileFunc(StdCompiler *comp) override;
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::string GetLiteralString() const override;

protected:
	NodeType GetNodeType() const override { return NodeType::BoolLiteral; }

protected:
	bool value;

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};

class C4IDLiteral : public ConstantLiteral
{
public:
	C4IDLiteral(const std::intptr_t position, const C4ID value) : ConstantLiteral{position, C4V_C4ID}, value{value} {}

protected:
	explicit C4IDLiteral() : ConstantLiteral{} {}

public:
	void CompileFunc(StdCompiler *comp) override;
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::string GetLiteralString() const override;

protected:
	NodeType GetNodeType() const override { return NodeType::C4IDLiteral; }

protected:
	C4ID value;

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};


class StringLiteral : public ConstantLiteral
{
public:
	StringLiteral(const std::intptr_t position, std::string value) : ConstantLiteral{position, C4V_String}, value{std::move(value)} {}

protected:
	explicit StringLiteral() : ConstantLiteral{} {}

public:
	void CompileFunc(StdCompiler *comp) override;
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::string GetLiteralString() const override;

protected:
	NodeType GetNodeType() const override { return NodeType::StringLiteral; }

protected:
	std::string value;

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};


class ArrayLiteral : public Literal
{
public:
	ArrayLiteral(const std::intptr_t position, std::vector<std::unique_ptr<Expression>> expressions)
		: Literal{position, C4V_Array}, expressions{std::move(expressions)} {}

protected:
	explicit ArrayLiteral() : ArrayLiteral({}, {}) {}

public:
	void CompileFunc(StdCompiler *comp) override;
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::generator<std::string> ToTreeInternal() const override;

protected:
	NodeType GetNodeType() const override { return NodeType::ArrayLiteral; }

protected:
	std::vector<std::unique_ptr<Expression>> expressions;

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};

class MapLiteral : public Literal
{
public:
	struct KeyValuePair
	{
		std::unique_ptr<Expression> Key;
		std::unique_ptr<Expression> Value;

		void CompileFunc(StdCompiler *comp);
		std::generator<std::string> ToTreeInternal() const;
	};

		   //using KeyValuePair = std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>;

public:
	MapLiteral(const std::intptr_t position, std::vector<KeyValuePair> expressions)
		: Literal{position, C4V_Map}, expressions{std::move(expressions)} {}

protected:
	explicit MapLiteral() : MapLiteral({}, {}) {}

public:
	void CompileFunc(StdCompiler *comp) override;
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::generator<std::string> ToTreeInternal() const override;

protected:
	NodeType GetNodeType() const override { return NodeType::MapLiteral; }

protected:
	std::vector<KeyValuePair> expressions;

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};

class GlobalConstant : public Expression
{
public:
	GlobalConstant(const std::intptr_t position, std::string identifier, std::unique_ptr<C4AulAST::Literal> value)
		: Expression{position, value->GetType()}, identifier{std::move(identifier)}, value{std::move(value)} {}

protected:
	explicit GlobalConstant() : Expression({}, {}) {}

public:
	void CompileFunc(StdCompiler *comp) override;
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::generator<std::string> ToTreeInternal() const override;

	const std::unique_ptr<C4AulAST::Literal> &GetValue() const noexcept { return value; }

protected:
	NodeType GetNodeType() const override { return NodeType::GlobalConstant; }

protected:
	std::string identifier;
	std::unique_ptr<C4AulAST::Literal> value;

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};

class ParN : public Expression, public NoRef
{
public:
	ParN(const std::intptr_t position, const C4V_Type type, const std::size_t n)
		: Expression{position, type}, n{n} {}

protected:
	explicit ParN() : ParN{{}, {}, {}} {}

public:

	void CompileFunc(StdCompiler *comp) override;
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::generator<std::string> ToTreeInternal() const override;

protected:
	NodeType GetNodeType() const override { return NodeType::ParN; }

protected:
	std::size_t n;

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};

class VarN : public Expression, public NoRef
{
public:
	VarN(const std::intptr_t position, const C4V_Type type, std::string identifier)
		: Expression{position, type}, identifier{std::move(identifier)} {}

protected:
	explicit VarN() : VarN{{}, {}, {}} {}

public:

	void CompileFunc(StdCompiler *comp) override;
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::generator<std::string> ToTreeInternal() const override;

protected:
	NodeType GetNodeType() const override { return NodeType::VarN; }

protected:
	std::string identifier;

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};


class Par : public Expression, public NoRef
{
public:
	Par(const std::intptr_t position, const C4V_Type type, std::unique_ptr<Expression> expression)
		: Expression{position, type}, expression{std::move(expression)} {}

protected:
	explicit Par() : Par{{}, {}, {}} {}

public:

	void CompileFunc(StdCompiler *comp) override;
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::generator<std::string> ToTreeInternal() const override;

protected:
	NodeType GetNodeType() const override { return NodeType::Par; }

protected:
	std::unique_ptr<Expression> expression;

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};

class Var : public Expression, public NoRef
{
public:
	Var(const std::intptr_t position, const C4V_Type type, std::unique_ptr<Expression> expression)
		: Expression{position, type}, expression{std::move(expression)} {}

protected:
	explicit Var() : Var{{}, {}, {}} {}

public:

	void CompileFunc(StdCompiler *comp) override;
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::generator<std::string> ToTreeInternal() const override;

protected:
	NodeType GetNodeType() const override { return NodeType::Var; }

protected:
	std::unique_ptr<Expression> expression;

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};


class LocalN : public Expression, public NoRef
{
public:
	LocalN(const std::intptr_t position, const C4V_Type type, std::string identifier)
		: Expression{position, type}, identifier{std::move(identifier)} {}

protected:
	explicit LocalN() : LocalN{{}, {}, {}} {}

public:

	void CompileFunc(StdCompiler *comp) override;
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::generator<std::string> ToTreeInternal() const override;

protected:
	NodeType GetNodeType() const override { return NodeType::VarN; }

protected:
	std::string identifier;

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};

class GlobalN : public Expression, public NoRef
{
public:
	GlobalN(const std::intptr_t position, const C4V_Type type, std::string identifier)
		: Expression{position, type}, identifier{std::move(identifier)} {}

protected:
	explicit GlobalN() : GlobalN{{}, {}, {}} {}

public:

	void CompileFunc(StdCompiler *comp) override;
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::generator<std::string> ToTreeInternal() const override;

protected:
	NodeType GetNodeType() const override { return NodeType::VarN; }

protected:
	std::string identifier;

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};

class Declaration : public Statement
{
public:
	enum class Type
	{
		Var,
		Local,
		Static,
		StaticConst
	};

public:
	Declaration(const std::intptr_t position, const Type type, std::string identifier, std::unique_ptr<Expression> value)
		: Statement{position}, type{type}, identifier{std::move(identifier)}, value{std::move(value)} {}

protected:
	Declaration() : Declaration{{}, {}, {}, {}} {}

public:
	void CompileFunc(StdCompiler *comp) override;
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::generator<std::string> ToTreeInternal() const override;

	std::string_view GetIdentifier() const noexcept { return identifier; }

protected:
	NodeType GetNodeType() const override { return NodeType::Declaration; }

protected:
	Type type;
	std::string identifier;
	std::unique_ptr<Expression> value;

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};

class Declarations : public Statement
{
public:
	Declarations(const std::intptr_t position, std::vector<std::unique_ptr<Declaration>> declarations)
		: declarations{std::move(declarations)} {}

private:
	explicit Declarations() : Declarations{{}, {}} {}

public:
	void CompileFunc(StdCompiler *comp) override;
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::generator<std::string> ToTreeInternal() const override;

	auto GetIdentifiers() const noexcept { return declarations | std::views::transform(&Declaration::GetIdentifier); }

protected:
	NodeType GetNodeType() const override { return NodeType::Declarations; }

protected:
	std::vector<std::unique_ptr<Declaration>> declarations;

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};

class BinaryExpression : public Expression
{
public:
	BinaryExpression(const std::intptr_t position, const C4V_Type type, std::unique_ptr<Expression> &&lhs, std::unique_ptr<Expression> &&rhs)
		: Expression{position, type}, lhs{std::move(lhs)}, rhs{std::move(rhs)} {}
	virtual ~BinaryExpression() = 0;

protected:
	std::unique_ptr<Expression> lhs;
	std::unique_ptr<Expression> rhs;

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};

class UnaryOperator : public Expression
{
public:
	UnaryOperator(const std::intptr_t position, const std::size_t opID, std::unique_ptr<Expression> &&side)
		: Expression{position, C4ScriptOpMap[opID].RetType}, opID{opID}, side{std::move(side)}
	{
	}

private:
	explicit UnaryOperator() : Expression{{}, {}}, opID{}, side{} {}

public:

	void CompileFunc(StdCompiler *comp) override;
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::generator<std::string> ToTreeInternal() const override;

protected:
	NodeType GetNodeType() const override { return NodeType::UnaryOperator; }

protected:
	std::size_t opID;
	std::unique_ptr<Expression> side;

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};

class BinaryOperator : public BinaryExpression
{
public:
	BinaryOperator(const std::intptr_t position, const std::size_t opID, std::unique_ptr<Expression> &&lhs, std::unique_ptr<Expression> &&rhs)
		: BinaryExpression{position, C4ScriptOpMap[opID].RetType, std::move(lhs), std::move(rhs)}, opID{opID}
	{
		assert(!C4ScriptOpMap[opID].NoSecondStatement);
	}

private:
	explicit BinaryOperator() : BinaryExpression{{}, {}, {}, {}} {}

public:

	void CompileFunc(StdCompiler *comp) override;
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::generator<std::string> ToTreeInternal() const override;

protected:
	NodeType GetNodeType() const override { return NodeType::BinaryOperator; }

protected:
	std::size_t opID;

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};

class Prototype : public Statement
{
public:
	struct Parameter
	{
		C4V_Type Type;
		std::string Name;
		bool IsRef;

		void CompileFunc(StdCompiler *comp);
	};

public:
	Prototype(const std::intptr_t position, const C4V_Type returnType, const bool returnRef, const C4AulAccess access, std::string name, std::vector<Parameter> parameters)
		: Statement{position}, returnType{returnType}, returnRef{returnRef}, access{access}, name{std::move(name)}, parameters{parameters} {}

private:
	explicit Prototype() : Prototype{{}, {}, {}, {}, {}, {}} {}

public:
	const std::string &GetName() const { return name; }
	const std::vector<Parameter> &GetParameters() const { return parameters; }

public:

	void CompileFunc(StdCompiler *comp) override;
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::generator<std::string> ToTreeInternal() const override;

protected:
	NodeType GetNodeType() const override { return NodeType::Prototype; }

protected:
	C4V_Type returnType;
	bool returnRef;
	C4AulAccess access;
	std::string name;
	std::vector<Parameter> parameters;

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};

class Function : public Statement
{
public:
	Function(const std::intptr_t position, std::unique_ptr<Prototype> prototype, std::string description, std::unique_ptr<Statement> body)
		: Statement{position}, prototype{std::move(prototype)}, description{std::move(description)}, body{std::move(body)} {}

private:
	explicit Function() : Function{{}, {}, {}, {}} {}

public:

	void CompileFunc(StdCompiler *comp) override;
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::generator<std::string> ToTreeInternal() const override;

protected:
	NodeType GetNodeType() const override { return NodeType::Function; }

protected:
	std::unique_ptr<Prototype> prototype;
	std::string description;
	std::unique_ptr<Statement> body;

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};

class Block : public Statement
{
public:
	Block(const std::intptr_t position, std::vector<std::unique_ptr<Statement>> statements)
		: Statement{position}, statements{std::move(statements)} {}

private:
	explicit Block() : Block{{}, {}} {}

public:

	void CompileFunc(StdCompiler *comp) override;
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::generator<std::string> ToTreeInternal() const override;

protected:
	NodeType GetNodeType() const override { return NodeType::Block; }

protected:
	std::vector<std::unique_ptr<Statement>> statements;

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};

class Include : public Statement
{
public:
	Include(const std::intptr_t position, const C4ID id, const bool nowarn)
		: Statement{position}, id{id}, nowarn{nowarn} {}

private:
	explicit Include() : Include{{}, {}, {}} {}

public:

	void CompileFunc(StdCompiler *comp) override;
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::generator<std::string> ToTreeInternal() const override;

protected:
	NodeType GetNodeType() const override { return NodeType::Include; }

protected:
	C4ID id;
	bool nowarn;

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};

class Append : public Statement
{
public:
	Append(const std::intptr_t position, const C4ID id, const bool nowarn)
		: Statement{position}, id{id}, nowarn{nowarn} {}

private:
	explicit Append() : Append{{}, {}, {}} {}

public:

	void CompileFunc(StdCompiler *comp) override;
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::generator<std::string> ToTreeInternal() const override;

protected:
	NodeType GetNodeType() const override { return NodeType::Append; }

protected:
	C4ID id;
	bool nowarn;

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};

class Strict : public Statement
{
public:
	Strict(const std::intptr_t position, const C4AulScriptStrict strict)
		: Statement{position}, strict{strict} {}

private:
	explicit Strict() : Strict{{}, {}} {}

public:

	void CompileFunc(StdCompiler *comp) override;
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::generator<std::string> ToTreeInternal() const override;

protected:
	NodeType GetNodeType() const override { return NodeType::Strict; }

protected:
	C4AulScriptStrict strict;

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};

class Return : public Statement
{
public:
	Return(const std::intptr_t position, std::vector<std::unique_ptr<Expression>> expressions, const bool preferStack)
		: Statement{position}, expressions{std::move(expressions)}, preferStack{preferStack} {}

private:
	explicit Return() : Return{{}, {}, {}} {}

public:
	void CompileFunc(StdCompiler *comp) override;
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::generator<std::string> ToTreeInternal() const override;

protected:
	NodeType GetNodeType() const override { return NodeType::Return; }

protected:
	std::vector<std::unique_ptr<Expression>> expressions;
	bool preferStack;

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};

class ReturnAsParam : public Expression
{
public:
	ReturnAsParam(const std::intptr_t position, std::unique_ptr<Expression> expression, const bool preferStack)
		: Expression{position, expression->GetType()}, expression{std::move(expression)}, preferStack{preferStack} {}

private:
	explicit ReturnAsParam() : ReturnAsParam{{}, {}, {}} {}

public:

	void CompileFunc(StdCompiler *comp) override;
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::generator<std::string> ToTreeInternal() const override;

protected:
	NodeType GetNodeType() const override { return NodeType::ReturnAsParam; }

protected:
	std::unique_ptr<Expression> expression;
	bool preferStack;

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};

class FunctionCall : public Expression
{
public:
	FunctionCall(const std::intptr_t position, std::string identifier, std::vector<std::unique_ptr<Expression>> arguments)
		: Expression{position, C4V_Any}, identifier{std::move(identifier)}, arguments{std::move(arguments)} {}

protected:
	explicit FunctionCall() : Expression{{}, {}} {}

public:
	void CompileFunc(StdCompiler *comp) override;
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::generator<std::string> ToTreeInternal() const override;

protected:
	NodeType GetNodeType() const override { return NodeType::FunctionCall; }

protected:
	std::string identifier;
	std::vector<std::unique_ptr<Expression>> arguments;

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};

class NilTestExpression
{
public:
	virtual ~NilTestExpression() = 0;

public:
	void AddNilTest() { testForNil = true; }
	void Dump(std::ostream &out) const
	{
		if (testForNil)
		{
			out << "testForNil";
		}
	}

	void CompileFunc(StdCompiler *const comp)
	{
		comp->Value(mkNamingAdapt(testForNil, "TestForNil"));
	}

protected:
	bool testForNil{false};

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};

class ArrayAccess : public BinaryExpression, public NilTestExpression, public NoRef
{
public:
	ArrayAccess(const std::intptr_t position, std::unique_ptr<Expression> array, std::unique_ptr<Expression> index)
		: BinaryExpression{position, C4V_Any, std::move(array), std::move(index)} {}

private:
	explicit ArrayAccess() : ArrayAccess{{}, {}, {}} {}

public:
	void CompileFunc(StdCompiler *comp) override;
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::generator<std::string> ToTreeInternal() const override;

protected:
	void OnSetNoRef() override;

	NodeType GetNodeType() const override { return NodeType::ArrayAccess; }

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};

class ArrayAppend : public Expression, public NilTestExpression
{
public:
	ArrayAppend(const std::intptr_t position, std::unique_ptr<Expression> array)
		: Expression{position, C4V_Any}, array{std::move(array)} {}

private:
	explicit ArrayAppend() : ArrayAppend{{}, {}} {}

public:

	void CompileFunc(StdCompiler *comp) override;
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::generator<std::string> ToTreeInternal() const override;

protected:
	NodeType GetNodeType() const override { return NodeType::ArrayAppend; }

protected:
	std::unique_ptr<Expression> array;

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};

class PropertyAccess : public Expression, public NilTestExpression, public NoRef
{
public:
	PropertyAccess(const std::intptr_t position, std::unique_ptr<Expression> object, std::string property)
		: Expression{position, C4V_Any}, object{std::move(object)}, property{std::move(property)} {}

private:
	explicit PropertyAccess() : PropertyAccess{{}, {}, {}} {}

public:

	void CompileFunc(StdCompiler *comp) override;
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::generator<std::string> ToTreeInternal() const override;

protected:
	void OnSetNoRef() override;

	NodeType GetNodeType() const override { return NodeType::PropertyAccess; }

protected:
	std::unique_ptr<Expression> object;
	std::string property;

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};

class IndirectCall : public Expression, public NilTestExpression
{
public:
	IndirectCall(const std::intptr_t position, std::unique_ptr<Expression> callee, const C4ID namespaceId, std::string identifier, std::vector<std::unique_ptr<Expression>> arguments, const bool failSafe, const bool globalCall)
		: Expression{position, C4V_Any}, callee{std::move(callee)}, namespaceId{namespaceId}, identifier{std::move(identifier)}, arguments{std::move(arguments)}, failSafe{failSafe}, globalCall{globalCall} {}

protected:
	explicit IndirectCall() : Expression{{}, {}} {}

public:

	void CompileFunc(StdCompiler *comp) override;
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::generator<std::string> ToTreeInternal() const override;

protected:
	NodeType GetNodeType() const override { return NodeType::IndirectCall; }

protected:
	std::unique_ptr<Expression> callee;
	C4ID namespaceId{};
	std::string identifier;
	std::vector<std::unique_ptr<Expression>> arguments;
	bool failSafe{};
	bool globalCall{};

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};

/*class DefinitionCall : public IndirectCall
{
public:
	DefinitionCall(const C4ID callee, std::unique_ptr<Identifier> identifier, std::vector<std::unique_ptr<Expression>> arguments, bool failSafe)
		: IndirectCall{std::make_unique<C4AulAST::Literal>(C4VID(callee)), std::move(identifier), std::move(arguments), failSafe} {}
};*/

class If : public Statement
{
public:
	If(const std::intptr_t position, std::unique_ptr<Expression> condition, std::unique_ptr<Statement> then, std::unique_ptr<Statement> other)
		: Statement{position}, condition{std::move(condition)}, then{std::move(then)}, other{std::move(other)} {}

private:
	explicit If() : Statement{{}} {}

public:

	void CompileFunc(StdCompiler *comp) override;
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::generator<std::string> ToTreeInternal() const override;

protected:
	NodeType GetNodeType() const override { return NodeType::If; }

protected:
	std::unique_ptr<Expression> condition;
	std::unique_ptr<Statement> then;
	std::unique_ptr<Statement> other;

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};

class ExprIf : public Expression
{
public:
	ExprIf(const std::intptr_t position, std::unique_ptr<Expression> condition, std::unique_ptr<Expression> then, std::unique_ptr<Expression> other)
		: Expression{position, then->GetType()}, condition{std::move(condition)}, then{std::move(then)}, other{std::move(other)} {}

private:
	explicit ExprIf() : Expression{{}, {}} {}

public:

	void CompileFunc(StdCompiler *comp) override;
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::generator<std::string> ToTreeInternal() const override;

protected:
	NodeType GetNodeType() const override { return NodeType::ExprIf; }

protected:
	std::unique_ptr<Expression> condition;
	std::unique_ptr<Expression> then;
	std::unique_ptr<Expression> other;

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};

class While : public Statement
{
public:
	While(const std::intptr_t position, std::unique_ptr<Expression> condition, std::unique_ptr<Statement> body)
		: Statement{position}, condition{std::move(condition)}, body{std::move(body)} {}

private:
	explicit While() : Statement{{}} {}

public:

	void CompileFunc(StdCompiler *comp) override;
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::generator<std::string> ToTreeInternal() const override;

protected:
	NodeType GetNodeType() const override { return NodeType::While; }

protected:
	std::unique_ptr<Expression> condition;
	std::unique_ptr<Statement> body;

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};

class For : public Statement
{
public:
	For(const std::intptr_t position, std::unique_ptr<Statement> init, std::unique_ptr<Expression> condition, std::unique_ptr<Statement> after, std::unique_ptr<Statement> body)
		: Statement{position}, init{std::move(init)}, condition{std::move(condition)}, after{std::move(after)}, body{std::move(body)} {}

private:
	explicit For() : Statement{{}} {}

public:

	void CompileFunc(StdCompiler *comp) override;
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::generator<std::string> ToTreeInternal() const override;

protected:
	NodeType GetNodeType() const override { return NodeType::For; }

protected:
	std::unique_ptr<Statement> init;
	std::unique_ptr<Expression> condition;
	std::unique_ptr<Statement> after;
	std::unique_ptr<Statement> body;

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};

class ForEach : public Statement
{
public:
	ForEach(const std::intptr_t position, std::unique_ptr<Statement> init, std::unique_ptr<Expression> iterable, std::unique_ptr<Statement> body)
		: Statement{position}, init{std::move(init)}, iterable{std::move(iterable)}, body{std::move(body)} {}

private:
	explicit ForEach() : Statement{{}} {}

public:
	void CompileFunc(StdCompiler *comp) override;
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::generator<std::string> ToTreeInternal() const override;

protected:
	NodeType GetNodeType() const override { return NodeType::ForEach; }

protected:
	std::unique_ptr<Statement> init;
	std::unique_ptr<Expression> iterable;
	std::unique_ptr<Statement> body;

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};

class Break : public Statement
{
public:
	Break(const std::intptr_t position)
		: Statement{position} {}

private:
	explicit Break() : Statement{{}} {}

public:
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::generator<std::string> ToTreeInternal() const override;

protected:
	NodeType GetNodeType() const override { return NodeType::Break; }

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};

class Continue : public Statement
{
public:
	Continue(const std::intptr_t position)
		: Statement{position} {}

private:
	explicit Continue() : Statement{{}} {}

public:
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::generator<std::string> ToTreeInternal() const override;

protected:
	NodeType GetNodeType() const override { return NodeType::Continue; }

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};

class Inherited : public FunctionCall
{
public:
	Inherited(const std::intptr_t position, std::vector<std::unique_ptr<Expression>> arguments, bool failSafe, bool found)
		: FunctionCall{position, failSafe ? "_inherited" : "inherited", std::move(arguments)}, failSafe{failSafe}, found{found} {}

private:
	explicit Inherited() : FunctionCall{} {}

public:
	void CompileFunc(StdCompiler *comp) override;
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::generator<std::string> ToTreeInternal() const override;

protected:
	NodeType GetNodeType() const override { return NodeType::Inherited; }

protected:
	bool failSafe;
	bool found;

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};

class This : public Expression
{
public:
	This(const std::intptr_t position) : Expression{position, C4V_C4Object} {}

private:
	explicit This() : Expression{{}, {}} {}

public:
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::generator<std::string> ToTreeInternal() const override;

protected:
	NodeType GetNodeType() const override { return NodeType::This; }

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};

class Nop : public Statement
{
public:
	Nop(const std::intptr_t position) : Statement{position} {}

private:
	explicit Nop() : Statement{{}} {}

public:
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::generator<std::string> ToTreeInternal() const override;

protected:
	NodeType GetNodeType() const override { return NodeType::Nop; }

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
	friend class ::C4AulBCCGenerator;
};

class Error : public Expression
{
public:
	Error(const std::intptr_t position) : Expression{position, C4V_Any} {}

private:
	explicit Error() : Expression{{}, {}} {}

public:
	void EmitInternal(C4AulBCCGenerator &generator) const override;
	std::generator<std::string> ToTreeInternal() const override;

protected:
	NodeType GetNodeType() const override { return NodeType::Error; }

	template<std::derived_from<Statement> T>
	friend void CompileNewFunc(T *&, StdCompiler *);
};

template<std::derived_from<Statement> T>
inline void CompileNewFunc(T *&obj, StdCompiler *const comp)
{
	Statement *statement;
	NodeType nodeType;

	static constexpr StdEnumEntry<NodeType> NodeTypes[]
	{
#define NODE(x) {#x, NodeType::x}
		NODE(Script),
		NODE(Nil),
		NODE(IntLiteral),
		NODE(C4IDLiteral),
		NODE(BoolLiteral),
		NODE(StringLiteral),
		NODE(ArrayLiteral),
		NODE(MapLiteral),
		NODE(GlobalConstant),
		NODE(ParN),
		NODE(Par),
		NODE(VarN),
		NODE(Var),
		NODE(LocalN),
		NODE(GlobalN),
		NODE(Identifier),
		NODE(Declaration),
		NODE(Declarations),
		NODE(UnaryOperator),
		NODE(BinaryOperator),
		NODE(Prototype),
		NODE(Function),
		NODE(Block),
		NODE(Include),
		NODE(Append),
		NODE(Strict),
		NODE(Return),
		NODE(ReturnAsParam),
		NODE(FunctionCall),
		NODE(ArrayAccess),
		NODE(ArrayAppend),
		NODE(PropertyAccess),
		NODE(IndirectCall),
		NODE(If),
		NODE(ExprIf),
		NODE(While),
		NODE(For),
		NODE(ForEach),
		NODE(Break),
		NODE(Continue),
		NODE(Inherited),
		NODE(This),
		NODE(Nop),
		NODE(Error),
#undef NODE
	};

	comp->Value(mkNamingAdapt(mkEnumAdaptT<NodeType>(nodeType, NodeTypes), "NodeType"));

	switch (nodeType)
	{
#define NODE(x) case NodeType::x: statement = new x{}; break

		NODE(Script);
		NODE(Nil);
		NODE(IntLiteral);
		NODE(C4IDLiteral);
		NODE(BoolLiteral);
		NODE(StringLiteral);
		NODE(ArrayLiteral);
		NODE(MapLiteral);
		NODE(GlobalConstant);
		NODE(ParN);
		NODE(Par);
		NODE(VarN);
		NODE(Var);
		NODE(LocalN);
		NODE(GlobalN);
		NODE(Declaration);
		NODE(Declarations);
		NODE(UnaryOperator);
		NODE(BinaryOperator);
		NODE(Prototype);
		NODE(Function);
		NODE(Block);
		NODE(Include);
		NODE(Append);
		NODE(Strict);
		NODE(Return);
		NODE(ReturnAsParam);
		NODE(FunctionCall);
		NODE(ArrayAccess);
		NODE(ArrayAppend);
		NODE(PropertyAccess);
		NODE(IndirectCall);
		NODE(If);
		NODE(ExprIf);
		NODE(While);
		NODE(For);
		NODE(ForEach);
		NODE(Break);
		NODE(Continue);
		NODE(Inherited);
		NODE(This);
		NODE(Nop);
		NODE(Error);

#undef NODE

	default:
		comp->excCorrupt("Invalid node");
	}

	if (auto *const ptr = dynamic_cast<T *>(statement); ptr)
	{
		ptr->CompileFunc(comp);
		obj = ptr;
	}
	else
	{
		comp->excCorrupt("Node not assignable");
	}
}
};

// call context
struct C4AulContext
{
	C4Object *Obj;
	C4Def *Def;
	struct C4AulScriptContext *Caller;

	bool CalledWithStrictNil() const noexcept;
};

// execution context
struct C4AulScriptContext : public C4AulContext
{
	C4Value *Return;
	C4Value *Pars;
	C4Value *Vars;
	C4AulScriptFunc *Func;
	bool TemporaryScript;
	C4ValueList NumVars;
	C4AulBCC *CPos;
	time_t tTime; // initialized only by profiler if active

	size_t ParCnt() const { return Vars - Pars; }
	void dump(std::string Dump = "");
};

// base function class
class C4AulFunc
{
	friend class C4AulScript;
	friend class C4AulScriptEngine;
	friend class C4AulFuncMap;
	friend class C4AulBCCGenerator;
	friend class C4AulParseState;
	friend class C4AulAST::Prototype;

public:
	C4AulFunc(C4AulScript *pOwner, const char *pName, bool bAtEnd = true);
	virtual ~C4AulFunc();

	C4AulScript *Owner; // owner
	char Name[C4AUL_MAX_Identifier]; // function name

protected:
	C4AulFunc *Prev, *Next; // linked list members
	C4AulFunc *MapNext; // map member
	C4AulFunc *LinkedTo; // points to next linked function; destructor will destroy linked func, too

public:
	C4AulFunc *OverloadedBy; // function by which this one is overloaded
	C4AulFunc *NextSNFunc; // next script func using the same name (list build in AfterLink)

	virtual C4AulScriptFunc *SFunc() { return nullptr; } // type check func...

	// Wether this function should be visible to players
	virtual bool GetPublic() { return false; }
	virtual int GetParCount() { return C4AUL_MAX_Par; }
	virtual const C4V_Type *GetParType() { return nullptr; }
	virtual C4V_Type GetRetType() { return C4V_Any; }
	virtual C4Value Exec(C4AulContext *pCallerCtx, const C4Value pPars[], bool fPassErrors = false) { return C4Value(); } // execute func (script call)
	virtual C4Value Exec(C4Object *pObj = nullptr, const C4AulParSet &pPars = C4AulParSet{}, bool fPassErrors = false, bool nonStrict3WarnConversionOnly = false, bool convertNilToIntBool = true); // execute func (engine call)
	virtual void UnLink() { OverloadedBy = NextSNFunc = nullptr; }

	C4AulFunc *GetLocalSFunc(const char *szIdtf); // find script function in own scope

	C4AulFunc *FindSameNameFunc(C4Def *pScope); // Find a function of the same name for given scope

protected:
	void DestroyLinked(); // destroys linked functions
};

// script function class
class C4AulScriptFunc : public C4AulFunc
{
public:
	C4AulFunc *OwnerOverloaded; // overloaded owner function; if present
	C4AulScriptFunc *SFunc() override { return this; } // type check func...

protected:
	void ParseDesc(); // evaluate desc (i.e. get idImage and Condition

public:
	C4AulAccess Access;
	StdStrBuf Desc; // full function description block, including image and condition
	StdStrBuf DescText; // short function description text (name of menu entry)
	StdStrBuf DescLong; // secondary function description
	C4ID idImage; // associated image
	int32_t iImagePhase; // Image phase
	C4AulFunc *Condition; // func condition
	int32_t ControlMethod; // 0 = all, 1 = Classic, 2 = Jump+Run
	const char *Script; // script pos
	C4AulBCC *Code; // code pos
	std::unique_ptr<C4AulAST::Block> Body; // boy
	C4ValueMapNames VarNamed; // list of named vars in this function
	C4ValueMapNames ParNamed; // list of named pars in this function
	C4V_Type ParType[C4AUL_MAX_Par]; // parameter types
	bool bNewFormat; // new func format? [ func xyz(par abc) { ... } ]
	bool bReturnRef; // return reference
	C4AulScript *pOrgScript; // the original script (!= Owner if included or appended)

	C4AulScriptFunc(C4AulScript *pOwner, const char *pName, bool bAtEnd = true) : C4AulFunc(pOwner, pName, bAtEnd),
		idImage(C4ID_None), iImagePhase(0), Condition(nullptr), ControlMethod(C4AUL_ControlMethod_All), OwnerOverloaded(nullptr),
		bReturnRef(false), tProfileTime(0)
	{
		for (int i = 0; i < C4AUL_MAX_Par; i++) ParType[i] = C4V_Any;
	}

	virtual void UnLink() override;

	virtual bool GetPublic() override { return true; }
	virtual const C4V_Type *GetParType() override { return ParType; }
	virtual C4V_Type GetRetType() override { return bReturnRef ? C4V_pC4Value : C4V_Any; }
	virtual C4Value Exec(C4AulContext *pCallerCtx, const C4Value pPars[], bool fPassErrors = false) override; // execute func (script call, should not happen)
	virtual C4Value Exec(C4Object *pObj = nullptr, const C4AulParSet &pPars = C4AulParSet{}, bool fPassErrors = false, bool nonStrict3WarnConversionOnly = false, bool convertNilToIntBool = true) override; // execute func (engine call)

	void CopyBody(C4AulScriptFunc &FromFunc); // copy script/code, etc from given func

	std::string GetFullName(); // get a fully classified name (C4ID::Name) for debug output

	time_t tProfileTime; // internally set by profiler

	bool HasStrictNil() const noexcept;

	friend class C4AulScript;
};

class C4AulFuncMap
{
public:
	C4AulFuncMap();
	~C4AulFuncMap();
	C4AulFunc *GetFunc(const char *Name, const C4AulScript *Owner, const C4AulFunc *After);
	C4AulFunc *GetFirstFunc(const char *Name);
	C4AulFunc *GetNextSNFunc(const C4AulFunc *After);

private:
	C4AulFunc **Funcs;
	int FuncCnt;
	int Capacity;
	static unsigned int Hash(const char *Name);

protected:
	void Add(C4AulFunc *func, bool bAtEnd = true);
	void Remove(C4AulFunc *func);

	friend class C4AulFunc;
};

// aul script state
enum C4AulScriptState
{
	ASS_ERROR,     // erroneous script
	ASS_NONE,      // nothing
	ASS_PREPARSED, // function list built; CodeSize set
	ASS_LINKED,    // includes and appends resolved
	ASS_PARSED     // byte code generated
};

// script profiler entry
class C4AulProfiler
{
private:
	// map entry
	struct Entry
	{
		C4AulScriptFunc *pFunc;
		time_t tProfileTime;

		bool operator<(const Entry &e2) const { return tProfileTime < e2.tProfileTime; }
	};

	// items
	std::vector<Entry> Times;
	std::shared_ptr<spdlog::logger> logger;

public:
	C4AulProfiler(std::shared_ptr<spdlog::logger> logger) : logger{std::move(logger)} {}

	void CollectEntry(C4AulScriptFunc *pFunc, time_t tProfileTime);
	void Show();

	static void Abort();
	static void StartProfiling(C4AulScript *pScript);
	static void StopProfiling();
};

C4LOGGERCONFIG_NAME_TYPE(C4AulProfiler);

template<>
struct C4LoggerConfig::Defaults<C4AulProfiler>
{
	static constexpr spdlog::level::level_enum GuiLogLevel{spdlog::level::info};
	static constexpr bool ShowLoggerNameInGui{false};
};

// script class
class C4AulScript
{
public:
	C4AulScript();
	virtual ~C4AulScript();
	void Default(); // init
	void Clear(); // remove script, byte code and children
	void Reg2List(C4AulScriptEngine *pEngine, C4AulScript *pOwner); // reg to linked list
	void Unreg(); // remove from list
	virtual bool Delete() { return true; } // allow deletion on pure class

protected:
	struct Append
	{
		const C4ID id;
		const bool nowarn;

		bool operator==(C4ID other) const { return id == other; }
	};

	C4AulFunc *Func0, *FuncL; // owned functions
	C4AulScriptEngine *Engine; // owning engine
	C4AulScript *Owner, *Prev, *Next, *Child0, *ChildL; // tree structure

	StdStrBuf Script; // script
	C4AulBCC *Code, *CPos; // compiled script (/pos)
	C4AulScriptState State; // script state
	int CodeSize; // current number of byte code chunks in Code
	int CodeBufSize; // size of Code buffer
	bool Preparsing; // set while preparse
	bool Resolving; // set while include-resolving, to catch circular includes

	std::list<C4ID> Includes; // include list
	std::list<Append> Appends; // append list

	std::unique_ptr<C4AulAST::Script> AST;

	// internal function used to find overloaded functions
	C4AulFunc *GetOverloadedFunc(C4AulFunc *ByFunc);
	C4AulFunc *GetFunc(const char *pIdtf); // get local function by name

	void AddBCC(C4AulBCCType eType, std::intptr_t = 0, const char *SPos = nullptr); // add byte code chunk and advance
	bool Preparse(); // preparse script; return if successful
	bool BuildAST(); // preparse script; return if successfull
	void ParseFn(C4AulScriptFunc *Fn, bool fExprOnly = false); // parse single script function

	bool Parse(); // parse preparsed script; return if successful
	void ParseDescs(); // parse function descs

	bool ResolveIncludes(C4DefList *rDefs); // resolve includes
	bool ResolveAppends(C4DefList *rDefs); // resolve appends
	bool IncludesResolved;
	void AppendTo(C4AulScript &Scr, bool bHighPrio); // append to given script
	void UnLink(); // reset to unlinked state
	virtual void AfterLink(); // called after linking is completed; presearch common funcs here & search same-named funcs
	virtual bool ReloadScript(const char *szPath); // reload given script

	C4AulScript *FindFirstNonStrictScript(); // find first script that is not #strict

	size_t GetCodePos() const { return CPos - Code; }
	C4AulBCC *GetCodeByPos(size_t iPos) { return Code + iPos; }

public:
	std::string ScriptName; // script name
	C4Def *Def; // owning def file
	C4ValueMapNames LocalNamed;
	C4ID idDef; // script id (to resolve includes)
	C4AulScriptStrict Strict; // new or even newer syntax?
	bool Temporary; // set for DirectExec-scripts; do not parse those

	C4AulScriptEngine *GetEngine() { return Engine; }
	C4AulScript *GetOwner() const { return Owner; }
	const char *GetScript() const { return Script.getData(); }

	C4AulFunc *GetFuncRecursive(const char *pIdtf); // search function by identifier, including global funcs
	C4AulScriptFunc *GetSFunc(const char *pIdtf, C4AulAccess AccNeeded, bool fFailSafe = false); // get local sfunc, check access, check '~'-safety
	C4AulScriptFunc *GetSFunc(const char *pIdtf); // get local script function by name
	C4AulScriptFunc *GetSFunc(int iIndex, const char *szPattern = nullptr, C4AulAccess AccNeeded = AA_PRIVATE); // get local script function by index
	C4AulScriptFunc *GetSFuncWarn(const char *pIdtf, C4AulAccess AccNeeded, const char *WarnStr); // get function; return nullptr and warn if not existent
	C4AulAccess GetAllowedAccess(C4AulFunc *func, C4AulScript *caller);

public:
	C4Value DirectExec(C4Object *pObj, const char *szScript, const char *szContext, bool fPassErrors = false, C4AulScriptStrict Strict = C4AulScriptStrict::MAXSTRICT); // directly parse uncompiled script (WARG! CYCLES!)
	void ResetProfilerTimes(); // zero all profiler times of owned functions
	void CollectProfilerTimes(class C4AulProfiler &rProfiler);

	bool IsReady() { return State == ASS_PARSED; } // whether script calls may be done

	// helper functions
	void Warn(std::string_view msg, const char *pIdtf);

	friend class C4AulParseError;
	friend class C4AulFunc;
	friend class C4AulScriptFunc;
	friend class C4AulScriptEngine;
	friend class C4AulBCCGenerator;
	friend class C4AulParseState;

	friend void C4Aul::AddBCC(std::intptr_t, C4AulBCCType, std::intptr_t, std::intptr_t &, C4AulScript &, bool &);
};

C4LOGGERCONFIG_NAME_TYPE(C4AulScript);

// holds all C4AulScripts
class C4AulScriptEngine : public C4AulScript
{
protected:
	C4AulFuncMap FuncLookUp;

public:
	int warnCnt, errCnt; // number of warnings/errors
	int nonStrictCnt; // number of non-strict scripts
	int lineCnt; // line count parsed

	C4ValueList Global;
	C4ValueMapNames GlobalNamedNames;
	C4ValueMapData GlobalNamed;

	C4StringTable Strings;

	// global constants (such as "static const C4D_Structure = 2;")
	// cannot share var lists, because it's so closely tied to the data lists
	// constants are used by the Parser only, anyway, so it's not
	// necessary to pollute the global var list here
	C4ValueMapNames GlobalConstNames;
	C4ValueMapData GlobalConsts;

	C4AulScriptEngine();
	~C4AulScriptEngine();
	void Clear(); // clear data
	void Link(C4DefList *rDefs); // link and parse all scripts
	void ReLink(C4DefList *rDefs); // unlink + relink and parse all scripts
	bool ReloadScript(const char *szScript, C4DefList *pDefs); // search script and reload + relink, if found

	C4AulFunc *GetFirstFunc(const char *Name)
	{
		return FuncLookUp.GetFirstFunc(Name);
	}

	C4AulFunc *GetFunc(const char *Name, const C4AulScript *Owner, const C4AulFunc *After)
	{
		return FuncLookUp.GetFunc(Name, Owner, After);
	}

	C4AulFunc *GetNextSNFunc(const C4AulFunc *After)
	{
		return FuncLookUp.GetNextSNFunc(After);
	}

	// For the list of functions in the PropertyDlg
	C4AulFunc *GetFirstFunc() { return Func0; }
	C4AulFunc *GetNextFunc(C4AulFunc *pFunc) { return pFunc->Next; }

	void RegisterGlobalConstant(const char *szName, const C4Value &rValue); // creates a new constants or overwrites an old one
	bool GetGlobalConstant(const char *szName, C4Value *pTargetValue); // check if a constant exists; assign value to pTargetValue if not nullptr

	bool DenumerateVariablePointers();
	void UnLink(); // called when a script is being reloaded (clears string table)
	// Compile scenario script data (without strings and constants)
	void CompileFunc(StdCompiler *pComp);

	friend class C4AulFunc;
	friend class C4AulBCCGenerator;
	friend class C4AulParseState;
};

class C4AulExec;

C4LOGGERCONFIG_NAME_TYPE(C4AulExec);

template<>
struct C4LoggerConfig::Defaults<C4AulExec>
{
	static constexpr auto GuiLogLevel = spdlog::level::info;
	static constexpr bool ShowLoggerNameInGui{false};
};

class C4AulBCCGenerator
{
private:
	struct Loop
	{
		struct Control
		{
			bool Break;
			std::size_t Pos;
		};

		std::vector<Control> Controls;
		std::intptr_t StackSize;
	};

public:
	C4AulBCCGenerator(C4AulScript &script, C4AulScriptFunc *const func)
		: Script{script}, Function{func} {}

public:
	const std::vector<C4AulBCC> &Generate(const std::unique_ptr<C4AulAST::Block> &block);

	void AddBCC(intptr_t offset, C4AulBCCType type, std::intptr_t bccX = 0);
	std::size_t JumpHere(); // Get position for a later jump to next instruction added
	void SetJumpHere(std::size_t jumpOp); // Use the next inserted instruction as jump target for the given jump operation
	void SetJump(std::size_t jumpOp, std::size_t where);
	void AddJump(intptr_t offset, C4AulBCCType type, std::size_t where);

	void PushLoop();
	void PopLoop();
	void AddLoopControl(std::size_t position, bool fBreak);
	[[noreturn]] void Error(const char *message, const char * identifier = nullptr);

	std::size_t GetCodePos() const noexcept { return code.size(); }
	C4AulBCC *GetCodeByPos(const std::size_t pos) noexcept { return code.data() + pos; }

	std::vector<Loop::Control> GetLoopControls() const
	{
		return loopStack.top().Controls;
	}

	std::size_t GetStackSize() const noexcept { return stack; }

	void AddError(std::size_t position);
	void RemoveBCC() { code.pop_back(); }

public:
	C4AulScript &Script;
	C4AulScriptFunc *Function{nullptr};

private:
	bool done{false};
	bool jump{false};
	bool error{false};
	std::vector<C4AulBCC> code;
	std::intptr_t stack{0};
	std::stack<Loop> loopStack;
};
