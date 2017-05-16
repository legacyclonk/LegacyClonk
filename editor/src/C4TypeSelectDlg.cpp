
#include "C4Explorer.h"

#include "C4TypeSelectDlg.h"

BEGIN_MESSAGE_MAP(C4TypeSelectDlg, CDialog)
	//{{AFX_MSG_MAP(C4TypeSelectDlg)
	ON_WM_ERASEBKGND()
	ON_NOTIFY(NM_DBLCLK, IDC_LISTTYPE, OnDblclkListtype)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


C4TypeSelectDlg::C4TypeSelectDlg(CWnd* pParent /*=NULL*/)
	: CDialog(C4TypeSelectDlg::IDD, pParent)
	{
	//{{AFX_DATA_INIT(C4TypeSelectDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	}

void C4TypeSelectDlg::DoDataExchange(CDataExchange* pDX)
	{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(C4TypeSelectDlg)
	DDX_Control(pDX, IDOK, m_ButtonOK);
	DDX_Control(pDX, IDCANCEL, m_ButtonCancel);
	DDX_Control(pDX, IDC_STATICLOCATION, m_StaticLocation);
	DDX_Control(pDX, IDC_LISTTYPE, m_ListCtrl);
	//}}AFX_DATA_MAP
	}


BOOL C4TypeSelectDlg::OnInitDialog() 
	{
	CDialog::OnInitDialog();
	
	// Set control text
	UpdateText();

	// Image list
	LoadImageList(IDB_ITEMICONS,ImageList,16,16);
	m_ListCtrl.SetImageList(&ImageList,LVSIL_SMALL);

	// Insert items
	int iIndex = 0;
	for (int cnt=0; cnt<C4IT_Max; cnt++)
		if (ItemType[cnt].New>=NewMode)
			{
			m_ListCtrl.InsertItem(iIndex,LoadResStr(ItemType[cnt].Name),ItemType[cnt].Icon);
			m_ListCtrl.SetItemData(iIndex,cnt);
			iIndex++;
			}

	// Location
	m_StaticLocation.SetWindowText(Location);

	// Default selection
	Selection = C4IT_Unknown;

	// Set focus
	m_ListCtrl.SetFocus();

	return FALSE;
	}

C4TypeSelectDlg::~C4TypeSelectDlg()
	{
	Clear();
	}

void C4TypeSelectDlg::Clear()
	{
	ImageList.DeleteImageList();
	}

void C4TypeSelectDlg::OnOK() 
	{

	// Get selected type
	int iIndex = m_ListCtrl.GetNextItem(-1,LVNI_SELECTED);
	if (iIndex != -1) Selection = m_ListCtrl.GetItemData(iIndex);

	CDialog::OnOK();
	}

void C4TypeSelectDlg::OnCancel() 
	{	
	
	CDialog::OnCancel();
	}

void C4TypeSelectDlg::OnDblclkListtype(NMHDR* pNMHDR, LRESULT* pResult) 
	{
	*pResult = 0;
	OnOK();
	}

void C4TypeSelectDlg::UpdateText()
	{
	SetWindowText(LoadResStr("IDS_DLG_NEW"));
	m_ButtonOK.SetWindowText(LoadResStr("IDS_BTN_OK"));
	m_ButtonCancel.SetWindowText(LoadResStr("IDS_BTN_CANCEL"));
	}

void C4TypeSelectDlg::OnPaint() 
	{
	CPaintDC dc(this);
	
	}

/*BOOL C4TypeSelectDlg::OnEraseBkgnd(CDC* pDC) 
	{
	GetApp()->dibPaper.DrawTiles(pDC, this);
	return true;
	}*/

