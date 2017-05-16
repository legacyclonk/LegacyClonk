/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Program page in options */

#include "C4Explorer.h"
#include "C4PageProgram.h"

#include "C4Language.h"

//IMPLEMENT_DYNCREATE(C4PageProgram, CPropertyPage)

BEGIN_MESSAGE_MAP(C4PageProgram, CPropertyPage)
	//{{AFX_MSG_MAP(C4PageProgram)
	ON_CBN_SELENDOK(IDC_COMBO_LANGUAGE, OnSelEndOkComboLanguage)
	ON_CBN_EDITCHANGE(IDC_COMBO_LANGUAGE, OnEditChangeComboLanguage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

C4PageProgram::C4PageProgram(): CPropertyPage(C4PageProgram::IDD)
	{
	m_psp.dwFlags &= ~(PSP_HASHELP);
	//{{AFX_DATA_INIT(C4PageProgram)
	m_Language = _T("");
	//}}AFX_DATA_INIT
	SetData();
	}

C4PageProgram::~C4PageProgram()
	{
	}

void C4PageProgram::DoDataExchange(CDataExchange* pDX)
	{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(C4PageProgram)
	DDX_Control(pDX, IDC_STATICLANGINFO, m_StaticLanguageInfo);
	DDX_Control(pDX, IDC_COMBO_LANGUAGE, m_ComboLanguage);
	DDX_Control(pDX, IDC_STATICLANGUAGE, m_StaticLanguage);
	DDX_CBString(pDX, IDC_COMBO_LANGUAGE, m_Language);
	//}}AFX_DATA_MAP
	}

BOOL C4PageProgram::OnInitDialog() 
	{
	CPropertyPage::OnInitDialog();
	UpdateText();

	// Load name, info, fallbacks from available languages
	C4LanguageInfo *pInfo;
	for (int i = 0; pInfo = Languages.GetInfo(i); i++)
		{
		// Append separators
		if (i > 0) { m_LanguageNames += "|"; m_LanguageInfos += "|"; m_LanguageFallbacks += "|"; }
		// Apend language info
		m_LanguageNames += pInfo->Code; m_LanguageNames += " - "; m_LanguageNames += pInfo->Name;
		m_LanguageInfos += pInfo->Info;
		m_LanguageFallbacks += pInfo->Fallback;
		}

	// Fill language combo list
	char strLanguage[128 + 1];
	for (int j = 0; SCopySegment(m_LanguageNames, j, strLanguage, '|', 128); j++)
		m_ComboLanguage.AddString(strLanguage);

	// Update language info
	UpdateLanguageInfo();

	return TRUE;
	}
	
void C4PageProgram::UpdateText()
	{
	SetWindowText(LoadResStr("IDS_DLG_PROGRAM"));
	m_StaticLanguage.SetWindowText(LoadResStr("IDS_CTL_LANGUAGE"));
	}

void C4PageProgram::SetData()
	{
	// Language
	m_Language = GetCfg()->General.Language;
	}

void C4PageProgram::GetData()
	{
	// Store language user selection
	SCopy(m_Language, GetCfg()->General.Language, CFG_MaxString);
	// Compose language fallback list from user selection and language defined fallbacks
	SCopy(GetLanguageFallbacks(), GetCfg()->General.LanguageEx, CFG_MaxString);
	// Store charset for selected language
	C4LanguageInfo *pLanguage = Languages.FindInfo(m_Language);
	if (pLanguage) SCopy(pLanguage->Charset, GetCfg()->General.LanguageCharset);

	}

void C4PageProgram::OnSelEndOkComboLanguage() 
{
	// Get currently selected language
	m_ComboLanguage.GetLBText(m_ComboLanguage.GetCurSel(), m_Language);
	// Update language info
	UpdateLanguageInfo();
	// Third party language
	if (!SEqual2(m_Language, "DE") && !SEqual2(m_Language, "US"))
		// Display disclaimer (unless turned off)
		if (!GetCfg()->Explorer.DontShowLanguageDisclaimer)
			C4MessageBox("IDS_MSG_LANGUAGEDISCLAIMER", C4MsgBox_Ok, "IDS_MSG_DONTSHOW", (int *)&GetCfg()->Explorer.DontShowLanguageDisclaimer);
}

void C4PageProgram::UpdateLanguageInfo()
{
	// Find currently specified language in language list
	if (!m_Language.IsEmpty())
	{
		char strPack[2 + 1];
		for (int i = 0; SCopySegment(m_LanguageNames, i, strPack, '|', 2, true); i++)
			// Found a matching language pack
			if (SEqual2NoCase(m_Language, strPack, 2))
			{
				// Display language pack info
				char strLangInfo[1024 + 1];
				SCopySegment(m_LanguageInfos, i, strLangInfo, '|', 1024);
				m_StaticLanguageInfo.SetWindowText(strLangInfo);
				return;
			}
	}
	// No matching language pack found
	m_StaticLanguageInfo.SetWindowText(LoadResStr("IDS_CTL_NOLANGINFO"));
}

void C4PageProgram::OnEditChangeComboLanguage() 
{
	// Get currently selected language
	m_ComboLanguage.GetWindowText(m_Language);
	// Update language info
	UpdateLanguageInfo();
}

CString C4PageProgram::GetLanguageFallbacks()
{
	// Get string buffer
	CString strResult;
	char *strBuffer = strResult.GetBuffer(1024);
	// Process specified language 
	if (!m_Language.IsEmpty())
	{
		// Store user defined language code(s)
		GetCfg()->General.GetLanguageSequence(m_Language, strBuffer);
		// Find language pack for specified language
		char strPack[2 + 1];
		for (int i = 0; SCopySegment(m_LanguageNames, i, strPack, '|', 2, true); i++)
			// Found a matching language pack
			if (SEqual2NoCase(m_Language, strPack, 2))
			{
				// Get language pack fallback sequence
				char strFallbacks[1024 + 1];
				SCopySegment(m_LanguageFallbacks, i, strFallbacks, '|', 1024);
				// Fallbacks defined: append to sequence
				if (strFallbacks[0])
				{
					SAppendChar(',', strBuffer);
					GetCfg()->General.GetLanguageSequence(strFallbacks, strBuffer + SLen(strBuffer));
				}
				break;
			}
	}
	// Internal fallbacks: if not already in sequence, always append US and DE for safety
	if (!SSearch(strBuffer, "US"))
	{
		if (strBuffer[0]) SAppendChar(',', strBuffer);
		SAppend("US", strBuffer);
	}
	if (!SSearch(strBuffer, "DE"))
	{
		if (strBuffer[0]) SAppendChar(',', strBuffer);
		SAppend("DE", strBuffer);
	}
	// Release string buffer and return string
	strResult.ReleaseBuffer();
	return strResult;
}
