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

#include "C4Aul.h"
#include "C4Def.h"
#include "C4Game.h"
#include "C4Log.h"
#include "C4ScriptHost.h"

#define DEBUG_BYTECODE_DUMP 0

class C4AulBCCGeneratorError : public C4AulError
{
public:
	C4AulBCCGeneratorError(C4AulBCCGenerator &generator, const char *message, const char *identifier = nullptr)
	{
		this->message = message;
	}
};

void C4AulBCCGenerator::AddBCC(const std::intptr_t offset, C4AulBCCType type, std::intptr_t bccX)
{
	switch (type)
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
		++stack;
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
		--stack;
		break;

	case AB_FUNC:
		stack -= reinterpret_cast<C4AulFunc *>(bccX)->GetParCount() - 1;
		break;

	case AB_CALL:
	case AB_CALLFS:
	case AB_CALLGLOBAL:
		stack -= C4AUL_MAX_Par;
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
		stack += bccX;
		break;

	case AB_MAP:
		stack -= 2 * bccX - 1;
		break;

	case AB_ARRAY:
		stack -= bccX - 1;
		break;

	default:
	case AB_NilCoalescing:
		assert(false);
	}

	// Use stack operation instead of 0-Int (enable optimization)
	if ((type == AB_INT || type == AB_BOOL) && !bccX && Script.Strict < C4AulScriptStrict::STRICT3)
	{
		type = AB_STACK;
		bccX = 1;
	}

	// Join checks only if it's not a jump target
	if (!jump)
	{
		// Join together stack operations
		if (type == AB_STACK &&
			!code.empty() &&
			code.back().bccType == AB_STACK
			&& (bccX <= 0 || code.back().bccX >= 0))
		{
			if ((code.back().bccX += bccX) == 0)
			{
				code.pop_back();
			}
			return;
		}
	}

	// Add
	code.emplace_back(type, bccX, Script.Script.getData() + offset);

	// Reset jump flag
	jump = false;
}

const std::vector<C4AulBCC> &C4AulBCCGenerator::Generate(const std::unique_ptr<C4AulAST::Statement> &statement)
{
	code.clear();

	try
	{
		statement->Accept(*this);
	}
	catch (Error) // Generation hit an error node; abort further bytecode emission
	{
	}

	return code;
}

std::size_t C4AulBCCGenerator::JumpHere()
{
	// Set flag so the next generated code chunk won't get joined
	jump = true;
	return GetCodePos();
}

void C4AulBCCGenerator::SetJumpHere(const std::size_t jumpOp)
{
	// Set target
	C4AulBCC *pBCC = GetCodeByPos(jumpOp);
	assert(C4Aul::IsJumpType(pBCC->bccType));
	pBCC->bccX = GetCodePos() - jumpOp;
	// Set flag so the next generated code chunk won't get joined
	jump = true;
}

void C4AulBCCGenerator::SetJump(std::size_t jumpOp, std::size_t where)
{
	// Set target
	C4AulBCC *pBCC = GetCodeByPos(jumpOp);
	assert(C4Aul::IsJumpType(pBCC->bccType));
	pBCC->bccX = where - jumpOp;
}

void C4AulBCCGenerator::AddJump(std::intptr_t offset, C4AulBCCType eType, size_t iWhere)
{
	AddBCC(offset, eType, iWhere - GetCodePos());
}

void C4AulBCCGenerator::PushLoop()
{
	loopStack.push({.StackSize = stack});
}

void C4AulBCCGenerator::PopLoop()
{
	loopStack.pop();
}

void C4AulBCCGenerator::AddLoopControl(const std::size_t position, const bool fBreak)
{
	if (loopStack.top().StackSize != stack)
	{
		AddBCC(position, AB_STACK, loopStack.top().StackSize - stack);
	}

	loopStack.top().Controls.emplace_back(fBreak, GetCodePos());

	AddBCC(position, AB_JUMP);
}

