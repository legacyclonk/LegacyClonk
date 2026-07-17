/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
 * Copyright (c) 2017-2024, The LegacyClonk Team and contributors
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

/* Console mode dialog for object properties and script interface */

#include <C4Include.h>
#include <C4PropertyDlg.h>

#include <C4Console.h>
#include <C4Application.h>
#include <C4Object.h>
#include <C4Wrappers.h>
#include <C4Player.h>

#include "imgui/imgui.h"

C4PropertyDlg::C4PropertyDlg()
{
	Default();
}

C4PropertyDlg::~C4PropertyDlg()
{
	Clear();
}

void C4PropertyDlg::Open()
{
	opened = true;
}

void C4PropertyDlg::Update(C4ObjectList &rSelection)
{
	if (!opened)
	{
		return;
	}
	// Set new selection
	selection.Copy(rSelection);
	// Update contents
	Update();
}

void C4PropertyDlg::Update()
{
	if (!opened)
	{
		return;
	}

	idSelectedDef = C4ID_None;

	// Compose info text by selected object(s)
	switch (selection.ObjectCount())
	{
	// No selection
	case 0:
		selectionText.Ref(LoadResStr(C4ResStrTableKey::IDS_CNS_NOOBJECT));
		break;
	// One selected object
	case 1:
	{
		C4Object *cobj{selection.GetObject()};
		// Type
		std::string name{cobj->GetName()};
		std::string id{C4IdText(cobj->Def->id)};
		StdStrBuf typeFormat{LoadResStrV(C4ResStrTableKey::IDS_CNS_TYPE)};
		typeFormat.Replace("%s", "{}");
		selectionText = std::vformat(typeFormat.getData(), std::make_format_args(name, id)).c_str();
		// Owner
		if (ValidPlr(cobj->Owner))
		{
			selectionText.Append(LineFeed);

			StdStrBuf ownerFormat{LoadResStrV(C4ResStrTableKey::IDS_CNS_OWNER)};
			ownerFormat.Replace("%s", "{}");
			std::string owner{Game.Players.Get(cobj->Owner)->GetName()};
			selectionText += std::vformat(ownerFormat.getData(), std::make_format_args(owner)).c_str();
		}
		// Contents
		if (cobj->Contents.ObjectCount())
		{
			selectionText.Append(LineFeed);
			selectionText.Append(LoadResStr(C4ResStrTableKey::IDS_CNS_CONTENTS));
			selectionText.Append(cobj->Contents.GetNameList(Game.Defs).c_str());
		}
		// Action
		if (cobj->Action.Act != ActIdle)
		{
			selectionText.Append(LineFeed);
			selectionText.Append(LoadResStr(C4ResStrTableKey::IDS_CNS_ACTION));
			selectionText.Append(cobj->Def->ActMap[cobj->Action.Act].Name);
		}
		// Locals
		int cnt; bool fFirstLocal{true};
		for (cnt = 0; cnt < cobj->Local.GetSize(); cnt++)
		{
			if (cobj->Local[cnt])
			{
				// Header
				if (fFirstLocal)
				{
					selectionText.Append(LineFeed);
					selectionText.Append(LoadResStr(C4ResStrTableKey::IDS_CNS_LOCALS));
					fFirstLocal = false;
				}
				selectionText.Append(LineFeed);
				// Append id
				selectionText += std::format(" Local({}) = ", cnt).c_str();
				// write value
				selectionText.Append(cobj->Local[cnt].GetDataString().c_str());
			}
		}
		// Locals (named)
		for (cnt = 0; cnt < cobj->LocalNamed.GetAnzItems(); cnt++)
		{
			// Header
			if (fFirstLocal)
			{
				selectionText.Append(LineFeed);
				selectionText.Append(LoadResStr(C4ResStrTableKey::IDS_CNS_LOCALS));
				fFirstLocal = false;
			}
			selectionText.Append(LineFeed);
			// Append name
			selectionText += std::format(" {} = ", cobj->LocalNamed.pNames->pNames[cnt]).c_str();
			// write value
			selectionText.Append(cobj->LocalNamed.pData[cnt].GetDataString().c_str());
		}
		// Effects
		for (C4Effect *pEffect{cobj->pEffects}; pEffect; pEffect = pEffect->pNext)
		{
			// Header
			if (pEffect == cobj->pEffects)
			{
				selectionText.Append(LineFeed);
				selectionText.Append(LoadResStr(C4ResStrTableKey::IDS_CNS_EFFECTS));
			}
			selectionText.Append(LineFeed);
			// Effect name
			selectionText += std::format(" {}: Interval {}", pEffect->Name, pEffect->iIntervall).c_str();
		}
		// Store selected def
		idSelectedDef = cobj->id;
		break;
	}
	// Multiple selected objects
	default:
		std::string objectCount{""+selection.ObjectCount()};
		StdStrBuf multipleObjectsFormat{LoadResStrV(C4ResStrTableKey::IDS_CNS_MULTIPLEOBJECTS)};
		multipleObjectsFormat.Replace("%i", "{}");
		multipleObjectsFormat.Replace("%s", "{}");
		selectionText = std::vformat(multipleObjectsFormat.getData(), std::make_format_args(objectCount)).c_str();
		break;
	}
}

