/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
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

#include <C4Include.h>
#include <C4Value.h>
#include <C4Aul.h>
#include <C4StringTable.h>
#include <C4ValueList.h>
#include <C4ValueHash.h>

#include <cinttypes>
#include <functional>
#include <format>
#include <string_view>

#include <C4Game.h>
#include <C4Object.h>
#include <C4Log.h>

const C4Value C4VNull{};
const C4Value C4VTrue{C4VBool(true)};
const C4Value C4VFalse{C4VBool(false)};

C4Value::~C4Value()
{
	// resolve all C4Values referencing this Value
	while (FirstRef)
		FirstRef->Set(*this);

	// delete contents
	DelDataRef(Data, Type, GetNextRef(), GetBaseContainer());
}

std::optional<StdStrBuf> C4Value::toString() const
{
	const C4Value &val = GetRefVal();
	switch (val.Type)
	{
	case C4V_String:
		return {val._getStr()->Data};

	case C4V_Bool:
	case C4V_Int:
		return {StdStrBuf{std::format("{}", val._getInt()).c_str()}};

	case C4V_C4ID:
		return {StdStrBuf(C4IdText(val._getC4ID()))};

	default:
		return {};
	}
}

C4Value &C4Value::operator=(const C4Value &nValue)
{
	// set referenced value
	if (Type == C4V_pC4Value)
		GetRefVal().operator=(nValue);
	else
		Set(nValue.GetRefVal());

	return *this;
}

void C4Value::AddDataRef()
{
	switch (Type)
	{
	case C4V_pC4Value: Data.Ref->AddRef(this); break;
	case C4V_Any: if (Data) { GuessType(); } break;
	case C4V_Array: case C4V_Map: Data.Container = Data.Container->IncRef(); break;
	case C4V_String: Data.Str->IncRef(); break;
	case C4V_C4Object:
		Data.Obj->AddRef(this);
#ifndef NDEBUG
		// check if the object actually exists
		if (!Game.ObjectNumber(Data.Obj))
		{
			LogNTr(spdlog::level::warn, "using wild object ptr {}!", static_cast<void *>(Data.Obj));
		}
		else if (!Data.Obj->Status)
		{
			LogNTr(spdlog::level::warn, "using ptr on deleted object {} ({})!", static_cast<void *>(Data.Obj), Data.Obj->GetName());
		}
#endif
		break;
	default: break;
	}
}

void C4Value::DelDataRef(C4V_Data Data, C4V_Type Type, C4Value *pNextRef, C4ValueContainer *pBaseContainer)
{
	// clean up
	switch (Type)
	{
	case C4V_pC4Value:
		// Save because AddDataRef does not set this flag
		HasBaseContainer = false;
		Data.Ref->DelRef(this, pNextRef, pBaseContainer);
		break;
	case C4V_C4Object: Data.Obj->DelRef(this, pNextRef); break;
	case C4V_Array: case C4V_Map: Data.Container->DecRef(); break;
	case C4V_String: Data.Str->DecRef(); break;
	default: break;
	}
}

void C4Value::Set(C4V_Data nData, C4V_Type nType)
{
	// Do not add this to the same linked list twice.
	if (Data == nData && Type == nType) return;

	C4V_Data oData = Data;
	C4V_Type oType = Type;
	C4Value *oNextRef = NextRef;
	auto *oBaseContainer = BaseContainer;
	auto oHasBaseContainer = HasBaseContainer;

	// change
	Data = nData;
	Type = (nData || nType == C4V_Int || nType == C4V_Bool) ? nType : C4V_Any;

	// hold
	AddDataRef();

	// clean up
	DelDataRef(oData, oType, oHasBaseContainer ? nullptr : oNextRef, oHasBaseContainer ? oBaseContainer : nullptr);

	CheckRemoveFromMap();
}

void C4Value::Set0()
{
	C4V_Data oData = Data;
	C4V_Type oType = Type;

	// change
	Data.Raw = 0;
	Type = C4V_Any;

	// clean up (save even if Data was 0 before)
	DelDataRef(oData, oType, HasBaseContainer ? nullptr : NextRef, HasBaseContainer ? BaseContainer : nullptr);

	CheckRemoveFromMap();
}

