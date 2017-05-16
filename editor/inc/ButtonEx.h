/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Extends CButton as owner draw with bitmap background */

class CButtonEx: public CButton
	{
	public:
		CButtonEx();
		~CButtonEx();

	private:
		CDIBitmap* pBkDIB;

	public:
		void SetBitmap(CDIBitmap *pBmp);
		void Invalidate();
		void Set(const char* idCaption);

		//{{AFX_VIRTUAL(CButtonEx)
		public:
		virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
		//}}AFX_VIRTUAL

	protected:
		//{{AFX_MSG(CButtonEx)
		afx_msg void OnLButtonDown(UINT Flags, CPoint Point);
		//}}AFX_MSG
		DECLARE_MESSAGE_MAP()
	};
