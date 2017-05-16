/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Plus/Minus/Add buttons for object lists */

class C4IdCtrlButtons: public CStatic
	{
	public:
		C4IdCtrlButtons();
		~C4IdCtrlButtons();

	private:
		C4IdListCtrl* pUnitList;
		static CDIBitmap DIB;
		int Height;
		int Width;

	public:
		void Set(C4IdListCtrl *pCtrl);

		//{{AFX_VIRTUAL(C4IdCtrlButtons)
		protected:
		virtual void PreSubclassWindow();
		//}}AFX_VIRTUAL

	protected:
		//{{AFX_MSG(C4IdCtrlButtons)
		afx_msg void OnPaint();
		afx_msg void OnLButtonDown(UINT Flags, CPoint Point);
		//}}AFX_MSG
		DECLARE_MESSAGE_MAP()
	};
