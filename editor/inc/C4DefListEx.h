/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* C4DefList with additional image list */

class C4DefListEx: public C4DefList  
	{
	public:
		C4DefListEx();
	public:
		CImageList ImgList;
	public:
		int Load(C4Group &Group, DWORD LoadWhat, CString Lang);
		int Load(CString Filename, DWORD LoadWhat, CString Lang);
		int LoadForScenario(CString Filename, CString Definitions, DWORD LoadWhat, CString Lang);
		void UpdateImgList();
		int RemoveTemporary();
	};
