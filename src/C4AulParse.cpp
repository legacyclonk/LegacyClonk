/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2001, Sven2
 * Copyright (c) 2017-2020, The LegacyClonk Team and contributors
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

// parses scripts

#include <C4Include.h>
#include <C4Aul.h>

#include <C4Def.h>
#include <C4Game.h>
#include <C4Wrappers.h>

#include <cinttypes>

#define DEBUG_BYTECODE_DUMP 0

#define C4AUL_Include "#include"
#define C4AUL_Strict  "#strict"
#define C4AUL_Append  "#appendto"
#define C4AUL_NoWarn "nowarn"

#define C4AUL_Func "func"

#define C4AUL_Private   "private"
#define C4AUL_Protected "protected"
#define C4AUL_Public    "public"
#define C4AUL_Global    "global"
#define C4AUL_Const     "const"

#define C4AUL_If            "if"
#define C4AUL_Else          "else"
#define C4AUL_While         "while"
#define C4AUL_For           "for"
#define C4AUL_In            "in"
#define C4AUL_Return        "return"
#define C4AUL_Var           "Var"
#define C4AUL_Par           "Par"
#define C4AUL_Goto          "goto"
#define C4AUL_Break         "break"
#define C4AUL_Continue      "continue"
#define C4AUL_Inherited     "inherited"
#define C4AUL_SafeInherited "_inherited"
#define C4AUL_this          "this"

#define C4AUL_Image     "Image"
#define C4AUL_Contents  "Contents"
#define C4AUL_Condition "Condition"
#define C4AUL_Method    "Method"
#define C4AUL_Desc      "Desc"

#define C4AUL_MethodAll        "All"
#define C4AUL_MethodNone       "None"
#define C4AUL_MethodClassic    "Classic"
#define C4AUL_MethodJumpAndRun "JumpAndRun"

#define C4AUL_GlobalNamed "static"
#define C4AUL_LocalNamed  "local"
#define C4AUL_VarNamed    "var"

#define C4AUL_TypeInt      "int"
#define C4AUL_TypeBool     "bool"
#define C4AUL_TypeC4ID     "id"
#define C4AUL_TypeC4Object "object"
#define C4AUL_TypeString   "string"
#define C4AUL_TypeArray    "array"
#define C4AUL_TypeMap      "map"
#define C4AUL_TypeAny      "any"

#define C4AUL_Nil  "nil"

#define C4AUL_True  "true"
#define C4AUL_False "false"

#define C4AUL_CodeBufSize 16

// script token type
enum C4AulTokenType
{
	ATT_INVALID,  // invalid token
	ATT_DIR,      // directive
	ATT_IDTF,     // identifier
	ATT_INT,      // integer constant
	ATT_BOOL,     // boolean constant
	ATT_NIL,      // nil
	ATT_STRING,   // string constant
	ATT_C4ID,     // C4ID constant
	ATT_COMMA,    // ","
	ATT_COLON,    // ":"
	ATT_DCOLON,   // "::"
	ATT_SCOLON,   // ";"
	ATT_BOPEN,    // "("
	ATT_BCLOSE,   // ")"
	ATT_BOPEN2,   // "["
	ATT_BCLOSE2,  // "]"
	ATT_BLOPEN,   // "{"
	ATT_BLCLOSE,  // "}"
	ATT_SEP,      // "|"
	ATT_CALL,     // "->"
	ATT_GLOBALCALL, // "global->"
	ATT_STAR,     // "*"
	ATT_AMP,      // "&"
	ATT_TILDE,    // '~'
	ATT_LDOTS,    // '...'
	ATT_DOT,      // '.'
	ATT_QMARK,    // '?'
	ATT_OPERATOR, // operator
	ATT_EOF       // end of file
};

class C4AulParseState
{
public:
	typedef enum { PARSER, PREPARSER } TypeType;
	C4AulParseState(C4AulScriptFunc *Fn, C4AulScript *a, TypeType Type) :
		Fn(Fn), a(a), SPos(Fn ? Fn->Script : a->Script.getData()),
		Done(false),
		Type(Type),
		fJump(false),
		iStack(0),
		pLoopStack(nullptr) {}

	~C4AulParseState()
	{
		while (pLoopStack) PopLoop();
	}

	C4AulScriptFunc *Fn; C4AulScript *a;
	const char *SPos; // current position in the script
	char Idtf[C4AUL_MAX_Identifier]; // current identifier
	C4AulTokenType TokenType; // current token type
	std::intptr_t cInt; // current int constant (std::intptr_t for compatibility with x86_64)
	bool Done; // done parsing?
	TypeType Type; // emitting bytecode?
	void Parse_Script();
	void Parse_FuncHead();
	void Parse_Desc();
	void Parse_Function();
	void Parse_Statement();
	void Parse_Block();
	int Parse_Params(int iMaxCnt, const char *sWarn, C4AulFunc *pFunc = nullptr);
	void Parse_Array();
	void Parse_Map();
	void Parse_While();
	void Parse_If();
	void Parse_For();
	void Parse_ForEach();
	void Parse_Expression(int iParentPrio = -1); // includes Parse_Expression2
	void Parse_Expression2(int iParentPrio = -1); // parses operators + Parse_Expression3
	bool Parse_Expression3(); // navigation: ->, [], . and ?
	void Parse_Var();
	void Parse_Local();
	void Parse_Static();
	void Parse_Const();

	bool IsMapLiteral();

	template<C4AulTokenType closingAtt>
	void SkipBlock();

	bool AdvanceSpaces(); // skip whitespaces; return whether script ended
	int GetOperator(const char *pScript);
	enum HoldStringsPolicy { Discard, Hold, Ref };
	C4AulTokenType GetNextToken(char *pToken, std::intptr_t *pInt, HoldStringsPolicy HoldStrings, bool bOperator); // get next token of SPos

	void Shift(HoldStringsPolicy HoldStrings = Hold, bool bOperator = true);
	void Match(C4AulTokenType TokenType, const char *Message = nullptr);
	void UnexpectedToken(const char *Expected);
	const char *GetTokenName(C4AulTokenType TokenType);

	void Warn(const char *pMsg, const char *pIdtf = nullptr);
	void Strict2Error(const char *pMsg, const char *pIdtf = nullptr);

	void SetNoRef(); // Switches the bytecode to generate a value instead of a reference

private:
	bool fJump;
	std::intptr_t iStack;

	void AddBCC(C4AulBCCType eType, std::intptr_t X = 0);

	size_t JumpHere(); // Get position for a later jump to next instruction added
	void SetJumpHere(size_t iJumpOp); // Use the next inserted instruction as jump target for the given jump operation
	void SetJump(size_t iJumpOp, size_t iWhere);
	void AddJump(C4AulBCCType eType, size_t iWhere);

	// Keep track of loops and break/continue usages
	struct Loop
	{
		struct Control
		{
			bool Break;
			size_t Pos;
			Control *Next;
		};
		Control *Controls;
		size_t StackSize;
		Loop *Next;
	};
	Loop *pLoopStack;

	void PushLoop();
	void PopLoop();
	void AddLoopControl(bool fBreak);
};

void C4AulScript::Warn(const char *pMsg, const char *pIdtf)
{
	// display error

	C4AulParseError warning(this, pMsg, pIdtf, true);
	// display it
	warning.show();
	// count warnings
	++Game.ScriptEngine.warnCnt;
}

void C4AulParseState::Warn(const char *pMsg, const char *pIdtf)
{
	// do not show errors for System.c4g scripts that appear to be pure #appendto scripts
	if (Fn && !Fn->Owner->Def && !Fn->Owner->Appends.empty()) return;

	// display error

	C4AulParseError warning(this, pMsg, pIdtf, true);
	// display it
	warning.show();
	if (Fn && Fn->pOrgScript != a)
		DebugLogF("  (as #appendto/#include to %s)", Fn->Owner->ScriptName.getData());
	// count warnings
	++Game.ScriptEngine.warnCnt;
}

void C4AulParseState::Strict2Error(const char *pMsg, const char *pIdtf)
{
	if (Fn ? (Fn->pOrgScript->Strict < C4AulScriptStrict::STRICT2) : (a->Strict < C4AulScriptStrict::STRICT2))
		Warn(pMsg, pIdtf);
	else
		throw C4AulParseError(this, pMsg, pIdtf);
}

C4AulParseError::C4AulParseError(C4AulParseState *state, const char *pMsg, const char *pIdtf, bool Warn)
	: C4AulError()
{
	// compose error string
	sMessage.Format("%s: %s%s",
		Warn ? "WARNING" : "ERROR",
		pMsg,
		pIdtf ? pIdtf : "");
	if (state->Fn && *(state->Fn->Name))
	{
		// Show function name
		sMessage.AppendFormat(" (in %s", state->Fn->Name);

		// Exact position
		if (state->Fn->pOrgScript && state->SPos)
			sMessage.AppendFormat(", %s:%d:%d)",
				state->Fn->pOrgScript->ScriptName.getData(),
				SGetLine(state->Fn->pOrgScript->Script.getData(), state->SPos),
				SLineGetCharacters(state->Fn->pOrgScript->Script.getData(), state->SPos));
		else
			sMessage.AppendChar(')');
	}
	else if (state->a)
	{
		// Script name
		sMessage.AppendFormat(" (%s:%d:%d)",
			state->a->ScriptName.getData(),
			SGetLine(state->a->Script.getData(), state->SPos),
			SLineGetCharacters(state->a->Script.getData(), state->SPos));
	}
}

C4AulParseError::C4AulParseError(C4AulScript *pScript, const char *pMsg, const char *pIdtf, bool Warn)
{
	// compose error string
	sMessage.Format("%s: %s%s",
		Warn ? "WARNING" : "ERROR",
		pMsg,
		pIdtf ? pIdtf : "");
	if (pScript)
	{
		// Script name
		sMessage.AppendFormat(" (%s)",
			pScript->ScriptName.getData());
	}
}

void C4AulScriptFunc::ParseDesc()
{
	// do nothing if no desc is given
	if (!Desc.getLength()) return;
	const char *DPos = Desc.getData();
	// parse desc
	while (*DPos)
	{
		const char *DPos0 = DPos; // beginning of segment
		const char *DPos2 = nullptr; // pos of equal sign, if found
		// parse until end of segment
		while (*DPos && (*DPos != '|'))
		{
			// store break pos if found
			if (*DPos == '=') if (!DPos2) DPos2 = DPos;
			DPos++;
		}

		// if this was an assignment segment, get value to assign
		if (DPos2)
		{
			char Val[C4AUL_MAX_String + 1];
			SCopyUntil(DPos2 + 1, Val, *DPos, C4AUL_MAX_String);
			// Image
			if (SEqual2(DPos0, C4AUL_Image))
			{
				// image: special contents-image?
				if (SEqual(Val, C4AUL_Contents))
					idImage = C4ID_Contents;
				else
				{
					// Find phase separator (:)
					char *colon;
					for (colon = Val; *colon != ':' && *colon != '\0'; ++colon);
					if (*colon == ':') *colon = '\0';
					else colon = nullptr;
					// get image id
					idImage = C4Id(Val);
					// get image phase
					if (colon)
						iImagePhase = atoi(colon + 1);
				}
			}
			// Condition
			else if (SEqual2(DPos0, C4AUL_Condition))
				// condition? get condition func
				Condition = Owner->GetFuncRecursive(Val);
			// Method
			else if (SEqual2(DPos0, C4AUL_Method))
			{
				if (SEqual2(Val, C4AUL_MethodAll))
					ControlMethod = C4AUL_ControlMethod_All;
				else if (SEqual2(Val, C4AUL_MethodNone))
					ControlMethod = C4AUL_ControlMethod_None;
				else if (SEqual2(Val, C4AUL_MethodClassic))
					ControlMethod = C4AUL_ControlMethod_Classic;
				else if (SEqual2(Val, C4AUL_MethodJumpAndRun))
					ControlMethod = C4AUL_ControlMethod_JumpAndRun;
				else
					// unrecognized: Default to all
					ControlMethod = C4AUL_ControlMethod_All;
			}
			// Long Description
			else if (SEqual2(DPos0, C4AUL_Desc))
			{
				DescLong.CopyUntil(Val, '|');
			}
			// unrecognized? never mind
		}

		if (*DPos) DPos++;
	}
	assert(!Condition || !Condition->Owner->Def || Condition->Owner->Def == Owner->Def);
	// copy desc text
	DescText.CopyUntil(Desc.getData(), '|');
}

bool C4AulParseState::AdvanceSpaces()
{
	char C, C2{0};
	// defaultly, not in comment
	int InComment = 0; // 0/1/2 = no comment/line comment/multi line comment
	// don't go past end
	while (C = *SPos)
	{
		// loop until out of comment and non-whitespace is found
		switch (InComment)
		{
		case 0:
			if (C == '/')
			{
				SPos++;
				switch (*SPos)
				{
				case '/': InComment = 1; break;
				case '*': InComment = 2; break;
				default: SPos--; return true;
				}
			}
			else if (static_cast<uint8_t>(C) > 32) return true;
			break;
		case 1:
			if ((static_cast<uint8_t>(C) == 13) || (static_cast<uint8_t>(C) == 10)) InComment = 0;
			break;
		case 2:
			if ((C == '/') && (C2 == '*')) InComment = 0;
			break;
		}
		// next char; store prev
		SPos++; C2 = C;
	}
	// end of script reached; return false
	return false;
}

// C4Script Operator Map