void C4AulBCCGenerator::AddError(const std::size_t position)
{
	// make all jumps that don't have their destination yet jump here
	for (std::size_t i{0}; i < code.size(); ++i)
	{
		if (C4Aul::IsJumpType(code[i].bccType) && !code[i].bccX)
		{
			code[i].bccX = code.size() - i;
		}
	}

	AddBCC(position, AB_ERR);
	throw Error{};
}

void C4AulBCCGenerator::Visit(C4AulAST::Nil &nil)
{
	if (nil.PreferStack)
	{
		AddBCC(nil.GetPosition(), AB_STACK, 1);
	}
	else
	{
		AddBCC(nil.GetPosition(), AB_NIL);
	}
}

void C4AulBCCGenerator::Visit(C4AulAST::IntLiteral &intLiteral)
{
	AddBCC(intLiteral.GetPosition(), AB_INT, intLiteral.Value);
}

void C4AulBCCGenerator::Visit(C4AulAST::C4IDLiteral &idLiteral)
{
	AddBCC(idLiteral.GetPosition(), AB_C4ID, idLiteral.Value);
}

void C4AulBCCGenerator::Visit(C4AulAST::BoolLiteral &boolLiteral)
{
	AddBCC(boolLiteral.GetPosition(), AB_BOOL, boolLiteral.Value);
}

void C4AulBCCGenerator::Visit(C4AulAST::StringLiteral &stringLiteral)
{
	C4StringTable &stringTable{Script.GetEngine()->Strings};
	C4String *string{stringTable.FindString(stringLiteral.Value.c_str())};
	if (!string)
	{
		string = stringTable.RegString(stringLiteral.Value.c_str());
	}

	string->Hold = true;
	AddBCC(stringLiteral.GetPosition(), AB_STRING, reinterpret_cast<std::intptr_t>(string));
}

void C4AulBCCGenerator::Visit(C4AulAST::ArrayLiteral &arrayLiteral)
{
	Visitor::Visit(arrayLiteral);

	AddBCC(arrayLiteral.GetPosition(), AB_ARRAY, arrayLiteral.Expressions.size());
}

void C4AulBCCGenerator::Visit(C4AulAST::MapLiteral &mapLiteral)
{
	Visitor::Visit(mapLiteral);

	AddBCC(mapLiteral.GetPosition(), AB_MAP, mapLiteral.Expressions.size());
}

void C4AulBCCGenerator::Visit(C4AulAST::ParN &parN)
{
	AddBCC(parN.GetPosition(), parN.IsNoRef() ? AB_PARN_V : AB_PARN_R, parN.N);
}

void C4AulBCCGenerator::Visit(C4AulAST::Par &par)
{
	Visitor::Visit(par);

	AddBCC(par.GetPosition(), par.IsNoRef() ? AB_PAR_V : AB_PAR_R);
}

void C4AulBCCGenerator::Visit(C4AulAST::VarN &varN)
{
	AddBCC(varN.GetPosition(), varN.IsNoRef() ? AB_VARN_V : AB_VARN_R, Function->VarNamed.GetItemNr(varN.Identifier.c_str()));
}

void C4AulBCCGenerator::Visit(C4AulAST::Var &var)
{
	Visitor::Visit(var);

	AddBCC(var.GetPosition(), var.IsNoRef() ? AB_VAR_V : AB_VAR_R);
}

void C4AulBCCGenerator::Visit(C4AulAST::LocalN &localN)
{
	AddBCC(localN.GetPosition(), localN.IsNoRef() ? AB_LOCALN_V : AB_LOCALN_R, Script.LocalNamed.GetItemNr(localN.Identifier.c_str()));
}

void C4AulBCCGenerator::Visit(C4AulAST::GlobalN &globalN)
{    
	AddBCC(globalN.GetPosition(), globalN.IsNoRef() ? AB_GLOBALN_V : AB_GLOBALN_R, Script.GetEngine()->GlobalNamedNames.GetItemNr(globalN.Identifier.c_str()));
}