void C4Value::CheckRemoveFromMap()
{
	if (Type == C4V_Any && Data.Raw == 0 && OwningMap)
	{
		OwningMap->removeValue(this);
	}
}

void C4Value::Move(C4Value *nValue)
{
	nValue->Set(*this);

	// change references
	for (C4Value *pVal = FirstRef; pVal; pVal = pVal->GetNextRef())
		pVal->Data.Ref = nValue;

	// copy ref list
	assert(!nValue->FirstRef);
	nValue->FirstRef = FirstRef;

	// delete usself
	FirstRef = nullptr;
	Set0();
}

void C4Value::GetContainerElement(C4Value *index, C4Value &target, C4AulContext *pctx, bool noref)
{
	try
	{
		C4Value &Ref = GetRefVal();
		if (Ref.Type == C4V_C4Object)
		{
			if (index->ConvertTo(C4V_String) && index->_getStr())
			{
				auto var = Ref.Data.Obj->LocalNamed.GetItem(index->_getStr()->Data.getData());
				if (var) target.SetRef(var);
				else target.Set0();
			}
			else
				throw C4AulExecError(pctx->Obj, "indexed access on object: only string keys are allowed");
			return;
		}
		// No array (and no nullpointer because Data==0 => Type==any)
		if (Ref.Type != C4V_Array && Ref.Type != C4V_Map)
			throw C4AulExecError(pctx->Obj, "indexed access: array or map expected");
		if (noref)
		{
			// Get the item, but do not resize the array - it might be used more than once
			if (Ref.Data.Container->hasIndex(*index))
				target.Set((*Ref.Data.Container)[*index]);
			else
				target.Set0();
		}
		else
		{
			index->Deref();
			// Is target the first ref?
			if (!Ref.Data.Container->hasIndex(*index) || !(*Ref.Data.Container)[*index].FirstRef)
			{
				Ref.Data.Container = Ref.Data.Container->IncElementRef();
				target.SetRef(&(*Ref.Data.Container)[*index]);
				if (target.Type == C4V_pC4Value)
				{
					assert(!target.NextRef);
					target.BaseContainer = Ref.Data.Container;
					target.HasBaseContainer = true;
				}
				// else target apparently owned the last reference to the array
			}
			else
			{
				target.SetRef(&(*Ref.Data.Container)[*index]);
			}
		}
	}
	catch (const std::runtime_error &e)
	{
		throw C4AulExecError(pctx->Obj, e.what());
	}
}

void C4Value::SetArrayLength(int32_t size, C4AulContext *cthr)
{
	C4Value &Ref = GetRefVal();
	// No array
	if (Ref.Type != C4V_Array)
		throw C4AulExecError(cthr->Obj, "SetLength: array expected");
	Ref.Data.Array = Ref.Data.Array->SetLength(size);
}

const C4Value &C4Value::GetRefVal() const
{
	const C4Value *pVal = this;
	while (pVal->Type == C4V_pC4Value)
		pVal = pVal->Data.Ref;
	return *pVal;
}

C4Value &C4Value::GetRefVal()
{
	C4Value *pVal = this;
	while (pVal->Type == C4V_pC4Value)
		pVal = pVal->Data.Ref;
	return *pVal;
}

void C4Value::AddRef(C4Value *pRef)
{
	pRef->NextRef = FirstRef;
	FirstRef = pRef;
}

void C4Value::DelRef(const C4Value *pRef, C4Value *pNextRef, C4ValueContainer *pBaseContainer)
{
	if (pRef == FirstRef)
		FirstRef = pNextRef;
	else
	{
		C4Value *pVal = FirstRef;
		while (pVal->NextRef != pRef)
		{
			// assert that pRef really was in the list
			assert(pVal->NextRef && !pVal->HasBaseContainer);
			pVal = pVal->NextRef;
		}
		pVal->NextRef = pNextRef;
		if (pBaseContainer)
		{
			pVal->HasBaseContainer = true;
			pVal->BaseContainer = pBaseContainer;
		}
	}
	// Was pRef the last ref to an array element?
	if (pBaseContainer && !FirstRef)
	{
		pBaseContainer->DecElementRef();
	}
}

