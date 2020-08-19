/*
 * LegacyClonk
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2010-2016, The OpenClonk Team and contributors
 * Copyright (c) 2018-2019, The LegacyClonk Team and contributors
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

#include "C4Include.h"
#include "C4StartupAboutDlg.h"

#include "C4Version.h"
#include "C4GraphicsResource.h"
#include "C4UpdateDlg.h"

#include <fstream>
#include <sstream>

namespace
{
	constexpr auto COPYING =
		#include "generated/COPYING.h"
	;

	constexpr auto TRADEMARK =
		#include "generated/TRADEMARK.h"
	;
}

enum {
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

namespace {
	constexpr auto SEE_LGPL = "See LGPL.txt";
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

		for (auto& p : developers)
		{
			textbox->AddTextLine(p.nick ? FormatString("%s <c f7f76f>(%s)</c>", p.name, p.nick).getData() : p.name, &font, C4GUI_MessageFontClr, false, true);
		}
	}

	std::string ToString(bool newline = true, bool with_color = false) override
	{
		const char *opening_tag = with_color ? "<c f7f76f>" : "";
		const char *closing_tag = with_color ? "</c>" : "";
		std::stringstream out;
		for (auto& p : developers)
		{
			out << p.name;
			if (p.nick)
			{
				out << opening_tag << " (" << p.nick << ")" << closing_tag;
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
	{"Martin Plicht", "Mortimer"}
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
	{"Viktor Yuschuk", "Viktor"}
},
additionalArt =
{
	{"Erik Nitzschke", "DukeAufDune"},
	{"Merten Ehmig", "pluto"},
	{"Matthias Rottl\xe4nder", "Matthi"},
	{"Christopher Reimann", "Benzol"},
	{"Jonathan Veit", "AniProGuy"},
	{"Arthur M\xf6ller", "Aqua"},
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
},
libs =
{
	{"zlib", "Jean-Loup Gailly, Mark Adler"},
	{"libpng", "Glenn Randers-Pehrson"},
	{"jpeglib", "Independent JPEG Group"},
	{"fmod", "Firelight Multimedia"},
	{"freetype", "The FreeType Project"},
	{"Allegro", "Shawn Hargreaves"},
	{"OpenSSL", "See OpenSSL.txt"},
	{"GTK+", SEE_LGPL},
	{"SDL", SEE_LGPL},
	{"SDL_mixer", SEE_LGPL}
};

/*static struct ContributorList : public PersonList
{
	static const std::vector<Entry>contributors, packageMaintainers;

	std::string ConcatNames(const std::vector<Entry>& names, bool with_color)
	{
		const char *opening_tag = with_color ? "<c f7f76f>" : "";
		const char *closing_tag = with_color ? "</c>" : "";
		std::stringstream result;
		bool first = true;
		for (auto& p : names)
		{
			if (!first) result << ", ";
			first = false;
			if (p.nick)
				result << p.name << " " << opening_tag << "(" << p.nick << ")" << closing_tag;
			else
				result << p.name;
		}
		return result.str();
	}

	template<typename Func>
	std::string WriteLines(Func f, bool with_color)
	{
		const char *opening_tag = with_color ? "<c ff0000>" : "";
		const char *closing_tag = with_color ? "</c>" : "";

		std::stringstream text;
		text << opening_tag << "Contributors: " << closing_tag;
		text << ConcatNames(contributors, with_color);
		f(text);

		text << opening_tag << "Package maintainers: " << closing_tag;
		text << ConcatNames(packageMaintainers, with_color);
		f(text);

		return text.str();
	}

	void WriteTo(C4GUI::TextWindow *textbox, CStdFont &font)
	{
		WriteLines([&](std::stringstream& text)
		{
			textbox->AddTextLine(text.str().c_str(), &font, C4GUI_MessageFontClr, false, true);
			text.str("");
		}, true);
	}

	std::string ToString()
	{
		return WriteLines([&](std::stringstream& text)
		{
			text << "\n";
		}, false);
	}
};*

// First real names sorted by last name (sort -k2), then nicks (sort)
const std::vector<ContributorList::Entry> ContributorList::contributors = {
};

const std::vector<ContributorList::Entry> ContributorList::packageMaintainers = {
};*/

template<int32_t left, int32_t top, int32_t right, int32_t bottom>
class CustomMarginTextWindow : public C4GUI::TextWindow {
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

// ------------------------------------------------
// --- C4StartupAboutDlg

C4StartupAboutDlg::C4StartupAboutDlg() : C4StartupDlg(LoadResStr("IDS_DLG_ABOUT"))
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
	AddElement(btn = new C4GUI::CallbackButton<C4StartupAboutDlg>(LoadResStr("IDS_BTN_BACK"), caButtons.GetGridCell(0,3,0,1,iButtonWidth,C4GUI_ButtonHgt,true), &C4StartupAboutDlg::OnBackBtn));
	btn->SetToolTip(LoadResStr("IDS_DLGTIP_BACKMAIN"));
	AddElement(btn = new C4GUI::CallbackButton<C4StartupAboutDlg>(LoadResStr("IDS_BTN_CHECKFORUPDATES"), caButtons.GetGridCell(2,4,0,1,iButtonWidth,C4GUI_ButtonHgt,true), &C4StartupAboutDlg::OnUpdateBtn));
	btn->SetToolTip(LoadResStr("IDS_DESC_CHECKONLINEFORNEWVERSIONS"));
	AddElement(btnAdvance = new C4GUI::CallbackButton<C4StartupAboutDlg>(LoadResStr("IDS_BTN_LICENSES"),
		caButtons.GetGridCell(3,4,0,1,iButtonWidth,C4GUI_ButtonHgt,true), &C4StartupAboutDlg::OnAdvanceButton));

