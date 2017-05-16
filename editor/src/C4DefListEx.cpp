/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* C4DefList with additional image list */

#include "C4Explorer.h"

C4DefListEx::C4DefListEx()
	{
	}

//---------------------------------------------------------------------------

int C4DefListEx::Load(C4Group &Group, DWORD LoadWhat, CString Lang)
{
	int Result = C4DefList::Load(Group, LoadWhat, Lang, NULL, TRUE);
	if (Result) UpdateImgList();
	return Result;
}

//---------------------------------------------------------------------------

int C4DefListEx::Load(CString Filename, DWORD LoadWhat, CString Lang)
{
	int Result = C4DefList::Load(Filename, LoadWhat, Lang, NULL, TRUE);
	if (Result) UpdateImgList();
	return Result;
}

//---------------------------------------------------------------------------

int C4DefListEx::LoadForScenario(CString Filename, CString Definitions, DWORD LoadWhat, CString Lang)
{
	int Result = C4DefList::LoadForScenario(Filename, Definitions, LoadWhat, Lang, NULL, TRUE);
	if (Result) UpdateImgList();
	return Result;
}

//---------------------------------------------------------------------------

int C4DefListEx::RemoveTemporary()
{
	int Result = C4DefList::RemoveTemporary();
	UpdateImgList();
	return Result;
}

//---------------------------------------------------------------------------

void C4DefListEx::UpdateImgList()
{
	ImgList.DeleteImageList();
	VERIFY(ImgList.Create(32,32, ILC_COLOR16 | ILC_MASK, 0, 256));
	C4Def* pDef = NULL;
	for (int iDef=0; (pDef = GetDef(iDef)) != NULL; iDef++)
	{
		CBitmap TempBmp;
		TempBmp.Attach((HBITMAP) CopyImage(pDef->Image, IMAGE_BITMAP, 32,32, LR_COPYRETURNORG));
		ImgList.Add(&TempBmp, TRANSPCOLOR);
	}
}