C4ScriptOpDef C4ScriptOpMap[] =
{
	// priority                       postfix  RetType
	// |  identifier                  |  right-associative
	// |  |      Bytecode             |  |  no second id ParType1      ParType2
	// prefix
	{ 16, "++",  AB_Inc1,             0, 1, 0, C4V_Int,    C4V_pC4Value, C4V_Any },
	{ 16, "--",  AB_Dec1,             0, 1, 0, C4V_Int,    C4V_pC4Value, C4V_Any },
	{ 16, "~",   AB_BitNot,           0, 1, 0, C4V_Int,    C4V_Int,      C4V_Any },
	{ 16, "!",   AB_Not,              0, 1, 0, C4V_Bool,   C4V_Bool,     C4V_Any },
	{ 16, "+",   AB_ERR,              0, 1, 0, C4V_Int,    C4V_Int,      C4V_Any },
	{ 16, "-",   AB_Neg,              0, 1, 0, C4V_Int,    C4V_Int,      C4V_Any },

	// postfix (whithout second statement)
	{ 17, "++",  AB_Inc1_Postfix,     1, 1, 1, C4V_Int,    C4V_pC4Value, C4V_Any },
	{ 17, "--",  AB_Dec1_Postfix,     1, 1, 1, C4V_Int,    C4V_pC4Value, C4V_Any },

	// postfix
	{ 15, "**",  AB_Pow,              1, 0, 0, C4V_Int,    C4V_Int,      C4V_Int },
	{ 14, "/",   AB_Div,              1, 0, 0, C4V_Int,    C4V_Int,      C4V_Int },
	{ 14, "*",   AB_Mul,              1, 0, 0, C4V_Int,    C4V_Int,      C4V_Int },
	{ 14, "%",   AB_Mod,              1, 0, 0, C4V_Int,    C4V_Int,      C4V_Int },
	{ 13, "-",   AB_Sub,              1, 0, 0, C4V_Int,    C4V_Int,      C4V_Int },
	{ 13, "+",   AB_Sum,              1, 0, 0, C4V_Int,    C4V_Int,      C4V_Int },
	{ 12, "<<",  AB_LeftShift,        1, 0, 0, C4V_Int,    C4V_Int,      C4V_Int },
	{ 12, ">>",  AB_RightShift,       1, 0, 0, C4V_Int,    C4V_Int,      C4V_Int },
	{ 11, "<",   AB_LessThan,         1, 0, 0, C4V_Bool,   C4V_Int,      C4V_Int },
	{ 11, "<=",  AB_LessThanEqual,    1, 0, 0, C4V_Bool,   C4V_Int,      C4V_Int },
	{ 11, ">",   AB_GreaterThan,      1, 0, 0, C4V_Bool,   C4V_Int,      C4V_Int },
	{ 11, ">=",  AB_GreaterThanEqual, 1, 0, 0, C4V_Bool,   C4V_Int,      C4V_Int },
	{ 10, "..",  AB_Concat,           1, 0, 0, C4V_String, C4V_Any,      C4V_Any},
	{ 9, "==",   AB_Equal,            1, 0, 0, C4V_Bool,   C4V_Any,      C4V_Any },
	{ 9, "!=",   AB_NotEqual,         1, 0, 0, C4V_Bool,   C4V_Any,      C4V_Any },
	{ 9, "S=",   AB_SEqual,           1, 0, 0, C4V_Bool,   C4V_String,   C4V_String },
	{ 9, "eq",   AB_SEqual,           1, 0, 0, C4V_Bool,   C4V_String,   C4V_String },
	{ 9, "ne",   AB_SNEqual,          1, 0, 0, C4V_Bool,   C4V_String,   C4V_String },
	{ 8, "&",    AB_BitAnd,           1, 0, 0, C4V_Int,    C4V_Int,      C4V_Int },
	{ 6, "^",    AB_BitXOr,           1, 0, 0, C4V_Int,    C4V_Int,      C4V_Int },
	{ 6, "|",    AB_BitOr,            1, 0, 0, C4V_Int,    C4V_Int,      C4V_Int },
	{ 5, "&&",   AB_And,              1, 0, 0, C4V_Bool,   C4V_Bool,     C4V_Bool },
	{ 4, "||",   AB_Or,               1, 0, 0, C4V_Bool,   C4V_Bool,     C4V_Bool },
	{ 3, "??",   AB_NilCoalescing,    1, 0, 0, C4V_Any,    C4V_Any,      C4V_Any },
	{ 2, "**=",  AB_PowIt,            1, 1, 0, C4V_Any,    C4V_pC4Value, C4V_Int },
	{ 2, "*=",   AB_MulIt,            1, 1, 0, C4V_Any,    C4V_pC4Value, C4V_Int },
	{ 2, "/=",   AB_DivIt,            1, 1, 0, C4V_Any,    C4V_pC4Value, C4V_Int },
	{ 2, "%=",   AB_ModIt,            1, 1, 0, C4V_Any,    C4V_pC4Value, C4V_Int },
	{ 2, "+=",   AB_Inc,              1, 1, 0, C4V_Any,    C4V_pC4Value, C4V_Int },
	{ 2, "-=",   AB_Dec,              1, 1, 0, C4V_Any,    C4V_pC4Value, C4V_Int },
	{ 2, "<<=",  AB_LeftShiftIt,      1, 1, 0, C4V_Any,    C4V_pC4Value, C4V_Int },
	{ 2, ">>=",  AB_RightShiftIt,     1, 1, 0, C4V_Any,    C4V_pC4Value, C4V_Int },
	{ 2, "..=",  AB_ConcatIt,         1, 1, 0, C4V_Any,    C4V_pC4Value, C4V_Any },
	{ 2, "&=",   AB_AndIt,            1, 1, 0, C4V_Any,    C4V_pC4Value, C4V_Int },
	{ 2, "|=",   AB_OrIt,             1, 1, 0, C4V_Any,    C4V_pC4Value, C4V_Int },
	{ 2, "^=",   AB_XOrIt,            1, 1, 0, C4V_Any,    C4V_pC4Value, C4V_Int },
	{ 2, "??=",  AB_NilCoalescingIt,  1, 1, 0, C4V_Any,    C4V_pC4Value, C4V_Any },
	{ 2, "=",    AB_Set,              1, 1, 0, C4V_Any,    C4V_pC4Value, C4V_Any },

	{ 0, nullptr,  AB_ERR,            0, 0, 0, C4V_Any,    C4V_Any,      C4V_Any }
};

int C4AulParseState::GetOperator(const char *pScript)
{
	// return value:
	// >= 0: operator found. could be found in C4ScriptOfDef
	// -1:   isn't an operator

	unsigned int i;

	if (!*pScript) return 0;
	// text with > 2 characters or text and #strict 2?
	// then break (may be misinterpreted as operator
	if ((*pScript >= 'a' && *pScript <= 'z') ||
		(*pScript >= 'A' && *pScript <= 'Z'))
	{
		if (Fn ? (Fn->pOrgScript->Strict >= C4AulScriptStrict::STRICT2) : (a->Strict >= C4AulScriptStrict::STRICT2))
			return -1;
		if ((*(pScript + 1) >= 'a' && *(pScript + 1) <= 'z') ||
			(*(pScript + 1) >= 'A' && *(pScript + 1) <= 'Z'))
			if ((*(pScript + 2) >= 'a' && *(pScript + 2) <= 'z') ||
				(*(pScript + 2) >= 'A' && *(pScript + 2) <= 'Z') ||
				(*(pScript + 2) >= '0' && *(pScript + 2) <= '9') ||
				*(pScript + 2) == '_')
				return -1;
	}

	// check from longest to shortest
	for (size_t len = 3; len > 0; --len)
		for (i = 0; C4ScriptOpMap[i].Identifier; i++)
			if (SLen(C4ScriptOpMap[i].Identifier) == len)
				if (SEqual2(pScript, C4ScriptOpMap[i].Identifier))
					return i;

	return -1;
}

C4AulTokenType C4AulParseState::GetNextToken(char *pToken, std::intptr_t *pInt, HoldStringsPolicy HoldStrings, bool bOperator)
{
	// move to start of token
	if (!AdvanceSpaces()) return ATT_EOF;
	// store offset
	const char *SPos0 = SPos;
	int Len = 0;
	// token get state
	enum TokenGetState
	{
		TGS_None,   // just started
		TGS_Ident,  // getting identifier
		TGS_Int,    // getting integer
		TGS_IntHex, // getting hexadecimal integer
		TGS_C4ID,   // getting C4ID
		TGS_String, // getting string
		TGS_Dir     // getting directive
	};
	TokenGetState State = TGS_None;

	static char StrBuff[C4AUL_MAX_String + 1];
	char *pStrPos = StrBuff;

	const auto strictLevel = Fn ? Fn->pOrgScript->Strict : a->Strict;

	// loop until finished
	while (true)
	{
		// get char
		char C = *SPos;

		switch (State)
		{
		case TGS_None:
		{
			// get token type by first char (+/- are operators)
			if (((C >= '0') && (C <= '9')))                    // integer by 0-9
				State = TGS_Int;
			else if (C == '"')  State = TGS_String;            // string by "
			else if (C == '#')  State = TGS_Dir;               // directive by "#"
			else if (C == ',') { SPos++; return ATT_COMMA; }   // ","
			else if (C == ';') { SPos++; return ATT_SCOLON; }  // ";"
			else if (C == '(') { SPos++; return ATT_BOPEN; }   // "("
			else if (C == ')') { SPos++; return ATT_BCLOSE; }  // ")"
			else if (C == '[') { SPos++; return ATT_BOPEN2; }  // "["
			else if (C == ']') { SPos++; return ATT_BCLOSE2; } // "]"
			else if (C == '{') { SPos++; return ATT_BLOPEN; }  // "{"
			else if (C == '}') { SPos++; return ATT_BLCLOSE; } // "}"
			else if (C == ':')                                 // ":"
			{
				SPos++;
				// double-colon?
				if (*SPos == ':') // "::"
				{
					SPos++;
					return ATT_DCOLON;
				}
				else              // ":"
					return ATT_COLON;
			}
			else if (C == '-' && *(SPos + 1) == '>')           // "->"
			{
				SPos += 2; return ATT_CALL;
			}
			else if (C == '.' && *(SPos + 1) == '.' && *(SPos + 2) == '.') // "..."
			{
				SPos += 3; return ATT_LDOTS;
			}
			else
			{
				if (bOperator)
				{
					// may it be an operator?
					int iOpID;
					if ((iOpID = GetOperator(SPos)) != -1)
					{
						// store operator ID in pInt
						*pInt = iOpID;
						SPos += SLen(C4ScriptOpMap[iOpID].Identifier);
						return ATT_OPERATOR;
					}
				}
				else if (C == '*') { SPos++; return ATT_STAR; }  // "*"
				else if (C == '&') { SPos++; return ATT_AMP; }   // "&"
				else if (C == '~') { SPos++; return ATT_TILDE; } // "~"

				if (C == '.')
				{
					++SPos;
					return ATT_DOT; // "."
				}

				if (C == '?')
				{
					++SPos;
					return ATT_QMARK; // "?"
				}

				// identifier by all non-special chars
				if (C >= '@')
				{
					// Old Scripts could have wacky identifier
					if (strictLevel < C4AulScriptStrict::STRICT2)
					{
						State = TGS_Ident;
						break;
					}
					// But now only the alphabet and '_' is allowed
					else if ((C >= 'A' && C <= 'Z') || (C >= 'a' && C <= 'z') || (C == '_'))
					{
						State = TGS_Ident;
						break;
					}
					// unrecognized char
					// make sure to skip the invalid char so the error won't be output forever
					++SPos;
				}
				else
				{
					// examine next char
					++SPos; ++Len;
					// no operator expected and '-' or '+' found?
					// this could be an int const; parse it directly
					if (!bOperator && (C == '-' || C == '+'))
					{
						// skip spaces between sign and int constant
						if (AdvanceSpaces())
						{
							// continue parsing int, if a numeral follows
							C = *SPos;
							if (((C >= '0') && (C <= '9')))
							{
								State = TGS_Int;
								break;
							}
						}
					}
					// special char and/or error getting it as a signed int
				}
				// unrecognized char
				// show appropriate error message
				if (C >= '!' && C <= '~')
					throw C4AulParseError(this, FormatString("unexpected character '%c' found", C).getData());
				else
					throw C4AulParseError(this, FormatString("unexpected character 0x%x found", C).getData());
			}
			break;
		}
		case TGS_Ident: // ident and directive: parse until non ident-char is found
		case TGS_Dir:
			if (!Inside(C, '0', '9')
				&& !Inside(C, 'a', 'z')
				&& !Inside(C, 'A', 'Z')
				&& C != '_')
			{
				// return ident/directive
				Len = (std::min)(Len, C4AUL_MAX_Identifier);
				SCopy(SPos0, pToken, Len);
				// check if it's a C4ID (and NOT a label)
				bool fllid = LooksLikeID(pToken);
				if ((C != '(') && (C != ':' || *(SPos + 1) == ':') && fllid)
				{
					// will be parsed next time
					State = TGS_C4ID; SPos--; Len--;
				}
				else
				{
					// warn if using C4ID as func label
					if (fllid) Strict2Error("stupid func label: ", pToken);
					// directive?
					if (State == TGS_Dir) return ATT_DIR;
					// check reserved names
					if (SEqual(pToken, C4AUL_False)) { *pInt = false; return ATT_BOOL; }
					if (SEqual(pToken, C4AUL_True)) { *pInt = true; return ATT_BOOL; }
					if (SEqual(pToken, C4AUL_Nil) && strictLevel >= C4AulScriptStrict::STRICT3) { return ATT_NIL; }
					if (SEqual(pToken, C4AUL_Global) && strictLevel >= C4AulScriptStrict::STRICT3 && C == '-' && *(SPos + 1) == '>') // "global->"
					{
						SPos += 2;
						return ATT_GLOBALCALL;
					}
					// everything else is an identifier
					return ATT_IDTF;
				}
			}
			break;

		case TGS_Int: // integer: parse until non-number is found
		case TGS_IntHex:
			if ((C < '0') || (C > '9'))
			{
				const char *szScanCode;
				if (State == TGS_Int)
				{
					// turn to hex mode?
					if (*SPos0 == '0' && C == 'x' && Len == 1)
					{
						State = TGS_IntHex;
						break;
					}
					// some strange C4ID?
					if (((C >= 'A') && (C <= 'Z')) || (C == '_'))
					{
						State = TGS_C4ID;
						break;
					}
					// parse as decimal int
					szScanCode = "%" SCNdPTR;
				}
				else
				{
					// parse as hexadecimal int: Also allow 'a' to 'f' and 'A' to 'F'
					if ((C >= 'A' && C <= 'F') || (C >= 'a' && C <= 'f')) break;
					szScanCode = "%" SCNxPTR;
				}
				// return integer
				Len = (std::min)(Len, C4AUL_MAX_Identifier);
				SCopy(SPos0, pToken, Len);
				// or is it a func label?
				if ((C == '(') || (C == ':'))
				{
					Strict2Error("stupid func label: ", pToken);
					return ATT_IDTF;
				}
				// it's not, so return the int
				sscanf(pToken, szScanCode, pInt);
				return ATT_INT;
			}
			break;

		case TGS_C4ID: // c4id: parse until non-ident char is found
			if (!Inside(C, '0', '9')
				&& !Inside(C, 'a', 'z')
				&& !Inside(C, 'A', 'Z'))
			{
				// return C4ID string
				Len = (std::min)(Len, C4AUL_MAX_Identifier);
				SCopy(SPos0, pToken, Len);
				// another stupid label identifier?
				if ((C == '(') || (C == ':' && *(SPos + 1) != ':'))
				{
					Strict2Error("stupid func label: ", pToken);
					return ATT_IDTF;
				}
				// check if valid
				if (!LooksLikeID(pToken)) throw C4AulParseError(this, "erroneous Ident: ", pToken);
				// get id of it
				*pInt = static_cast<intptr_t>(C4Id(pToken));
				return ATT_C4ID;
			}
			break;

		case TGS_String: // string: parse until '"'; check for eof!
			// string end
			if (C == '"')
			{
				*pStrPos = 0;
				SPos++;
				// no string expected?
				if (HoldStrings == Discard) return ATT_STRING;
				// reg string (if not already done so)
				C4String *pString;
				if (!(pString = a->Engine->Strings.FindString(StrBuff)))
					pString = a->Engine->Strings.RegString(StrBuff);
				if (HoldStrings == Hold) pString->Hold = 1;
				// return pointer on string object
				*pInt = reinterpret_cast<std::intptr_t>(pString);
				return ATT_STRING;
			}
			// check: enough buffer space?
			if (pStrPos - StrBuff >= C4AUL_MAX_String)
				Warn("string too long", nullptr);
			else
			{
				if (C == '\\') // escape
					switch (*(SPos + 1))
					{
					case '"':  SPos++; *(pStrPos++) = '"';  break;
					case '\\': SPos++; *(pStrPos++) = '\\'; break;
					default:
					{
						// just insert "\"
						*(pStrPos++) = '\\';
						// show warning
						char strEscape[2] = { *(SPos + 1), 0 };
						Warn("unknown escape: ", strEscape);
					}
					}
				else if (C == 0 || C == 10 || C == 13) // line break / feed
					throw C4AulParseError(this, "string not closed");
				else
					// copy character
					*(pStrPos++) = C;
			}
			break;
		}
		// next char
		SPos++; Len++;
	}
}

