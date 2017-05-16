/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Extends CComboBox to handle some icons */

class CIconCombo : public CComboBox
	{
	public:
		CIconCombo();
		~CIconCombo();

		void Init(UINT BmpID, int Width=16, int Height=16, int ColorDepth=8, bool PreFilled=false, int iSelItem=-2);

	private:
		CImageList ImageList;
		int ItemHeight;

		//{{AFX_VIRTUAL(CIconCombo)
		public:
		virtual void DrawItem(DRAWITEMSTRUCT* pDIS);
		virtual void MeasureItem(MEASUREITEMSTRUCT* pMIS);
		//}}AFX_VIRTUAL

	protected:
		//{{AFX_MSG(CIconCombo)
		//}}AFX_MSG
		DECLARE_MESSAGE_MAP()
	};
