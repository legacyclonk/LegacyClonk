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

#pragma once

#include "C4ObjectList.h"

class C4PropertyDlg
{
public:
	C4PropertyDlg();
	~C4PropertyDlg();
	void Default();
	void Clear();
	void Execute();
	void ClearPointers(C4Object *pObj);
	void Open();
	void Update();
	void Update(C4ObjectList &rSelection);
	void Draw();

	static int C4PropertyDlg::TextEditCallbackStub(struct ImGuiInputTextCallbackData* data);

public:
	bool Active;

protected:
	C4ID idSelectedDef;
	C4ObjectList Selection;
	StdStrBuf selectionText;
	C4AulFunc *selectedFunction;
};
