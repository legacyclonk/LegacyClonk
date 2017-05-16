/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Edit group file components by extracting them to a temp directory */

class C4EditSlots;

class C4EditSlot  
	{
	friend C4EditSlots;
	public:
		C4EditSlot();
		~C4EditSlot();
	protected:
		CString ItemPath;
		CString EditPath;
		CString EditFolder;
		CString EditorPath;
		bool Original;
		bool Directory;
		int ItemTime;
		HANDLE hProcess;
		C4EditSlot *Next;
	public:
		bool Init(CString &rItemPath, bool fOriginal);
		void Default();
		void Clear();
	protected:
		bool UpdateItem();
		bool IsModified();
		bool IsFinished();
		bool LaunchEditor();
	private:
		bool CreateEditFolder(CString &rPath);
	};

class C4EditSlots
	{
	public:
		C4EditSlots();
		~C4EditSlots();
	protected:
		C4EditSlot *GetItemSlot(CString &rItemPath);
		C4EditSlot *First;
	public:
		void ClearLeftover();
		bool ClearItems(const char *szPath);
		bool RemoveItem(const char *szItemPath);
		bool ItemsStillEdited();
		bool CloseFinishedItems();
		bool UpdateModifiedItems(bool fBackground=false);
		bool IsItemEdited(CString &rItemPath);
		bool EditItem(CString &rItemPath, bool fOriginal);
		void Default();
		void Clear();
	};