C4V_Type C4Value::GuessType()
{
	// guaranteed by the caller
	assert(Data);

	if (Type != C4V_Any) return Type;

	// C4ID?
	if (LooksLikeID(Data.ID) && Data.ID >= 10000)
		return Type = C4V_C4ID;

	// object?
	if (Game.ObjectNumber(Data.Obj))
	{
		Type = C4V_C4Object;
		// With the type now known, the destructor will clean up the reference
		// which only works if the reference is added first
		AddDataRef();
		return Type;
	}

	// string?
	if (Game.ScriptEngine.Strings.FindString(Data.Str))
	{
		Type = C4V_String;
		// see above
		AddDataRef();
		return Type;
	}

	// must be int now
	return Type = C4V_Int;
}

void C4Value::HintType(C4V_Type type)
{
	auto &ref = GetRefVal();
	if (ref.Data.Int != 0 || type == C4V_Bool || type == C4V_Int || type == C4V_C4ID)
		ref.Type = type;
}

const char *GetC4VName(const C4V_Type Type)
{
	switch (Type)
	{
	case C4V_Any:
		return "any";
	case C4V_Int:
		return "int";
	case C4V_Bool:
		return "bool";
	case C4V_C4Object:
		return "object";
	case C4V_C4ID:
		return "id";
	case C4V_String:
		return "string";
	case C4V_Array:
		return "array";
	case C4V_Map:
		return "map";
	case C4V_pC4Value:
		return "&";
	case C4V_C4ObjectEnum:
		; // fallthrough
	}
	return "!Fehler!";
}

char GetC4VID(const C4V_Type Type)
{
	switch (Type)
	{
	case C4V_Any:
		return 'A';
	case C4V_Int:
		return 'i';
	case C4V_Bool:
		return 'b';
	case C4V_C4Object:
		return 'o';
	case C4V_C4ID:
		return 'I';
	case C4V_String:
		return 'S';
	case C4V_pC4Value:
		return 'V'; // should never happen
	case C4V_C4ObjectEnum:
		return 'O';
	case C4V_Array:
		return 'a';
	case C4V_Map:
		return 'm';
	}
	return ' ';
}

C4V_Type GetC4VFromID(const char C4VID)
{
	switch (C4VID)
	{
	case 'A':
		return C4V_Any;
	case 'i':
		return C4V_Int;
	case 'b':
		return C4V_Bool;
	case 'o':
		return C4V_C4Object;
	case 'I':
		return C4V_C4ID;
	case 'S':
		return C4V_String;
	case 'V':
		return C4V_pC4Value;
	case 'O':
		return C4V_C4ObjectEnum;
	case 'a':
		return C4V_Array;
	case 'm':
		return C4V_Map;
	}
	return C4V_Any;
}

const char *C4Value::GetTypeInfo()
{
	return GetC4VName(GetType());
}

// converter functions

static bool FnCnvDirectOld(C4Value *Val, C4V_Type toType, bool fStrict)
{
	// new syntax: failure
	if (fStrict) return false;
	// old syntax: do nothing
	return true;
}

static bool FnCnvError(C4Value *Val, C4V_Type toType, bool fStrict)
{
	// deny convert
	return false;
}

static bool FnCnvDeref(C4Value *Val, C4V_Type toType, bool fStrict)
{
	// resolve reference of Value
	Val->Deref();
	// retry to check convert
	return Val->ConvertTo(toType, fStrict);
}

bool C4Value::FnCnvGuess(C4Value *Val, C4V_Type toType, bool fStrict)
{
	if (Val->Data)
	{
		// guess type (always possible because data is not 0)
		Val->GuessType();
		// try to convert new type
		return Val->ConvertTo(toType, fStrict);
	}
	else
	{
		// nil is every possible type except a reference at the same time
		return true;
	}
}

bool C4Value::FnCnvInt2Id(C4Value *Val, C4V_Type toType, bool fStrict)
{
	// inside range?
	const auto i = Val->Data.Int;
	if (!Inside<decltype(i)>(i, 0, 9999)) return false;
	// convert
	Val->Type = C4V_C4ID;
	Val->Data.ID = static_cast<C4ID>(i);
	return true;
}

// Type conversion table
#define CnvOK        nullptr, false // allow conversion by same value
#define CnvError     FnCnvError, true
#define CnvGuess     C4Value::FnCnvGuess, false
#define CnvInt2Id    C4Value::FnCnvInt2Id, false
#define CnvDirectOld FnCnvDirectOld, true
#define CnvDeref     FnCnvDeref, false