	using ElementVector = std::vector<std::pair<C4GUI::TextWindow *, C4GUI::Label *>>;
	ElementVector page1;

	C4GUI::ComponentAligner caDevelopers(caMain.GetAll(), 0,0, false);

	C4GUI::ComponentAligner caDevelopersCol1(caDevelopers.GetFromLeft(caMain.GetWidth()*1/3), 0, 0, false);
	page1.emplace_back(DrawPersonList(gameDesign, "Game Design", caDevelopersCol1.GetFromTop(caDevelopersCol1.GetHeight()*1/5)));
	page1.emplace_back(DrawPersonList(code, "Engine and Tools", caDevelopersCol1.GetAll()));

	C4GUI::ComponentAligner caDevelopersCol2(caDevelopers.GetFromLeft(caMain.GetWidth()*1/3), 0,0, false);
	page1.emplace_back(DrawPersonList(scripting, "Scripting", caDevelopersCol2.GetFromTop(caDevelopersCol2.GetHeight()*2/3)));
	page1.emplace_back(DrawPersonList(additionalArt, "Additional Art", caDevelopersCol2.GetAll()));

	C4GUI::ComponentAligner caDevelopersCol3(caDevelopers.GetFromLeft(caMain.GetWidth()*1/3), 0,0, false);
	page1.emplace_back(DrawPersonList(music, "Music", caDevelopersCol3.GetFromTop(caDevelopersCol3.GetHeight()*1/3)));
	page1.emplace_back(DrawPersonList(voice, "Voice", caDevelopersCol3.GetFromTop(caDevelopersCol3.GetHeight()*3/10)));
	page1.emplace_back(DrawPersonList(web, "Web", caDevelopersCol3.GetAll()));

	ElementVector page2;

	C4GUI::ComponentAligner caLicenses(caMain.GetAll(), 0,0, false);
	page2.emplace_back(DrawPersonList(libs, "Libraries", caLicenses.GetFromLeft(caLicenses.GetWidth() / 4)));

	C4Rect rect1, rect2;
	if (Config.Graphics.ResX >= 1280)
	{
		rect1 = caLicenses.GetFromLeft(caLicenses.GetWidth() / 2);
		rect2 = caLicenses.GetAll();
	}
	else
	{
		C4GUI::ComponentAligner licenseTexts(caLicenses.GetAll(), 0, 0, false);
		rect1 = licenseTexts.GetFromTop(licenseTexts.GetHeight() / 2);
		rect2 = licenseTexts.GetAll();
	}

	page2.emplace_back(CreateTextWindowWithText(rect1, COPYING, "COPYING"));
	page2.emplace_back(CreateTextWindowWithText(rect2, TRADEMARK, "TRADEMARK"));

	aboutPages.emplace_back(page1);
	aboutPages.emplace_back(page2);
	SwitchPage(0);
}

C4StartupAboutDlg::~C4StartupAboutDlg() = default;

std::pair<C4GUI::TextWindow *, C4GUI::Label *> C4StartupAboutDlg::CreateTextWindowWithText(C4Rect &rect, const std::string &text, const std::string &title)
{
	CStdFont &rUseFont = C4GUI::GetRes()->TextFont;
	CStdFont &captionFont = C4GUI::GetRes()->TitleFont;
	C4GUI::Label *label = nullptr;
	if (!title.empty())
	{
		int height = captionFont.GetLineHeight();
		label = DrawCaption(rect, title.c_str());
		rect.y += height; rect.Hgt -= height;
	}

	auto textbox = new CustomMarginTextWindow<0, 8, 0, 8>(rect, 0, 0, 0, 100, 8000, "", true, nullptr, 0, true);
	AddElement(textbox);
	textbox->SetDecoration(false, false, nullptr, true);

	std::stringstream str{text};
	for (std::string line; std::getline(str, line, '\n'); )
	{
		textbox->AddTextLine(line.c_str(), &rUseFont, C4GUI_MessageFontClr, false, true);
	}
	textbox->UpdateHeight();

	return {textbox, label};
}

std::pair<C4GUI::TextWindow *, C4GUI::Label *> C4StartupAboutDlg::DrawPersonList(PersonList &persons, const char *title, C4Rect &rect, uint8_t flags)
{
	return CreateTextWindowWithText(rect, persons.ToString(!(flags & PERSONLIST_NONEWLINE), true), flags & PERSONLIST_NOCAPTION ? "" : title);
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
	currentPage = number;
	for (size_t i = 0; i < aboutPages.size(); ++i) // index is needed
	{
		for (auto &p : aboutPages[i])
		{
			if (p.first)
				p.first->SetVisibility(currentPage == i);

			if (p.second)
				p.second->SetVisibility(currentPage == i);
		}
	}

	btnAdvance->SetVisibility(currentPage != aboutPages.size() - 1);
}

void C4StartupAboutDlg::OnUpdateBtn(C4GUI::Control *btn)
{
	C4UpdateDlg::CheckForUpdates(GetScreen());
}