static const char *GetTTName(C4AulBCCType e)
{
	switch (e)
	{
	case AB_DEREF:     return "AB_DEREF";     // deref current value
	case AB_MAPA_R:    return "AB_MAPA_R";    // map access via .
	case AB_MAPA_V:    return "AB_MAPA_V";    // not creating a reference
	case AB_ARRAYA_R:  return "AB_ARRAYA_R";  // array access
	case AB_ARRAYA_V:  return "AB_ARRAYA_V";  // not creating a reference
	case AB_ARRAY_APPEND:  return "AB_ARRAY_APPEND";  // not creating a reference
	case AB_VARN_R:    return "AB_VARN_R";    // a named var
	case AB_VARN_V:    return "AB_VARN_V";
	case AB_PARN_R:    return "AB_PARN_R";    // a named parameter
	case AB_PARN_V:    return "AB_PARN_V";
	case AB_LOCALN_R:  return "AB_LOCALN_R";  // a named local
	case AB_LOCALN_V:  return "AB_LOCALN_V";
	case AB_GLOBALN_R: return "AB_GLOBALN_R"; // a named global
	case AB_GLOBALN_V: return "AB_GLOBALN_V";
	case AB_VAR_R:     return "AB_VAR_R";     // Var statement
	case AB_VAR_V:     return "AB_VAR_V";
	case AB_PAR_R:     return "AB_PAR_R";     // Par statement
	case AB_PAR_V:     return "AB_PAR_V";
	case AB_FUNC:      return "AB_FUNC";      // function

	// prefix
	case AB_Inc1:   return "AB_Inc1";   // ++
	case AB_Dec1:   return "AB_Dec1";   // --
	case AB_BitNot: return "AB_BitNot"; // ~
	case AB_Not:    return "AB_Not";    // !
	case AB_Neg:    return "AB_Neg";    // -

	// postfix (whithout second statement)
	case AB_Inc1_Postfix: return "AB_Inc1_Postfix"; // ++
	case AB_Dec1_Postfix: return "AB_Dec1_Postfix"; // --

	// postfix
	case AB_Pow:              return "AB_Pow";              // **
	case AB_Div:              return "AB_Div";              // /
	case AB_Mul:              return "AB_Mul";              // *
	case AB_Mod:              return "AB_Mod";              // %
	case AB_Sub:              return "AB_Sub";              // -
	case AB_Sum:              return "AB_Sum";              // +
	case AB_LeftShift:        return "AB_LeftShift";        // <<
	case AB_RightShift:       return "AB_RightShift";       // >>
	case AB_LessThan:         return "AB_LessThan";         // <
	case AB_LessThanEqual:    return "AB_LessThanEqual";    // <=
	case AB_GreaterThan:      return "AB_GreaterThan";      // >
	case AB_GreaterThanEqual: return "AB_GreaterThanEqual"; // >=
	case AB_Concat:           return "AB_Concat";           // ..
	case AB_EqualIdent:       return "AB_EqualIdent";       // old ==
	case AB_Equal:            return "AB_Equal";            // new ==
	case AB_NotEqualIdent:    return "AB_NotEqualIdent";    // old !=
	case AB_NotEqual:         return "AB_NotEqual";         // new !=
	case AB_SEqual:           return "AB_SEqual";           // S=, eq
	case AB_SNEqual:          return "AB_SNEqual";          // ne
	case AB_BitAnd:           return "AB_BitAnd";           // &
	case AB_BitXOr:           return "AB_BitXOr";           // ^
	case AB_BitOr:            return "AB_BitOr";            // |
	case AB_And:              return "AB_And";              // &&
	case AB_Or:               return "AB_Or";               // ||
	case AB_NilCoalescing:    return "AB_NilCoalescing";    // ??
	case AB_PowIt:            return "AB_PowIt";            // **=
	case AB_MulIt:            return "AB_MulIt";            // *=
	case AB_DivIt:            return "AB_DivIt";            // /=
	case AB_ModIt:            return "AB_ModIt";            // %=
	case AB_Inc:              return "AB_Inc";              // +=
	case AB_Dec:              return "AB_Dec";              // -=
	case AB_LeftShiftIt:      return "AB_LeftShiftIt";      // <<=
	case AB_RightShiftIt:     return "AB_RightShiftIt";     // >>=
	case AB_ConcatIt:         return "AB_ConcatIt";         // ..=
	case AB_AndIt:            return "AB_AndIt";            // &=
	case AB_OrIt:             return "AB_OrIt";             // |=
	case AB_XOrIt:            return "AB_XOrIt";            // ^=
	case AB_NilCoalescingIt:  return "AB_NilCoalescingIt";  // ??=
	case AB_Set:              return "AB_Set";              // =

	case AB_CALL:             return "AB_CALL";             // direct object call
	case AB_CALLGLOBAL:       return "AB_CALLGLOBAL";       // global context call
	case AB_CALLFS:           return "AB_CALLFS";           // failsafe direct call
	case AB_CALLNS:           return "AB_CALLNS";           // direct object call: namespace operator
	case AB_STACK:            return "AB_STACK";            // push nulls / pop
	case AB_NIL:              return "AB_NIL";              // constant: nil
	case AB_INT:              return "AB_INT";              // constant: int
	case AB_BOOL:             return "AB_BOOL";             // constant: bool
	case AB_STRING:           return "AB_STRING";           // constant: string
	case AB_C4ID:             return "AB_C4ID";             // constant: C4ID
	case AB_ARRAY:            return "AB_ARRAY";            // semi-constant: array
	case AB_MAP:              return "AB_MAP";              // semi-constant: map
	case AB_IVARN:            return "AB_IVARN";            // initialization of named var
	case AB_JUMP:             return "AB_JUMP";             // jump
	case AB_JUMPAND:          return "AB_JUMPAND";
	case AB_JUMPOR:           return "AB_JUMPOR";
	case AB_JUMPNIL:          return "AB_JUMPNIL";
	case AB_JUMPNOTNIL:       return "AB_JUMPNOTNIL";
	case AB_CONDN:            return "AB_CONDN";            // conditional jump (negated, pops stack)
	case AB_FOREACH_NEXT:     return "AB_FOREACH_NEXT";     // foreach: next element
	case AB_FOREACH_MAP_NEXT: return "AB_FOREACH_MAP_NEXT"; // foreach: next element
	case AB_RETURN:           return "AB_RETURN";           // return statement
	case AB_ERR:              return "AB_ERR";              // parse error at this position
	case AB_EOFN:             return "AB_EOFN";             // end of function
	case AB_EOF:              return "AB_EOF";

	default: return "?";
	}
}

void C4AulScript::AddBCC(C4AulBCCType eType, std::intptr_t X, const char *SPos)
{
	// range check
	if (CodeSize >= CodeBufSize)
	{
		// create new buffer
		CodeBufSize = CodeBufSize ? 2 * CodeBufSize : C4AUL_CodeBufSize;
		C4AulBCC *nCode = new C4AulBCC[CodeBufSize];
		// copy data
		memcpy(nCode, Code, sizeof(*Code) * CodeSize);
		// replace buffer
		delete[] Code;
		Code = nCode;
		// adjust pointer
		CPos = Code + CodeSize;
	}
	// store chunk
	CPos->bccType = eType;
	CPos->bccX = X;
	CPos->SPos = SPos;
	CPos++; CodeSize++;
}

bool C4AulScript::Preparse()
{
	// handle easiest case first
	if (State < ASS_NONE) return false;
	if (!Script) { State = ASS_PREPARSED; return true; }

	// clear stuff
	Includes.clear(); Appends.clear();
	CPos = Code;
	while (Func0)
	{
		// belongs to this script?
		if (Func0->SFunc())
			if (Func0->SFunc()->pOrgScript == this)
				// then desroy linked funcs, too
				Func0->DestroyLinked();
		// destroy func
		delete Func0;
	}

	C4AulParseState state(nullptr, this, C4AulParseState::PREPARSER);
	state.Parse_Script();

	// no #strict? we don't like that :(
	if (Strict == C4AulScriptStrict::NONSTRICT)
	{
		Engine->nonStrictCnt++;
	}

	// done, reset state var
	Preparsing = false;

	// #include will have to be resolved now...
	IncludesResolved = false;

	// return success
	C4AulScript::State = ASS_PREPARSED;
	return true;
}

void C4AulParseState::AddBCC(C4AulBCCType eType, std::intptr_t X)
{
	if (Type != PARSER) return;
	// Track stack size
	switch (eType)
	{
	case AB_NIL:
	case AB_INT:
	case AB_BOOL:
	case AB_STRING:
	case AB_C4ID:
	case AB_VARN_R:
	case AB_VARN_V:
	case AB_PARN_R:
	case AB_PARN_V:
	case AB_LOCALN_R:
	case AB_LOCALN_V:
	case AB_GLOBALN_R:
	case AB_GLOBALN_V:
		iStack++;
		break;

	case AB_Pow:
	case AB_Div:
	case AB_Mul:
	case AB_Mod:
	case AB_Sub:
	case AB_Sum:
	case AB_LeftShift:
	case AB_RightShift:
	case AB_LessThan:
	case AB_LessThanEqual:
	case AB_GreaterThan:
	case AB_GreaterThanEqual:
	case AB_Concat:
	case AB_Equal:
	case AB_EqualIdent:
	case AB_NotEqual:
	case AB_NotEqualIdent:
	case AB_SEqual:
	case AB_SNEqual:
	case AB_BitAnd:
	case AB_BitXOr:
	case AB_BitOr:
	case AB_And:
	case AB_Or:
	case AB_PowIt:
	case AB_MulIt:
	case AB_DivIt:
	case AB_ModIt:
	case AB_Inc:
	case AB_Dec:
	case AB_LeftShiftIt:
	case AB_RightShiftIt:
	case AB_ConcatIt:
	case AB_AndIt:
	case AB_OrIt:
	case AB_XOrIt:
	case AB_Set:
	case AB_ARRAYA_R:
	case AB_ARRAYA_V:
	case AB_CONDN:
	case AB_IVARN:
	case AB_RETURN:
	// JUMPAND/JUMPOR/JUMPNOTNIL are special: They either jump over instructions adding one to the stack
	// or decrement the stack. Thus, for stack counting purposes, they decrement.
	case AB_JUMPAND:
	case AB_JUMPOR:
	case AB_JUMPNOTNIL:
		iStack--;
		break;

	case AB_FUNC:
		iStack -= reinterpret_cast<C4AulFunc *>(X)->GetParCount() - 1;
		break;

	case AB_CALL:
	case AB_CALLFS:
	case AB_CALLGLOBAL:
		iStack -= C4AUL_MAX_Par;
		break;

	case AB_DEREF:
	case AB_JUMPNIL:
	case AB_MAPA_R:
	case AB_MAPA_V:
	case AB_ARRAY_APPEND:
	case AB_Inc1:
	case AB_Dec1:
	case AB_BitNot:
	case AB_Not:
	case AB_Neg:
	case AB_Inc1_Postfix:
	case AB_Dec1_Postfix:
	case AB_VAR_R:
	case AB_VAR_V:
	case AB_PAR_R:
	case AB_PAR_V:
	case AB_FOREACH_NEXT:
	case AB_FOREACH_MAP_NEXT:
	case AB_ERR:
	case AB_EOFN:
	case AB_EOF:
	case AB_JUMP:
	case AB_CALLNS:
	case AB_NilCoalescingIt: // does not pop, the following AB_Set does
		break;

	case AB_STACK:
		iStack += X;
		break;

	case AB_MAP:
		iStack -= 2 * X - 1;
		break;

	case AB_ARRAY:
		iStack -= X - 1;
		break;

	default:
	case AB_NilCoalescing:
		assert(false);
	}

	// Use stack operation instead of 0-Int (enable optimization)
	if ((eType == AB_INT || eType == AB_BOOL) && !X && (Fn ? (Fn->pOrgScript->Strict < C4AulScriptStrict::STRICT3) : (a->Strict < C4AulScriptStrict::STRICT3)))
	{
		eType = AB_STACK;
		X = 1;
	}

	// Join checks only if it's not a jump target
	if (!fJump)
	{
		// Join together stack operations
		if (eType == AB_STACK &&
			a->CPos > a->Code &&
			(a->CPos - 1)->bccType == AB_STACK
			&& (X <= 0 || (a->CPos - 1)->bccX >= 0))
		{
			(a->CPos - 1)->bccX += X;
			// Empty? Remove it.
			if (!(a->CPos - 1)->bccX)
			{
				a->CPos--;
				a->CodeSize--;
			}
			return;
		}
	}

	// Add
	a->AddBCC(eType, X, SPos);

	// Reset jump flag
	fJump = false;
}

namespace
{
	void SkipExpressions(intptr_t n, C4AulBCC *&CPos, C4AulBCC *const Code)
	{
		while (n > 0 && CPos > Code)
		{
			switch (CPos->bccType)
			{
				case AB_STACK:
					if (CPos->bccX > 0) n -= CPos--->bccX;
					break;

				case AB_INT: case AB_BOOL: case AB_STRING: case AB_C4ID:
				case AB_PARN_R: case AB_PARN_V: case AB_VARN_R: case AB_VARN_V:
				case AB_LOCALN_R: case AB_LOCALN_V:
				case AB_GLOBALN_R: case AB_GLOBALN_V:
					--n;
					--CPos;
					break;

				case AB_MAPA_R: case AB_MAPA_V: case AB_ARRAY_APPEND:
					--CPos;
					SkipExpressions(1, CPos, Code);
					--n;
					break;

				case AB_ARRAYA_R: case AB_ARRAYA_V:
					--CPos;
					SkipExpressions(2, CPos, Code);
					--n;
					break;

				case AB_ARRAY:
				{
					const auto size = CPos->bccX;
					--CPos;
					SkipExpressions(size, CPos, Code);
					--n;
					break;
				}

				case AB_MAP:
				{
					const auto size = 2 * CPos->bccX;
					--CPos;
					SkipExpressions(size, CPos, Code);
					--n;
					break;

				}

				case AB_PAR_R: case AB_PAR_V: case AB_VAR_R: case AB_VAR_V:
					--CPos;
					SkipExpressions(1, CPos, Code);
					--n;
					break;

				case AB_FUNC:
				{
					const auto pars = reinterpret_cast<C4AulFunc *>(CPos->bccX)->GetParCount();
					--CPos;
					SkipExpressions(pars, CPos, Code);
					--n;
					break;
				}

				case AB_CALLNS:
					--CPos;
					break;

				case AB_CALL: case AB_CALLFS: case AB_CALLGLOBAL:
					--CPos;
					SkipExpressions(C4AUL_MAX_Par + (CPos->bccType != AB_CALLGLOBAL ? 1 : 0), CPos, Code);
					--n;
					break;

				default:
					// operator?
					if (Inside(CPos->bccType, AB_Inc1, AB_Set) && CPos > Code)
					{
						const auto &op = C4ScriptOpMap[CPos->bccX];
						--CPos;
						SkipExpressions(op.NoSecondStatement || !op.Postfix ? 1 : 2, CPos, Code);
						--n;
					}
					else
						return;
			}
		}
	}
}

