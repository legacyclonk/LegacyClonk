/*
 * LegacyClonk
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2010-2016, The OpenClonk Team and contributors
 * Copyright (c) 2018-2021, The LegacyClonk Team and contributors
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

// Credits screen

#include "C4GuiListBox.h"
#include "C4GuiResource.h"
#include "C4Include.h"
#include "C4StartupAboutDlg.h"

#include "C4Version.h"
#include "C4GraphicsResource.h"
#include "C4UpdateDlg.h"

#include <initializer_list>
#include <sstream>
#include <utility>

enum
{
	PERSONLIST_NOCAPTION = 1 << 0,
	PERSONLIST_NONEWLINE = 1 << 1
};

struct PersonList
{
	struct Entry
	{
		const char *name, *nick;
	};
	const char *title;
	virtual void WriteTo(C4GUI::TextWindow *textbox, CStdFont &font, bool newline = true) = 0;
	virtual std::string ToString(bool newline = true, bool with_color = false) = 0;
	virtual ~PersonList() { }
};

static struct DeveloperList : public PersonList
{
	std::vector<Entry> developers;

	DeveloperList(std::initializer_list<Entry> l) : developers(l) { }

	void WriteTo(C4GUI::TextWindow *textbox, CStdFont &font, bool newline = true) override
	{
		if (!newline)
		{
			textbox->AddTextLine(ToString(false, true).c_str(), &font, C4GUI_MessageFontClr, false, true);
			return;
		}

		for (auto &p : developers)
		{
			textbox->AddTextLine(p.nick ? (p.name ? std::format("{} <c f7f76f>({})</c>", p.name, p.nick).c_str() : std::format("<c f7f76f>{}</c>", p.nick).c_str()) : p.name, &font, C4GUI_MessageFontClr, false, true);
		}
	}

	std::string ToString(bool newline = true, bool with_color = false) override
	{
		const char *opening_tag = with_color ? "<c f7f76f>" : "";
		const char *closing_tag = with_color ? "</c>" : "";
		std::stringstream out;
		for (auto &p : developers)
		{
			if (p.name)
			{
				out << p.name;
				if (p.nick)
				{
					out << opening_tag << " (" << p.nick << ")" << closing_tag;
				}
			}
			else
			{
				out << opening_tag << p.nick << closing_tag;
			}
			out << (newline ? "\n" : ", ");
		}
		return out.str();
	}
}

gameDesign =
{
	{"Matthes Bender", "matthes"},
},
code =
{
	{"Sven Eberhardt", "Sven2"},
	{"Peter Wortmann", "PeterW"},
	{"G\xfcnther Brammer", "G\xfcnther"},
	{"Armin Burgmeier", "Clonk-Karl"},
	{"Julian Raschke", "survivor"},
	{"Alexander Post", "qualle"},
	{"Jan Heberer", "Jan"},
	{"Markus Mittendrein", "Der Tod"},
	{"Dominik Bayerl", "Kanibal"},
	{"George Tokmaji", "Fulgen"},
	{"Martin Plicht", "Mortimer"},
	{"Matthias Brehmer", "Bratkartoffl"},
	{"Tim Kuhrt", "TLK"}
},
scripting =
{
	{"Felix Wagner", "Clonkonaut"},
	{"Richard Gerum", "Randrian"},
	{"Markus Hoppe", "Shamino"},
	{"David Dormagen", "Zapper"},
	{"Florian Gro\xdf", "flgr"},
	{"Tobias Zwick", "Newton"},
	{"Bernhard Bonigl", "boni"},
	{"Viktor Yuschuk", "Viktor"},
	{nullptr, "Raven"}
},
additionalArt =
{
	{"Erik Nitzschke", "DukeAufDune"},
	{"Merten Ehmig", "pluto"},
	{"Matthias Rottl\xe4nder", "Matthi"},
	{"Christopher Reimann", "Benzol"},
	{"Jonathan Veit", "AniProGuy"},
	{"Arthur M\xf6ller", "Aqua"},
	{"Tobias Zwick", "Newton"},
	{nullptr, "Raven"}
},
music =
{
	{"Hans-Christan K\xfchl", "HCK"},
	{"Sebastian Burkhart", "hypo"},
	{"Florian Boos", "Flobby"},
	{"Martin Strohmeier", "K-Pone"}
},
voice =
{
	{"Klemens K\xf6hring", nullptr}
},
web =
{
	{"Markus Wichitill", "mawic"},
	{"Martin Schuster", "knight_k"},
	{"Arne Bochem", "ArneB"},
	{"Lukas Werling", "Luchs"},
	{"Florian Graier", "Nachtfalter"},
	{"Benedict Etzel", "B_E"}
};

template<int32_t left, int32_t top, int32_t right, int32_t bottom>
class CustomMarginTextWindow : public C4GUI::TextWindow
{
public:
	CustomMarginTextWindow(C4Rect &rtBounds, size_t iPicWdt = 0, size_t iPicHgt = 0, size_t iPicPadding = 0, size_t iMaxLines = 100, size_t iMaxTextLen = 4096, const char *szIndentChars = "    ", bool fAutoGrow = false, const C4Facet *pOverlayPic = nullptr, int iOverlayBorder = 0, bool fMarkup = false) : C4GUI::TextWindow{rtBounds, iPicWdt, iPicHgt, iPicPadding, iMaxLines, iMaxTextLen, szIndentChars, fAutoGrow, pOverlayPic, iOverlayBorder, fMarkup}
	{
		UpdateSize();
	}

	virtual int32_t GetMarginTop() override    { return top; }
	virtual int32_t GetMarginLeft() override   { return left; }
	virtual int32_t GetMarginRight() override  { return right; }
	virtual int32_t GetMarginBottom() override { return bottom; }
};

namespace
{
	void AddTextWindowLines(C4GUI::TextWindow *textbox, const std::string &text)
	{
		CStdFont &rUseFont = C4GUI::GetRes()->TextFont;
		std::stringstream str{text};
		for (std::string line; std::getline(str, line, '\n'); )
		{
			textbox->AddTextLine(line.c_str(), &rUseFont, C4GUI_MessageFontClr, false, true);
		}
		textbox->UpdateHeight();
	}

	struct License
	{
		std::string title;
		std::string licenseTitle;
		std::string text;
	};

	std::vector<License> licenses
	{
		#include "generated/licenses.h"
	};

	class LicenseWindow : public C4GUI::Window
	{
	public:
		LicenseWindow(const C4Rect &bounds, const std::vector<License> &licenses)
		{
			SetBounds(bounds);
			C4GUI::ComponentAligner aligner{GetClientRect(), 0, 10};
			tabList = new C4GUI::ListBox{aligner.GetFromLeft(aligner.GetWidth() / 5)};
			tabList->SetSelectionChangeCallbackFn(new C4GUI::CallbackHandler<LicenseWindow>{this, &LicenseWindow::ChangeTab});
			AddElement(tabList);

			textWindow = new C4GUI::TextWindow{aligner.GetAll(), 0, 0, 0, 1000, 50000, ""};
			AddElement(textWindow);

			for (const auto &license : licenses)
			{
				tabList->AddElement(new LicenseTab{license.title, license.licenseTitle, license.text});
			}
			tabList->SelectFirstEntry(false);
		}

	private:
		void ChangeTab(C4GUI::Element *element)
		{
			if (!element) return;

			const auto *const tab = static_cast<LicenseTab *>(element);
			std::string title{tab->GetTitle()};
			const auto &licenseTitle = tab->GetLicenseTitle();
			if (!licenseTitle.empty())
			{
				title += " (";
				title += licenseTitle;
				title += ")";
			}
			textWindow->ClearText(false);
			textWindow->AddTextLine(title.c_str(), &C4GUI::GetRes()->TitleFont, C4GUI_Caption2FontClr, false, false);
			AddTextWindowLines(textWindow, tab->GetLicenseText());
		}

		C4GUI::ListBox *tabList;
		C4GUI::TextWindow *textWindow;

		class LicenseTab : public C4GUI::Label
		{
		public:
			LicenseTab(std::string title, std::string licenseTitle, std::string licenseText)
			: C4GUI::Label{title.c_str(), 0, 0}, title{std::move(title)}, licenseTitle{std::move(licenseTitle)}, licenseText{std::move(licenseText)} {}
			const std::string &GetTitle() const noexcept { return title; }
			const std::string &GetLicenseTitle() const noexcept { return licenseTitle; }
			const std::string &GetLicenseText() const noexcept { return licenseText; }

		private:
			std::string title;
			std::string licenseTitle;
			std::string licenseText;
		};
	};
}

// C4StartupAboutDlg

C4StartupAboutDlg::C4StartupAboutDlg() : C4StartupDlg(LoadResStr(C4ResStrTableKey::IDS_DLG_ABOUT))
{
	// ctor
	UpdateSize();

	CStdFont &rTrademarkFont = C4GUI::GetRes()->MiniFont;
	C4Rect rcClient = GetContainedClientRect();
	// bottom line buttons and copyright messages
	C4GUI::ComponentAligner caMain(rcClient, 0,0, true);
	C4GUI::ComponentAligner caButtons(caMain.GetFromBottom(caMain.GetHeight()*1/8), 0,0, false);
	C4GUI::CallbackButton<C4StartupAboutDlg> *btn;

	AddElement(new C4GUI::Label(FANPROJECTTEXT "   " TRADEMARKTEXT,
		caButtons.GetFromBottom(rTrademarkFont.GetLineHeight()), ARight, 0xffffffff, &rTrademarkFont));

	int32_t iButtonWidth = caButtons.GetInnerWidth() / 4;
	AddElement(btn = new C4GUI::CallbackButton<C4StartupAboutDlg>(LoadResStr(C4ResStrTableKey::IDS_BTN_BACK), caButtons.GetGridCell(0,3,0,1,iButtonWidth,C4GUI_ButtonHgt,true), &C4StartupAboutDlg::OnBackBtn));
	btn->SetToolTip(LoadResStr(C4ResStrTableKey::IDS_DLGTIP_BACKMAIN));
	AddElement(btn = new C4GUI::CallbackButton<C4StartupAboutDlg>(LoadResStr(C4ResStrTableKey::IDS_BTN_CHECKFORUPDATES), caButtons.GetGridCell(2,4,0,1,iButtonWidth,C4GUI_ButtonHgt,true), &C4StartupAboutDlg::OnUpdateBtn));
	btn->SetToolTip(LoadResStr(C4ResStrTableKey::IDS_DESC_CHECKONLINEFORNEWVERSIONS));
	AddElement(btnAdvance = new C4GUI::CallbackButton<C4StartupAboutDlg>(LoadResStr(C4ResStrTableKey::IDS_BTN_LICENSES),
		caButtons.GetGridCell(3,4,0,1,iButtonWidth,C4GUI_ButtonHgt,true), &C4StartupAboutDlg::OnAdvanceButton));

	using ElementVector = decltype(aboutPages)::value_type;
	ElementVector page1;

	C4GUI::ComponentAligner caDevelopers(caMain.GetAll(), 0,0, false);

	C4GUI::ComponentAligner caDevelopersCol1(caDevelopers.GetFromLeft(caMain.GetWidth()*1/3), 0, 0, false);
	DrawPersonList(page1, gameDesign, "Game Design", caDevelopersCol1.GetFromTop(caDevelopersCol1.GetHeight()*1/5));
	DrawPersonList(page1, code, "Engine and Tools", caDevelopersCol1.GetAll());

	C4GUI::ComponentAligner caDevelopersCol2(caDevelopers.GetFromLeft(caMain.GetWidth()*1/3), 0,0, false);
	DrawPersonList(page1, scripting, "Scripting", caDevelopersCol2.GetFromTop(caDevelopersCol2.GetHeight()*1/2));
	DrawPersonList(page1, additionalArt, "Additional Art", caDevelopersCol2.GetAll());

	C4GUI::ComponentAligner caDevelopersCol3(caDevelopers.GetFromLeft(caMain.GetWidth()*1/3), 0,0, false);
	DrawPersonList(page1, music, "Music", caDevelopersCol3.GetFromTop(caDevelopersCol3.GetHeight()*1/3));
	DrawPersonList(page1, voice, "Voice", caDevelopersCol3.GetFromTop(caDevelopersCol3.GetHeight()*3/10));
	DrawPersonList(page1, web, "Web", caDevelopersCol3.GetAll());

	aboutPages.emplace_back(std::move(page1));

	auto licenseWindow = new LicenseWindow{caMain.GetAll(), licenses};
	AddElement(licenseWindow);
	aboutPages.push_back({licenseWindow});

	for (uint32_t page = 1; page < aboutPages.size(); ++page)
	{
		SetPageVisibility(page, false);
	}
}

void C4StartupAboutDlg::AddLicense(std::string title, std::string licenseTitle, std::string licenseText)
{
	licenses.push_back({std::move(title), std::move(licenseTitle), std::move(licenseText)});
}

void C4StartupAboutDlg::CreateTextWindowWithText(std::vector<C4GUI::Element *> &page, C4Rect &rect, const std::string &text, const std::string &title)
{
	CStdFont &captionFont = C4GUI::GetRes()->TitleFont;
	if (!title.empty())
	{
		int height = captionFont.GetLineHeight();
		page.push_back(DrawCaption(rect, title.c_str()));
		rect.y += height; rect.Hgt -= height;
	}

	auto textbox = new CustomMarginTextWindow<0, 8, 0, 8>(rect, 0, 0, 0, 100, 8000, "", true, nullptr, 0, true);
	AddElement(textbox);
	textbox->SetDecoration(false, false, nullptr, true);

	AddTextWindowLines(textbox, text);
	page.push_back(textbox);
}

void C4StartupAboutDlg::DrawPersonList(std::vector<C4GUI::Element *> &page, PersonList &persons, const char *title, C4Rect &rect, uint8_t flags)
{
	CreateTextWindowWithText(page, rect, persons.ToString(!(flags & PERSONLIST_NONEWLINE), true), flags & PERSONLIST_NOCAPTION ? "" : title);
}

C4GUI::Label *C4StartupAboutDlg::DrawCaption(C4Rect &rect, const char *text)
{
	CStdFont &captionFont = C4GUI::GetRes()->CaptionFont;
	auto caption = new C4GUI::Label(text, rect, ALeft, C4GUI_Caption2FontClr, &captionFont);
	AddElement(caption);
	return caption;
}

void C4StartupAboutDlg::DoBack()
{
	C4Startup::Get()->SwitchDialog(C4Startup::SDID_Main);
}

void C4StartupAboutDlg::DrawElement(C4FacetEx &cgo)
{
	DrawBackground(cgo, C4Startup::Get()->Graphics.fctAboutBG);
}

void C4StartupAboutDlg::SwitchPage(uint32_t number)
{
	SetPageVisibility(currentPage, false);
	currentPage = number;
	SetPageVisibility(currentPage, true);
	btnAdvance->SetVisibility(currentPage != aboutPages.size() - 1);
}

void C4StartupAboutDlg::SetPageVisibility(uint32_t number, bool visible)
{
	for (auto *element : aboutPages[number])
	{
		element->SetVisibility(visible);
	}
}

void C4StartupAboutDlg::OnUpdateBtn(C4GUI::Control *btn)
{
	C4UpdateDlg::CheckForUpdates(GetScreen());
}
