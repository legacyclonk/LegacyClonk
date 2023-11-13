/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2008, Sven2
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

// Menus attached to objects; script created or internal

#include <C4Include.h>
#include <C4ObjectMenu.h>

#include <C4Object.h>
#include <C4ObjectCom.h>
#include <C4Wrappers.h>
#include <C4Player.h>
#include <C4Viewport.h>

#include <cassert>
#include <format>

// C4ObjectMenu

C4ObjectMenu::C4ObjectMenu() : C4Menu()
{
	Default();
}

C4ObjectMenu::~C4ObjectMenu()
{
	if (ClearObjectPtr)
	{
		*ClearObjectPtr = nullptr;
	}

	if (IsDeleted)
	{
		*IsDeleted = true;
	}
}

void C4ObjectMenu::Default()
{
	C4Menu::Default();
	eCallbackType = CB_None;
	Object = ParentObject = RefillObject = nullptr;
	RefillObjectContentsCount = 0;
	UserMenu = false;
	CloseQuerying = false;
}

bool C4ObjectMenu::IsCloseDenied()
{
	// abort if menu is permanented by script; stop endless recursive calls if user opens a new menu by CloseQuerying-flag
	if (UserMenu && !CloseQuerying)
	{
		CloseQuerying = true;
		bool fResult = false;
		C4AulParSet pars(C4VInt(Selection), C4VObj(ParentObject));
		if (eCallbackType == CB_Object)
		{
			if (Object) fResult = static_cast<bool>(Object->Call(PSF_MenuQueryCancel, pars));
		}
		else if (eCallbackType == CB_Scenario)
			fResult = static_cast<bool>(Game.Script.Call(PSF_MenuQueryCancel, pars));
		CloseQuerying = false;
		if (fResult) return true;
	}
	// close OK
	return false;
}

void C4ObjectMenu::LocalInit(C4Object *pObject, bool fUserMenu)
{
	Object = pObject;
	UserMenu = fUserMenu;
	ParentObject = GetParentObject();
	if (pObject) eCallbackType = CB_Object; else eCallbackType = CB_Scenario;
}

bool C4ObjectMenu::Init(C4FacetExSurface &fctSymbol, const char *szEmpty, C4Object *pObject, int32_t iExtra, int32_t iExtraData, int32_t iId, int32_t iStyle, bool fUserMenu)
{
	if (!DoInit(fctSymbol, szEmpty, iExtra, iExtraData, iId, iStyle)) return false;
	LocalInit(pObject, fUserMenu);
	return true;
}

void C4ObjectMenu::OnSelectionChanged(int32_t iNewSelection)
{
	// do selection callback
	if (UserMenu)
	{
		C4AulParSet pars(C4VInt(iNewSelection), C4VObj(ParentObject));
		if (eCallbackType == CB_Object && Object)
			Object->Call(PSF_MenuSelection, pars);
		else if (eCallbackType == CB_Scenario)
			Game.Script.Call(PSF_MenuSelection, pars);
	}
}

void C4ObjectMenu::ClearPointers(C4Object *pObj)
{
	if (Object == pObj) { Object = nullptr; }
	if (ParentObject == pObj) ParentObject = nullptr; // Reason for menu close anyway.
	if (RefillObject == pObj) RefillObject = nullptr;
	if (ClearObjectPtr && *ClearObjectPtr == pObj) *ClearObjectPtr = nullptr;
	C4Menu::ClearPointers(pObj);
}

C4Object *C4ObjectMenu::GetParentObject()
{
	auto allObjects = Game.GetAllObjects();
	if (const auto it = std::ranges::find(allObjects, this, &C4Object::Menu); it != std::ranges::end(allObjects))
	{
		return *it;
	}
	return nullptr;
}

void C4ObjectMenu::SetRefillObject(C4Object *pObj)
{
	RefillObject = pObj;
	NeedRefill = true;
	Refill();
}