void C4AulParseState::SetNoRef()
{
	if (Type != PARSER) return;
	for(C4AulBCC *CPos = a->CPos - 1; CPos >= a->Code; )
	{
		switch (CPos->bccType)
		{
		case AB_MAPA_R:
			CPos->bccType = AB_MAPA_V;
			--CPos;
			// propagate back to the accessed map
			break;
		case AB_ARRAYA_R:
			CPos->bccType = AB_ARRAYA_V;
			--CPos;
			// propagate back to the accessed array
			SkipExpressions(1, CPos, a->Code);
			break;
		case AB_PAR_R: CPos->bccType = AB_PAR_V; return;
		case AB_VAR_R: CPos->bccType = AB_VAR_V; return;
		case AB_PARN_R: CPos->bccType = AB_PARN_V; return;
		case AB_VARN_R: CPos->bccType = AB_VARN_V; return;
		case AB_LOCALN_R: CPos->bccType = AB_LOCALN_V; return;
		case AB_GLOBALN_R: CPos->bccType = AB_GLOBALN_V; return;
		default: return;
		}
	}
}

size_t C4AulParseState::JumpHere()
{
	// Set flag so the next generated code chunk won't get joined
	fJump = true;
	return a->GetCodePos();
}

namespace
{
	bool IsJumpType(C4AulBCCType type) noexcept
	{
		return type == AB_JUMP || type == AB_JUMPAND || type == AB_JUMPOR || type == AB_CONDN || type == AB_JUMPNIL || type == AB_JUMPNOTNIL || type == AB_NilCoalescingIt;
	}
}

void C4AulParseState::SetJumpHere(size_t iJumpOp)
{
	if (Type != PARSER) return;
	// Set target
	C4AulBCC *pBCC = a->GetCodeByPos(iJumpOp);
	assert(IsJumpType(pBCC->bccType));
	pBCC->bccX = a->GetCodePos() - iJumpOp;
	// Set flag so the next generated code chunk won't get joined
	fJump = true;
}

void C4AulParseState::SetJump(size_t iJumpOp, size_t iWhere)
{
	if (Type != PARSER) return;
	// Set target
	C4AulBCC *pBCC = a->GetCodeByPos(iJumpOp);
	assert(IsJumpType(pBCC->bccType));
	pBCC->bccX = iWhere - iJumpOp;
}

void C4AulParseState::AddJump(C4AulBCCType eType, size_t iWhere)
{
	AddBCC(eType, iWhere - a->GetCodePos());
}

void C4AulParseState::PushLoop()
{
	if (Type != PARSER) return;
	Loop *pNew = new Loop();
	pNew->StackSize = iStack;
	pNew->Controls = nullptr;
	pNew->Next = pLoopStack;
	pLoopStack = pNew;
}

void C4AulParseState::PopLoop()
{
	if (Type != PARSER) return;
	// Delete loop controls
	Loop *pLoop = pLoopStack;
	while (pLoop->Controls)
	{
		// Unlink
		Loop::Control *pCtrl = pLoop->Controls;
		pLoop->Controls = pCtrl->Next;
		// Delete
		delete pCtrl;
	}
	// Unlink & delete
	pLoopStack = pLoop->Next;
	delete pLoop;
}

void C4AulParseState::AddLoopControl(bool fBreak)
{
	if (Type != PARSER) return;
	Loop::Control *pNew = new Loop::Control();
	pNew->Break = fBreak;
	pNew->Pos = a->GetCodePos();
	pNew->Next = pLoopStack->Controls;
	pLoopStack->Controls = pNew;
}

const char *C4AulParseState::GetTokenName(C4AulTokenType TokenType)
{
	switch (TokenType)
	{
	case ATT_INVALID:  return "invalid token";
	case ATT_DIR:      return "directive";
	case ATT_IDTF:     return "identifier";
	case ATT_INT:      return "integer constant";
	case ATT_BOOL:     return "boolean constant";
	case ATT_NIL:      return "nil";
	case ATT_STRING:   return "string constant";
	case ATT_C4ID:     return "id constant";
	case ATT_COMMA:    return "','";
	case ATT_COLON:    return "':'";
	case ATT_DCOLON:   return "'::'";
	case ATT_SCOLON:   return "';'";
	case ATT_BOPEN:    return "'('";
	case ATT_BCLOSE:   return "')'";
	case ATT_BOPEN2:   return "'['";
	case ATT_BCLOSE2:  return "']'";
	case ATT_BLOPEN:   return "'{'";
	case ATT_BLCLOSE:  return "'}'";
	case ATT_SEP:      return "'|'";
	case ATT_CALL:     return "'->'";
	case ATT_STAR:     return "'*'";
	case ATT_AMP:      return "'&'";
	case ATT_TILDE:    return "'~'";
	case ATT_LDOTS:    return "'...'";
	case ATT_DOT:      return "'.'";
	case ATT_QMARK:    return "'?'";
	case ATT_OPERATOR: return "operator";
	case ATT_EOF:      return "end of file";
	default:           return "unrecognized token";
	}
}

void C4AulParseState::Shift(HoldStringsPolicy HoldStrings, bool bOperator)
{
	TokenType = GetNextToken(Idtf, &cInt, HoldStrings, bOperator);
}

void C4AulParseState::Match(C4AulTokenType RefTokenType, const char *Message)
{
	if (TokenType != RefTokenType)
		// error
		throw C4AulParseError(this, Message ? Message :
			FormatString("%s expected, but found %s", GetTokenName(RefTokenType), GetTokenName(TokenType)).getData());
	Shift();
}

void C4AulParseState::UnexpectedToken(const char *Expected)
{
	throw C4AulParseError(this, FormatString("%s expected, but found %s", Expected, GetTokenName(TokenType)).getData());
}

void C4AulScript::ParseFn(C4AulScriptFunc *Fn, bool fExprOnly)
{
	// check if fn overloads other fn (all func tables are built now)
	// *MUST* check Fn->Owner-list, because it may be the engine (due to linked globals)
	if (Fn->OwnerOverloaded = Fn->Owner->GetOverloadedFunc(Fn))
		if (Fn->Owner == Fn->OwnerOverloaded->Owner)
			Fn->OwnerOverloaded->OverloadedBy = Fn;
	// reset pointer to next same-named func (will be set in AfterLink)
	Fn->NextSNFunc = nullptr;
	// store byte code pos
	// (relative position to code start; code pointer may change while
	//  parsing)
	Fn->Code = reinterpret_cast<C4AulBCC *>(CPos - Code);
	// parse
	C4AulParseState state(Fn, this, C4AulParseState::PARSER);
	// get first token
	state.Shift();
	if (!fExprOnly)
		state.Parse_Function();
	else
	{
		state.Parse_Expression();
		state.SetNoRef();
		AddBCC(AB_RETURN, 0, state.SPos);
	}
	// done
	return;
}

void C4AulParseState::Parse_Script()
{
	bool fDone = false;
	const char *SPos0 = SPos;
	bool all_ok = true;
	while (!fDone) try
	{
		// Go to the next token if the current token could not be processed or no token has yet been parsed
		if (SPos == SPos0)
		{
			Shift();
		}
		SPos0 = SPos;
		switch (TokenType)
		{
		case ATT_DIR:
		{
			// check for include statement
			if (SEqual(Idtf, C4AUL_Include))
			{
				Shift();
				// get id of script to include
				if (TokenType != ATT_C4ID)
					UnexpectedToken("id constant");
				C4ID Id = static_cast<C4ID>(cInt);
				Shift();
				// add to include list
				a->Includes.push_front(Id);
			}
			else if (SEqual(Idtf, C4AUL_Append))
			{
				// for #appendto * '*' needs to be ATT_STAR, not an operator.
				Shift(Hold, false);
				// get id of script to include/append
				bool nowarn = false;
				C4ID Id;
				switch (TokenType)
				{
				case ATT_C4ID:
					Id = static_cast<C4ID>(cInt);
					Shift();
					if (TokenType == ATT_IDTF && SEqual(Idtf, C4AUL_NoWarn))
					{
						nowarn = true;
						Shift();
					}
					break;
				case ATT_STAR: // "*"
					Id = ~0;
					Shift();
					break;
				default:
					// -> ID expected
					UnexpectedToken("id constant");
				}
				// add to append list
				a->Appends.push_back({Id, nowarn});
			}
			else if (SEqual(Idtf, C4AUL_Strict))
			{
				// declare it as strict
				a->Strict = C4AulScriptStrict::STRICT1;
				Shift();
				if (TokenType == ATT_INT)
				{
					if (cInt == 2)
						a->Strict = C4AulScriptStrict::STRICT2;
					else if (cInt == 3)
						a->Strict = C4AulScriptStrict::STRICT3;
					else
						throw C4AulParseError(this, "unknown strict level");
					Shift();
				}
			}
			else
				// -> unknown directive
				throw C4AulParseError(this, "unknown directive: ", Idtf);
			break;
		}
		case ATT_IDTF:
		{
			if (SEqual(Idtf, C4AUL_For))
			{
				throw C4AulParseError(this, "unexpected for outside function");
			}
			// check for variable definition (var)
			else if (SEqual(Idtf, C4AUL_VarNamed))
			{
				throw C4AulParseError(this, "unexpected variable definition outside function");
			}
			// check for object-local variable definition (local)
			else if (SEqual(Idtf, C4AUL_LocalNamed))
			{
				Shift();
				Parse_Local();
				Match(ATT_SCOLON);
				break;
			}
			// check for variable definition (static)
			else if (SEqual(Idtf, C4AUL_GlobalNamed))
			{
				Shift();
				// constant?
				if (TokenType == ATT_IDTF && SEqual(Idtf, C4AUL_Const))
				{
					Shift();
					Parse_Const();
				}
				else
					Parse_Static();
				Match(ATT_SCOLON);
				break;
			}
			else
				Parse_FuncHead();
			break;
		}
		case ATT_EOF:
			fDone = true;
			break;
		default:
			UnexpectedToken("declaration");
		}
		all_ok = true;
	}
	catch (const C4AulError &err)
	{
		// damn! something went wrong, print it out
		// but only one error per function
		if (all_ok)
			err.show();
		all_ok = false;
	}
}

void C4AulParseState::Parse_FuncHead()
{
	C4AulAccess Acc = AA_PUBLIC;
	// Access?
	if      (SEqual(Idtf, C4AUL_Private))   { Acc = AA_PRIVATE;   Shift(); }
	else if (SEqual(Idtf, C4AUL_Protected)) { Acc = AA_PROTECTED; Shift(); }
	else if (SEqual(Idtf, C4AUL_Public))    { Acc = AA_PUBLIC;    Shift(); }
	else if (SEqual(Idtf, C4AUL_Global))    { Acc = AA_GLOBAL;    Shift(); }
	// check for func declaration
	if (SEqual(Idtf, C4AUL_Func))
	{
		Shift(Discard, false);
		bool bReturnRef = false;
		// get next token, must be func name or "&"
		if (TokenType == ATT_AMP)
		{
			bReturnRef = true;
			Shift(Discard, false);
		}
		if (TokenType != ATT_IDTF)
			UnexpectedToken("function name");
		// check: symbol already in use?
		switch (Acc)
		{
		case AA_PRIVATE:
		case AA_PROTECTED:
		case AA_PUBLIC:
			if (a->LocalNamed.GetItemNr(Idtf) != -1)
				throw C4AulParseError(this, "function definition: name already in use (local variable)");
			if (a->Def)
				break;
			// func in global context: fallthru
		case AA_GLOBAL:
			if (a->Engine->GlobalNamedNames.GetItemNr(Idtf) != -1)
				throw C4AulParseError(this, "function definition: name already in use (global variable)");
			if (a->Engine->GlobalConstNames.GetItemNr(Idtf) != -1)
				Strict2Error("function definition: name already in use (global variable)", nullptr);
		}
		// create script fn
		if (Acc == AA_GLOBAL)
		{
			// global func
			Fn = new C4AulScriptFunc(a->Engine, Idtf);
			C4AulFunc *FnLink = new C4AulFunc(a, nullptr);
			FnLink->LinkedTo = Fn; Fn->LinkedTo = FnLink;
			Acc = AA_PUBLIC;
		}
		else
		{
			// normal, local func
			Fn = new C4AulScriptFunc(a, Idtf);
		}
		// set up func (in the case we got an error)
		Fn->Script = SPos; // temporary
		Fn->Access = Acc; Fn->pOrgScript = a;
		Fn->bNewFormat = true; Fn->bReturnRef = bReturnRef;
		Shift(Discard, false);
		// expect an opening bracket now
		if (TokenType != ATT_BOPEN)
			UnexpectedToken("'('");
		Shift(Discard, false);
		// get pars
		Fn->ParNamed.Reset(); // safety :)
		int cpar = 0;
		while (1)
		{
			// closing bracket?
			if (TokenType == ATT_BCLOSE)
			{
				Fn->Script = SPos;
				Shift();
				// end of params
				break;
			}
			// too many parameters?
			if (cpar >= C4AUL_MAX_Par)
				throw C4AulParseError(this, "'func' parameter list: too many parameters (max 10)");

			if (TokenType == ATT_LDOTS)
			{
				Fn->Script = SPos;
				Shift();
				Match(ATT_BCLOSE);
				break;
			}
			// must be a name or type now
			if (TokenType != ATT_IDTF && TokenType != ATT_AMP)
			{
				UnexpectedToken("parameter or closing bracket");
			}
			char TypeIdtf[C4AUL_MAX_Identifier] = ""; // current identifier
			if (TokenType == ATT_IDTF)
			{
				SCopy(Idtf, TypeIdtf);
			}
			// type identifier?
			if (SEqual(Idtf, C4AUL_TypeInt)) { Fn->ParType[cpar] = C4V_Int; Shift(Discard, false); }
			else if (SEqual(Idtf, C4AUL_TypeBool)) { Fn->ParType[cpar] = C4V_Bool; Shift(Discard, false); }
			else if (SEqual(Idtf, C4AUL_TypeC4ID)) { Fn->ParType[cpar] = C4V_C4ID; Shift(Discard, false); }
			else if (SEqual(Idtf, C4AUL_TypeC4Object)) { Fn->ParType[cpar] = C4V_C4Object; Shift(Discard, false); }
			else if (SEqual(Idtf, C4AUL_TypeString)) { Fn->ParType[cpar] = C4V_String; Shift(Discard, false); }
			else if (SEqual(Idtf, C4AUL_TypeArray)) { Fn->ParType[cpar] = C4V_Array; Shift(Discard, false); }
			else if (SEqual(Idtf, C4AUL_TypeMap)) { Fn->ParType[cpar] = C4V_Map; Shift(Discard, false); }
			else if (SEqual(Idtf, C4AUL_TypeAny)) { Fn->ParType[cpar] = C4V_Any; Shift(Discard, false); }
			// ampersand?
			if (TokenType == ATT_AMP) { Fn->ParType[cpar] = C4V_pC4Value; Shift(Discard, false); }
			if (TokenType != ATT_IDTF)
			{
				if (SEqual(TypeIdtf, ""))
					UnexpectedToken("parameter or closing bracket");
				// A parameter with the same name as a type
				Strict2Error("parameter has the same name as type ", TypeIdtf);
				Fn->ParType[cpar] = C4V_Any;
				Fn->ParNamed.AddName(TypeIdtf);
			}
			else
			{
				Fn->ParNamed.AddName(Idtf);
				Shift();
			}
			// end of params?
			if (TokenType == ATT_BCLOSE)
			{
				Fn->Script = SPos;
				Shift();
				break;
			}
			// must be a comma now
			if (TokenType != ATT_COMMA)
				UnexpectedToken("comma or closing bracket");
			Shift(Discard, false);
			cpar++;
		}
		// ok, expect opening block
		if (TokenType != ATT_BLOPEN)
		{
			// warn
			Strict2Error("'func': expecting opening block ('{') after func declaration", nullptr);
			// not really new syntax (a sort of legacy mode)
			Fn->bNewFormat = false;
		}
		else
		{
			Fn->Script = SPos;
			Shift();
		}
		Parse_Desc();
		Parse_Function();
		Match(ATT_BLCLOSE);
		return;
	}
	// Must be old-style function declaration now
	if (a->Strict >= C4AulScriptStrict::STRICT2)
		throw C4AulParseError(this, "Declaration expected, but found identifier ", Idtf);
	// check: symbol already in use?
	switch (Acc)
	{
	case AA_PRIVATE:
	case AA_PROTECTED:
	case AA_PUBLIC:
		if (a->LocalNamed.GetItemNr(Idtf) != -1)
			throw C4AulParseError(this, "function definition: name already in use (local variable)");
		if (a->Def)
			break;
		// func in global context: fallthru
	case AA_GLOBAL:
		if (a->Engine->GlobalNamedNames.GetItemNr(Idtf) != -1)
			throw C4AulParseError(this, "function definition: name already in use (global variable)");
	}
	char FuncIdtf[C4AUL_MAX_Identifier] = ""; // current identifier
	SCopy(Idtf, FuncIdtf);
	Shift();
	if (TokenType != ATT_COLON)
		throw C4AulParseError(this, FormatString("declaration expected, but found identifier '%s'", FuncIdtf).getData());
	// create script fn
	if (Acc == AA_GLOBAL)
	{
		// global func
		Fn = new C4AulScriptFunc(a->Engine, FuncIdtf);
		C4AulFunc *FnLink = new C4AulFunc(a, nullptr);
		FnLink->LinkedTo = Fn; Fn->LinkedTo = FnLink;
		Acc = AA_PUBLIC;
	}
	else
	{
		// normal, local func
		Fn = new C4AulScriptFunc(a, FuncIdtf);
	}
	Fn->Script = SPos;
	Fn->Access = Acc;
	Fn->pOrgScript = a;
	Fn->bNewFormat = false;
	Fn->bReturnRef = false;
	Shift();
	Parse_Desc();
	const char *SPos0 = SPos;
	while (1) switch (TokenType)
	{
	// end of function
	case ATT_EOF: case ATT_DIR: return;
	case ATT_IDTF:
	{
		// check for func declaration
		if (SEqual(Idtf, C4AUL_Private)) return;
		else if (SEqual(Idtf, C4AUL_Protected)) return;
		else if (SEqual(Idtf, C4AUL_Public)) return;
		else if (SEqual(Idtf, C4AUL_Global)) return;
		else if (SEqual(Idtf, C4AUL_Func)) return;
		// check for variable definition (var)
		else if (SEqual(Idtf, C4AUL_VarNamed))
		{
			Shift();
			Parse_Var();
		}
		// check for variable definition (local)
		else if (SEqual(Idtf, C4AUL_LocalNamed))
		{
			if (a->Def)
			{
				Shift();
				Parse_Local();
			}
			else
				throw C4AulParseError(this, "'local' variable declaration in global script");
		}
		// check for variable definition (static)
		else if (SEqual(Idtf, C4AUL_GlobalNamed))
		{
			Shift();
			Parse_Static();
		}
		// might be old style declaration
		else
		{
			const char *SPos0_ = SPos;
			Shift();
			if (TokenType == ATT_COLON)
			{
				// Reset source position to the point before the label
				SPos = SPos0;
				Shift();
				return;
			}
			else
			{
				// The current token might be a label
				// In that case the next round of the loop will need to reset the position to what's in SPos0_ now
				SPos0 = SPos0_;
			}
		}
		break;
	}
	default:
	{
		SPos0 = SPos;
		Shift();
		break;
	}
	}
}

