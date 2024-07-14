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

// Engine internal C4Menus: Main menu, Options, Player join, Hostility, etc.

#include <C4Include.h>
#include <C4MainMenu.h>

#include <C4FullScreen.h>
#include <C4Viewport.h>
#include <C4Wrappers.h>
#include <C4Player.h>
#include <C4GameOverDlg.h>
#include <C4Object.h>

#include <format>

// C4MainMenu

C4MainMenu::C4MainMenu() : C4Menu() // will be re-adjusted later
{
	Clear();
}

void C4MainMenu::Default()
{
	C4Menu::Default();
	Player = NO_OWNER;
}

bool C4MainMenu::Init(C4FacetExSurface &fctSymbol, const char *szEmpty, int32_t iPlayer, int32_t iExtra, int32_t iExtraData, int32_t iId, int32_t iStyle)
{
	if (!DoInit(fctSymbol, szEmpty, iExtra, iExtraData, iId, iStyle)) return false;
	Player = iPlayer;
	return true;
}

bool C4MainMenu::InitRefSym(const C4FacetEx &fctSymbol, const char *szEmpty, int32_t iPlayer, int32_t iExtra, int32_t iExtraData, int32_t iId, int32_t iStyle)
{
	if (!DoInitRefSym(fctSymbol, szEmpty, iExtra, iExtraData, iId, iStyle)) return false;
	Player = iPlayer;
	return true;
}

bool C4MainMenu::ActivateNewPlayer(int32_t iPlayer)
{
	const auto symbolSize = GetSymbolSize();

	// league or replay game
	if (Game.Parameters.isLeague() || Game.C4S.Head.Replay) return false;
	// Max player limit
	if (Game.Players.GetCount() >= Game.Parameters.MaxPlayers) return false;

	// Menu symbol/init
	if (GfxR->fctPlayerClr.Surface)
		GfxR->fctPlayerClr.Surface->SetClr(0xff);
	InitRefSym(GfxR->fctPlayerClr, LoadResStr(C4ResStrTableKey::IDS_MENU_NOPLRFILES), iPlayer);
	std::string command;
	for (DirectoryIterator iter(Config.General.PlayerPath); *iter; ++iter)
		if (WildcardMatch("*.c4p", *iter))
		{
			char szFilename[_MAX_PATH + 1];
			SCopy(*iter, szFilename, _MAX_PATH);
			if (DirectoryExists(szFilename)) continue;
			if (Game.Players.FileInUse(szFilename)) continue;
			// Open group
			C4Group hGroup;
			if (!hGroup.Open(szFilename)) continue;
			// Load player info
			C4PlayerInfoCore C4P;
			if (!C4P.Load(hGroup)) { hGroup.Close(); continue; }
			// Load custom portrait
			C4FacetExSurface fctPortrait;
			if (Config.Graphics.ShowPortraits)
				if (!fctPortrait.Load(hGroup, C4CFN_BigIcon, C4FCT_Full, C4FCT_Full, false, true))
					if (!fctPortrait.Load(hGroup, C4CFN_Portrait, C4FCT_Full, C4FCT_Full, false, true))
						fctPortrait.Load(hGroup, C4CFN_Portrait_Old, C4FCT_Full, C4FCT_Full, false, true);
			// Close group
			hGroup.Close();
			// Add player item
			command = std::format("JoinPlayer:{}", szFilename);
			const std::string itemText{LoadResStr(C4ResStrTableKey::IDS_MENU_NEWPLAYER, C4P.PrefName)};
			// No custom portrait: use default player image
			if (!fctPortrait.Surface)
			{
				fctPortrait.Create(symbolSize, symbolSize);
				GfxR->fctPlayerClr.DrawClr(fctPortrait, true, 0xff);
			}
			// Create color overlay for portrait
			C4FacetExSurface fctPortraitClr;
			fctPortraitClr.CreateClrByOwner(fctPortrait.Surface);
			// Create menu symbol from colored portrait
			C4FacetExSurface fctSymbol;
			fctSymbol.Create(symbolSize, symbolSize);
			fctPortraitClr.DrawClr(fctSymbol, true, C4P.PrefColorDw);
			// Add menu item
			Add(itemText.c_str(), fctSymbol, command.c_str());
			// Reset symbol facet (menu holds on to the surface)
			fctSymbol.Default();
		}

	// Alignment
	SetAlignment(C4MN_Align_Left | C4MN_Align_Bottom);
	// Go back to options menu on close
	SetCloseCommand("ActivateMenu:Main");

	return true;
}

