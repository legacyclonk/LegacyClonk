/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
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

/* Basic class for vertex outlines */

#pragma once

#include "C4Constants.h"
#include "C4ForwardDeclarations.h"
#include "C4Rect.h"

#include <cstdint>

#define C4D_VertexCpyPos (C4D_MaxVertex/2)

// a functional class to provide density for coordinates
class C4DensityProvider
{
public:
	virtual int32_t GetDensity(int32_t x, int32_t y) const;
	virtual ~C4DensityProvider() {}
};

extern C4DensityProvider DefaultDensityProvider;

class C4Shape : public C4Rect
{
public:
	// remember to adjust C4Shape::CopyFrom and CreateOwnOriginalCopy when adding members here!
	int32_t FireTop{};
	int32_t VtxNum{};
	int32_t VtxX[C4D_MaxVertex]{};
	int32_t VtxY[C4D_MaxVertex]{};
	int32_t VtxCNAT[C4D_MaxVertex]{};
	int32_t VtxFriction[C4D_MaxVertex]{};
	int32_t ContactDensity;
	int32_t ContactCNAT{};
	int32_t ContactCount{};
	int32_t AttachMat;
	int32_t VtxContactCNAT[C4D_MaxVertex]{};
	int32_t VtxContactMat[C4D_MaxVertex]{};
	int32_t iAttachX{}, iAttachY{}, iAttachVtx{};

public:
	C4Shape();
	void Default();
	void Rotate(int32_t iAngle, bool bUpdateVertices);
	void Stretch(int32_t iPercent, bool bUpdateVertices);
	void Jolt(int32_t iPercent, bool bUpdateVertices);
	void GetVertexOutline(C4Rect &rRect);
	int32_t GetVertexY(int32_t iVertex);
	int32_t GetVertexX(int32_t iVertex);
	bool AddVertex(int32_t iX, int32_t iY);
	bool CheckContact(int32_t cx, int32_t cy);
	bool ContactCheck(int32_t cx, int32_t cy);
	bool Attach(int32_t &cx, int32_t &cy, uint8_t cnat_pos);
	bool LineConnect(int32_t tx, int32_t ty, int32_t cvtx, int32_t ld, int32_t oldx, int32_t oldy);
	bool InsertVertex(int32_t iPos, int32_t tx, int32_t ty);
	bool RemoveVertex(int32_t iPos);
	void CopyFrom(C4Shape rFrom, bool bCpyVertices, bool fCopyVerticesFromSelf);
	int32_t GetBottomVertex();
	int32_t GetVertexContact(int32_t iVtx, uint32_t dwCheckMask, int32_t tx, int32_t ty, const C4DensityProvider &rDensityProvider = DefaultDensityProvider); // get CNAT-mask for given vertex - does not check range for iVtx!
	void CreateOwnOriginalCopy(C4Shape &rFrom); // create copy of all vertex members in back area of own buffers
	void CompileFunc(StdCompiler *pComp, bool fRuntime);
};