C4VCnvFn C4Value::C4ScriptCnvMap[C4V_Last + 1][C4V_Last + 1] =
{
	{
		// C4V_Any - always try guess
		{ CnvOK },    // any      same
		{ CnvGuess }, // int
		{ CnvGuess }, // Bool
		{ CnvGuess }, // C4ID
		{ CnvGuess }, // C4Object
		{ CnvGuess }, // String
		{ CnvGuess }, // Array
		{ CnvGuess }, // Map
		{ CnvError }, // pC4Value
	},
	{
		// C4V_Int
		{ CnvOK },     // any
		{ CnvOK },     // int      same
		{ CnvOK },     // Bool
		{ CnvInt2Id }, // C4ID     numerical ID?
		{ CnvError },  // C4Object NEVER!
		{ CnvError },  // String   NEVER!
		{ CnvError },  // Array    NEVER!
		{ CnvError },  // Map      NEVER!
		{ CnvError },  // pC4Value
	},
	{
		// C4V_Bool
		{ CnvOK },        // any
		{ CnvOK },        // int      might be used
		{ CnvOK },        // Bool     same
		{ CnvDirectOld }, // C4ID     #strict forbid
		{ CnvError },     // C4Object NEVER!
		{ CnvError },     // String   NEVER!
		{ CnvError },     // Array    NEVER!
		{ CnvError },     // Map      NEVER!
		{ CnvError },     // pC4Value
	},
	{
		// C4V_C4ID
		{ CnvOK },        // any
		{ CnvDirectOld }, // int      #strict forbid
		{ CnvOK },        // Bool
		{ CnvOK },        // C4ID     same
		{ CnvError },     // C4Object NEVER!
		{ CnvError },     // String   NEVER!
		{ CnvError },     // Array    NEVER!
		{ CnvError },     // Map      NEVER!
		{ CnvError },     // pC4Value
	},
	{
		// C4V_Object
		{ CnvOK },        // any
		{ CnvDirectOld }, // int      #strict forbid
		{ CnvOK },        // Bool
		{ CnvError },     // C4ID     Senseless, thus error
		{ CnvOK },        // C4Object same
		{ CnvError },     // String   NEVER!
		{ CnvError },     // Array    NEVER!
		{ CnvError },     // Map      NEVER!
		{ CnvError },     // pC4Value
	},
	{
		// C4V_String
		{ CnvOK },        // any
		{ CnvDirectOld }, // int      #strict forbid
		{ CnvOK },        // Bool
		{ CnvError },     // C4ID     Sensless, thus error
		{ CnvError },     // C4Object NEVER!
		{ CnvOK },        // String   same
		{ CnvError },     // Array    NEVER!
		{ CnvError },     // Map      NEVER!
		{ CnvError },     // pC4Value
	},
	{
		// C4V_Array
		{ CnvOK },    // any
		{ CnvError }, // int      NEVER!
		{ CnvOK },    // Bool
		{ CnvError }, // C4ID     NEVER!
		{ CnvError }, // C4Object NEVER!
		{ CnvError }, // String   NEVER!
		{ CnvOK },    // Array    same
		{ CnvError }, // Map      NEVER!
		{ CnvError }, // pC4Value NEVER!
	},
	{
		// C4V_Map
		{ CnvOK },    // any
		{ CnvError }, // int      NEVER!
		{ CnvOK },    // Bool
		{ CnvError }, // C4ID     NEVER!
		{ CnvError }, // C4Object NEVER!
		{ CnvError }, // String   NEVER!
		{ CnvError }, // Array    NEVER!
		{ CnvOK },    // Map      same
		{ CnvError }, // pC4Value NEVER!
	},
	{
		// C4V_pC4Value - resolve reference and retry type check
		{ CnvDeref }, // any
		{ CnvDeref }, // int
		{ CnvDeref }, // Bool
		{ CnvDeref }, // C4ID
		{ CnvDeref }, // C4Object
		{ CnvDeref }, // String
		{ CnvDeref }, // Array
		{ CnvDeref }, // Map
		{ CnvOK },    // pC4Value same
	},
};