bool C4MainMenu::DoRefillInternal(bool &rfRefilled)
{
	const auto symbolSize = GetSymbolSize();

	// Variables
	C4FacetExSurface fctSymbol;
	C4Player *pPlayer;
	C4IDList ListItems;
	C4Facet fctTarget;
	bool fWasEmpty = !GetItemCount();

	// Refill
	switch (Identification)
	{
	case C4MN_Hostility:
	{
		// Clear items
		ClearItems();
		// Refill player
		if (!(pPlayer = Game.Players.Get(Player))) return false;
		// Refill items
		C4Player *pPlr; int32_t iIndex;
		for (iIndex = 0; pPlr = Game.Players.GetByIndex(iIndex); iIndex++)
			// Ignore player self and invisible
			if (pPlr != pPlayer) if (!pPlr->IsInvisible())
			{
				// Symbol
				fctSymbol.Create(symbolSize, symbolSize);
				pPlayer->DrawHostility(fctSymbol, iIndex);
				// Message
				const std::string msg{LoadResStrChoice(pPlayer->Hostility.GetIDCount(pPlr->Number + 1), C4ResStrTableKey::IDS_MENU_ATTACK, C4ResStrTableKey::IDS_MENU_NOATTACK, pPlr->GetName())};
				// Command
				const std::string command{std::format("SetHostility:{}", pPlr->Number)};
				// Info caption
				const std::string friendly{LoadResStrChoice(pPlr->Hostility.GetIDCount(pPlayer->Number + 1), C4ResStrTableKey::IDS_MENU_ATTACKHOSTILE, C4ResStrTableKey::IDS_MENU_ATTACKFRIENDLY)};
				std::string notFriendly;

				if (!pPlayer->Hostility.GetIDCount(pPlr->Number + 1))
				{
					notFriendly = LoadResStr(C4ResStrTableKey::IDS_MENU_ATTACKNOT);
				}

				const std::string infoCaption{LoadResStr(C4ResStrTableKey::IDS_MENU_ATTACKINFO, pPlr->GetName(), friendly, notFriendly)};

				// Add item
				Add(msg.c_str(), fctSymbol, command.c_str(), C4MN_Item_NoCount, nullptr, infoCaption.c_str());
				fctSymbol.Default();
			}
		break;
	}

	case C4MN_TeamSelection:
	case C4MN_TeamSwitch:
	{
		// Clear items
		ClearItems();
		// add all teams as menu items
		// 2do: Icon
		C4Team *pTeam; int32_t i = 0; bool fAddNewTeam = Game.Teams.IsAutoGenerateTeams();
		for (;;)
		{
			pTeam = Game.Teams.GetTeamByIndex(i);
			if (pTeam)
			{
				// next regular team
				++i;
				// do not add a new team if an empty team exists
				if (!pTeam->GetPlayerCount()) fAddNewTeam = false;
			}
			else if (fAddNewTeam)
			{
				// join new team
				fAddNewTeam = false;
			}
			else
			{
				// all teams done
				break;
			}
			// create team symbol: Icon spec if specified; otherwise flag for empty and crew for nonempty team
			fctSymbol.Create(symbolSize, symbolSize);
			const char *szIconSpec = pTeam ? pTeam->GetIconSpec() : nullptr;
			bool fHasIcon = false;
			if (szIconSpec && *szIconSpec)
			{
				C4FacetExSurface fctSrc;
				fHasIcon = Game.DrawTextSpecImage(fctSrc, szIconSpec, pTeam->GetColor());
				fctSrc.Draw(fctSymbol);
			}
			if (!fHasIcon)
			{
				if (pTeam && pTeam->GetPlayerCount())
					Game.GraphicsResource.fctCrewClr.DrawClr(fctSymbol, true, pTeam->GetColor());
				else
					C4GUI::Icon::GetIconFacet(C4GUI::Ico_Team).Draw(fctSymbol, true);
			}
			StdStrBuf sTeamName;
			if (pTeam)
			{
				sTeamName.Take(pTeam->GetNameWithParticipants());
			}
			else
				sTeamName.Ref(LoadResStr(C4ResStrTableKey::IDS_PRC_NEWTEAM));
			const char *szOperation = (Identification == C4MN_TeamSwitch) ? "TeamSwitch" : "TeamSel";
			Add(sTeamName.getData(), fctSymbol, std::format("{}:{}S", szOperation, pTeam ? pTeam->GetID() : TEAMID_New).c_str(),
				C4MN_Item_NoCount, nullptr, LoadResStr(C4ResStrTableKey::IDS_MSG_JOINTEAM, sTeamName.getData()).c_str(), C4ID(pTeam ? pTeam->GetID() : 0));
			fctSymbol.Default();
		}
		break;
	}

	case C4MN_Observer: // observer menu
	{
		// Clear items
		ClearItems();
		// Check validity
		C4Viewport *pVP = Game.GraphicsSystem.GetViewport(NO_OWNER);
		if (!pVP) return false;
		int32_t iInitialSelection = 0;
		// Add free view
		AddRefSym(LoadResStr(C4ResStrTableKey::IDS_MSG_FREEVIEW), C4GUI::Icon::GetIconFacet(C4GUI::Ico_Star), "Observe:Free", C4MN_Item_NoCount, nullptr, LoadResStr(C4ResStrTableKey::IDS_MSG_FREELYSCROLLAROUNDTHEMAP));
		// Add players
		C4Player *pPlr; int32_t iIndex;
		for (iIndex = 0; pPlr = Game.Players.GetByIndex(iIndex); iIndex++)
		{
			// Ignore invisible
			if (!pPlr->IsInvisible())
			{
				// Symbol
				fctSymbol.Create(symbolSize, symbolSize);
				Game.GraphicsResource.fctPlayerClr.DrawClr(fctSymbol, true, pPlr->ColorDw);
				// Message
				StdStrBuf sMsg;
				uint32_t dwClr = pPlr->ColorDw;
				const std::string msg{std::format("<c {:x}>{}</c>", C4GUI::MakeColorReadableOnBlack(dwClr), pPlr->GetName())};
				// Command
				const std::string command{std::format("Observe:{}", pPlr->Number)};
				// Info caption
				const std::string info{LoadResStr(C4ResStrTableKey::IDS_TEXT_FOLLOWVIEWOFPLAYER, pPlr->GetName())};
				// Add item
				Add(msg.c_str(), fctSymbol, command.c_str(), C4MN_Item_NoCount, nullptr, info.c_str());
				fctSymbol.Default();
				// check if this is the currently selected player
				if (pVP->GetPlayer() == pPlr->Number) iInitialSelection = GetItemCount() - 1;
			}
			// Initial selection on followed player
			if (fWasEmpty) SetSelection(iInitialSelection, false, true);
		}
	}
	break;

	default:
		// No internal refill needed
		return true;
	}

	// Successfull internal refill
	rfRefilled = true;
	return true;
}

void C4MainMenu::OnSelectionChanged(int32_t iNewSelection)
{
	// immediate player highlight in observer menu
	if (Identification == C4MN_Observer)
	{
		C4MenuItem *pItem = GetSelectedItem();
		if (pItem)
		{
			if (SEqual2(pItem->GetCommand(), "Observe:"))
				MenuCommand(pItem->GetCommand(), false);
		}
	}
}

void C4MainMenu::OnUserSelectItem(int32_t Player, int32_t iIndex)
{
	// direct selection for non-sync-menus
	SetSelection(iIndex, true, true);
}

