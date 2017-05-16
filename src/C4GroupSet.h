/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2001, Sven2
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

// a set of group files
// manages system file overwriting by scearios or folders

#ifndef INC_C4GroupSet
#define INC_C4GroupSet

// group set priorities
#define C4GSPrio_Base				  0 // lowest priority for global system files
#define C4GSPrio_ExtraRoot	  2 // overloads by Extra.c4g root folder
#define C4GSPrio_Extra			  3 // overloads by Extra.c4g
#define C4GSPrio_Folder		  100 // overloads by local scenario folder - each child folder has higher priority
#define C4GSPrio_Folder2	  199 // highest priority a folder may have
#define C4GSPrio_Scenario	  200 // overloads by scenario: highest priority

// group node contents
#define C4GSCnt_Graphics		1 // contains Graphics.c4g
#define C4GSCnt_Loaders			2 // contains loader files
#define C4GSCnt_Material		4 // contains Material.c4g
#define C4GSCnt_Music				8 // contains music
#define C4GSCnt_Definitions	16 // contains definition files
#define C4GSCnt_FontDefs    32 // contains font definitions
#define C4GSCnt_Language    64 // contains language files
#define C4GSCnt_Component   128 // other components

#define C4GSCnt_Folder		(C4GSCnt_Graphics | C4GSCnt_Loaders | C4GSCnt_Material | C4GSCnt_Music | C4GSCnt_FontDefs)
#define C4GSCnt_Directory (C4GSCnt_Loaders | C4GSCnt_Music)
#define C4GSCnt_Scenario	C4GSCnt_Folder
#define C4GSCnt_Extra			(C4GSCnt_Graphics | C4GSCnt_Loaders | C4GSCnt_Material | C4GSCnt_Music | C4GSCnt_FontDefs)
#define C4GSCnt_ExtraRoot	(C4GSCnt_Graphics | C4GSCnt_Loaders | C4GSCnt_Material | C4GSCnt_Music | C4GSCnt_FontDefs)

#define C4GSCnt_All				 ~0

// class predefs
class C4GroupSet;
class C4GroupSetNode;

// one node in the group set holds one group
class C4GroupSetNode
	{
	protected:
		C4GroupSet *pParent;	// owning set
		C4GroupSetNode *pPrev, *pNext; // linked list - always valid

		C4Group *pGroup;			// ptr to group owned by this node
		bool fGrpOwned;				// flag if group ptr is owned

		int32_t id;               // group node ID

	public:
		C4GroupSetNode(C4GroupSet &rParent, C4GroupSetNode *pPrev, C4Group &rGroup, bool fGrpOwned, int32_t id);
		~C4GroupSetNode();

		int32_t Priority;					// group priority
		int32_t Contents;					// content held by this group

	friend class C4GroupSet;
	};

// a group set manages file overloading within several groups
class C4GroupSet
	{
	protected:
		C4GroupSetNode *pFirst, *pLast;	// linked list
		int32_t iIndex; // index to keep track of group node IDs

	public:
		void Clear();
		void Default();

		C4GroupSet();
		C4GroupSet(C4GroupSet &) = delete;
		~C4GroupSet();

		bool RegisterGroup(C4Group &rGroup, bool fOwnGrp, int32_t Priority, int32_t Contents, bool fCheckContent=true); // add group to list
		bool RegisterGroups(C4GroupSet &rCopy, int32_t Contents, const char *szFilename=NULL, int32_t iMaxSkipID=0);	// add all matching (child-)groups of the set
		C4Group *FindGroup(int32_t Contents, C4Group *pAfter=NULL, bool fSamePrio=false);				// search for suitable group in list
		C4Group *FindEntry(const char *szWildcard, int32_t *pPriority=NULL, int32_t *pID=NULL);										// find entry in groups; store priority of group if ptr is given
		C4Group *GetGroup(int32_t iIndex);
		bool LoadEntryString(const char *szEntryName, StdStrBuf & rBuf);
		C4Group *RegisterParentFolders(const char *szScenFilename); // register all parent .c4f groups to the given scenario filename and return an open group file of the innermost parent c4f

		static int32_t CheckGroupContents(C4Group &rGroup, int32_t Contents);
		int32_t GetLastID() { return iIndex; } // return ID assigned to the last added group

		bool CloseFolders();			// remove all groups associated with scenario folders

	friend class C4GroupSetNode;
	};

#endif