#undef CnvOK
#undef CvnError
#undef CnvGuess
#undef CnvInt2Id
#undef CnvDirectOld
#undef CnvDeref

// Humanreadable debug output
std::string C4Value::GetDataString() const
{
	if (Type == C4V_pC4Value)
		return GetRefVal().GetDataString() + "*";

	// ouput by type info
	switch (GetType())
	{
	case C4V_Any:
		return "nil";
	case C4V_Int:
		return std::to_string(Data.Int);
	case C4V_Bool:
		return Data ? "true" : "false";
	case C4V_C4ID:
		return C4IdText(Data.ID);
	case C4V_C4Object:
	{
		// obj exists?
		if (!Game.ObjectNumber(Data.Obj) && !Game.FindFirstInAllObjects([obj{Data.Obj}](C4GameObjects &objects) { return objects.InactiveObjects.ObjectNumber(obj); }))
			return std::to_string(Data.Raw);
		else if (Data.Obj)
			if (Data.Obj->Status == C4OS_NORMAL)
				return std::format("{} #{}", Data.Obj->GetName(), static_cast<int>(Data.Obj->Number));
			else
				return std::format("{{{} #{}}}", Data.Obj->GetName(), static_cast<int>(Data.Obj->Number));
		else
			return "0"; // (impossible)
	}
	case C4V_String:
		return (Data.Str && Data.Str->Data.getData()) ? std::format("\"{}\"", Data.Str->Data.getData()) : "(nullstring)";
	case C4V_Array:
	{
		std::string dataString{"["};
		for (int32_t i = 0; i < Data.Array->GetSize(); i++)
		{
			if (i) dataString += ", ";
			dataString += Data.Array->GetItem(i).GetDataString();
		}
		dataString += ']';
		return dataString;
	}
	case C4V_Map:
	{
		if (Data.Map->size() == 0) return "{}";
		std::string dataString{"{ "};
		bool first = true;
		for (auto [key, value] : *Data.Map)
		{
			if (!first) dataString += ", ";
			else first = false;

			dataString += std::format("{} = {}", key.GetDataString(), value.GetDataString());
		}
		dataString += " }";
		return dataString;
	}
	default:
		return "-unknown type- ";
	}
}

C4Value C4VString(const char *strString)
{
	// safety
	if (!strString) return C4Value();
	return C4Value(new C4String(strString, &Game.ScriptEngine.Strings));
}

C4Value C4VString(StdStrBuf &&Str)
{
	// safety
	if (Str.isNull()) return C4Value();
	return C4Value(new C4String(std::forward<StdStrBuf>(Str), &Game.ScriptEngine.Strings));
}

void C4Value::DenumeratePointer(C4Section *const section)
{
	// array?
	if (Type == C4V_Array || Type == C4V_Map)
	{
		Data.Container->DenumeratePointers(section);
		return;
	}
	// object types only
	if (Type != C4V_C4ObjectEnum && Type != C4V_Any) return;
	// in range?
	if (Type != C4V_C4ObjectEnum && !Inside<std::intptr_t>(Data.Raw, C4EnumPointer1, C4EnumPointer2)) return;
	// get obj id, search object
	const auto iObjID = (Data.Int >= C4EnumPointer1 ? Data.Int - C4EnumPointer1 : Data.Int);
	C4Object *pObj = section ? section->Objects.ObjectPointer(iObjID) : Game.ObjectPointer(iObjID);
	if (!pObj)
		pObj = section ? section->Objects.InactiveObjects.ObjectPointer(iObjID) : Game.FindFirstInAllObjects([iObjID](C4GameObjects &objects) { return objects.InactiveObjects.ObjectPointer(iObjID); });
	if (pObj)
		// set
		SetObject(pObj);
	else
	{
		// any: guess type
		if (Type == C4V_Any)
		{
			if (Data) GuessType();
		}
		// object: invalid value - set to zero
		else
			Set0();
	}
}