void C4AulParseState::Parse_Desc()
{
	// check for function desc
	if (TokenType == ATT_BOPEN2)
	{
		// parse for end of desc
		const char *SPos0 = SPos;
		int Len = 0;
		int iBracketsOpen = 1;
		while (true)
		{
			// another bracket open
			if (*SPos == '[') iBracketsOpen++;
			// a bracket closed
			if (*SPos == ']') iBracketsOpen--;
			// last bracket closed: at end of desc block
			if (iBracketsOpen == 0) break;
			// check for eof
			if (!*SPos)
				// -> function desc not closed
				throw C4AulParseError(this, "function desc not closed");
			// next char
			SPos++; Len++;
		}
		SPos++;
		// extract desc
		Fn->Desc.Copy(SPos0, Len);
		Fn->Script = SPos;
		Shift();
	}
	else
		Fn->Desc.Clear();
}

void C4AulParseState::Parse_Function()
{
	iStack = 0;
	Done = false;
	while (!Done) switch (TokenType)
	{
	// a block end?
	case ATT_BLCLOSE:
	{
		// new-form func?
		if (Fn->bNewFormat)
		{
			// all ok, insert a return
			C4AulBCC *CPos = a->GetCodeByPos(std::max<size_t>(a->GetCodePos(), 1) - 1);
			if (!CPos || CPos->bccType != AB_RETURN || fJump)
			{
				AddBCC(AB_NIL);
				AddBCC(AB_RETURN);
			}
			// and break
			Done = true;
			// Do not blame this function for script errors between functions
			Fn = nullptr;
			return;
		}
		throw C4AulParseError(this, "no '{' found for '}'");
	}
	case ATT_EOF:
	{
		Done = true;
		return;
	}
	default:
	{
		Parse_Statement();
		assert(!iStack);
	}
	}
}

void C4AulParseState::Parse_Block()
{
	Match(ATT_BLOPEN);
	// insert block in byte code
	while (1) switch (TokenType)
	{
	case ATT_BLCLOSE:
		Shift();
		return;
	default:
	{
		Parse_Statement();
		break;
	}
	}
}

void C4AulParseState::Parse_Statement()
{
	switch (TokenType)
	{
	// do we have a block start?
	case ATT_BLOPEN:
	{
		const auto isMap = IsMapLiteral();
		TokenType = ATT_BLOPEN;
		if (!isMap)
		{
			Parse_Block();
			return;
		}

		// fall through
	}
	case ATT_BOPEN:
	case ATT_BOPEN2:
	case ATT_OPERATOR:
	case ATT_NIL:
	case ATT_INT:   // constant in cInt
	case ATT_BOOL:  // constant in cInt
	case ATT_STRING: // reference in cInt
	case ATT_C4ID: // converted ID in cInt
	{
		Parse_Expression();
		SetNoRef();
		AddBCC(AB_STACK, -1);
		Match(ATT_SCOLON);
		return;
	}
	// additional function separator
	case ATT_SCOLON:
	{
		Shift();
		break;
	}
	case ATT_IDTF:
	{
		// check for variable definition (var)
		if (SEqual(Idtf, C4AUL_VarNamed))
		{
			Shift();
			Parse_Var();
		}
		// check for variable definition (local)
		else if (SEqual(Idtf, C4AUL_LocalNamed))
		{
			Shift();
			Parse_Local();
		}
		// check for variable definition (static)
		else if (SEqual(Idtf, C4AUL_GlobalNamed))
		{
			Shift();
			Parse_Static();
		}
		// check for parameter
		else if (Fn->ParNamed.GetItemNr(Idtf) != -1)
		{
			Parse_Expression();
			SetNoRef();
			AddBCC(AB_STACK, -1);
		}
		// check for variable (var)
		else if (Fn->VarNamed.GetItemNr(Idtf) != -1)
		{
			Parse_Expression();
			SetNoRef();
			AddBCC(AB_STACK, -1);
		}
		// check for objectlocal variable (local)
		else if (a->LocalNamed.GetItemNr(Idtf) != -1)
		{
			// global func?
			if (Fn->Owner == &Game.ScriptEngine)
				throw C4AulParseError(this, "using local variable in global function!");
			// insert variable by id
			Parse_Expression();
			AddBCC(AB_STACK, -1);
		}
		// check for global variable (static)
		else if (a->Engine->GlobalNamedNames.GetItemNr(Idtf) != -1)
		{
			Parse_Expression();
			AddBCC(AB_STACK, -1);
		}
		// check new-form func begin
		else if (SEqual(Idtf, C4AUL_Func))
		{
			// break parsing: start of next func
			Done = true;
			break;
		}
		// get function by identifier: first check special functions
		else if (SEqual(Idtf, C4AUL_If)) // if
		{
			Shift();
			Parse_If();
			break;
		}
		else if (SEqual(Idtf, C4AUL_Else)) // else
		{
			throw C4AulParseError(this, "misplaced 'else'");
		}
		else if (SEqual(Idtf, C4AUL_While)) // while
		{
			Shift();
			Parse_While();
			break;
		}
		else if (SEqual(Idtf, C4AUL_For)) // for
		{
			Shift();
			// Look if it's the "for ([var] foo in array) or for ([var] k, v in map)"-form
			const char *SPos0 = SPos;
			// must be followed by a bracket
			Match(ATT_BOPEN);
			// optional var
			if (TokenType == ATT_IDTF && SEqual(Idtf, C4AUL_VarNamed))
				Shift();

			bool isForEach = false;
			// variable and "in"
			if (TokenType == ATT_IDTF
				&& GetNextToken(Idtf, &cInt, Discard, true) == ATT_IDTF
				&& SEqual(Idtf, C4AUL_In)) isForEach = true;

			SPos = SPos0;
			Shift();
			if (!isForEach)
			{
				if (TokenType == ATT_IDTF && SEqual(Idtf, C4AUL_VarNamed))
					Shift();
				if (TokenType == ATT_IDTF
				&& GetNextToken(Idtf, &cInt, Discard, true) == ATT_COMMA
				&& GetNextToken(Idtf, &cInt, Discard, true) == ATT_IDTF
				&& GetNextToken(Idtf, &cInt, Discard, true) == ATT_IDTF
				&& SEqual(Idtf, C4AUL_In)) isForEach = true;

				SPos = SPos0;
				Shift();
			}

			if (isForEach) Parse_ForEach();
			else Parse_For();
			break;
		}
		else if (SEqual(Idtf, C4AUL_Return)) // return
		{
			bool multi_params_hack = false;
			Shift();
			if (TokenType == ATT_BOPEN && Fn->pOrgScript->Strict < C4AulScriptStrict::STRICT2)
			{
				// parse return(retvals) - return(retval, unused, parameters, ...) allowed for backwards compatibility
				if (Parse_Params(1, nullptr) == 1)
				{
					// return (1 + 1) * 3 returns 6, not 2
					Parse_Expression2();
				}
				else
					multi_params_hack = true;
			}
			else if (TokenType == ATT_SCOLON)
			{
				// allow return; without return value (implies nil)
				AddBCC(AB_NIL);
			}
			else
			{
				Parse_Expression();
			}
			if (!Fn->bReturnRef)
				SetNoRef();
			AddBCC(AB_RETURN);
			if (multi_params_hack && TokenType != ATT_SCOLON)
			{
				// return (1, 1) * 3 returns 1
				// but warn about it, the * 3 won't get executed and a stray ',' could lead to unexpected results
				Warn("';' expected, but found ", GetTokenName(TokenType));
				AddBCC(AB_STACK, +1);
				Parse_Expression2();
				AddBCC(AB_STACK, -1);
			}
		}
		else if (SEqual(Idtf, C4AUL_this)) // this on top level
		{
			Parse_Expression();
			SetNoRef();
			AddBCC(AB_STACK, -1);
		}
		else if (SEqual(Idtf, C4AUL_Break)) // break
		{
			Shift();
			if (Type == PARSER)
			{
				// Must be inside a loop
				if (!pLoopStack)
				{
					Strict2Error("'break' is only allowed inside loops");
				}
				else
				{
					// Insert code
					if (pLoopStack->StackSize != iStack)
						AddBCC(AB_STACK, pLoopStack->StackSize - iStack);
					AddLoopControl(true);
					AddBCC(AB_JUMP);
				}
			}
		}
		else if (SEqual(Idtf, C4AUL_Continue)) // continue
		{
			Shift();
			if (Type == PARSER)
			{
				// Must be inside a loop
				if (!pLoopStack)
				{
					Strict2Error("'continue' is only allowed inside loops");
				}
				else
				{
					// Insert code
					if (pLoopStack->StackSize != iStack)
						AddBCC(AB_STACK, pLoopStack->StackSize - iStack);
					AddLoopControl(false);
					AddBCC(AB_JUMP);
				}
			}
		}
		else if (SEqual(Idtf, C4AUL_Var)) // Var
		{
			Parse_Expression();
			SetNoRef();
			AddBCC(AB_STACK, -1);
		}
		else if (SEqual(Idtf, C4AUL_Par)) // Par
		{
			Parse_Expression();
			SetNoRef();
			AddBCC(AB_STACK, -1);
		}
		else if (SEqual(Idtf, C4AUL_Inherited) || SEqual(Idtf, C4AUL_SafeInherited))
		{
			Parse_Expression();
			SetNoRef();
			AddBCC(AB_STACK, -1);
		}
		// now this may be the end of the function: first of all check for access directives
		else if (SEqual(Idtf, C4AUL_Private) ||
			SEqual(Idtf, C4AUL_Protected) ||
			SEqual(Idtf, C4AUL_Public) ||
			SEqual(Idtf, C4AUL_Global))
		{
			Shift();
			// check if it's followed by a function declaration
			// 'func' idtf?
			if (TokenType == ATT_IDTF && SEqual(Idtf, C4AUL_Func))
			{
				// ok, break here
				Done = true;
			}
			else
			{
				// expect function name and colon
				Match(ATT_IDTF);
				Match(ATT_COLON);
				// break here
				Done = true;
			}
			break;
		}
		else
		{
			bool gotohack = false;
			// none of these? then it's a function
			// if it's a label, it will be missinterpreted here, which will be corrected later
			// it may be the first goto() found? (old syntax only!)
			if (SEqual(Idtf, C4AUL_Goto) && Fn->pOrgScript->Strict == C4AulScriptStrict::NONSTRICT)
				// add AB_RETURN later on
				gotohack = true;
			C4AulFunc *FoundFn;
			// old syntax: do not allow recursive calls in overloaded functions
			if (Fn->pOrgScript->Strict == C4AulScriptStrict::NONSTRICT && Fn->OwnerOverloaded && SEqual(Idtf, Fn->Name))
				FoundFn = Fn->OwnerOverloaded;
			else
				// get regular function
				if (Fn->Owner == &Game.ScriptEngine)
					FoundFn = a->Owner->GetFuncRecursive(Idtf);
				else
					FoundFn = a->GetFuncRecursive(Idtf);

			if (FoundFn && FoundFn->SFunc() && FoundFn->SFunc()->Access < Fn->pOrgScript->GetAllowedAccess(FoundFn, Fn->pOrgScript))
			{
				throw C4AulParseError(this, "insufficient access level", Idtf);
			}

			// ignore func-not-found errors in the preparser, because the function tables are not built yet
			if (!FoundFn && Type == PARSER)
			{
				// the function could not be found
				// this *could* be because it's a label to the next function, which, however, is not visible in the current
				// context (e.g., global functions) [Soeren]
				// parsing would have to be aborted anyway, so have a short look at the next token
				if (GetNextToken(Idtf, &cInt, Discard, true) == ATT_COLON)
				{
					// OK, next function found. abort
					Done = true;
					break;
				}
				// -> func not found
				throw C4AulParseError(this, "unknown identifier: ", Idtf);
			}
			Shift();
			// check if it's a label - labels like OCF_Living are OK (ugh...)
			if (TokenType == ATT_COLON)
			{
				// break here
				Done = true;
				return;
			}
			// The preparser assumes the syntax is correct
			if (TokenType == ATT_BOPEN || Type == PARSER)
				Parse_Params(FoundFn ? FoundFn->GetParCount() : 10, FoundFn ? FoundFn->Name : Idtf, FoundFn);
			AddBCC(AB_FUNC, reinterpret_cast<std::intptr_t>(FoundFn));
			if (gotohack)
			{
				AddBCC(AB_RETURN);
				AddBCC(AB_STACK, +1);
			}
			Parse_Expression2();
			SetNoRef();
			AddBCC(AB_STACK, -1);
		}
		Match(ATT_SCOLON);
		break;
	}
	default:
	{
		// -> unexpected token
		UnexpectedToken("statement");
	}
	}
}

