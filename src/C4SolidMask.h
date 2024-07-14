/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2003, Sven2
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

/* Solid areas of objects, put into the landscape */

#pragma once

#include <C4ObjectList.h>
#include <C4Shape.h>

class C4SolidMask
{
protected:
	bool MaskPut; // if set, the mask is currently put into landscape
	int MaskPutRotation; // rotation in which the mask was put (and resides in the buffers)
	int MatBuffPitch; // pitch (and width) of mat buffer

	int32_t MaskRemovalX, MaskRemovalY; // last position mask was removed from
	class C4Object **ppAttachingObjects; // objects to be moved with mask motion
	int iAttachingObjectsCount, iAttachingObjectsCapacity;

	C4TargetRect MaskPutRect; // absolute bounding screen rect at which the mask is put - tx and ty are offsets within pSolidMask (for rects outside the landscape)

	uint8_t *pSolidMask; // solid mask data: 0x00 if not solid at this position; 0xff if solid
	uint8_t *pSolidMaskMatBuff; // material replaced by this solidmask. MCVehic if no solid mask data at this position OR another solidmask was already present during put

	C4Object *pForObject;

	// provides density within put SolidMask of an object
	class DensityProvider : public C4DensityProvider
	{
	private:
		class C4Object *pForObject;
		C4SolidMask &rSolidMaskData;

	public:
		DensityProvider(class C4Object *pForObject, C4SolidMask &rSolidMaskData)
			: pForObject(pForObject), rSolidMaskData(rSolidMaskData) {}

		virtual int32_t GetDensity(C4Landscape &landscape, int32_t x, int32_t y) const override;
	};

	// Remove the solidmask temporary
	void RemoveTemporary(C4Rect where);
	void PutTemporary(C4Rect where);
	// Reput and update Matbuf after landscape change underneath
	void Repair(C4Rect where);

	friend class C4Landscape;
	friend class DensityProvider;

public:
	// Linked list of all solidmasks
	static C4SolidMask *First;
	static C4SolidMask *Last;
	C4SolidMask *Prev;
	C4SolidMask *Next;

	void Put(bool fCauseInstability, C4TargetRect *pClipRect, bool fRestoreAttachment); // put mask to landscape
	void Remove(bool fCauseInstability, bool fBackupAttachment); // remove mask from landscape
	void Clear(); // clear any SolidMask-data

	C4SolidMask(C4Object *pForObject);
	~C4SolidMask();

#ifdef SOLIDMASK_DEBUG
	static bool CheckConsistency();
#else
	static bool CheckConsistency() { return true; }
#endif
};