void C4Value::CompileFunc(StdCompiler *pComp)
{
	// Type
	bool fCompiler = pComp->isCompiler();
	if (!fCompiler)
	{
		// Get type
		if (Type == C4V_Any && Data) GuessType();
		char cC4VID = GetC4VID(Type);
		// Object reference is saved enumerated
		if (Type == C4V_C4Object)
			cC4VID = GetC4VID(C4V_C4ObjectEnum);
		// Write
		pComp->Character(cC4VID);
	}
	else
	{
		// Clear
		Set0();
		// Read type
		char cC4VID;
		try
		{
			pComp->Character(cC4VID);
		}
		catch (const StdCompiler::NotFoundException &)
		{
			cC4VID = 'A';
		}
		Type = GetC4VFromID(cC4VID);
	}
	// Data
	int32_t iTmp;
	switch (Type)
	{
	// simple data types: just save
	case C4V_Any:
	case C4V_Int:
	case C4V_Bool:
		// these are 32-bit integers
		iTmp = Data.Int;
		pComp->Value(iTmp);
		Data.Int = iTmp;
		return;

	case C4V_C4ID:
		iTmp = static_cast<int32_t>(Data.ID);
		pComp->Value(iTmp);
		Data.ID = static_cast<C4ID>(iTmp);
		return;

	// object: save object number instead
	case C4V_C4Object:
		if (!fCompiler)
			iTmp = Game.ObjectNumber(getObj());
	case C4V_C4ObjectEnum:
		if (!fCompiler) if (Type == C4V_C4ObjectEnum)
			iTmp = Data.Int;
		pComp->Value(iTmp);
		if (fCompiler)
		{
			Type = C4V_C4ObjectEnum;
			Data.Int = iTmp; // must be denumerated later
		}
		return;

	// string: save string number
	case C4V_String:
		if (!fCompiler) iTmp = getStr()->iEnumID;
		pComp->Value(iTmp);
		// search
		if (fCompiler)
		{
			C4String *pString = Game.ScriptEngine.Strings.FindString(iTmp);
			if (pString)
			{
				Data.Str = pString;
				pString->IncRef();
			}
			else
				Type = C4V_Any;
		}
		return;

	case C4V_Array:
		pComp->Separator(StdCompiler::SEP_START2);
		pComp->Value(mkPtrAdapt(Data.Array, false));
		if (fCompiler) Data.Container = Data.Array->IncRef();
		pComp->Separator(StdCompiler::SEP_END2);
		return;

	case C4V_Map:
		pComp->Separator(StdCompiler::SEP_START2);
		pComp->Value(mkPtrAdapt(Data.Map, false));
		if (fCompiler) Data.Container = Data.Map->IncRef();
		pComp->Separator(StdCompiler::SEP_END2);
		return;

	// shouldn't happen
	case C4V_pC4Value:
		; // fallthrough
	}

	assert(false);
}

bool C4Value::Equals(const C4Value &other, C4AulScriptStrict strict) const
{
	switch (strict)
	{
		case C4AulScriptStrict::NONSTRICT: case C4AulScriptStrict::STRICT1:
			return _getRaw() == other._getRaw();
		case C4AulScriptStrict::STRICT2:
			return *this == other;
		case C4AulScriptStrict::STRICT3:
			if (Type != other.Type) return false;
			switch (Type)
			{
				case C4V_Any:
					return true;
				// object enum is needed for deserialization of C4ValueHash with objects as keys!
				case C4V_C4ObjectEnum:
				case C4V_Int:
					return Data.Int == other.Data.Int;
				case C4V_C4Object:
					return Data.Obj == other.Data.Obj;
				case C4V_C4ID:
					return Data.ID == other.Data.ID;
				case C4V_Bool:
					return _getBool() == other._getBool();
				case C4V_String:
					return Data.Str->Data == other.Data.Str->Data;
				case C4V_Array:
					return *Data.Array == *other.Data.Array;
				case C4V_Map:
					return *Data.Map == *other.Data.Map;
				case C4V_pC4Value:
					assert(!"Comparison between variables of types which should never be compared!");
					// fallthrough
			}
			return false;
	}
	return false;
}