void C4PropertyDlg::Default()
{
	opened = false;
	idSelectedDef = C4ID_None;
	selection.Default();
	selectedFunction = nullptr;
}

void C4PropertyDlg::Clear()
{
	selection.Clear();
	selectionText.Clear();
	opened = false;
}

void C4PropertyDlg::Execute()
{
	if (!Tick35) Update();
}

void C4PropertyDlg::ClearPointers(C4Object *pObj)
{
	selection.ClearPointers(pObj);
}

int C4PropertyDlg::TextEditCallbackStub(ImGuiInputTextCallbackData *data)
{
	C4PropertyDlg *PropertyDialog{reinterpret_cast<C4PropertyDlg*>(data->UserData)};
	// TODO: Use text edit functionality history and such from c4console
	return 1; // PropertyDialog->TextEditCallback(data);
}

void C4PropertyDlg::Draw()
{
	if (!opened)
	{
		return;
	}
	static ImVec2 propertySizeMin{200, 150};
	static ImVec2 propertySizeMax{500, 300};
	ImGui::SetNextWindowSizeConstraints(propertySizeMin, propertySizeMax);
	ImGui::Begin(LoadResStr(C4ResStrTableKey::IDS_DLG_PROPERTIES), &opened, ImGuiWindowFlags_NoFocusOnAppearing);

	if (ImGui::BeginChild("##properties", {0, ImGui::GetContentRegionAvail().y - 28}, true))
	{
		ImGui::TextWrapped("%s", selectionText.isNull() ? "" : selectionText.getData());
	}
	ImGui::EndChild(); // Note: Unlike other elements this must be outside the if-statement.

	// Command-line
	float commandLineWidth{ImGui::GetContentRegionAvail().x};
	ImGui::SetNextItemWidth(commandLineWidth - 30);
	bool reclaimFocus{false};
	static char inputBuf[512];
	ImGuiInputTextFlags inputTextFlags{ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_ElideLeft};
	if (ImGui::InputText("", inputBuf, IM_COUNTOF(inputBuf), inputTextFlags, &TextEditCallbackStub, reinterpret_cast<void*>(this)))
	{
		StdStrBuf s{&inputBuf[0]};
		s.TrimSpaces();
		if (s[0])
		{
			Console.EditCursor.In(s.getData());
		}
		strcpy(inputBuf, "");
		reclaimFocus = true;
	}

	// Auto-focus on window apparition
	ImGui::SetItemDefaultFocus();
	if (reclaimFocus)
	{
		ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget
	}

	const auto addFunctionEntry = [this](C4AulFunc *const func)
	{
		if (ImGui::Selectable(std::string{func->Name}.append("()").c_str(), selectedFunction == func))
		{
			selectedFunction = func;
			SAppend(func->Name, inputBuf);
			SAppend("()", inputBuf);
		}
	};

	ImGui::SameLine();

	if (ImGui::BeginCombo("##funcselectorproperties", selectedFunction ? selectedFunction->Name : nullptr, ImGuiComboFlags_NoPreview))
	{
		// Add global and standard functions
		for (C4AulFunc *func{Game.ScriptEngine.GetFirstFunc()}; func; func = Game.ScriptEngine.GetNextFunc(func))
		{
			if (func->GetPublic())
			{
				addFunctionEntry(func);
			}
		}

		// Add scenario script functions
		C4AulScriptFunc *func;
		if (C4Object *const obj{selection.GetObject()}; obj)
		{
			for (std::int32_t i{0}; (func = obj->Def->Script.GetSFunc(i)); ++i)
			{
				// Public functions only
				if (func->Access == AA_PUBLIC)
				{
					addFunctionEntry(func);
				}
			}
		}

		ImGui::EndCombo();
	}

	ImGui::End();

	if (!opened)
	{
		Clear();
	}
}
