/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Extended tab control with owner draw tab */

class CTabCtrlEx: public CTabCtrl
	{
	public:
		CTabCtrlEx();
		~CTabCtrlEx();

	private:
		CDIBitmap* pBkDIB;

		//{{AFX_VIRTUAL(CTabCtrlEx)
		//}}AFX_VIRTUAL

	protected:
		//{{AFX_MSG(CTabCtrlEx)
		afx_msg void DrawItem(DRAWITEMSTRUCT* pDIS);
		afx_msg BOOL OnEraseBkgnd(CDC* pDC);
		//}}AFX_MSG
		DECLARE_MESSAGE_MAP()
	};