int C4AulParseState::Parse_Params(int iMaxCnt, const char *sWarn, C4AulFunc *pFunc)
{
	int size = 0;
	// so it's a regular function; force "("
	Match(ATT_BOPEN);
	bool fDone = false;
	do
		switch (TokenType)
		{
		case ATT_BCLOSE:
		{
			Shift();
			// () -> size 0, (*,) -> size 2, (*,*,) -> size 3
			if (size > 0)
			{
				AddBCC(AB_NIL);
				++size;
			}
			fDone = true;
			break;
		}
		case ATT_COMMA:
		{
			// got no parameter before a ","? then push a 0-constant
			AddBCC(AB_NIL);
			Shift();
			++size;
			break;
		}
		case ATT_LDOTS:
		{
			Shift();
			// Push all unnamed parameters of the current function as parameters
			int i = Fn->ParNamed.iSize;
			while (size < iMaxCnt && i < C4AUL_MAX_Par)
			{
				AddBCC(AB_PARN_R, i);
				++i;
				++size;
			}
			// Do not allow more parameters even if there is place left
			fDone = true;
			Match(ATT_BCLOSE);
			break;
		}
		default:
		{
			// get a parameter
			Parse_Expression();
			if (pFunc)
			{
				bool anyfunctakesref = pFunc->GetParCount() > size && (pFunc->GetParType()[size] == C4V_pC4Value);
				// pFunc either was the return value from a GetFuncFast-Call or
				// pFunc is the only function that could be called, so this loop is superflous
				C4AulFunc *pFunc2 = pFunc;
				while (pFunc2 = a->Engine->GetNextSNFunc(pFunc2))
					if (pFunc2->GetParCount() > size && pFunc2->GetParType()[size] == C4V_pC4Value) anyfunctakesref = true;
				// Change the bytecode to the equivalent that does not produce a reference.
				if (!anyfunctakesref)
					SetNoRef();
			}
			++size;
			// end of parameter list?
			if (TokenType == ATT_COMMA)
				Shift();
			else if (TokenType == ATT_BCLOSE)
			{
				Shift();
				fDone = true;
			}
			else UnexpectedToken("',' or ')'");
			break;
		}
		}
	while (!fDone);
	// too many parameters?
	if (sWarn && size > iMaxCnt && Type == PARSER)
		Warn(FormatString("%s: passing %d parameters, but only %d are used", sWarn, size, iMaxCnt).getData(), nullptr);
	// Balance stack
	if (size != iMaxCnt)
		AddBCC(AB_STACK, iMaxCnt - size);
	return size;
}

void C4AulParseState::Parse_Array()
{
	// force "["
	Match(ATT_BOPEN2);
	// Create an array
	int size = 0;
	bool fDone = false;
	do
		switch (TokenType)
		{
		case ATT_BCLOSE2:
		{
			Shift();
			// [] -> size 0, [*,] -> size 2, [*,*,] -> size 3
			if (size > 0)
			{
				AddBCC(AB_NIL);
				++size;
			}
			fDone = true;
			break;
		}
		case ATT_COMMA:
		{
			// got no parameter before a ","? then push a 0-constant
			AddBCC(AB_NIL);
			Shift();
			++size;
			break;
		}
		default:
		{
			Parse_Expression();
			SetNoRef();
			++size;
			if (TokenType == ATT_COMMA)
				Shift();
			else if (TokenType == ATT_BCLOSE2)
			{
				Shift();
				fDone = true;
				break;
			}
			else
				UnexpectedToken("',' or ']'");
		}
		}
	while (!fDone);
	// add terminator
	AddBCC(AB_ARRAY, size);
}

void C4AulParseState::Parse_Map()
{
	// force "{"
	Match(ATT_BLOPEN);
	// Create a map
	int size = 0;
	bool fDone = false;
	do
		switch (TokenType)
		{
		case ATT_BLCLOSE:
		{
			Shift();
			fDone = true;
			break;
		}
		case ATT_COMMA:
		{
			// got no parameter before a ","? this is an error in a map
			throw C4AulParseError(this, "',' found in a map without preceding key-value assignment ");
			break;
		}
		default:
		{
			switch (TokenType)
			{
				case ATT_IDTF:
				{
					C4String *string;
					if (!(string = a->Engine->Strings.FindString(Idtf)))
						string = a->Engine->Strings.RegString(Idtf);
					if (Type == PARSER) string->Hold = true;
					AddBCC(AB_STRING, reinterpret_cast<std::intptr_t>(string));
					Shift();
					break;
				}
				case ATT_STRING:
					AddBCC(AB_STRING, cInt);
					Shift();
					break;
				case ATT_BOPEN2:
					Shift();
					Parse_Expression();
					SetNoRef();
					Match(ATT_BCLOSE2);
					break;
				default:
					UnexpectedToken("string or identifier");
			}

			if (TokenType != ATT_OPERATOR || !SEqual(C4ScriptOpMap[cInt].Identifier, "="))
			{
				UnexpectedToken("'='");
			}
			Shift();

			Parse_Expression();
			SetNoRef();
			++size;
			if (TokenType == ATT_COMMA)
				Shift();
			else if (TokenType == ATT_BLCLOSE)
			{
				Shift();
				fDone = true;
				break;
			}
			else
				UnexpectedToken("',' or '}'");
		}
		}
	while (!fDone);
	// add terminator
	AddBCC(AB_MAP, size);
}

void C4AulParseState::Parse_While()
{
	// Save position for later jump back
	const auto iStart = JumpHere();
	// Execute condition
	if (Fn->pOrgScript->Strict >= C4AulScriptStrict::STRICT2)
	{
		Match(ATT_BOPEN);
		Parse_Expression();
		Match(ATT_BCLOSE);
	}
	else
		Parse_Params(1, C4AUL_While);
	SetNoRef();
	// Check condition
	const auto iCond = a->GetCodePos();
	AddBCC(AB_CONDN);
	// We got a loop
	PushLoop();
	// Execute body
	Parse_Statement();
	if (Type != PARSER) return;
	// Jump back
	AddJump(AB_JUMP, iStart);
	// Set target for conditional jump
	SetJumpHere(iCond);
	// Set targets for break/continue
	for (Loop::Control *pCtrl = pLoopStack->Controls; pCtrl; pCtrl = pCtrl->Next)
		if (pCtrl->Break)
			SetJumpHere(pCtrl->Pos);
		else
			SetJump(pCtrl->Pos, iStart);
	PopLoop();
}

void C4AulParseState::Parse_If()
{
	if (Fn->pOrgScript->Strict >= C4AulScriptStrict::STRICT2)
	{
		Match(ATT_BOPEN);
		Parse_Expression();
		Match(ATT_BCLOSE);
	}
	else
		Parse_Params(1, C4AUL_If);
	SetNoRef();
	// create bytecode, remember position
	const auto iCond = a->GetCodePos();
	AddBCC(AB_CONDN);
	// parse controlled statement
	Parse_Statement();
	if (TokenType == ATT_IDTF && SEqual(Idtf, C4AUL_Else))
	{
		// add jump
		const auto iJump = a->GetCodePos();
		AddBCC(AB_JUMP);
		// set condition jump target
		SetJumpHere(iCond);
		Shift();
		// expect a command now
		Parse_Statement();
		// set jump target
		SetJumpHere(iJump);
	}
	else
		// set condition jump target
		SetJumpHere(iCond);
}

void C4AulParseState::Parse_For()
{
	// Initialization
	if (TokenType == ATT_IDTF && SEqual(Idtf, C4AUL_VarNamed))
	{
		Shift();
		Parse_Var();
	}
	else if (TokenType != ATT_SCOLON)
	{
		Parse_Expression();
		SetNoRef();
		AddBCC(AB_STACK, -1);
	}
	// Consume first semicolon
	Match(ATT_SCOLON);
	// Condition
	size_t iCondition = SizeMax, iJumpBody = SizeMax, iJumpOut = SizeMax;
	if (TokenType != ATT_SCOLON)
	{
		// Add condition code
		iCondition = JumpHere();
		Parse_Expression();
		SetNoRef();
		// Jump out
		iJumpOut = a->GetCodePos();
		AddBCC(AB_CONDN);
	}
	// Consume second semicolon
	Match(ATT_SCOLON);
	// Incrementor
	size_t iIncrementor = SizeMax;
	if (TokenType != ATT_BCLOSE)
	{
		// Must jump over incrementor
		iJumpBody = a->GetCodePos();
		AddBCC(AB_JUMP);
		// Add incrementor code
		iIncrementor = JumpHere();
		Parse_Expression();
		SetNoRef();
		AddBCC(AB_STACK, -1);
		// Jump to condition
		if (iCondition != SizeMax)
			AddJump(AB_JUMP, iCondition);
	}
	// Consume closing bracket
	Match(ATT_BCLOSE);
	// Allow break/continue from now on
	PushLoop();
	// Body
	const auto iBody = JumpHere();
	if (iJumpBody != SizeMax)
		SetJumpHere(iJumpBody);
	Parse_Statement();
	if (Type != PARSER) return;
	// Where to jump back?
	size_t iJumpBack;
	if (iIncrementor != SizeMax)
		iJumpBack = iIncrementor;
	else if (iCondition != SizeMax)
		iJumpBack = iCondition;
	else
		iJumpBack = iBody;
	AddJump(AB_JUMP, iJumpBack);
	// Set target for condition
	if (iJumpOut != SizeMax)
		SetJumpHere(iJumpOut);
	// Set targets for break/continue
	for (Loop::Control *pCtrl = pLoopStack->Controls; pCtrl; pCtrl = pCtrl->Next)
		if (pCtrl->Break)
			SetJumpHere(pCtrl->Pos);
		else
			SetJump(pCtrl->Pos, iJumpBack);
	PopLoop();
}

void C4AulParseState::Parse_ForEach()
{
	bool forMap = false;

	if (TokenType == ATT_IDTF && SEqual(Idtf, C4AUL_VarNamed))
	{
		Shift();
	}
	// get variable name
	if (TokenType != ATT_IDTF)
		UnexpectedToken("variable name");
	if (Type == PREPARSER)
	{
		// insert variable
		Fn->VarNamed.AddName(Idtf);
	}
	// search variable (fail if not found)
	int iVarID = Fn->VarNamed.GetItemNr(Idtf);
	if (iVarID < 0)
		throw C4AulParseError(this, "internal error: var definition: var not found in variable table");
	Shift();

	int iVarIDForMapValue = 0;
	if (TokenType == ATT_COMMA)
	{
		Shift();
		if (TokenType == ATT_IDTF && !SEqual(Idtf, C4AUL_In))
		{
			forMap = true;

			if (Type == PREPARSER)
			{
				// insert variable
				Fn->VarNamed.AddName(Idtf);
			}
			// search variable (fail if not found)
			iVarIDForMapValue = Fn->VarNamed.GetItemNr(Idtf);
			if (iVarIDForMapValue < 0)
				throw C4AulParseError(this, "internal error: var definition: var not found in variable table");
			Shift();
		}
	}

	if (TokenType != ATT_IDTF || !SEqual(Idtf, C4AUL_In))
		UnexpectedToken("'in'");
	Shift();
	// get expression for array or map
	Parse_Expression();
	Match(ATT_BCLOSE);
	// push second var id for the map iteration
	if (forMap) AddBCC(AB_INT, iVarIDForMapValue);
	// push initial position (0)
	AddBCC(AB_INT);
	// get array element
	const auto iStart = a->GetCodePos();
	AddBCC(forMap ? AB_FOREACH_MAP_NEXT : AB_FOREACH_NEXT, iVarID);
	// jump out (FOREACH[_MAP]_NEXT will jump over this if
	// we're not at the end of the array yet)
	const auto iCond = a->GetCodePos();
	AddBCC(AB_JUMP);
	// got a loop...
	PushLoop();
	// loop body
	Parse_Statement();
	if (Type != PARSER) return;
	// jump back
	AddJump(AB_JUMP, iStart);
	// set condition jump target
	SetJumpHere(iCond);
	// set jump targets for break/continue
	for (Loop::Control *pCtrl = pLoopStack->Controls; pCtrl; pCtrl = pCtrl->Next)
		if (pCtrl->Break)
			SetJumpHere(pCtrl->Pos);
		else
			SetJump(pCtrl->Pos, iStart);
	PopLoop();
	// remove array/map and counter/iterator from stack
	AddBCC(AB_STACK, forMap ? -3 : -2);
}