void C4AulBCCGenerator::Visit(C4AulAST::Declaration &declaration)
{
	if (declaration.Type == C4AulAST::Declaration::DeclarationType::Var && declaration.Value)
	{
		declaration.Value->Accept(*this);
		AddBCC(declaration.GetPosition(), AB_IVARN, Function->VarNamed.GetItemNr(declaration.Identifier.c_str()));
	}
}

void C4AulBCCGenerator::Visit(C4AulAST::UnaryOperator &unaryOperator)
{
	Visitor::Visit(unaryOperator);

	AddBCC(unaryOperator.GetPosition(), C4ScriptOpMap[unaryOperator.OpID].Code, unaryOperator.OpID);
}

void C4AulBCCGenerator::Visit(C4AulAST::BinaryOperator &binaryOperator)
{
	binaryOperator.LHS->Accept(*this);

	C4AulBCCType opCode{C4ScriptOpMap[binaryOperator.OpID].Code};
	const std::size_t jumpCondition{GetCodePos()};
	bool jump{false};

	if (opCode == AB_NilCoalescingIt)
	{
		AddBCC(binaryOperator.GetPosition(), AB_NilCoalescingIt);
		jump = true;
	}
	else if (((opCode == AB_And || opCode == AB_Or) && Function->pOrgScript->Strict >= C4AulScriptStrict::STRICT2) || opCode == AB_NilCoalescing)
	{
		// Jump or discard first parameter
		AddBCC(binaryOperator.GetPosition(), opCode == AB_And ? AB_JUMPAND : opCode == AB_Or ? AB_JUMPOR : AB_JUMPNOTNIL);
		jump = true;
	}
	else if (opCode == AB_Equal && Function->pOrgScript->Strict < C4AulScriptStrict::STRICT2)
	{
		opCode = AB_EqualIdent;
	}
	else if (opCode == AB_NotEqual && Function->pOrgScript->Strict < C4AulScriptStrict::STRICT2)
	{
		opCode = AB_NotEqualIdent;
	}

	binaryOperator.RHS->Accept(*this);

	if (jump)
	{
		if (opCode == AB_NilCoalescingIt)
		{
			AddBCC(binaryOperator.GetPosition(), AB_Set, binaryOperator.OpID);
		}

		SetJumpHere(jumpCondition);
	}
	else
	{
		// The operator BCC has already been generated above
		AddBCC(binaryOperator.GetPosition(), opCode, binaryOperator.OpID);
	}
}

void C4AulBCCGenerator::Visit(C4AulAST::Block &block)
{
	for (const auto &statement : block.Statements)
	{
		AcceptStatement(statement);
	}
}

void C4AulBCCGenerator::Visit(C4AulAST::Return &returnNode)
{
	if (returnNode.Expressions.empty())
	{
		if (returnNode.PreferStack)
		{
			AddBCC(returnNode.GetPosition(), AB_STACK, 1);
		}
		else
		{
			AddBCC(returnNode.GetPosition(), AB_NIL);
		}
	}
	else
	{
		Visitor::Visit(returnNode);

		if (returnNode.Expressions.size() > 1)
		{
			AddBCC(returnNode.GetPosition(), AB_STACK, -(returnNode.Expressions.size() - 1));
		}
	}

	AddBCC(returnNode.GetPosition(), AB_RETURN);
}

void C4AulBCCGenerator::Visit(C4AulAST::ReturnAsParam &returnAsParam)
{
	if (returnAsParam.Expr)
	{
		returnAsParam.Expr->Accept(*this);
	}
	else
	{
		if (returnAsParam.PreferStack)
		{
			AddBCC(returnAsParam.GetPosition(), AB_STACK, 1);
		}
		else
		{
			AddBCC(returnAsParam.GetPosition(), AB_NIL);
		}
	}

	AddBCC(returnAsParam.GetPosition(), AB_RETURN);
}