bool C4ObjectMenu::DoRefillInternal(bool &rfRefilled)
{
	const auto symbolSize = GetSymbolSize();

	// Variables
	C4FacetExSurface fctSymbol;
	C4Object *pObj;
	std::string caption;
	std::string command;
	std::string command2;
	int32_t cnt, iCount;
	C4Def *pDef;
	C4Player *pPlayer;
	C4Object *pTarget;
	C4Facet fctTarget;

	const auto selectedItem = GetSelectedItem();
	const auto checkIDSelection = [selectedID = selectedItem ? selectedItem->GetC4ID() : C4ID_None, this]
	{
		if (selectedID == C4ID_None) return;
		if (const auto newSelectedItem = GetSelectedItem(); newSelectedItem && newSelectedItem->GetC4ID() == selectedID)
		{
			return;
		}

		for (std::int32_t i = 0; i < GetItemCount(); ++i)
		{
			const auto item = GetItem(i);
			if (item && item->GetC4ID() == selectedID)
			{
				Selection = i;
				return;
			}
		}
	};

	// Refill
	switch (Identification)
	{
	case C4MN_Activate:
		// Clear items
		ClearItems();
		// Refill target
		if (!(pTarget = RefillObject)) return false;
		{
			// Add target contents items
			C4ObjectListIterator iter(pTarget->Contents);
			while (pObj = iter.GetNext(&iCount, C4D_Activate))
			{
				pDef = pObj->Def;
				if (pDef->NoGet) continue;
				// Prefer fully constructed objects
				if (~pObj->OCF & OCF_FullCon)
				{
					// easy way: only if first concat check matches
					// this doesn't catch all possibilities, but that will rarely matter
					C4Object *pObj2 = pTarget->Contents.Find(pDef->id, ANY_OWNER, OCF_FullCon);
					if (pObj2) if (pObj2->CanConcatPictureWith(pObj)) pObj = pObj2;
				}
				// Caption
				caption = LoadResStr(C4ResStrTableKey::IDS_MENU_ACTIVATE, pObj->GetName());
				// Picture
				fctSymbol.Set(fctSymbol.Surface, 0, 0, symbolSize, symbolSize);
				pObj->Picture2Facet(fctSymbol);
				// Commands
				command = std::format("SetCommand(this,\"Activate\",Object({}))&&ExecuteCommand()", pObj->Number);
				command2 = std::format("SetCommand(this,\"Activate\", ,{},0,Object({}),{})&&ExecuteCommand()", pTarget->Contents.ObjectCount(pDef->id), pTarget->Number, C4IdText(pDef->id));
				// Add menu item
				Add(caption.c_str(), fctSymbol, command.c_str(), iCount, pObj, pDef->GetDesc(), pDef->id, command2.c_str(), true, pObj->GetValue(pTarget, NO_OWNER));
				// facet taken over (arrg!)
				fctSymbol.Default();
			}
			checkIDSelection();
		}
		break;

	case C4MN_Buy:
	{
		// Clear items
		ClearItems();
		// Refill target
		if (!(pTarget = RefillObject)) return false;
		// Base buying disabled? Fail.
		if (~pTarget->Section->C4S.Game.Realism.BaseFunctionality & BASEFUNC_Buy) return false;
		// Add base owner's homebase material
		if (!(pPlayer = Game.Players.Get(pTarget->Base))) return false;
		C4Player *pBuyPlayer = Object ? Game.Players.Get(Object->Owner) : nullptr;
		C4ID idDef;
		for (cnt = 0; idDef = pPlayer->HomeBaseMaterial.GetID(Game.Defs, C4D_All, cnt, &iCount); cnt++)
		{
			pDef = Game.Defs.ID2Def(idDef);
			if (!pDef) continue; // skip invalid defs
			// Caption
			caption = LoadResStr(C4ResStrTableKey::IDS_MENU_BUY, pDef->GetName());
			// Picture
			pDef->Picture2Facet(fctSymbol, pBuyPlayer ? pBuyPlayer->ColorDw : 0);
			// Command
			command = std::format("AppendCommand(this,\"Buy\",Object({}),{},0,,0,{})&&ExecuteCommand()", pTarget->Number, 1, C4IdText(pDef->id));
			command2 = std::format("AppendCommand(this,\"Buy\",Object({}),{},0,,0,{})&&ExecuteCommand()", pTarget->Number, iCount, C4IdText(pDef->id));
			// Buying value
			int32_t iBuyValue = pDef->GetValue(pTarget, pPlayer->Number);
			// Add menu item
			Add(caption.c_str(), fctSymbol, command.c_str(), iCount, nullptr, pDef->GetDesc(), pDef->id, command2.c_str(), true, iBuyValue);
		}
		break;
	}

	case C4MN_Sell:
		// Clear items
		ClearItems();
		// Refill target
		if (!(pTarget = RefillObject)) return false;
		// Base sale disabled? Fail.
		if (~pTarget->Section->C4S.Game.Realism.BaseFunctionality & BASEFUNC_Sell) return false;
		{
			// Add target contents items
			C4ObjectListIterator iter(pTarget->Contents);
			while (pObj = iter.GetNext(&iCount, C4D_Sell))
			{
				pDef = pObj->Def;
				if (pDef->NoSell) continue;
				// Prefer fully constructed objects
				if (~pObj->OCF & OCF_FullCon)
				{
					// easy way: only if first concat check matches
					// this doesn't catch all possibilities, but that will rarely matter
					C4Object *pObj2 = pTarget->Contents.Find(pDef->id, ANY_OWNER, OCF_FullCon);
					if (pObj2) if (pObj2->CanConcatPictureWith(pObj)) pObj = pObj2;
				}
				// Caption
				caption = LoadResStr(C4ResStrTableKey::IDS_MENU_SELL, pObj->GetName());
				// Picture
				fctSymbol.Set(fctSymbol.Surface, 0, 0, symbolSize, symbolSize);
				pObj->Picture2Facet(fctSymbol);
				// Commands
				command = std::format("AppendCommand(this,\"Sell\",Object({}),{},0,Object({}),0,{})&&ExecuteCommand()", pTarget->Number, 1, pObj->Number, C4IdText(pDef->id));
				command2 = std::format("AppendCommand(this,\"Sell\",Object({}),{},0,Object({}),0,{})&&ExecuteCommand()", pTarget->Number, pObj->Contents.ObjectCount(pDef->id), pObj->Number, C4IdText(pDef->id));
				// Selling value
				int32_t iSellValue = pObj->GetValue(pTarget, Object ? Object->Owner : NO_OWNER);
				// Add menu item
				Add(caption.c_str(), fctSymbol, command.c_str(), iCount, nullptr, pDef->GetDesc(), pDef->id, command2.c_str(), true, iSellValue);
				fctSymbol.Default();
			}
			checkIDSelection();
		}
		// Success
		break;

	case C4MN_Get:
	case C4MN_Contents:
		// Clear items
		ClearItems();
		// Refill target
		if (!(pTarget = RefillObject)) return false;
		{
			// Add target contents items
			C4ObjectListIterator iter(pTarget->Contents);
			while (pObj = iter.GetNext(&iCount, C4D_Get))
			{
				pDef = pObj->Def;
				if (pDef->NoGet) continue;
				// Prefer fully constructed objects
				if (~pObj->OCF & OCF_FullCon)
				{
					// easy way: only if first concat check matches
					// this doesn't catch all possibilities, but that will rarely matter
					C4Object *pObj2 = pTarget->Contents.Find(pDef->id, ANY_OWNER, OCF_FullCon);
					if (pObj2) if (pObj2->CanConcatPictureWith(pObj)) pObj = pObj2;
				}
				// Determine whether to get or activate
				bool fGet = true;
				if (!(pObj->OCF & OCF_Carryable)) fGet = false; // not a carryable item
				if (Identification == C4MN_Contents)
				{
					if (Object && Object->Def->CollectionLimit && (Object->Contents.ObjectCount() >= Object->Def->CollectionLimit)) fGet = false; // collection limit reached
					if (Object && Object->Call(PSF_RejectCollection, {C4VID(pObj->Def->id), C4VObj(pObj)})) fGet = false; // collection rejected
				}
				if (!(pTarget->OCF & OCF_Entrance)) fGet = true; // target object has no entrance: cannot activate - force get
				// Caption
				caption = LoadResStrChoice(fGet, C4ResStrTableKey::IDS_MENU_GET, C4ResStrTableKey::IDS_MENU_ACTIVATE, pObj->GetName());
				// Picture
				fctSymbol.Set(fctSymbol.Surface, 0, 0, symbolSize, symbolSize);
				pObj->Picture2Facet(fctSymbol);
				// Primary command: get/activate single object
				command = std::format("SetCommand(this, \"{}\", Object({})) && ExecuteCommand()", fGet ? "Get" : "Activate", pObj->Number);
				// Secondary command: get/activate all objects of the chosen type
				int32_t iAllCount;
				if ((iAllCount = pTarget->Contents.ObjectCount(pDef->id)) > 1)
					command2 = std::format("SetCommand(this, \"{}\", , {},0, Object({}), {}) && ExecuteCommand()", fGet ? "Get" : "Activate", iAllCount, pTarget->Number, C4IdText(pDef->id));
				// Add menu item (with object)
				Add(caption.c_str(), fctSymbol, command.c_str(), iCount, pObj, pDef->GetDesc(), pDef->id, command2.c_str());
				fctSymbol.Default();
			}
		}
		checkIDSelection();
		break;

	case C4MN_Context:
	{
		// Clear items
		ClearItems();
		if (!(pTarget = RefillObject)) return false;
		if (!Object) return false;

		// Put - if target is a container...
		if (pTarget->OCF & OCF_Container)
			// ...and we have something to put...
			if (Object->Contents.GetObject(0))
				// ...and if we are in that container
				if ((pTarget == Object->Contained)
					// ...or if we are grabbing the container and it has grab-put enabled
					|| ((Object->GetProcedure() == DFA_PUSH) && (Object->Action.Target == pTarget) && (pTarget->Def->GrabPutGet & C4D_Grab_Put)))
				{
					// Primary command: put first inventory item (all selected clonks)
					command = std::format("PlayerObjectCommand({}, \"Put\", Object({}), 0, 0) && ExecuteCommand()", Object->Owner, pTarget->Number);
					// Secondary command: put all inventory items (all selected clonks)
					if ((Object->Contents.ObjectCount() > 1) || (Game.Players.Get(Object->Owner)->GetSelectedCrewCount() > 1))
						command2 = std::format("PlayerObjectCommand({}, \"Put\", Object({}), 1000, 0) && ExecuteCommand()", Object->Owner, pTarget->Number); // Workaround: specifying a really high put count here; will be adjusted later by C4Menu::ObjectCommand...
					// Create symbol
					fctSymbol.Create(symbolSize, symbolSize);
					fctTarget = fctSymbol.GetFraction(85, 85, C4FCT_Right, C4FCT_Top);
					Object->Contents.GetObject(0)->DrawPicture(fctTarget);
					fctTarget = fctSymbol.GetFraction(85, 85, C4FCT_Left, C4FCT_Bottom);
					Game.GraphicsResource.fctHand.Draw(fctTarget, true, 0);
					// Add menu item
					Add(LoadResStr(C4ResStrTableKey::IDS_CON_PUT2), fctSymbol, command.c_str(), C4MN_Item_NoCount, nullptr, nullptr, C4ID_None, command2.c_str());
					// Preserve symbol
					fctSymbol.Default();
				}

		// Contents - if target is a container...
		if (pTarget->OCF & OCF_Container)
			// ...and if we are in that container
			if ((pTarget == Object->Contained)
				// ...or if we are grabbing the container and it has grab-get enabled
				|| ((Object->GetProcedure() == DFA_PUSH) && (Object->Action.Target == pTarget) && (pTarget->Def->GrabPutGet & C4D_Grab_Get))
				// ...or if the container is owned by us or a friendly player - this is for remote mouse-button-clicks
				|| (ValidPlr(pTarget->Owner) && !Hostile(pTarget->Owner, Object->Owner)))
			{
				command = std::format("SetCommand(this,\"Get\",Object({}),0,0,,2)&&ExecuteCommand()", pTarget->Number);
				fctSymbol.Create(symbolSize, symbolSize); pTarget->DrawPicture(fctSymbol);
				Add(LoadResStr(C4ResStrTableKey::IDS_CON_CONTENTS), fctSymbol, command.c_str());
				fctSymbol.Default();
			}

		// These ought to be moved into a flag/homebase script...
		// Homebase buy/sell (if friendly base)
		if (ValidPlr(pTarget->Base) && !Hostile(pTarget->Base, Object->Owner))
		{
			// Buy
			if (pTarget->Section->C4S.Game.Realism.BaseFunctionality & BASEFUNC_Buy)
			{
				command = std::format("SetCommand(this,\"Buy\",Object({}))&&ExecuteCommand()", pTarget->Number);
				fctSymbol.Create(symbolSize, symbolSize); DrawMenuSymbol(C4MN_Buy, fctSymbol, pTarget->Base, pTarget);
				Add(LoadResStr(C4ResStrTableKey::IDS_CON_BUY), fctSymbol, command.c_str());
				fctSymbol.Default();
			}
			// Sell
			if (pTarget->Section->C4S.Game.Realism.BaseFunctionality & BASEFUNC_Sell)
			{
				command = std::format("SetCommand(this,\"Sell\",Object({}))&&ExecuteCommand()", pTarget->Number);
				fctSymbol.Create(symbolSize, symbolSize); DrawMenuSymbol(C4MN_Sell, fctSymbol, pTarget->Base, pTarget);
				Add(LoadResStr(C4ResStrTableKey::IDS_CON_SELL), fctSymbol, command.c_str());
				fctSymbol.Default();
			}
		}

		// Scripted context functions
		AddContextFunctions(pTarget);

		// Show needed material (if construction site)
		if (pTarget->OCF & OCF_Construct && Object->r == 0 && (Game.Rules & C4RULE_ConstructionNeedsMaterial))
		{
			command = std::format("PlayerMessage(GetOwner(), Object({})->GetNeededMatStr(), Object({}))", pTarget->Number, pTarget->Number);
			fctSymbol.Create(symbolSize, symbolSize); GfxR->fctConstruction.Draw(fctSymbol, true);
			Add(LoadResStr(C4ResStrTableKey::IDS_CON_BUILDINFO), fctSymbol, command.c_str());
			fctSymbol.Default();
		}

		// Target info (if desc available)
		if (pTarget->Def->GetDesc() && *pTarget->Def->GetDesc())
		{
			// Symbol
			fctSymbol.Create(symbolSize, symbolSize);
			fctTarget = fctSymbol.GetFraction(85, 85, C4FCT_Left, C4FCT_Bottom);
			pTarget->DrawPicture(fctTarget);
			C4Facet fctTarget = fctSymbol.GetFraction(85, 85, C4FCT_Right, C4FCT_Top);
			GfxR->fctOKCancel.Draw(fctTarget, true, 0, 1);
			// Command
			command = std::format("ShowInfo(Object({}))", pTarget->Number);
			// Add item
			Add(LoadResStr(C4ResStrTableKey::IDS_CON_INFO), fctSymbol, command.c_str());
			fctSymbol.Default();
		}

		// Exit (if self contained in target container)
		if (pTarget->OCF & OCF_Container)
			if (pTarget == Object->Contained)
			{
				fctSymbol.Create(symbolSize, symbolSize);
				Game.GraphicsResource.fctExit.Draw(fctSymbol);
				Add(LoadResStr(C4ResStrTableKey::IDS_COMM_EXIT), fctSymbol, "PlayerObjectCommand(GetOwner(), \"Exit\") && ExecuteCommand()"); // Exit all selected clonks...
				fctSymbol.Default();
			}
	}
	break;

	default:
		// Not an internal menu
		return true;
	}

	// Successfull internal refill
	rfRefilled = true;
	return true;
}

