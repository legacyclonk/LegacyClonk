/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Slider control */

// Control Notifications

#define SLN_CHANGE     7

class CSlider: public CStatic
	{
	public:
		CSlider();
		~CSlider();

	protected:
		int iValue;
		int iRandom;
		const int HThumbWidth;  // Die halbe Breite des Schiebers
		int ValPos;             // Die Pixelposition des Schiebers
		int RndPos;             // Die Pixel-Zufallsabweichung zu beiden Seiten
		CRect InvalRect;        // Die letzte Position zum Invalidieren
		bool Sound;
		COLORREF Color;         // Farbe ohne Focus
		COLORREF FocusColor;    // Farbe mit Focus
		COLORREF BackgroundColor;    // Farbe mit Focus
		CWnd *pNotify;

	public:
		int MinValue;           // Vom Dialog zu setzen
		int MaxValue;           // Vom Dialog zu setzen
		int MinRandom;          // Vom Dialog zu setzen
		int MaxRandom;          // Vom Dialog zu setzen
		bool HasRandom;	        // Random einstellen möglich?

	public:
		void Invalidate();
		bool Attach(CWnd *pBuddy);
		void Init(COLORREF lColor = 0);
		void Get(C4SVal &rVal);
		void Set(C4SVal &rVal);
		void EnableRandom(bool fEnable);
		void EnableSound(bool fEnable);
		void SetColor(COLORREF lColor, COLORREF lColorBackground=0xFFFFFF);
		int GetValue();
		int GetRandom();
		void SetValue(int Value);
		void SetRandom(int Random);

		//{{AFX_VIRTUAL(CSlider)
	protected:
		virtual void PreSubclassWindow();
		//}}AFX_VIRTUAL

	protected:
		void SendChangeNotification();
		//{{AFX_MSG(CSlider)
		afx_msg void OnPaint();
		afx_msg void OnMouseMove(UINT Flags, CPoint Point);
		afx_msg void OnLButtonDown(UINT Flags, CPoint Point);
		afx_msg void OnLButtonUp(UINT Flags, CPoint Point);
		afx_msg void OnKeyDown(UINT Char, UINT RepCnt, UINT Flags);
		afx_msg void OnKillFocus(CWnd* pNewWnd);
		afx_msg void OnRButtonDown(UINT Flags, CPoint Point);
		afx_msg void OnSetFocus(CWnd* pOldWnd);
		afx_msg void OnRButtonUp(UINT Flags, CPoint Point);
		afx_msg UINT OnGetDlgCode();
		//}}AFX_MSG
		DECLARE_MESSAGE_MAP()
	
	};