void C4AulBCCGenerator::Visit(C4AulAST::FunctionCall &functionCall)
{
	C4AulFunc *foundFn;
	if (Function->pOrgScript->Strict == C4AulScriptStrict::NONSTRICT && Function->OwnerOverloaded && functionCall.Identifier == Function->Name)
	{
		// old syntax: do not allow recursive calls in overloaded functions
		foundFn = Function->OwnerOverloaded;
	}
	else if (Function->Owner == &Game.ScriptEngine)
	{
		foundFn = Script.GetOwner()->GetFuncRecursive(functionCall.Identifier.c_str());
	}
	else
	{
		foundFn = Script.GetFuncRecursive(functionCall.Identifier.c_str());
	}

	EmitFunctionCall(functionCall.GetPosition(), foundFn, functionCall.Arguments);
}

void C4AulBCCGenerator::Visit(C4AulAST::ArrayAccess &arrayAccess)
{
	Visitor::Visit(arrayAccess);

	AddBCC(arrayAccess.GetPosition(), arrayAccess.IsNoRef() ? AB_ARRAYA_V : AB_ARRAYA_R);
}

void C4AulBCCGenerator::Visit(C4AulAST::ArrayAppend &arrayAppend)
{
	Visitor::Visit(arrayAppend);

	AddBCC(arrayAppend.GetPosition(), AB_ARRAY_APPEND);
}

void C4AulBCCGenerator::Visit(C4AulAST::PropertyAccess &propertyAccess)
{
	C4String *const string{Script.GetEngine()->Strings.FindString(propertyAccess.Property.c_str())};
	assert(string && string->Hold);

	Visitor::Visit(propertyAccess);
	AddBCC(propertyAccess.GetPosition(), propertyAccess.IsNoRef() ? AB_MAPA_V : AB_MAPA_R, reinterpret_cast<std::intptr_t>(string));
}

void C4AulBCCGenerator::Visit(C4AulAST::IndirectCall &indirectCall)
{
	if (indirectCall.Callee)
	{
		indirectCall.Callee->Accept(*this);
	}
	else
	{
		// allocate space for return value, otherwise the call-target-variable is used, which is not present here
		AddBCC(indirectCall.GetPosition(), AB_STACK, 1);
	}

	for (const auto &argument : indirectCall.Arguments)
	{
		argument->Accept(*this);
	}

	C4AulFunc *func;
	C4AulBCCType callBcc;

	if (indirectCall.GlobalCall)
	{
		func = Script.GetEngine()->GetFunc(indirectCall.Identifier.c_str(), Script.GetEngine(), nullptr);
		callBcc = AB_CALLGLOBAL;
	}
	else
	{
		if (indirectCall.NamespaceId)
		{
			C4Def *const def{C4Id2Def(indirectCall.NamespaceId)};
			func = def->Script.GetSFunc(indirectCall.Identifier.c_str());
		}
		else
		{
			func = Script.GetEngine()->GetFirstFunc(indirectCall.Identifier.c_str());
		}

		callBcc = indirectCall.FailSafe ? AB_CALLFS : AB_CALL;
	}

	if (!func)
	{
		if (indirectCall.FailSafe)
		{
			// nothing to call - just execute parameters and discard them
			AddBCC(indirectCall.GetPosition(), AB_STACK, -(indirectCall.Arguments.size() + 1));
			AddBCC(indirectCall.GetPosition(), AB_STACK, 1);
			return;
		}
		else
		{
			throw std::runtime_error{"No func"}; // TODO
		}
	}

	if (indirectCall.Arguments.size() != /*func->GetParCount()*/ C4AUL_MAX_Par)
	{
		AddBCC(indirectCall.GetPosition(), AB_STACK, /*func->GetParCount()*/ C4AUL_MAX_Par - indirectCall.Arguments.size());
	}

	if (indirectCall.NamespaceId)
	{
		AddBCC(indirectCall.GetPosition(), AB_CALLNS, indirectCall.NamespaceId);
	}

	AddBCC(indirectCall.GetPosition(), callBcc, reinterpret_cast<std::intptr_t>(func));
}

