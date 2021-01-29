/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
 * Copyright (c) 2017-2019, The LegacyClonk Team and contributors
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

/* Dynamic object list */

#pragma once

#include "C4Id.h"
#include "C4Def.h"
#include "C4DelegatedIterable.h"
#include "C4Region.h"

#include <list>
#include <memory>
#include <vector>

class C4Object;
class C4FacetEx;

constexpr std::int32_t
	C4EnumPointer1 = 1000000000,
	C4EnumPointer2 = 1001000000;

class C4ObjectList : public C4DelegatedIterable<C4ObjectList>
{
	using Container = std::list<C4Object *>;

public:
	C4ObjectList();
	C4ObjectList(const C4ObjectList &List);
	virtual ~C4ObjectList();

	int Mass;

	enum SortType { stNone = 0, stMain, stContents, stReverse, };
	using iterator = Container::const_iterator;
	using mutable_iterator = Container::iterator;

	// An iterator which survives if an object is removed from the list
	class SafeIterator
	{
	public:
		~SafeIterator();
		SafeIterator &operator++(); // prefix ++
		SafeIterator(const SafeIterator &iter);
		C4Object *operator*();
		C4Object *operator->() { return operator*(); }
		bool operator==(const SafeIterator &iter) const;
		bool operator!=(const SafeIterator &iter) const;

		SafeIterator &operator=(const SafeIterator &iter);

	private:
		explicit SafeIterator(C4ObjectList &List);
		SafeIterator(C4ObjectList &List, const C4ObjectList::iterator &pLink);
		C4ObjectList &List;
		C4ObjectList::iterator pLink;

		friend class C4ObjectList;
	};
	SafeIterator safeBegin();
	const SafeIterator safeEnd();

	void SortByCategory();
	void Clear();
	void Enumerate();
	void Denumerate();
	void Copy(const C4ObjectList &rList);
	void DrawAll(C4FacetEx &cgo, int iPlayer = -1); // draw all objects, including bg
	void DrawIfCategory(C4FacetEx &cgo, int iPlayer, uint32_t dwCat, bool fInvert); // draw all objects that match dwCat (or don't match if fInvert)
	void Draw(C4FacetEx &cgo, int iPlayer = -1); // draw all objects
	void DrawIDList(C4Facet &cgo, int iSelection, C4DefList &rDefs, int32_t dwCategory, C4RegionList *pRegions = nullptr, int iRegionCom = COM_None, bool fDrawOneCounts = true);
	void DrawSelectMark(C4FacetEx &cgo);
	void CloseMenus();
	void UpdateGraphics(bool fGraphicsChanged);
	void UpdateFaces(bool bUpdateShape);
	void SyncClearance();
	void ResetAudibility();
	void UpdateTransferZones();
	void SetOCF();
	void ClearInfo(C4ObjectInfo *pInfo);
	void ClearDefPointers(C4Def *pDef); // clear all pointers into definition
	void UpdateDefPointers(C4Def *pDef); // restore any cleared pointers after def reload

	bool Add(C4Object *nObj, SortType eSort, C4ObjectList *pLstSorted = nullptr);
	virtual bool Remove(C4Object *pObj);

	void AssignInfo();
	void ValidateOwners();
	void AssignPlrViewRange();
	StdStrBuf GetNameList(C4DefList &rDefs, uint32_t dwCategory = C4D_All);
	bool IsClear() const;
	bool DenumerateRead();
	void CompileFunc(StdCompiler *pComp, bool fSaveRefs = true, bool fSkipPlayerObjects = false);

	int32_t ObjectNumber(C4Object *pObj);
	bool IsContained(C4Object *pObj);
	int ClearPointers(C4Object *pObj);
	int ObjectCount(C4ID id = C4ID_None, int32_t dwCategory = C4D_All) const;
	int MassCount();