void C4MainMenu::OnUserEnter(int32_t Player, int32_t iIndex, bool fRight)
{
	// direct menu control
	// but ensure selection is Okay before
	SetSelection(iIndex, true, false);
	Enter(fRight);
}

void C4MainMenu::OnUserClose()
{
	// just close
	TryClose(false, true);
}

void C4MainMenu::OnClosed(bool ok)
{
	if (const auto player = GetControllingPlayer(); player != NO_OWNER)
	{
		if (const auto plr = Game.Players.Get(player); plr)
		{
			plr->ClearPressedComsSynced();
		}
	}

	C4Menu::OnClosed(ok);
}

bool C4MainMenu::ActivateGoals(int32_t iPlayer, bool fDoActivate)
{
	const auto symbolSize = GetSymbolSize();

	C4FacetExSurface fctSymbol;
	C4FacetEx fctGF; // goal fulfilled facet

	if (fDoActivate)
	{
		// Menu symbol/init
		InitRefSym(GfxR->fctMenu.GetPhase(4), LoadResStr(C4ResStrTableKey::IDS_MENU_CPGOALS), iPlayer);
		SetAlignment(C4MN_Align_Left | C4MN_Align_Bottom);
		const auto scale = Application.GetScale();
		const auto starWidth = Game.GraphicsResource.fctCaptain.Wdt * scale;
		const auto starHeight = Game.GraphicsResource.fctCaptain.Hgt * scale;
		SetPermanent(false);
		fctGF.Set(nullptr, symbolSize - starWidth - 2 * scale, 2 * scale, starWidth, starHeight);
	}
	// determine if the goals are fulfilled - do the calls even if the menu is not to be opened to ensure synchronization
	C4IDList GoalList, FulfilledGoalList;
	C4RoundResults::EvaluateGoals(GoalList, FulfilledGoalList, iPlayer);
	// Add Items
	if (fDoActivate)
	{
		int32_t iNumGoals = GoalList.GetNumberOfIDs(), cnt;
		C4ID idGoal; C4Def *pDef;
		for (int32_t i = 0; i < iNumGoals; ++i)
			if (idGoal = GoalList.GetID(i, &cnt))
				if (pDef = Game.Defs.ID2Def(idGoal))
				{
					fctSymbol.Create(symbolSize, symbolSize);
					// 2do: If an object instance is known, draw the object instead?
					// this would allow us to do dynamic pictures and overlays; e.g. draw the actual, required settlement score
					// for settlement score goals
					// Same for pDef->GetName(), pDef->GetDesc()
					pDef->Draw(fctSymbol);
					if (FulfilledGoalList.GetIDCount(idGoal))
					{
						fctGF.Surface = fctSymbol.Surface;
						Game.GraphicsResource.fctCaptain.Draw(fctGF);
					}
					Add(pDef->GetName(), fctSymbol, std::format("Player:Goal:{}", C4IdText(idGoal)).c_str(), C4MN_Item_NoCount, nullptr, pDef->GetDesc());
				}
		// Go back to options menu on close
		SetCloseCommand("ActivateMenu:Main");
	}
	// Done
	return true;
}

bool C4MainMenu::ActivateRules(int32_t iPlayer)
{
	const auto symbolSize = GetSymbolSize();

	// Menu symbol/init
	std::string command;
	C4FacetExSurface fctSymbol;
	InitRefSym(GfxR->fctMenu.GetPhase(5), LoadResStr(C4ResStrTableKey::IDS_MENU_CPRULES), iPlayer);
	SetAlignment(C4MN_Align_Left | C4MN_Align_Bottom);
	SetPermanent(false);
	// Items
	int32_t cnt; C4ID idGoal; C4Def *pDef;

	for (const auto &section : Game.Sections)
	{
		for (cnt = 0; idGoal = section->Objects.ObjectsInt().GetListID(C4D_Rule, cnt); cnt++)
			if (pDef = Game.Defs.ID2Def(idGoal))
			{
				fctSymbol.Create(symbolSize, symbolSize); pDef->Draw(fctSymbol);
				command = std::format("Player:Rule:{}", C4IdText(idGoal));
				Add(pDef->GetName(), fctSymbol, command.c_str(), C4MN_Item_NoCount, nullptr, pDef->GetDesc());
			}
	}
	// Go back to options menu on close
	SetCloseCommand("ActivateMenu:Main");
	// Done
	return true;
}

bool LooksLikeInteger(const char *szInt)
{
	// safety
	if (!szInt) return false;
	// check sign
	if (*szInt == '+' || *szInt == '-') ++szInt;
	// check int32_t length
	if (!*szInt) return false;
	// must contain only digits now
	char c;
	while (c = *(szInt++)) if (!Inside<char>(c, '0', '9')) return false;
	// it's an int32_t
	return true;
}