void C4AulBCCGenerator::Visit(C4AulAST::If &ifNode)
{
	ifNode.Condition->Accept(*this);

	const auto cond = GetCodePos();
	AddBCC(ifNode.GetPosition(), AB_CONDN);

	if (ifNode.Then)
	{
		AcceptStatement(ifNode.Then);
	}

	if (ifNode.Other)
	{
		const auto jump = GetCodePos();
		AddBCC(ifNode.GetPosition(), AB_JUMP);
		// set condition jump target
		SetJumpHere(cond);

		const auto codePos = GetCodePos();

		AcceptStatement(ifNode.Other);

		if (codePos == GetCodePos())
		{
			// No else block? Avoid a redundant AB_JUMP 1
			RemoveBCC();
			--GetCodeByPos(cond)->bccX;
		}
		else
		{
			// set jump target
			SetJumpHere(jump);
		}
	}
	else
	{
		SetJumpHere(cond);
	}
}

void C4AulBCCGenerator::Visit(C4AulAST::While &whileNode)
{
	const auto start = JumpHere();
	whileNode.Condition->Accept(*this);

	const auto cond = GetCodePos();
	AddBCC(whileNode.GetPosition(), AB_CONDN);

	PushLoop();

	if (whileNode.Body)
	{
		AcceptStatement(whileNode.Body);
	}

	AddJump(whileNode.GetPosition(), AB_JUMP, start);
	SetJumpHere(cond);

	for (const auto &control : GetLoopControls())
	{
		if (control.Break)
		{
			SetJumpHere(control.Pos);
		}
		else
		{
			SetJump(control.Pos, start);
		}
	}

	PopLoop();
}

void C4AulBCCGenerator::Visit(C4AulAST::For &forNode)
{
	if (forNode.Init)
	{
		AcceptStatement(forNode.Init);
	}

	std::size_t jumpCondition{SizeMax};
	std::size_t jumpBody{SizeMax};
	std::size_t jumpOut{SizeMax};

	if (forNode.Condition)
	{
		jumpCondition = JumpHere();
		forNode.Condition->Accept(*this);
		jumpOut = GetCodePos();
		AddBCC(forNode.GetPosition(), AB_CONDN);
	}

	std::size_t jumpIncrementor{SizeMax};

	if (forNode.After)
	{
		// Must jump over incrementor
		jumpBody = GetCodePos();
		AddBCC(forNode.GetPosition(), AB_JUMP);

		jumpIncrementor = JumpHere();
		AcceptStatement(forNode.After);

		if (jumpCondition != SizeMax)
		{
			AddJump(forNode.GetPosition(), AB_JUMP, jumpCondition);
		}
	}

	PushLoop();

	std::size_t bodyPos{JumpHere()};
	if (jumpBody != SizeMax)
	{
		SetJumpHere(jumpBody);
	}

	if (forNode.Body)
	{
		AcceptStatement(forNode.Body);
	}

	std::size_t jumpBack;
	if (jumpIncrementor != SizeMax)
	{
		jumpBack = jumpIncrementor;
	}
	else if (jumpCondition != SizeMax)
	{
		jumpBack = jumpCondition;
	}
	else
	{
		jumpBack = bodyPos;
	}

	AddJump(forNode.GetPosition(), AB_JUMP, jumpBack);

	// Set target for condition
	if (jumpOut != SizeMax)
	{
		SetJumpHere(jumpOut);
	}

	for (const auto &control : GetLoopControls())
	{
		if (control.Break)
		{
			SetJumpHere(control.Pos);
		}
		else
		{
			SetJump(control.Pos, jumpBack);
		}
	}

	PopLoop();
}