void C4AulParseState::Parse_Expression(int iParentPrio)
{
	switch (TokenType)
	{
	case ATT_IDTF:
	{
		// check for parameter (par)
		if (Fn->ParNamed.GetItemNr(Idtf) != -1)
		{
			// insert variable by id
			AddBCC(AB_PARN_R, Fn->ParNamed.GetItemNr(Idtf));
			Shift();
		}
		// check for variable (var)
		else if (Fn->VarNamed.GetItemNr(Idtf) != -1)
		{
			// insert variable by id
			AddBCC(AB_VARN_R, Fn->VarNamed.GetItemNr(Idtf));
			Shift();
		}
		// check for variable (local)
		else if (a->LocalNamed.GetItemNr(Idtf) != -1)
		{
			// global func?
			if (Fn->Owner == &Game.ScriptEngine)
				throw C4AulParseError(this, "using local variable in global function!");
			// insert variable by id
			AddBCC(AB_LOCALN_R, a->LocalNamed.GetItemNr(Idtf));
			Shift();
		}
		// check for global variable (static)
		else if (a->Engine->GlobalNamedNames.GetItemNr(Idtf) != -1)
		{
			// insert variable by id
			AddBCC(AB_GLOBALN_R, a->Engine->GlobalNamedNames.GetItemNr(Idtf));
			Shift();
		}
		// function identifier: check special functions
		else if (SEqual(Idtf, C4AUL_If))
			// -> if is not a valid parameter
			throw C4AulParseError(this, "'if' may not be used as a parameter");
		else if (SEqual(Idtf, C4AUL_While))
			// -> while is not a valid parameter
			throw C4AulParseError(this, "'while' may not be used as a parameter");
		else if (SEqual(Idtf, C4AUL_Else))
			// -> else is not a valid parameter
			throw C4AulParseError(this, "misplaced 'else'");
		else if (SEqual(Idtf, C4AUL_For))
			// -> for is not a valid parameter
			throw C4AulParseError(this, "'for' may not be used as a parameter");
		else if (SEqual(Idtf, C4AUL_Return))
		{
			// return: treat as regular function with special byte code
			Strict2Error("return used as a parameter", nullptr);
			Shift();
			Parse_Params(1, nullptr);
			AddBCC(AB_RETURN);
			AddBCC(AB_STACK, +1);
		}
		else if (SEqual(Idtf, C4AUL_Par))
		{
			// and for Par
			Shift();
			Parse_Params(1, C4AUL_Par);
			AddBCC(AB_PAR_R);
		}
		else if (SEqual(Idtf, C4AUL_Var))
		{
			// same for Var
			Shift();
			Parse_Params(1, C4AUL_Var);
			AddBCC(AB_VAR_R);
		}
		else if (SEqual(Idtf, C4AUL_Inherited) || SEqual(Idtf, C4AUL_SafeInherited))
		{
			Shift();
			// inherited keyword: check strict syntax
			if (Fn->pOrgScript->Strict == C4AulScriptStrict::NONSTRICT) throw C4AulParseError(this, "inherited disabled; use #strict syntax!");
			// get function
			if (Fn->OwnerOverloaded)
			{
				// add direct call to byte code
				Parse_Params(Fn->OwnerOverloaded->GetParCount(), nullptr, Fn->OwnerOverloaded);
				AddBCC(AB_FUNC, reinterpret_cast<std::intptr_t>(Fn->OwnerOverloaded));
			}
			else
				// not found? raise an error, if it's not a safe call
				if (SEqual(Idtf, C4AUL_Inherited) && Type == PARSER)
					throw C4AulParseError(this, "inherited function not found, use _inherited to call failsafe");
				else
				{
					// otherwise, parse parameters, but discard them
					Parse_Params(0, nullptr);
					// Push a null as return value
					AddBCC(AB_STACK, 1);
				}
		}
		else if (Fn->pOrgScript->Strict == C4AulScriptStrict::NONSTRICT && Fn->OwnerOverloaded && SEqual(Idtf, Fn->Name))
		{
			// old syntax: do not allow recursive calls in overloaded functions
			Shift();
			Parse_Params(Fn->OwnerOverloaded->GetParCount(), Fn->Name, Fn);
			AddBCC(AB_FUNC, reinterpret_cast<std::intptr_t>(Fn->OwnerOverloaded));
		}
		else
		{
			C4AulFunc *FoundFn;
			// get regular function
			if (Fn->Owner == &Game.ScriptEngine)
				FoundFn = Fn->Owner->GetFuncRecursive(Idtf);
			else
				FoundFn = a->GetFuncRecursive(Idtf);
			if (Type == PREPARSER)
			{
				Shift();
				// The preparser just assumes that the syntax is correct: if no '(' follows, it must be a constant
				if (TokenType == ATT_BOPEN)
					Parse_Params(FoundFn ? FoundFn->GetParCount() : C4AUL_MAX_Par, Idtf, FoundFn);
			}
			else if (FoundFn)
			{
				Shift();
				// Function parameters for all functions except "this", which can be used without
				if (!SEqual(FoundFn->Name, C4AUL_this) || TokenType == ATT_BOPEN)
					Parse_Params(FoundFn->GetParCount(), FoundFn->Name, FoundFn);
				else
					AddBCC(AB_STACK, FoundFn->GetParCount());
				AddBCC(AB_FUNC, reinterpret_cast<std::intptr_t>(FoundFn));
			}
			else
			{
				// -> func not found
				// check for global constant (static const)
				// global constants have lowest priority for backwards compatibility
				// it is now allowed to have functional overloads of these constants
				C4Value val;
				if (a->Engine->GetGlobalConstant(Idtf, &val))
				{
					// store as direct constant
					switch (val.GetType())
					{
					case C4V_Int:    AddBCC(AB_INT, val._getInt()); break;
					case C4V_Bool:   AddBCC(AB_BOOL, val._getBool()); break;
					case C4V_String: AddBCC(AB_STRING, reinterpret_cast<std::intptr_t>(val.GetData().Str)); break;
					case C4V_C4ID:   AddBCC(AB_C4ID, val._getC4ID()); break;
					case C4V_Any:    AddBCC(AB_NIL); break;
					default:
					{
						throw C4AulParseError(this, FormatString("internal error: constant %s has undefined type %d", Idtf, val.GetType()).getData());
					}
					}
					Shift();
					// now let's check whether it used old- or new-style
					if (TokenType == ATT_BOPEN && Fn->pOrgScript->Strict < C4AulScriptStrict::STRICT2)
					{
						// old-style usage: ignore function call
						// must not use parameters here (although generating the byte code for that would be possible)
						Shift();
						Match(ATT_BCLOSE, "parameters not allowed in functional usage of constants");
					}
				}
				else
				{
					// identifier could not be resolved
					throw C4AulParseError(this, "unknown identifier: ", Idtf);
				}
			}
		}
		break;
	}
	case ATT_NIL:
	{
		AddBCC(AB_NIL);
		Shift();
		break;
	}
	case ATT_INT: // constant in cInt
	{
		AddBCC(AB_INT, cInt);
		Shift();
		break;
	}
	case ATT_BOOL: // constant in cInt
	{
		AddBCC(AB_BOOL, cInt);
		Shift();
		break;
	}
	case ATT_STRING: // reference in cInt
	{
		AddBCC(AB_STRING, cInt);
		Shift();
		break;
	}
	case ATT_C4ID: // converted ID in cInt
	{
		AddBCC(AB_C4ID, cInt);
		Shift();
		break;
	}
	case ATT_OPERATOR:
	{
		// -> must be a prefix operator
		// get operator ID
		const auto OpID = cInt;
		// postfix?
		if (C4ScriptOpMap[OpID].Postfix)
			// oops. that's wrong
			throw C4AulParseError(this, "postfix operator without first expression");
		Shift();
		// generate code for the following expression
		Parse_Expression(C4ScriptOpMap[OpID].Priority);
		// ignore?
		if (SEqual(C4ScriptOpMap[OpID].Identifier, "+"))
			break;
		// negate constant?
		if (Type == PARSER && SEqual(C4ScriptOpMap[OpID].Identifier, "-"))
			if ((a->CPos - 1)->bccType == AB_INT)
			{
				(a->CPos - 1)->bccX = -(a->CPos - 1)->bccX;
				break;
			}
		// write byte code
		AddBCC(C4ScriptOpMap[OpID].Code, OpID);
		break;
	}
	case ATT_BOPEN:
	{
		// parse it like a function...
		Shift();
		Parse_Expression();
		Match(ATT_BCLOSE);
		break;
	}
	case ATT_BOPEN2:
	{
		// Arrays are not tested in non-strict mode at all
		if (Fn->pOrgScript->Strict == C4AulScriptStrict::NONSTRICT)
			throw C4AulParseError(this, "unexpected '['");
		Parse_Array();
		break;
	}
	case ATT_BLOPEN:
	{
		// Maps are not tested below strict 3 mode at all
		if (Fn->pOrgScript->Strict < C4AulScriptStrict::STRICT3)
			throw C4AulParseError(this, "unexpected '{'");
		Parse_Map();
		break;
	}
	case ATT_GLOBALCALL:
		break;
	default:
	{
		// -> unexpected token
		UnexpectedToken("expression");
	}
	}
	Parse_Expression2(iParentPrio);
}

void C4AulParseState::Parse_Expression2(int iParentPrio)
{
	while (1) switch (TokenType)
	{
	case ATT_OPERATOR:
	{
		// expect postfix operator
		auto OpID = cInt;
		if (!C4ScriptOpMap[OpID].Postfix)
		{
			// does an operator with the same name exist?
			// when it's a postfix-operator, it can be used instead.
			auto nOpID = OpID + 1;
			for (; C4ScriptOpMap[nOpID].Identifier; nOpID++)
				if (SEqual(C4ScriptOpMap[OpID].Identifier, C4ScriptOpMap[nOpID].Identifier))
					if (C4ScriptOpMap[nOpID].Postfix)
						break;
			// not found?
			if (!C4ScriptOpMap[nOpID].Identifier)
			{
				throw C4AulParseError(this, "unexpected prefix operator: ", C4ScriptOpMap[OpID].Identifier);
			}
			// otherwise use the new-found correct postfix operator
			OpID = nOpID;
		}
		// lower priority?
		if (C4ScriptOpMap[OpID].RightAssociative ?
			C4ScriptOpMap[OpID].Priority < iParentPrio :
			C4ScriptOpMap[OpID].Priority <= iParentPrio)
			return;
		// If the operator does not modify the first argument, no reference is necessary
		if (C4ScriptOpMap[OpID].Type1 != C4V_pC4Value)
			SetNoRef();
		Shift();

		if (((C4ScriptOpMap[OpID].Code == AB_And || C4ScriptOpMap[OpID].Code == AB_Or) && Fn->pOrgScript->Strict >= C4AulScriptStrict::STRICT2) || C4ScriptOpMap[OpID].Code == AB_NilCoalescing || C4ScriptOpMap[OpID].Code == AB_NilCoalescingIt)
		{
			// create bytecode, remember position
			const auto iCond = a->GetCodePos();

			if (C4ScriptOpMap[OpID].Code == AB_NilCoalescingIt)
			{
				AddBCC(AB_NilCoalescingIt);
			}
			else
			{
				// Jump or discard first parameter
				AddBCC(C4ScriptOpMap[OpID].Code == AB_And ? AB_JUMPAND : C4ScriptOpMap[OpID].Code == AB_Or ? AB_JUMPOR : AB_JUMPNOTNIL);
			}

			// parse second expression
			Parse_Expression(C4ScriptOpMap[OpID].Priority);

			if (C4ScriptOpMap[OpID].Code == AB_NilCoalescingIt)
			{
				AddBCC(AB_Set, OpID);
			}

			// set condition jump target
			SetJumpHere(iCond);
			break;
		}
		else
		{
			// expect second parameter for operator
			if (!C4ScriptOpMap[OpID].NoSecondStatement)
			{
				switch (TokenType)
				{
				case ATT_IDTF: case ATT_NIL: case ATT_INT: case ATT_BOOL: case ATT_STRING: case ATT_C4ID:
				case ATT_OPERATOR: case ATT_BOPEN: case ATT_BOPEN2: case ATT_BLOPEN:
					Parse_Expression(C4ScriptOpMap[OpID].Priority);
					// If the operator does not modify the second argument, no reference is necessary
					if (C4ScriptOpMap[OpID].Type2 != C4V_pC4Value)
						SetNoRef();
					break;
				default:
					// Stuff like foo(42+,1) used to silently work
					Strict2Error(FormatString("Operator %s: Second expression expected, but %s found",
						C4ScriptOpMap[OpID].Identifier, GetTokenName(TokenType)).getData(), nullptr);
					AddBCC(AB_INT, 0);
					break;
				}
			}
			// write byte code, with a few backward compat changes
			if (C4ScriptOpMap[OpID].Code == AB_Equal && Fn->pOrgScript->Strict < C4AulScriptStrict::STRICT2)
				AddBCC(AB_EqualIdent, OpID);
			else if (C4ScriptOpMap[OpID].Code == AB_NotEqual && Fn->pOrgScript->Strict < C4AulScriptStrict::STRICT2)
				AddBCC(AB_NotEqualIdent, OpID);
			else
				AddBCC(C4ScriptOpMap[OpID].Code, OpID);
		}
		break;
	}
	default:
	{
		if (!Parse_Expression3()) return;
	}
	}
}

bool C4AulParseState::Parse_Expression3()
{
	switch (TokenType)
	{
		case ATT_BOPEN2:
		{
			// Arrays are not tested in non-strict mode at all
			if (Fn->pOrgScript->Strict == C4AulScriptStrict::NONSTRICT)
				throw C4AulParseError(this, "unexpected '['");
			// Access the array
			const char *SPos0 = SPos;
			Shift();
			if (TokenType == ATT_BCLOSE2)
			{
				Shift();
				AddBCC(AB_ARRAY_APPEND);
				break;
			}

			// optimize map["foo"] to map.foo (because map.foo execution is less complicated)
			if (TokenType == ATT_STRING && GetNextToken(Idtf, &cInt, Discard, true) == ATT_BCLOSE2)
			{
				AddBCC(AB_MAPA_R, cInt);
				Shift();
				break;
			}

			SPos = SPos0;
			Shift();

			Parse_Expression();
			SetNoRef();
			Match(ATT_BCLOSE2);
			AddBCC(AB_ARRAYA_R);
			break;
		}
		case ATT_QMARK:
		{
			if (Fn->pOrgScript->Strict < C4AulScriptStrict::STRICT3)
				throw C4AulParseError(this, "unexpected '?'");

			SetNoRef();

			const auto here = a->GetCodePos();
			AddBCC(AB_JUMPNIL);

			Shift();
			if (TokenType == ATT_QMARK)
				UnexpectedToken("navigation operator (->, [], .)");

			if (!Parse_Expression3())
				UnexpectedToken("navigation operator (->, [], .)");
			SetNoRef();

			while (Parse_Expression3()) SetNoRef();

			// safe navigation mustn't return a reference because it doesn't make any sense if it is expected to return non-ref nil occasionally
			AddBCC(AB_DEREF);
			SetJumpHere(here);
			break;
		}
		case ATT_DOT:
		{
			if (Fn->pOrgScript->Strict < C4AulScriptStrict::STRICT3)
				throw C4AulParseError(this, "unexpected '.'");
			Shift();
			if (TokenType == ATT_IDTF)
			{
				C4String *string;
				if (!(string = a->Engine->Strings.FindString(Idtf)))
					string = a->Engine->Strings.RegString(Idtf);
				if (Type == PARSER) string->Hold = true;
				AddBCC(AB_MAPA_R, reinterpret_cast<std::intptr_t>(string));
				Shift();
				break;
			}
			else
			{
				UnexpectedToken("identifier");
			}
			break;
		}
		case ATT_GLOBALCALL:
		case ATT_CALL:
		{
			C4AulBCCType eCallType = (TokenType == ATT_GLOBALCALL ? AB_CALLGLOBAL : AB_CALL);
			// Here, a '~' is not an operator, but a token
			Shift(Discard, false);
			// C4ID -> namespace given
			C4AulFunc *pFunc = nullptr;
			C4ID idNS = 0;
			if (TokenType == ATT_C4ID && eCallType != AB_CALLGLOBAL)
			{
				// from now on, stupid func names must stay outside ;P
				idNS = static_cast<C4ID>(cInt);
				Shift();
				// expect namespace-operator now
				Match(ATT_DCOLON);
				// next, we need a function name
				if (TokenType != ATT_IDTF) UnexpectedToken("function name");
				if (Type == PARSER)
				{
					// get def from id
					C4Def *pDef = C4Id2Def(idNS);
					if (!pDef)
					{
						throw C4AulParseError(this, "direct object call: def not found: ", C4IdText(idNS));
					}
					// search func
					if (!(pFunc = pDef->Script.GetSFunc(Idtf)))
					{
						throw C4AulParseError(this, FormatString("direct object call: function %s::%s not found", C4IdText(idNS), Idtf).getData());
					}

					if (pFunc->SFunc() && pFunc->SFunc()->Access < pDef->Script.GetAllowedAccess(pFunc, Fn->pOrgScript))
					{
						throw C4AulParseError(this, "insufficient access level", Idtf);
					}

					// write namespace chunk to byte code
					AddBCC(AB_CALLNS, static_cast<std::intptr_t>(idNS));
				}
			}
			else
			{
				auto failSafe = false;
				// may it be a failsafe call?
				if (TokenType == ATT_TILDE)
				{
					// store this and get the next token
					if (eCallType != AB_CALLGLOBAL)
						eCallType = AB_CALLFS;
					Shift();
					failSafe = true;
				}
				// expect identifier of called function now
				if (TokenType != ATT_IDTF) throw C4AulParseError(this, "expecting func name after '->'");
				// search a function with the given name
				if (eCallType == AB_CALLGLOBAL)
				{
					pFunc = a->Engine->GetFunc(Idtf, a->Engine, nullptr);
					// allocate space for return value, otherwise the call-target-variable is used, which is not present here
					AddBCC(AB_STACK, +1);
				}
				else
				{
					pFunc = a->Engine->GetFirstFunc(Idtf);
				}
				if (!pFunc)
				{
					// not failsafe?
					if (!failSafe && Type == PARSER)
					{
						throw C4AulParseError(this, FormatString("direct object call: function %s not found", Idtf).getData());
					}
					// otherwise: nothing to call - just execute parameters and discard them
					Shift();
					Parse_Params(0, nullptr);
					// remove target from stack, push a zero value as result
					AddBCC(AB_STACK, -1);
					AddBCC(AB_STACK, +1);
					// done
					break;
				}

				else if (pFunc->SFunc() && pFunc->SFunc()->Access < Fn->pOrgScript->GetAllowedAccess(pFunc, Fn->pOrgScript))
				{
					throw C4AulParseError(this, "insufficient access level", Idtf);
				}
			}
			// add call chunk
			Shift();
			Parse_Params(C4AUL_MAX_Par, pFunc ? pFunc->Name : nullptr, pFunc);
			if (idNS != 0)
				AddBCC(AB_CALLNS, static_cast<std::intptr_t>(idNS));
			AddBCC(eCallType, reinterpret_cast<std::intptr_t>(pFunc));
			break;
		}
		default:
			return false;
	}
	return true;
}