bool C4Value::operator==(const C4Value &Value2) const
{
	switch (Type)
	{
	case C4V_Any:
		assert(!Data);
		return Data == Value2.Data;
	case C4V_Int:
		switch (Value2.Type)
		{
		case C4V_Any:
			assert(!Value2.Data);
			return Data == Value2.Data;
		case C4V_Int:
		case C4V_Bool:
		case C4V_C4ID:
			return Data == Value2.Data;
		default:
			return false;
		}
	case C4V_Bool:
		switch (Value2.Type)
		{
		case C4V_Any:
			assert(!Value2.Data);
			return Data == Value2.Data;
		case C4V_Int:
		case C4V_Bool:
			return Data == Value2.Data;
		default:
			return false;
		}
	case C4V_C4ID:
		switch (Value2.Type)
		{
		case C4V_Any:
			assert(!Value2.Data);
			[[fallthrough]];
		case C4V_Int:
		case C4V_C4ID:
			return Data == Value2.Data;
		default:
			return false;
		}
	case C4V_C4Object:
		return Data == Value2.Data && Type == Value2.Type;
	case C4V_String:
		return Type == Value2.Type && Data.Str->Data == Value2.Data.Str->Data;
	case C4V_Array:
		return Type == Value2.Type && *(Data.Array) == *(Value2.Data.Array);
		break;
	case C4V_Map:
		return Type == Value2.Type && *(Data.Map) == *(Value2.Data.Map);
	default:
		// C4AulExec should have dereferenced both values, no need to implement comparison here
		return Data == Value2.Data;
	}
	return GetData() == Value2.GetData();
}

namespace
{
	// based on boost container_hash's hashCombine
	constexpr void hashCombine(std::size_t &hash, std::size_t nextHash)
	{
		if constexpr (sizeof(std::size_t) == 4)
		{
#define rotateLeft32(x, r) (x << r) | (x >> (32 - r))
			constexpr std::size_t c1 = 0xcc9e2d51;
			constexpr std::size_t c2 = 0x1b873593;

			nextHash *= c1;
			nextHash = rotateLeft32(nextHash, 15);
			nextHash *= c2;

			hash ^= nextHash;
			hash = rotateLeft32(hash, 13);
			hash = hash * 5 + 0xe6546b64;
#undef rotateLeft32
		}
		else if constexpr (sizeof(std::size_t) == 8)
		{
			constexpr std::size_t m = 0xc6a4a7935bd1e995;
			constexpr int r = 47;

			nextHash *= m;
			nextHash ^= nextHash >> r;
			nextHash *= m;

			hash ^= nextHash;
			hash *= m;

			// Completely arbitrary number, to prevent 0's
			// from hashing to 0.
			hash += 0xe6546b64;
		}
		else
		{
			hash ^= nextHash + 0x9e3779b9 + (hash << 6) + (hash >> 2);
		}
	}
}

std::size_t std::hash<C4Value>::operator()(C4Value value) const
{
	C4Value &ref = value.GetRefVal();
	std::size_t hash = std::hash<C4V_Type>{}(ref.GetType());

	if (ref.GetType() == C4V_C4ObjectEnum)
	{
		hash = std::hash<C4V_Type>{}(C4V_C4Object);
		hashCombine(hash, std::hash<C4ValueInt>{}(ref._getInt()));
		return hash;
	}

	switch (ref.GetType())
	{
		case C4V_Any:
			break;

		case C4V_Int: case C4V_C4ID:
			hashCombine(hash, std::hash<C4ValueInt>{}(ref._getInt()));
			break;

		case C4V_Bool:
			hashCombine(hash, std::hash<bool>{}(ref._getBool()));
			break;

		case C4V_C4Object:
			hashCombine(hash, std::hash<int32_t>{}(ref._getObj()->Number));
			break;

		case C4V_String:
		{
			const auto &str = ref._getStr()->Data;
			hashCombine(hash, std::hash<std::string_view>{}({str.getData(), str.getLength()}));
			break;
		}
		case C4V_Array:
		{
			const auto &array = *ref._getArray();
			for (size_t i = 0; i < array.GetSize(); ++i)
			{
				hashCombine(hash, (*this)(array.GetItem(i)));
			}
			break;
		}
		case C4V_Map:
		{
			std::size_t contentHash = 0;
			auto &map = *ref._getMap();
			for (const auto &it : map)
			{
				std::size_t itemHash = (*this)(it.first);
				hashCombine(itemHash, (*this)(it.second));

				// order mustn't matter
				contentHash ^= itemHash;
			}
			hashCombine(hash, contentHash);
			break;
		}
		default:
			throw std::runtime_error("Invalid value type for hashing C4Value");
	}

	return hash;
}