void C4AulBCCGenerator::Visit(C4AulAST::ForEach &forEach)
{
	AcceptStatement(forEach.Init);
	forEach.Iterable->Accept(*this);

	bool forMap{false};
	std::int32_t varId;
	std::int32_t varIdForMapValue;

	if (auto declarations = dynamic_cast<C4AulAST::Declarations *>(forEach.Init.get()))
	{
		forMap = true;
		std::tie(varId, varIdForMapValue) = *(declarations->GetIdentifiers()
			| std::views::transform([&](const std::string_view identifier) { return Function->VarNamed.GetItemNr(identifier.data()); })
			| std::views::pairwise | std::views::take(1)).begin();

		// push second var id for the map iteration
		if (forMap) AddBCC(forEach.GetPosition(), AB_INT, varIdForMapValue);
	}
	else
	{
		auto *const declaration = static_cast<C4AulAST::Declaration *>(forEach.Init.get());
		varId = Function->VarNamed.GetItemNr(declaration->GetIdentifier().data());
	}
	// push initial forEach.GetPosition() (0)
	AddBCC(forEach.GetPosition(), AB_INT);

	const auto start = GetCodePos();
	AddBCC(forEach.GetPosition(), forMap ? AB_FOREACH_MAP_NEXT : AB_FOREACH_NEXT, varId);

	// jump out (FOREACH[_MAP]_NEXT will jump over this if
	// we're not at the end of the array yet)
	const std::size_t jumpCondition{GetCodePos()};
	AddBCC(forEach.GetPosition(), AB_JUMP);

	PushLoop();

	if (forEach.Body)
	{
		AcceptStatement(forEach.Body);
	}

	AddJump(forEach.GetPosition(), AB_JUMP, start);
	SetJumpHere(jumpCondition);

	for (const auto &control : GetLoopControls())
	{
		if (control.Break)
		{
			SetJumpHere(control.Pos);
		}
		else
		{
			SetJump(control.Pos, start);
		}
	}

	PopLoop();

	// remove array/map and counter/iterator from stack
	AddBCC(forEach.GetPosition(), AB_STACK, forMap ? -3 : -2);
}

void C4AulBCCGenerator::Visit(C4AulAST::Break &breakNode)
{
	AddLoopControl(breakNode.GetPosition(), true);
}

void C4AulBCCGenerator::Visit(C4AulAST::Continue &continueNode)
{
	AddLoopControl(continueNode.GetPosition(), false);
}

void C4AulBCCGenerator::Visit(C4AulAST::Inherited &inherited)
{
	if (inherited.Found)
	{
		EmitFunctionCall(inherited.GetPosition(), Function->OwnerOverloaded, inherited.Arguments);
	}
	else
	{
		for (const auto &argument : inherited.Arguments)
		{
			argument->Accept(*this);
		}

		AddBCC(inherited.GetPosition(), AB_STACK, -inherited.Arguments.size());
		AddBCC(inherited.GetPosition(), AB_STACK, 1);
	}
}

void C4AulBCCGenerator::Visit(C4AulAST::This &thisNode)
{
	std::abort();
}

void C4AulBCCGenerator::Visit(C4AulAST::Error &error)
{
	Visitor::Visit(error);

	AddError(error.GetPosition());
}

template<std::same_as<C4AulAST::Statement> T>
void C4AulBCCGenerator::AcceptStatement(const std::unique_ptr<T> &node)
{
	if (!node) return;

	else if (auto expression = dynamic_cast<C4AulAST::Expression *>(node.get()))
	{
		// Variable is an expression stored in a std::unique_ptr<Statement>? Pop the value.
		expression->Accept(*this);
		AddBCC(node->GetPosition(), AB_STACK, -1);
	}
	else
	{
		node->Accept(*this);
	}
}

void C4AulBCCGenerator::EmitFunctionCall(const std::intptr_t position, C4AulFunc *const func, const std::vector<std::unique_ptr<C4AulAST::Expression>> &arguments)
{
	for (const auto &argument : arguments)
	{
		argument->Accept(*this);
	}

	if (func->GetParCount() != arguments.size())
	{
		AddBCC(position, AB_STACK, func->GetParCount() - arguments.size());
	}

	AddBCC(position, AB_FUNC, reinterpret_cast<std::intptr_t>(func));
}