bool C4MainMenu::ActivateSavegame(int32_t iPlayer)
{
	// Check if saving is possible
	if (!Game.CanQuickSave()) return false;

	// Menu symbol/init
	char DirPath[_MAX_PATH + 1];
	char ScenName[_MAX_PATH + 1]; *ScenName = 0;

	InitRefSym(GfxR->fctMenu.GetPhase(0), LoadResStr(C4ResStrTableKey::IDS_MENU_CPSAVEGAME), iPlayer);
	SetAlignment(C4MN_Align_Left | C4MN_Align_Bottom);
	SetPermanent(true);

	// target file name mask
	// get folder & filename to store in
	// some magic is needed to ensure savegames are stored properly into their folders
	SCopy(GetFilename(Game.ScenarioFilename), DirPath);
	if (DirPath[strlen(DirPath) - 1] == '\\') DirPath[strlen(DirPath) - 1] = 0;
	RemoveExtension(DirPath);
	if (LooksLikeInteger(DirPath))
	{
		// ScenTitle.c4f\%d.c4s-names (old-style savegames)
		// get owning folder
		if (Game.pParentGroup)
		{
			// owning folder determines filename
			SCopy(GetFilenameOnly(Game.pParentGroup->GetName()), ScenName);
		}
		else
		{
			// no owning folder known: too bad
			// make a vague guess based on the scenario title
			SCopy(GetFilenameOnly(Game.ScenarioFilename), ScenName);
		}
	}
	else
	{
		// DirPath is a valid filename for now...
		SCopy(DirPath, ScenName);
		// but remove trailing numbers to adjust new-style savegames
		char *pScenNameEnd = ScenName + SLen(ScenName);
		while (Inside<char>(*--pScenNameEnd, '0', '9'))
			if (pScenNameEnd == ScenName)
			{
				// should not happen: digit-only-filenames should have been caught earlier
				SCopy("dbg_error!", ScenName);
				pScenNameEnd = ScenName + SLen(ScenName) - 1;
			}
		pScenNameEnd[1] = 0;
	}

	// New Style 2007:
	// * scenarios are saved into ScenName.c4f/ScenName123.c4s to keep umlauts out of filenames
	// * language titles are stored in folders as title component
	const std::string filename{std::format("{}.c4f" DirSep "{}{{}}.c4s", ScenName, ScenName)};

	// Create menu items
	std::string filenameIndexed;
	std::string command;
	std::string savePath;

	for (int32_t i = 1; i <= 10; i++)
	{
		// Index filename
		filenameIndexed = std::vformat(filename, std::make_format_args(i));
		// Compose commmand
		command = std::format("Save:Game:{}:{}", filenameIndexed, Game.Parameters.ScenarioTitle.getData()); // Notice: the language title might contain ':' and thus confuse the segment list - but C4Menu::MenuCommand will try to handle this...
		// Check free slot
		savePath = std::format("{}" DirSep "{}", Config.General.SaveGameFolder.getData(), filenameIndexed);
		bool fFree = !C4Group_IsGroup(savePath.c_str());
		// add menu item
		AddRefSym(LoadResStr(C4ResStrTableKey::IDS_MENU_CPSAVEGAME), GfxR->fctMenu.GetPhase(i - 1, fFree ? 2 : 1), command.c_str(), C4MN_Item_NoCount, nullptr, LoadResStr(C4ResStrTableKey::IDS_MENU_CPSAVEGAMEINFO));
	}

	// Go back to options menu on close
	SetCloseCommand("ActivateMenu:Main");

	return true;
}

bool C4MainMenu::ActivateHost(int32_t iPlayer)
{
	// Menu symbol/init
	InitRefSym(C4GUI::Icon::GetIconFacet(C4GUI::Ico_Disconnect), LoadResStr(C4ResStrTableKey::IDS_MENU_DISCONNECTCLIENT), iPlayer, C4MN_Extra_None, 0, 0, C4MN_Style_Context);
	SetAlignment(C4MN_Align_Left | C4MN_Align_Bottom);
	SetPermanent(true);
	// Clients
	for (C4Network2Client *pClient = Game.Network.Clients.GetNextClient(nullptr); pClient; pClient = Game.Network.Clients.GetNextClient(pClient))
	{
		bool fHost = (pClient->getID() == 0);
		const std::string text{std::format("{} ({})", pClient->getName(), pClient->getCore().getNick())};
		const std::string command{std::format("Host:Kick:{}", pClient->getID())};
		C4GUI::Icons iIcon = fHost ? C4GUI::Ico_Host : (pClient->isActivated() ? C4GUI::Ico_Client : C4GUI::Ico_ObserverClient);
		AddRefSym(text.c_str(), C4GUI::Icon::GetIconFacet(iIcon), command.c_str());
	}
	// Go back to options menu on close
	SetCloseCommand("ActivateMenu:Main");
	return true;
}

bool C4MainMenu::ActivateClient(int32_t iPlayer)
{
	const auto symbolSize = GetSymbolSize();

	// Menu symbol/init
	C4FacetExSurface fctSymbol;
	InitRefSym(C4GUI::Icon::GetIconFacet(C4GUI::Ico_Disconnect), LoadResStr(C4ResStrTableKey::IDS_MENU_DISCONNECTFROMSERVER), iPlayer, C4MN_Extra_None, 0, 0, C4MN_Style_Context);
	SetAlignment(C4MN_Align_Left | C4MN_Align_Bottom);
	fctSymbol.Create(symbolSize, symbolSize); GfxR->fctOKCancel.Draw(fctSymbol, true, 3, 0);
	Add(LoadResStr(C4ResStrTableKey::IDS_BTN_YES), fctSymbol, "Part");
	fctSymbol.Create(symbolSize, symbolSize); GfxR->fctOKCancel.Draw(fctSymbol, true, 1, 0);
	Add(LoadResStr(C4ResStrTableKey::IDS_BTN_NO), fctSymbol, "");
	SetCloseCommand("ActivateMenu:Main");
	return true;
}

bool C4MainMenu::ActivateSurrender(int32_t iPlayer)
{
	const auto symbolSize = GetSymbolSize();

	C4FacetExSurface fctSymbol;
	InitRefSym(C4GUI::Icon::GetIconFacet(C4GUI::Ico_Surrender), LoadResStr(C4ResStrTableKey::IDS_MENU_SURRENDER), iPlayer, C4MN_Extra_None, 0, 0, C4MN_Style_Context);
	SetAlignment(C4MN_Align_Left | C4MN_Align_Bottom);
	fctSymbol.Create(symbolSize, symbolSize); GfxR->fctOKCancel.Draw(fctSymbol, true, 3, 0);
	Add(LoadResStr(C4ResStrTableKey::IDS_BTN_YES), fctSymbol, "Surrender");
	fctSymbol.Create(symbolSize, symbolSize); GfxR->fctOKCancel.Draw(fctSymbol, true, 1, 0);
	Add(LoadResStr(C4ResStrTableKey::IDS_BTN_NO), fctSymbol, "");
	SetCloseCommand("ActivateMenu:Main");
	return true;
}