void C4ObjectMenu::Execute()
{
	if (!IsActive()) return;
	// Immediate refill check by RefillObject contents count check
	if (RefillObject)
		if (RefillObject->Contents.ObjectCount() != RefillObjectContentsCount)
		{
			NeedRefill = true; RefillObjectContentsCount = RefillObject->Contents.ObjectCount();
		}
	// inherited
	C4Menu::Execute();
}

void C4ObjectMenu::OnUserSelectItem(int32_t Player, int32_t iIndex)
{
	// queue....
	Game.Input.Add(CID_PlrControl, new C4ControlPlayerControl(Player, COM_MenuSelect, iIndex | C4MN_AdjustPosition));
}

void C4ObjectMenu::OnUserEnter(int32_t Player, int32_t iIndex, bool fRight)
{
	// object menu: Through queue
	Game.Input.Add(CID_PlrControl, new C4ControlPlayerControl(Player, fRight ? COM_MenuEnterAll : COM_MenuEnter, iIndex));
}

void C4ObjectMenu::OnUserClose()
{
	// Queue
	Game.Input.Add(CID_PlrControl, new C4ControlPlayerControl(Game.MouseControl.GetPlayer(), COM_MenuClose, 0));
}

bool C4ObjectMenu::IsReadOnly()
{
	// get viewport
	C4Viewport *pVP = GetViewport();
	if (!pVP) return false;
	// is it an observer viewport?
	if (pVP->fIsNoOwnerViewport)
		// is this a synced menu?
		if (eCallbackType == CB_Object || eCallbackType == CB_Scenario)
			// then don't control it!
			return true;
	// if the player is eliminated, do not control either!
	if (!pVP->fIsNoOwnerViewport)
	{
		C4Player *pPlr = Game.Players.Get(Game.MouseControl.GetPlayer());
		if (pPlr && pPlr->Eliminated) return true;
	}
	return false;
}