void C4AulParseState::Parse_Var()
{
	while (1)
	{
		// get desired variable name
		if (TokenType != ATT_IDTF)
			UnexpectedToken("variable name");
		if (Type == PREPARSER)
		{
			// insert variable
			Fn->VarNamed.AddName(Idtf);
		}
		// search variable (fail if not found)
		int iVarID = Fn->VarNamed.GetItemNr(Idtf);
		if (iVarID < 0)
			throw C4AulParseError(this, "internal error: var definition: var not found in variable table");
		Shift();
		if (TokenType == ATT_OPERATOR)
		{
			// only accept "="
			const auto iOpID = cInt;
			if (SEqual(C4ScriptOpMap[iOpID].Identifier, "="))
			{
				// insert initialization in byte code
				Shift();
				Parse_Expression();
				SetNoRef();
				AddBCC(AB_IVARN, iVarID);
			}
			else
				throw C4AulParseError(this, "unexpected operator");
		}
		switch (TokenType)
		{
		case ATT_COMMA:
		{
			Shift();
			break;
		}
		case ATT_SCOLON:
		{
			return;
		}
		default:
		{
			UnexpectedToken("',' or ';'");
		}
		}
	}
}

void C4AulParseState::Parse_Local()
{
	while (1)
	{
		if (Type == PREPARSER)
		{
			// get desired variable name
			if (TokenType != ATT_IDTF)
				UnexpectedToken("variable name");
			// check: symbol already in use?
			if (a->GetFunc(Idtf))
				throw C4AulParseError(this, "variable definition: name already in use");
			// insert variable
			a->LocalNamed.AddName(Idtf);
		}
		Match(ATT_IDTF);
		switch (TokenType)
		{
		case ATT_COMMA:
		{
			Shift();
			break;
		}
		case ATT_SCOLON:
		{
			return;
		}
		default:
		{
			UnexpectedToken("',' or ';'");
		}
		}
	}
}

void C4AulParseState::Parse_Static()
{
	while (1)
	{
		if (Type == PREPARSER)
		{
			// get desired variable name
			if (TokenType != ATT_IDTF)
				UnexpectedToken("variable name");
			// global variable definition
			// check: symbol already in use?
			if (a->Engine->GetFuncRecursive(Idtf)) throw C4AulParseError(this, "variable definition: name already in use");
			if (a->Engine->GetGlobalConstant(Idtf, nullptr)) Strict2Error("constant and variable with name ", Idtf);
			// insert variable if not defined already
			if (a->Engine->GlobalNamedNames.GetItemNr(Idtf) == -1)
				a->Engine->GlobalNamedNames.AddName(Idtf);
		}
		Match(ATT_IDTF);
		switch (TokenType)
		{
		case ATT_COMMA:
		{
			Shift();
			break;
		}
		case ATT_SCOLON:
		{
			return;
		}
		default:
		{
			UnexpectedToken("',' or ';'");
		}
		}
	}
}

void C4AulParseState::Parse_Const()
{
	// get global constant definition(s)
	while (1)
	{
		char Name[C4AUL_MAX_Identifier] = ""; // current identifier
		// get desired variable name
		if (TokenType != ATT_IDTF)
			UnexpectedToken("constant name");
		SCopy(Idtf, Name);
		// check func lists - functions of same name are allowed for backwards compatibility
		// (e.g., for overloading constants such as OCF_Living() in chaos scenarios)
		// it is not encouraged though, so better warn
		if (a->Engine->GetFuncRecursive(Idtf))
			Strict2Error("definition of constant hidden by function ", Idtf);
		if (a->Engine->GlobalNamedNames.GetItemNr(Idtf) != -1)
			Strict2Error("constant and variable with name ", Idtf);
		Match(ATT_IDTF);
		// expect '='
		if (TokenType != ATT_OPERATOR || !SEqual(C4ScriptOpMap[cInt].Identifier, "="))
			UnexpectedToken("'='");
		// expect value. Theoretically, something like C4AulScript::ExecOperator could be used here
		// this would allow for definitions like "static const OCF_Edible = 1 << 23"
		// However, such stuff should better be generalized, so the preparser (and parser)
		// can evaluate any constant expression, including functions with constant retval (e.g. Sqrt)
		// So allow only direct constants for now.
		// Do not set a string constant to "Hold" (which would delete it in the next UnLink())
		Shift(Ref, false);
		if (Type == PREPARSER)
		{
			C4Value vGlobalValue;
			switch (TokenType)
			{
			case ATT_NIL: vGlobalValue.Set0(); break;
			case ATT_INT: vGlobalValue.SetInt(static_cast<C4ValueInt>(cInt)); break;
			case ATT_BOOL: vGlobalValue.SetBool(!!cInt); break;
			case ATT_STRING: vGlobalValue.SetString(reinterpret_cast<C4String *>(cInt)); break; // increases ref count of C4String in cInt to 1
			case ATT_C4ID: vGlobalValue.SetC4ID(static_cast<C4ID>(cInt)); break;
			case ATT_IDTF:
				// identifier is only OK if it's another constant
				if (!a->Engine->GetGlobalConstant(Idtf, &vGlobalValue))
					UnexpectedToken("constant value");
				break;
			default:
				UnexpectedToken("constant value");
			}
			// register as constant
			a->Engine->RegisterGlobalConstant(Name, vGlobalValue);
		}
		// expect ',' (next global) or ';' (end of definition) now
		Shift();
		switch (TokenType)
		{
		case ATT_COMMA:
		{
			Shift();
			break;
		}
		case ATT_SCOLON:
		{
			return;
		}
		default:
		{
			UnexpectedToken("',' or ';'");
		}
		}
	}
}

template<C4AulTokenType closingAtt>
void C4AulParseState::SkipBlock()
{
	for (;;)
	{
		switch (GetNextToken(Idtf, &cInt, Discard, true))
		{
			case closingAtt: case ATT_EOF:
				return;
			case ATT_BOPEN:
				SkipBlock<ATT_BCLOSE>();
				break;
			case ATT_BOPEN2:
				SkipBlock<ATT_BCLOSE2>();
				break;
			case ATT_BLOPEN:
				SkipBlock<ATT_BLCLOSE>();
				break;
			default:
				continue;
		}
	}
}

bool C4AulParseState::IsMapLiteral()
{
	bool result = false;
	const char *SPos0 = SPos;
	for (;;)
	{
		switch (GetNextToken(Idtf, &cInt, Discard, true))
		{
			case ATT_BOPEN:
				SkipBlock<ATT_BCLOSE>();
				break;
			case ATT_BOPEN2:
				SkipBlock<ATT_BCLOSE2>();
				break;
			case ATT_BLOPEN:
				SkipBlock<ATT_BLCLOSE>();
				break;

			case ATT_OPERATOR:
				if (SEqual(C4ScriptOpMap[cInt].Identifier, "=")) result = true;
				break;

				// in uncertain cases, {} is considered as an empty block
			case ATT_BLCLOSE:
				goto loopEnd;
			case ATT_SCOLON:
				result = false;
				goto loopEnd;
			case ATT_EOF:
				goto loopEnd;
			default:
				// just continue
				continue;
		}
	}

loopEnd:
	SPos = SPos0;
	return result;
}

bool C4AulScript::Parse()
{
	if (DEBUG_BYTECODE_DUMP)
	{
		C4ScriptHost *scripthost{nullptr};
		if (Def) scripthost = &Def->Script;
		if (scripthost) LogSilentF("parsing %s...\n", scripthost->GetFilePath());
		else LogSilentF("parsing unknown...\n");
	}
	// parse children
	C4AulScript *s = Child0;
	while (s) { s->Parse(); s = s->Next; }
	// check state
	if (State != ASS_LINKED) return false;
	// don't parse global funcs again, as they're parsed already through links
	if (this == Engine) return false;
	// delete existing code
	delete[] Code;
	CodeSize = CodeBufSize = 0;
	// reset code and script pos
	CPos = Code;

	// parse script funcs
	C4AulFunc *f;
	for (f = Func0; f; f = f->Next)
	{
		// check whether it's a script func, or linked to one
		C4AulScriptFunc *Fn;
		if (!(Fn = f->SFunc()))
		{
			if (f->LinkedTo) Fn = f->LinkedTo->SFunc();
			// do only parse global funcs, because otherwise, the #append-links get parsed (->code overflow)
			if (Fn) if (Fn->Owner != Engine) Fn = nullptr;
		}
		if (Fn)
		{
			// parse function
			try
			{
				ParseFn(Fn);
			}
			catch (const C4AulError &err)
			{
				// do not show errors for System.c4g scripts that appear to be pure #appendto scripts
				if (Fn->Owner->Def || Fn->Owner->Appends.empty())
				{
					// show
					err.show();
					// show a warning if the error is in a remote script
					if (Fn->pOrgScript != this)
						DebugLogF("  (as #appendto/#include to %s)", Fn->Owner->ScriptName.getData());
					// and count (visible only ;) )
					++Game.ScriptEngine.errCnt;
				}
				// make all jumps that don't have their destination yet jump here
				// std::intptr_t to make it work on 64bit
				for (std::intptr_t i = reinterpret_cast<std::intptr_t>(Fn->Code); i < CPos - Code; i++)
				{
					C4AulBCC *pBCC = Code + i;
					if (IsJumpType(pBCC->bccType))
						if (!pBCC->bccX)
							pBCC->bccX = CPos - Code - i;
				}
				// add an error chunk
				AddBCC(AB_ERR);
			}

			// add separator
			AddBCC(AB_EOFN);
		}
	}

	// add eof chunk
	AddBCC(AB_EOF);

	// calc absolute code addresses for script funcs
	for (f = Func0; f; f = f->Next)
	{
		C4AulScriptFunc *Fn;
		if (!(Fn = f->SFunc()))
		{
			if (f->LinkedTo) Fn = f->LinkedTo->SFunc();
			if (Fn) if (Fn->Owner != Engine) Fn = nullptr;
		}
		if (Fn)
			Fn->Code = Code + reinterpret_cast<std::intptr_t>(Fn->Code);
	}

	// save line count
	Engine->lineCnt += SGetLine(Script.getData(), Script.getPtr(Script.getLength()));

	// dump bytecode
	if (DEBUG_BYTECODE_DUMP)
		for (f = Func0; f; f = f->Next)
		{
			C4AulScriptFunc *Fn;
			if (!(Fn = f->SFunc()))
			{
				if (f->LinkedTo) Fn = f->LinkedTo->SFunc();
				if (Fn) if (Fn->Owner != Engine) Fn = nullptr;
			}
			if (Fn)
			{
				LogSilentF("%s:", Fn->Name);
				for (C4AulBCC *pBCC = Fn->Code;; pBCC++)
				{
					C4AulBCCType eType = pBCC->bccType;
					const auto X = pBCC->bccX;
					switch (eType)
					{
					case AB_FUNC: case AB_CALL: case AB_CALLFS: case AB_CALLGLOBAL:
						LogSilentF("%s\t'%s'\n", GetTTName(eType), X ? (reinterpret_cast<C4AulFunc *>(X))->Name : ""); break;
					case AB_STRING:
						LogSilentF("%s\t'%s'\n", GetTTName(eType), X ? (reinterpret_cast<C4String *>(X))->Data.getData() : ""); break;
					default:
						LogSilentF("%s\t%" PRIdPTR "\n", GetTTName(eType), X); break;
					}
					if (eType == AB_EOFN) break;
				}
			}
		}

	// finished
	State = ASS_PARSED;

	return true;
}

void C4AulScript::ParseDescs()
{
	// parse children
	C4AulScript *s = Child0;
	while (s) { s->ParseDescs(); s = s->Next; }
	// check state
	if (State < ASS_LINKED) return;
	// parse descs of all script funcs
	for (C4AulFunc *f = Func0; f; f = f->Next)
		if (C4AulScriptFunc *Fn = f->SFunc()) Fn->ParseDesc();
}

C4AulScript *C4AulScript::FindFirstNonStrictScript()
{
	// self is not #strict?
	if (Script && Strict == C4AulScriptStrict::NONSTRICT) return this;
	// search children
	C4AulScript *pNonStrScr;
	for (C4AulScript *pScr = Child0; pScr; pScr = pScr->Next)
		if (pNonStrScr = pScr->FindFirstNonStrictScript())
			return pNonStrScr;
	// nothing found
	return nullptr;
}

#undef DEBUG_BYTECODE_DUMP