	virtual C4Object *ObjectPointer(int32_t iNumber);
	C4Object *SafeObjectPointer(int32_t iNumber);
	C4Object *GetObject(int Index = 0);
	C4Object *Find(C4ID id, int iOwner = ANY_OWNER, uint32_t dwOCF = OCF_All);
	C4Object *FindOther(C4ID id, int iOwner = ANY_OWNER);

	iterator GetLink(C4Object *pObj) const;

	std::vector<C4ID> GetListIDs(int32_t dwCategory);

	bool OrderObjectBefore(C4Object *pObj1, C4Object *pObj2); // order pObj1 before pObj2
	bool OrderObjectAfter(C4Object *pObj1, C4Object *pObj2); // order pObj1 after pObj2

	bool ShiftContents(C4Object *pNewFirst); // cycle list so pNewFirst is at front

	void DeleteObjects(); // delete all objects and links

	void UpdateScriptPointers(); // update pointers to C4AulScript *

	bool CheckSort(C4ObjectList *pList); // check that all objects of this list appear in the other list in the same order
	void CheckCategorySort(); // assertwhether sorting by category is done right

protected:
	// the virtuals are only for subclass callbacks
	virtual void InsertLinkBefore(const iterator &pLink, const iterator &pBefore) {}
	virtual void InsertLink(const iterator &pLink, const iterator &pAfter) {}
	virtual void RemoveLink(const iterator &pLnk) {}
	std::vector<SafeIterator*> SafeIterators;
	void AddIter(SafeIterator *iter);
	void RemoveIter(SafeIterator *iter);

	friend class SafeIterator;
	friend class C4ObjResort;

	// TODO: optional?
	std::unique_ptr<std::vector<int32_t>> Enumerated;
	Container Objects;

public:
	using Iterable = IterableMember<&C4ObjectList::Objects>;
};

class C4ObjectListChangeListener
{
public:
	virtual void OnObjectRemove(C4ObjectList *pList, const C4ObjectList::iterator &pLnk) = 0;
	virtual void OnObjectAdded(C4ObjectList *pList, const C4ObjectList::iterator &pLnk) = 0;
	virtual void OnObjectRename(C4ObjectList *pList, const C4ObjectList::iterator &pLnk) = 0;
	virtual ~C4ObjectListChangeListener() {}
};

extern C4ObjectListChangeListener &ObjectListChangeListener;

class C4NotifyingObjectList : public C4ObjectList
{
public:
	C4NotifyingObjectList() {}
	C4NotifyingObjectList(const C4NotifyingObjectList &List) : C4ObjectList(List) {}
	C4NotifyingObjectList(const C4ObjectList &List) : C4ObjectList(List) {}
	virtual ~C4NotifyingObjectList() {}

protected:
	virtual void InsertLinkBefore(const C4ObjectList::iterator &pLink, const C4ObjectList::iterator &pBefore) override;
	virtual void InsertLink(const C4ObjectList::iterator &pLink, const C4ObjectList::iterator &pAfter) override;
	virtual void RemoveLink(const C4ObjectList::iterator &pLnk) override;

	friend class C4ObjResort;
};

// This iterator is used to return objects of same ID and picture as grouped.
// It's taking advantage of the fact that object lists are sorted by ID.
// Used by functions such as C4ObjectList::DrawIDList, or the menu-filling of
// activation/selling menus
class C4ObjectListIterator
{
private:
	C4ObjectList &rList; // iterated list
	C4ObjectList::SafeIterator pCurr; // link to last returned object
	C4ObjectList::SafeIterator pCurrID; // link to head of link group with same ID

	C4ObjectListIterator(const C4ObjectListIterator &rCopy); // no copy ctor

public:
	C4ObjectListIterator(C4ObjectList &rList) : rList(rList), pCurr(rList.safeEnd()), pCurrID(rList.safeBegin()) {}
	C4Object *GetNext(int32_t *piCount, uint32_t dwCategory = 0); // get next object; return nullptr if end is reached
};
