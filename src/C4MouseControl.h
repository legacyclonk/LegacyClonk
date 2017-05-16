/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Mouse input */

#pragma once

#include "C4ObjectList.h"
#include "C4Region.h"

const int32_t C4MC_Button_None = 0,
              C4MC_Button_LeftDown = 1,
              C4MC_Button_LeftUp = 2,
              C4MC_Button_RightDown = 3,
              C4MC_Button_RightUp = 4,
              C4MC_Button_LeftDouble = 5,
              C4MC_Button_RightDouble = 6,
              C4MC_Button_Wheel = 7,
              C4MC_Button_MiddleDown = 8,
              C4MC_Button_MiddleUp = 9;

const int32_t C4MC_DragSensitivity = 5;

class C4MouseControl
{
	friend class C4Viewport;

public:
	C4MouseControl();
	~C4MouseControl();

protected:
	bool Active;
	bool fMouseOwned;
	int32_t Player;
	C4Player *pPlayer; // valid during Move()
	C4Viewport *Viewport; // valid during Move()
	StdStrBuf Caption;
	bool IsHelpCaption;
	int32_t Cursor;
	int32_t DownCursor;
	int32_t CaptionBottomY;
	int32_t VpX, VpY, X, Y, ViewX, ViewY;
	C4Facet fctViewport;
	int32_t DownX, DownY, DownOffsetX, DownOffsetY;
	int32_t ShowPointX, ShowPointY;
	int32_t KeepCaption;
	int32_t ScrollSpeed;
	int32_t Drag, DragSelecting;
	int32_t DragImagePhase;
	bool LeftButtonDown, RightButtonDown, LeftDoubleIgnoreUp;
	bool ControlDown;
	bool ShiftDown;
	bool Scrolling;
	bool InitCentered;
	bool Help;
	bool FogOfWar;
	bool Visible;
	C4ID DragID;
	C4ObjectList Selection;
	C4FacetEx DragImage;
	// Target object
	C4Object *TargetObject; // valid during Move()
	C4Object *DownTarget;
	int32_t TimeOnTargetObject;
	// Region
	C4Region *TargetRegion; // valid during Move()
	C4Region DownRegion;

public:
	void Default();
	void Clear();
	bool Init(int32_t iPlayer);
	void Execute();
	const char *GetCaption();
	void HideCursor();
	void ShowCursor();
	void Draw(C4FacetEx &cgo);
	void Move(int32_t iButton, int32_t iX, int32_t iY, uint32_t dwKeyFlags, bool fCenter = false);
	bool IsViewport(C4Viewport *pViewport);
	void ClearPointers(C4Object *pObj);
	void UpdateClip(); // update clipping region for mouse cursor
	void SetOwnedMouse(bool fToVal) { fMouseOwned = fToVal; }
	bool IsMouseOwned() { return fMouseOwned; }
	bool IsActive() { return !!Active; }

protected:
	void SendPlayerSelectNext();
	void UpdateFogOfWar();
	void RightUpDragNone();
	void ButtonUpDragConstruct();
	void ButtonUpDragMoving();
	void ButtonUpDragSelecting();
	void LeftUpDragNone();
	void DragConstruct();
	void Wheel(uint32_t dwFlags);
	void RightUp();
	void RightDown();
	void LeftDouble();
	void DragNone();
	void DragMoving();
	void LeftUp();
	void DragSelect();
	void LeftDown();
	void UpdateTargetRegion();
	void UpdateScrolling();
	void CreateDragImage(C4ID id);
	void UpdateCursorTarget();
	void SendCommand(int32_t iCommand, int32_t iX = 0, int32_t iY = 0, C4Object *pTarget = nullptr, C4Object *pTarget2 = nullptr, int32_t iData = 0, int32_t iAddMode = C4P_Command_Set);
	int32_t UpdateObjectSelection();
	int32_t UpdateCrewSelection();
	int32_t UpdateSingleSelection();
	bool SendControl(int32_t iCom, int32_t iData = 0);
	bool UpdatePutTarget(bool fVehicle);
	C4Object *GetTargetObject(int32_t iX, int32_t iY, uint32_t &dwOCF, C4Object *pExclude = nullptr);
	bool IsPassive(); // return whether mouse is only used to look around
	void ScrollView(int32_t iX, int32_t iY, int32_t ViewWdt, int32_t ViewHgt);

public:
	bool IsHelp() { return Help; }
	void AbortHelp() { Help = false; }
	bool IsDragging();
	void StartConstructionDrag(C4ID id);
	int32_t GetPlayer() { return Player; }
};