bool C4MainMenu::ActivateOptions(int32_t iPlayer, int32_t selection)
{
	// Menu symbol/init
	InitRefSym(GfxR->fctOptions.GetPhase(0), LoadResStr(C4ResStrTableKey::IDS_MNU_OPTIONS), iPlayer, C4MN_Extra_None, 0, 0, C4MN_Style_Context);
	SetAlignment(C4MN_Align_Left | C4MN_Align_Bottom);
	SetPermanent(true);
	// Sound
	AddRefSym(LoadResStr(C4ResStrTableKey::IDS_DLG_SOUND), GfxR->fctOptions.GetPhase(17 + Config.Sound.RXSound), "Options:Sound", C4MN_Item_NoCount);
	// Music
	AddRefSym(LoadResStr(C4ResStrTableKey::IDS_MNU_MUSIC), GfxR->fctOptions.GetPhase(1 + Config.Sound.RXMusic), "Options:Music", C4MN_Item_NoCount);
	// Mouse control
	C4Player *pPlr = Game.Players.Get(iPlayer);
	if (pPlr && !Game.C4S.Head.DisableMouse)
	{
		if (pPlr->MouseControl)
			AddRefSym(LoadResStr(C4ResStrTableKey::IDS_MNU_MOUSECONTROL), GfxR->fctOptions.GetPhase(11 + 1), "Options:Mouse");
		else if (!Game.Players.MouseControlTaken())
			AddRefSym(LoadResStr(C4ResStrTableKey::IDS_MNU_MOUSECONTROL), GfxR->fctOptions.GetPhase(11), "Options:Mouse");
	}
	// Music
	AddRefSym(LoadResStr(C4ResStrTableKey::IDS_MENU_DISPLAY), GfxR->fctMenu.GetPhase(8), "ActivateMenu:Display");
	// Restore selection
	SetSelection(selection, false, true);
	// Go back to main menu on close
	SetCloseCommand("ActivateMenu:Main");
	// Done
	return true;
}

bool C4MainMenu::ActivateDisplay(int32_t iPlayer, int32_t selection)
{
	// Menu symbol/init
	InitRefSym(GfxR->fctMenu.GetPhase(8), LoadResStr(C4ResStrTableKey::IDS_MENU_DISPLAY), iPlayer, C4MN_Extra_None, 0, 0, C4MN_Style_Context);
	SetAlignment(C4MN_Align_Left | C4MN_Align_Bottom);
	SetPermanent(true);
	// Crew player names
	AddRefSym(LoadResStr(C4ResStrTableKey::IDS_MNU_PLAYERNAMES), GfxR->fctOptions.GetPhase(7 + Config.Graphics.ShowCrewNames), "Display:PlayerNames", C4MN_Item_NoCount, nullptr, LoadResStr(C4ResStrTableKey::IDS_MENU_PLAYERNAMES_DESC));
	// Crew clonk names
	AddRefSym(LoadResStr(C4ResStrTableKey::IDS_MNU_CLONKNAMES), GfxR->fctOptions.GetPhase(9 + Config.Graphics.ShowCrewCNames), "Display:ClonkNames", C4MN_Item_NoCount, nullptr, LoadResStr(C4ResStrTableKey::IDS_MENU_CLONKNAMES_DESC));
	// Portraits
	AddRefSym(LoadResStr(C4ResStrTableKey::IDS_MNU_PORTRAITS), GfxR->fctOptions.GetPhase(13 + Config.Graphics.ShowPortraits), "Display:Portraits", C4MN_Item_NoCount);
	// ShowCommands
	AddRefSym(LoadResStr(C4ResStrTableKey::IDS_MENU_SHOWCOMMANDS), GfxR->fctOptions.GetPhase(19 + Config.Graphics.ShowCommands), "Display:ShowCommands", C4MN_Item_NoCount);
	// ShowCommandKeys
	AddRefSym(LoadResStr(C4ResStrTableKey::IDS_MENU_SHOWCOMMANDKEYS), GfxR->fctOptions.GetPhase(21 + Config.Graphics.ShowCommandKeys), "Display:ShowCommandKeys", C4MN_Item_NoCount);
	// Upper Board
	if (Application.isFullScreen)
	{
		std::string text{LoadResStr(C4ResStrTableKey::IDS_MNU_UPPERBOARD)};
		text += ": ";

		std::string_view modeName;

		if (Config.Graphics.UpperBoard >= C4UpperBoard::First && Config.Graphics.UpperBoard <= C4UpperBoard::Last)
		{
			static constexpr std::array<C4ResStrTableKeyFormat<>, 4> ModeNames
			{
				C4ResStrTableKey::IDS_MNU_UPPERBOARD_OFF,
				C4ResStrTableKey::IDS_MNU_UPPERBOARD_NORMAL,
				C4ResStrTableKey::IDS_MNU_UPPERBOARD_SMALL,
				C4ResStrTableKey::IDS_MNU_UPPERBOARD_MINI
			};

			modeName = LoadResStr(ModeNames[Config.Graphics.UpperBoard]);
		}
		else
		{
			modeName = "???";
		}

		text += modeName;
		AddRefSym(text.c_str(), GfxR->fctOptions.GetPhase(3 + (Config.Graphics.UpperBoard != C4UpperBoard::Hide)), "Display:UpperBoard", C4MN_Item_NoCount);
	}
	// FPS
	if (Application.isFullScreen)
		AddRefSym(LoadResStr(C4ResStrTableKey::IDS_MNU_FPS), GfxR->fctOptions.GetPhase(5 + Config.General.FPS), "Display:FPS", C4MN_Item_NoCount);
	// Clock
	if (Application.isFullScreen)
		AddRefSym(LoadResStr(C4ResStrTableKey::IDS_MNU_CLOCK), GfxR->fctOptions.GetPhase(15 + Config.Graphics.ShowClock), "Display:Clock", C4MN_Item_NoCount);
	// White chat
	if (Application.isFullScreen)
		AddRefSym(LoadResStr(C4ResStrTableKey::IDS_MNU_WHITECHAT), GfxR->fctOptions.GetPhase(3 + Config.General.UseWhiteIngameChat), "Display:WhiteChat", C4MN_Item_NoCount, nullptr, LoadResStr(C4ResStrTableKey::IDS_DESC_WHITECHAT_INGAME));
	// Restore selection
	SetSelection(selection, false, true);
	// Go back to options menu on close
	SetCloseCommand("ActivateMenu:Options");
	// Done
	return true;
}