int32_t C4ObjectMenu::GetControllingPlayer()
{
	// menu controlled by object controller
	return Object ? Object->Controller : NO_OWNER;
}

bool C4ObjectMenu::MenuCommand(const char *szCommand, bool fIsCloseCommand)
{
	// backup parameters to local stack because menu callback may delete this
	bool l_Permanent = !!Permanent;
	C4Object *l_Object = Object;
	int32_t l_LastSelection = LastSelection;
	if (l_Object)
	{
		ClearObjectPtr = &l_Object;
	}
	bool isDeleted{false};
	IsDeleted = &isDeleted;

	switch (eCallbackType)
	{
	case CB_Object:
		// Object menu
		if (Object) Object->MenuCommand(szCommand);
		break;

	case CB_Scenario:
		// Object menu with scenario script callback
		Game.Script.DirectExec(nullptr, szCommand, "MenuCommand", false, Game.Script.Strict);
		break;

	case CB_None:
		assert(!"Callback type is CB_None");
		break;
	}

	if ((!l_Permanent || fIsCloseCommand) && l_Object) l_Object->AutoContextMenu(l_LastSelection);

	if (!isDeleted)
	{
		ClearObjectPtr = nullptr;
		IsDeleted = nullptr;
	}

	return true;
}

int32_t C4ObjectMenu::AddContextFunctions(C4Object *pTarget, bool fCountOnly)
{
	const auto symbolSize = GetSymbolSize();

	int32_t iFunction, iResult = 0;
	C4AulScriptFunc *pFunction, *pFunction2;
	C4Object *cObj; C4ObjectLink *clnk;
	const char *strDescText;
	std::string command;
	C4Def *pDef;
	C4IDList ListItems;
	C4Facet fctTarget;
	C4FacetExSurface fctSymbol;

	// ActionContext functions of target's action target (for first target only, because otherwise strange stuff can happen with outdated Target2s...)
	if (pTarget->Action.Act > ActIdle)
		if (cObj = pTarget->Action.Target)
			for (iFunction = 0; pFunction = cObj->Def->Script.GetSFunc(iFunction, "ActionContext"); iFunction++)
				if (!pFunction->OverloadedBy)
					if (!pFunction->Condition || pFunction->Condition->Exec(cObj, {C4VObj(Object), C4VID(pFunction->idImage), C4VObj(pTarget)}))
						if (!fCountOnly)
						{
							command = std::format("ProtectedCall(Object({}),\"{}\",this,Object({}))", cObj->Number, pFunction->Name, pTarget->Number);
							if (pDef = Game.Defs.ID2Def(pFunction->idImage)) pDef->Picture2Facet(fctSymbol, 0, pFunction->iImagePhase);
							Add(pFunction->DescText.getData(), fctSymbol, command.c_str(), C4MN_Item_NoCount, nullptr, pFunction->DescLong.getData());
							iResult++;
						}
						else
							iResult++;

	// Effect context functions of target's effects
	for (C4Effect *pEff = pTarget->pEffects; pEff; pEff = pEff->pNext)
		if (pEff->IsActive())
		{
			C4AulScript *pEffScript = pEff->GetCallbackScript();
			if (pEffScript)
				for (iFunction = 0; pFunction = pEffScript->GetSFunc(iFunction, std::format(PSF_FxCustom, pEff->Name, "Context").c_str()); iFunction++)
					if (!pFunction->OverloadedBy)
						if (!pFunction->Condition || pFunction->Condition->Exec(pEff->pCommandTarget, {C4VObj(pTarget), C4VInt(pEff->iNumber), C4VObj(Object), C4VID(pFunction->idImage)}))
							if (!fCountOnly)
							{
								command = std::format("ProtectedCall(Object({}),\"{}\",Object({}),{},Object({}),{})", pEff->pCommandTarget->Number, pFunction->Name, pTarget->Number, static_cast<int>(pEff->iNumber), Object->Number, C4IdText(pFunction->idImage));
								if (pDef = Game.Defs.ID2Def(pFunction->idImage)) pDef->Picture2Facet(fctSymbol, 0, pFunction->iImagePhase);
								Add(pFunction->DescText.getData(), fctSymbol, command.c_str(), C4MN_Item_NoCount, nullptr, pFunction->DescLong.getData());
								fctSymbol.Default();
								iResult++;
							}
							else
								iResult++;
		}

	// Script context functions of any objects attached to target (search section list, because attachment objects might be moved just about anywhere...)
	for (clnk = pTarget->Section->Objects.First; clnk && (cObj = clnk->Obj); clnk = clnk->Next)
		if (cObj->Status && cObj->Action.Target == pTarget)
			if (cObj->Action.Act > ActIdle)
				if (cObj->Def->ActMap[cObj->Action.Act].Procedure == DFA_ATTACH)
					for (iFunction = 0; pFunction = cObj->Def->Script.GetSFunc(iFunction, "AttachContext"); iFunction++)
						if (!pFunction->OverloadedBy)
							if (!pFunction->Condition || pFunction->Condition->Exec(cObj, {C4VObj(Object), C4VID(pFunction->idImage), C4VObj(pTarget)}))
								if (!fCountOnly)
								{
									command = std::format("ProtectedCall(Object({}),\"{}\",this,Object({}))", cObj->Number, pFunction->Name, pTarget->Number);
									if (pDef = Game.Defs.ID2Def(pFunction->idImage)) pDef->Picture2Facet(fctSymbol, 0, pFunction->iImagePhase);
									Add(pFunction->DescText.getData(), fctSymbol, command.c_str(), C4MN_Item_NoCount, nullptr, pFunction->DescLong.getData());
									fctSymbol.Default();
									iResult++;
								}
								else
									iResult++;

	// 'Activate' and 'ControlDigDouble' script functions of target
	const char *func, *funcs[] = { "Activate", "ControlDigDouble", nullptr };
	for (int i = 0; func = funcs[i]; i++)
		// 'Activate' function only if in clonk's inventory; 'ControlDigDouble' function only if pushed by clonk
		if ((SEqual(func, "Activate") && (pTarget->Contained == Object)) || (SEqual(func, "ControlDigDouble") && (Object->GetProcedure() == DFA_PUSH) && (Object->Action.Target == pTarget)))
			// Find function
			if (pFunction = pTarget->Def->Script.GetSFunc(func))
				// Find function not overloaded
				if (!pFunction->OverloadedBy)
					// Function condition valid
					if (!pFunction->Condition || pFunction->Condition->Exec(pTarget, {C4VObj(Object), C4VID(pFunction->idImage)}))
					{
						// Get function text
						strDescText = pFunction->DescText.getData() ? pFunction->DescText.getData() : pTarget->GetName();
						// Check if there is a scripted context function doing exactly the same
						bool fDouble = false;
						for (iFunction = 0; pFunction2 = pTarget->Def->Script.GetSFunc(iFunction, "Context"); iFunction++)
							if (!pFunction2->OverloadedBy)
								if (!pFunction2->Condition || pFunction2->Condition->Exec(pTarget, {C4VObj(Object), C4VID(pFunction2->idImage)}))
									if (SEqual(strDescText, pFunction2->DescText.getData()))
										fDouble = true;
						// If so, skip this function to prevent duplicate entries
						if (fDouble) continue;
						// Count this function
						iResult++;
						// Count only: don't actually add
						if (fCountOnly) continue;
						// Command
						command = std::format("ProtectedCall(Object({}),\"{}\",this)", pTarget->Number, pFunction->Name);
						// Symbol
						if (pDef = Game.Defs.ID2Def(pFunction->idImage))
						{
							pDef->Picture2Facet(fctSymbol, 0, pFunction->iImagePhase);
						}
						else
						{
							fctSymbol.Create(symbolSize, symbolSize);
							pTarget->DrawPicture(fctSymbol);
						}
						// Add menu item
						Add(strDescText, fctSymbol, command.c_str(), C4MN_Item_NoCount, nullptr, pFunction->DescLong.getData());
						// Preserve symbol
						fctSymbol.Default();
					}

	// Script context functions of target
	if (!(pTarget->OCF & OCF_CrewMember) || (pTarget->Owner == Object->Owner)) // Crew member: only allow if owned by ourself
		if (!(pTarget->Category & C4D_Living) || pTarget->GetAlive()) // No dead livings
			for (iFunction = 0; pFunction = pTarget->Def->Script.GetSFunc(iFunction, "Context"); iFunction++)
				if (!pFunction->OverloadedBy)
					if (!pFunction->Condition || pFunction->Condition->Exec(pTarget, {C4VObj(Object), C4VID(pFunction->idImage)}))
						if (!fCountOnly)
						{
							command = std::format("ProtectedCall(Object({}),\"{}\",this)", pTarget->Number, pFunction->Name);
							if (pDef = Game.Defs.ID2Def(pFunction->idImage)) pDef->Picture2Facet(fctSymbol, 0, pFunction->iImagePhase);
							Add(pFunction->DescText.getData(), fctSymbol, command.c_str(), C4MN_Item_NoCount, nullptr, pFunction->DescLong.getData());
							fctSymbol.Default();
							iResult++;
						}
						else
							iResult++;

	// Context functions of the menu clonk itself (if not same as target)
	if (Object != pTarget)
		// Only if clonk is inside or grabbing or containing target (this excludes remote mouse-right-click context menus)
		if ((Object->Contained == pTarget) || ((Object->GetProcedure() == DFA_PUSH) && (Object->Action.Target == pTarget)) || (pTarget->Contained == Object))
			// No dead livings
			if (!(Object->Category & C4D_Living) || Object->GetAlive())
			{
				// Context menu for a building or grabbed vehicle: move the clonk functions into a submenu if more than two functions
				int32_t iSubMenuThreshold = 2;
				// Context menu for an inventory item: no threshold, always display clonk functions directly with target object functions
				if (pTarget->Contained == Object) iSubMenuThreshold = -1;
				// First count the available entries
				int32_t iFunctions = AddContextFunctions(Object, true);
				// Less than threshold or no threshold: display directly
				if ((iFunctions <= iSubMenuThreshold) || (iSubMenuThreshold == -1))
					iResult += AddContextFunctions(Object);
				// Above threshold: create sub-menu entry for the clonk
				else if (!fCountOnly)
				{
					Object->Def->Picture2Facet(fctSymbol, Object->Color);
					Add(Object->Def->GetName(), fctSymbol, "SetCommand(this,\"Context\",,0,0,this)&&ExecuteCommand()", C4MN_Item_NoCount, nullptr, LoadResStr(C4ResStrTableKey::IDS_MENU_CONTEXTSUBCLONKDESC));
					fctSymbol.Default();
					iResult++;
				}
				else
					iResult++;
			}

	// Done
	return iResult;
}