bool C4MainMenu::ActivateMain(int32_t iPlayer)
{
	const auto symbolSize = GetSymbolSize();

	// Determine player
	C4Player *pPlr = Game.Players.Get(iPlayer);
	// Menu symbol/init
	C4FacetExSurface fctSymbol;
	fctSymbol.Create(symbolSize, symbolSize);
	GfxR->fctOKCancel.Draw(fctSymbol, true, 1, 1);
	Init(fctSymbol, LoadResStrChoice(pPlr, C4ResStrTableKey::IDS_MENU_CPMAIN, C4ResStrTableKey::IDS_MENU_OBSERVER), iPlayer, C4MN_Extra_None, 0, 0, C4MN_Style_Context);
	SetAlignment(C4MN_Align_Left | C4MN_Align_Bottom);
	// Goals+Rules (player menu only)
	// Goal menu can't be shown because of script callbacks
	// Rule menu could be shown, but rule activation would issue script callbacks and trigger client activation
	// Showing rules but not showing goals would be strange anyway
	if (pPlr)
	{
		// Goals
		AddRefSym(LoadResStr(C4ResStrTableKey::IDS_MENU_CPGOALS), GfxR->fctMenu.GetPhase(4), "ActivateMenu:Goals", C4MN_Item_NoCount, nullptr, LoadResStr(C4ResStrTableKey::IDS_MENU_CPGOALSINFO));
		// Rules
		AddRefSym(LoadResStr(C4ResStrTableKey::IDS_MENU_CPRULES), GfxR->fctMenu.GetPhase(5), "ActivateMenu:Rules", C4MN_Item_NoCount, nullptr, LoadResStr(C4ResStrTableKey::IDS_MENU_CPRULESINFO));
	}
	// Observer menu in free viewport
	if (!pPlr)
	{
		AddRefSym(LoadResStr(C4ResStrTableKey::IDS_TEXT_VIEW), C4GUI::Icon::GetIconFacet(C4GUI::Ico_View), "ActivateMenu:Observer", C4MN_Item_NoCount, nullptr, LoadResStr(C4ResStrTableKey::IDS_TEXT_DETERMINEPLAYERVIEWTOFOLL));
	}
	// Hostility (player menu only)
	if (pPlr && (Game.Players.GetCount() > 1))
	{
		GfxR->fctFlagClr.Surface->SetClr(0xff0000);
		AddRefSym(LoadResStr(C4ResStrTableKey::IDS_MENU_CPATTACK), GfxR->fctMenu.GetPhase(7), "ActivateMenu:Hostility", C4MN_Item_NoCount, nullptr, LoadResStr(C4ResStrTableKey::IDS_MENU_CPATTACKINFO));
	}
	// Team change
	if (pPlr && Game.Teams.IsTeamSwitchAllowed())
	{
		C4FacetEx fctTeams; fctTeams = C4GUI::Icon::GetIconFacet(C4GUI::Ico_Team);
		AddRefSym(LoadResStr(C4ResStrTableKey::IDS_MSG_SELTEAM), fctTeams, "ActivateMenu:TeamSel", C4MN_Item_NoCount, nullptr, LoadResStr(C4ResStrTableKey::IDS_MSG_ALLOWSYOUTOJOINADIFFERENT));
	}
	// Player join
	if ((Game.Players.GetCount() < Game.Parameters.MaxPlayers) && !Game.Parameters.isLeague())
	{
		AddRefSym(LoadResStr(C4ResStrTableKey::IDS_MENU_CPNEWPLAYER), GfxR->fctPlayerClr.GetPhase(), "ActivateMenu:NewPlayer", C4MN_Item_NoCount, nullptr, LoadResStr(C4ResStrTableKey::IDS_MENU_CPNEWPLAYERINFO));
	}
	// Save game (player menu only - should we allow saving games with no players in it?)
	if (pPlr && (!Game.Network.isEnabled() || Game.Network.isHost()))
	{
		AddRefSym(LoadResStr(C4ResStrTableKey::IDS_MENU_CPSAVEGAME), GfxR->fctMenu.GetPhase(0), "ActivateMenu:Save:Game", C4MN_Item_NoCount, nullptr, LoadResStr(C4ResStrTableKey::IDS_MENU_CPSAVEGAMEINFO));
	}
	// Options
	AddRefSym(LoadResStr(C4ResStrTableKey::IDS_MNU_OPTIONS), GfxR->fctOptions.GetPhase(0), "ActivateMenu:Options", C4MN_Item_NoCount, nullptr, LoadResStr(C4ResStrTableKey::IDS_MNU_OPTIONSINFO));
	// Disconnect
	if (Game.Network.isEnabled())
	{
		// Host
		if (Game.Network.isHost() && Game.Clients.getClient(nullptr))
			AddRefSym(LoadResStr(C4ResStrTableKey::IDS_MENU_DISCONNECT), C4GUI::Icon::GetIconFacet(C4GUI::Ico_Disconnect), "ActivateMenu:Host", C4MN_Item_NoCount, nullptr, LoadResStr(C4ResStrTableKey::IDS_TEXT_KICKCERTAINCLIENTSFROMTHE));
		// Client
		if (!Game.Network.isHost())
			AddRefSym(LoadResStr(C4ResStrTableKey::IDS_MENU_DISCONNECT), C4GUI::Icon::GetIconFacet(C4GUI::Ico_Disconnect), "ActivateMenu:Client", C4MN_Item_NoCount, nullptr, LoadResStr(C4ResStrTableKey::IDS_TEXT_DISCONNECTTHEGAMEFROMTHES));
	}
	// Surrender (player menu only)
	if (pPlr)
		AddRefSym(LoadResStr(C4ResStrTableKey::IDS_MENU_CPSURRENDER), C4GUI::Icon::GetIconFacet(C4GUI::Ico_Surrender), "ActivateMenu:Surrender", C4MN_Item_NoCount, nullptr, LoadResStr(C4ResStrTableKey::IDS_MENU_CPSURRENDERINFO));
	// Abort
	if (Application.isFullScreen)
		AddRefSym(LoadResStr(C4ResStrTableKey::IDS_MENU_ABORT), C4GUI::Icon::GetIconFacet(C4GUI::Ico_Exit), "Abort", C4MN_Item_NoCount, nullptr, LoadResStr(C4ResStrTableKey::IDS_MENU_ABORT_DESC));
	// No empty menus
	if (GetItemCount() == 0) Close(false);
	// Done
	return true;
}

bool C4MainMenu::ActivateHostility(int32_t iPlayer)
{
	const auto symbolSize = GetSymbolSize();

	// Init menu
	C4FacetExSurface fctSymbol;
	fctSymbol.Create(symbolSize, symbolSize);
	GfxR->fctMenu.GetPhase(7).Draw(fctSymbol);
	Init(fctSymbol, LoadResStr(C4ResStrTableKey::IDS_MENU_CPATTACK), iPlayer, C4MN_Extra_None, 0, C4MN_Hostility);
	SetAlignment(C4MN_Align_Left | C4MN_Align_Bottom);
	SetPermanent(true);
	Refill();
	// Go back to options menu on close
	SetCloseCommand("ActivateMenu:Main");
	return true;
}

bool C4MainMenu::MenuCommand(const char *szCommand, bool fIsCloseCommand)
{
	// Determine player
	C4Player *pPlr = Game.Players.Get(Player);
	// Activate
	if (SEqual2(szCommand, "ActivateMenu:"))
	{
		if (C4GameOverDlg::IsShown()) return false; // no new menus during game over dlg
		if (SEqual(szCommand + 13, "Main")) return ActivateMain(Player);
		if (SEqual(szCommand + 13, "Hostility")) return ActivateHostility(Player);
		if (SEqual(szCommand + 13, "NewPlayer")) return ActivateNewPlayer(Player);
		if (SEqual(szCommand + 13, "Goals"))
		{
			Game.Control.DoInput(CID_ActivateGameGoalMenu, new C4ControlActivateGameGoalMenu(Player), CDT_Queue);
			return true;
		}
		if (SEqual(szCommand + 13, "Rules")) return ActivateRules(Player);
		if (SEqual(szCommand + 13, "Host")) return ActivateHost(Player);
		if (SEqual(szCommand + 13, "Client")) return ActivateClient(Player);
		if (SEqual(szCommand + 13, "Options")) return ActivateOptions(Player);
		if (SEqual(szCommand + 13, "Display")) return ActivateDisplay(Player);
		if (SEqual(szCommand + 13, "Save:Game")) return ActivateSavegame(Player);
		if (SEqual(szCommand + 13, "TeamSel")) return pPlr ? pPlr->ActivateMenuTeamSelection(true) : false;
		if (SEqual(szCommand + 13, "Surrender")) return ActivateSurrender(Player);
		if (SEqual(szCommand + 13, "Observer")) return ActivateObserver();
	}
	// JoinPlayer
	if (SEqual2(szCommand, "JoinPlayer:"))
	{
		// not in league or replay mode
		if (Game.Parameters.isLeague() || Game.C4S.Head.Replay) return false;
		// join player
		if (Game.Network.isEnabled())
			// 2do: not for observers and such?
			Game.Network.Players.JoinLocalPlayer(szCommand + 11, true);
		else
			Game.Players.CtrlJoinLocalNoNetwork(szCommand + 11, Game.Clients.getLocalID(), Game.Clients.getLocalName());
		return true;
	}
	// SetHostility
	if (SEqual2(szCommand, "SetHostility:"))
	{
		// only if allowed
		if (!Game.Teams.IsHostilityChangeAllowed()) return false;
		int32_t iOpponent; sscanf(szCommand + 13, "%i", &iOpponent);
		C4Player *pOpponent = Game.Players.Get(iOpponent);
		if (!pOpponent || pOpponent->GetType() != C4PT_User) return false;
		Game.Input.Add(CID_ToggleHostility, new C4ControlToggleHostility(Player, iOpponent));
		return true;
	}
	// Abort
	if (SEqual2(szCommand, "Abort"))
	{
		FullScreen.ShowAbortDlg();
		return true;
	}
	// Surrender
	if (SEqual2(szCommand, "Surrender"))
	{
		Game.Control.DoInput(CID_SurrenderPlayer, new C4ControlSurrenderPlayer(Player), CDT_Queue);
		return true;
	}
	// Save game
	if (SEqual2(szCommand, "Save:Game:"))
	{
		char strFilename[_MAX_PATH + 1]; SCopySegment(szCommand, 2, strFilename, ':', _MAX_PATH);
		char strTitle[_MAX_PATH + 1]; SCopy(szCommand + SCharPos(':', szCommand, 2) + 1, strTitle, _MAX_PATH);
		Game.QuickSave(strFilename, strTitle);
		ActivateSavegame(Player);
		return true;
	}
	// Kick
	if (SEqual2(szCommand, "Host:Kick:"))
	{
		int iClientID = atoi(szCommand + 10);
		if (iClientID && Game.Network.isEnabled())
			if (Game.Parameters.isLeague() && Game.Players.GetAtClient(iClientID))
				Game.Network.Vote(VT_Kick, true, iClientID);
			else
			{
				C4Client *pClient = Game.Clients.getClientByID(iClientID);
				if (pClient) Game.Clients.CtrlRemove(pClient, LoadResStr(C4ResStrTableKey::IDS_MSG_KICKBYMENU));
				Close(true);
			}
		return true;
	}
	// Part
	if (SEqual2(szCommand, "Part"))
	{
		if (Game.Network.isEnabled())
			if (Game.Parameters.isLeague() && Game.Players.GetLocalByIndex(0))
				Game.Network.Vote(VT_Kick, true, Game.Control.ClientID());
			else
			{
				Game.RoundResults.EvaluateNetwork(C4RoundResults::NR_NetError, LoadResStr(C4ResStrTableKey::IDS_ERR_GAMELEFTVIAPLAYERMENU));
				Game.Network.Clear();
			}
		return true;
	}
	// Options
	if (SEqual2(szCommand, "Options:"))
	{
		// Music
		if (SEqual(szCommand + 8, "Music"))
		{
			Application.MusicSystem->ToggleOnOff();
		}
		// Sound
		if (SEqual(szCommand + 8, "Sound"))
		{
			Application.SoundSystem->ToggleOnOff();
		}
		// Mouse control
		if (SEqual(szCommand + 8, "Mouse"))
			if (pPlr)
				pPlr->ToggleMouseControl();
		// Reopen with updated options
		ActivateOptions(Player, GetSelection());
		return true;
	}
	// Display
	if (SEqual2(szCommand, "Display:"))
	{
		// Upper board
		if (SEqual(szCommand + 8, "UpperBoard"))
		{
			++Config.Graphics.UpperBoard;
			if (Config.Graphics.UpperBoard > C4UpperBoard::Last)
				Config.Graphics.UpperBoard = C4UpperBoard::First;
			Game.InitFullscreenComponents(true);
		}
		// FPS
		if (SEqual(szCommand + 8, "FPS")) Config.General.FPS = !Config.General.FPS;
		// Player names
		if (SEqual(szCommand + 8, "PlayerNames")) Config.Graphics.ShowCrewNames = !Config.Graphics.ShowCrewNames;
		// Clonk names
		if (SEqual(szCommand + 8, "ClonkNames")) Config.Graphics.ShowCrewCNames = !Config.Graphics.ShowCrewCNames;
		// Portraits
		if (SEqual(szCommand + 8, "Portraits")) Config.Graphics.ShowPortraits = !Config.Graphics.ShowPortraits;
		// ShowCommands
		if (SEqual(szCommand + 8, "ShowCommands")) Config.Graphics.ShowCommands = !Config.Graphics.ShowCommands;
		// ShowCommandKeys
		if (SEqual(szCommand + 8, "ShowCommandKeys")) Config.Graphics.ShowCommandKeys = !Config.Graphics.ShowCommandKeys;
		// Clock
		if (SEqual(szCommand + 8, "Clock")) Config.Graphics.ShowClock = !Config.Graphics.ShowClock;
		// White chat
		if (SEqual(szCommand + 8, "WhiteChat")) Config.General.UseWhiteIngameChat = !Config.General.UseWhiteIngameChat;
		// Reopen with updated options
		ActivateDisplay(Player, GetSelection());
		return true;
	}
	// Goal info
	if (SEqual2(szCommand, "Player:Goal:") || SEqual2(szCommand, "Player:Rule:"))
	{
		if (!ValidPlr(Player)) return false; // observers may not look at goal/rule info, because it requires queue activation
		Close(true);
		// TODO!
		C4Object *pObj; C4ID idItem = C4Id(szCommand + 12);
		if (pObj = Game.FindFirstInAllObjects([idItem](C4GameObjects &objects) { return objects.FindInternal(idItem); }))
			Game.Control.DoInput(CID_ActivateGameGoalRule, new C4ControlActivateGameGoalRule(Player, pObj->Number), CDT_Queue);
		else
			return false;
		return true;
	}
	// Team selection
	if (SEqual2(szCommand, "TeamSel:"))
	{
		Close(true);
		int32_t idTeam = atoi(szCommand + 8);

		// OK, join this team
		if (pPlr) pPlr->DoTeamSelection(idTeam);
		return true;
	}
	// Team switch
	if (SEqual2(szCommand, "TeamSwitch:"))
	{
		Close(true);
		int32_t idTeam = atoi(szCommand + 11);

		// check if it's still allowed
		if (!Game.Teams.IsTeamSwitchAllowed()) return false;
		// OK, join this team
		Game.Control.DoInput(CID_SetPlayerTeam, new C4ControlSetPlayerTeam(Player, idTeam), CDT_Queue);
		return true;
	}
	// Observe
	if (SEqual2(szCommand, "Observe:"))
	{
		const char *szObserverTarget = szCommand + 8;
		C4Viewport *pVP = Game.GraphicsSystem.GetViewport(NO_OWNER);
		if (pVP) // viewport may have closed meanwhile
		{
			if (SEqual(szObserverTarget, "Free"))
			{
				// free view
				pVP->Init(NO_OWNER, true);
				return true;
			}
			else
			{
				// view following player
				int32_t iPlr = atoi(szObserverTarget);
				if (ValidPlr(iPlr))
				{
					pVP->Init(iPlr, true);
					return true;
				}
			}
		}
		return false;
	}
	// No valid command
	return false;
}

bool C4MainMenu::ActivateObserver()
{
	// Safety: Viewport lost?
	if (!Game.GraphicsSystem.GetViewport(NO_OWNER)) return false;
	// Menu symbol/init
	InitRefSym(C4GUI::Icon::GetIconFacet(C4GUI::Ico_View), LoadResStr(C4ResStrTableKey::IDS_TEXT_VIEW), NO_OWNER, C4MN_Extra_None, 0, C4MN_Observer, C4MN_Style_Context);
	SetAlignment(C4MN_Align_Left | C4MN_Align_Bottom);
	// Players added in Refill
	Refill();
	// Go back to main menu on close
	SetCloseCommand("ActivateMenu:Main");
	return true;
}
