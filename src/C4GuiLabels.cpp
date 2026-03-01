/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2001, Sven2
 * Copyright (c) 2017-2026, The LegacyClonk Team and contributors
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

// generic user interface
// eye candy

#include <C4Include.h>
#include <C4Gui.h>
#include <C4FullScreen.h>
#include <C4LoaderScreen.h>
#include "C4GuiResource.h"
#include "C4OpenURL.h"
#include <C4Application.h>

#include <cassert>
#include <compare>
#include <utility>

namespace C4GUI
{

// Label

void Label::DrawElement(C4FacetEx &cgo)
{
	// print out
	lpDDraw->TextOut(sText.getData(), *pFont, 1.0f, cgo.Surface, x0 + cgo.TargetX, rcBounds.y + cgo.TargetY, dwFgClr, iAlign, fMarkup);
	if (sHyperlink.getData())
	{
		int32_t iLinkWdt = 10, iLinkHgt = 10;
		pFont->GetTextExtent(sText.getData(), iLinkWdt, iLinkHgt, fMarkup);
		lpDDraw->DrawLineDw(cgo.Surface,
			static_cast<float>(rcBounds.x + cgo.TargetX),
			static_cast<float>(rcBounds.y + iLinkHgt - 2 + cgo.TargetY),
			static_cast<float>(rcBounds.x + iLinkWdt + cgo.TargetX),
			static_cast<float>(rcBounds.y + iLinkHgt - 2 + cgo.TargetY),
			C4GUI_HyperlinkFontClr & 0xffffff
		);
	}
}

Label::Label(std::string_view lblText, int32_t iX0, int32_t iTop, int32_t iAlign, uint32_t dwFClr, CStdFont *pFont, bool fMakeReadableOnBlack, bool fMarkup)
	: Element(), dwFgClr(dwFClr), x0(iX0), iAlign(iAlign), pFont(pFont), cHotkey(0), fAutosize(true), fMarkup(fMarkup), pClickFocusControl(nullptr)
{
	// make color readable
	if (fMakeReadableOnBlack) MakeColorReadableOnBlack(dwFgClr);
	// default font
	if (!this->pFont) this->pFont = &GetRes()->TextFont;
	// set top
	rcBounds.y = iTop;
	// update text
	SetText(lblText);
}

Label::Label(std::string_view lblText, const C4Rect &rcBounds, int32_t iAlign, uint32_t dwFClr, CStdFont *pFont, bool fMakeReadableOnBlack, bool fAutosize, bool fMarkup)
	: Element(), dwFgClr(dwFClr), iAlign(iAlign), pFont(pFont), cHotkey(0), fAutosize(fAutosize), fMarkup(fMarkup), pClickFocusControl(nullptr)
{
	// make color readable
	if (fMakeReadableOnBlack) MakeColorReadableOnBlack(dwFgClr);
	this->rcBounds = rcBounds;
	// default font
	if (!this->pFont) this->pFont = &GetRes()->TextFont;
	// set x0
	UpdateOwnPos();
	// update text
	SetText(lblText);
}

void Label::MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, uint32_t dwKeyParam)
{
	// left-click changes focus
	if (iButton == C4MC_Button_LeftDown)
	{
		if (pClickFocusControl) GetDlg()->SetFocus(pClickFocusControl, true);
		// left click opens URL
		if (sHyperlink.getData()) OpenURL(sHyperlink.getData());
	}
	// inherited
	Element::MouseInput(rMouse, iButton, iX, iY, dwKeyParam);
}

void Label::SetText(const char *szText, bool fAllowHotkey)
{
	return SetText(szText ? std::string_view{szText} : std::string_view{}, fAllowHotkey);
}

void Label::SetText(std::string_view toText, bool fAllowHotkey)
{
	// set new text
	if (!toText.empty())
	{
		sText.Copy(toText.data(), toText.size());
		// expand hotkey markup
		if (fAllowHotkey && fMarkup) ExpandHotkeyMarkup(sText, cHotkey);
	}
	else
	{
		sText = "";
		cHotkey = 0;
	}
	// update according to text only if autosize label (not wooden)
	if (!fAutosize) return;
	// get text extents
	pFont->GetTextExtent(sText.getData(), rcBounds.Wdt, rcBounds.Hgt, fMarkup);
	// update pos
	SetX0(x0);
}

void Label::UpdateOwnPos()
{
	// update text drawing pos
	switch (iAlign)
	{
	case ALeft: x0 = rcBounds.x + GetLeftIndent(); break;
	case ACenter: x0 = rcBounds.x + rcBounds.Wdt / 2; break;
	case ARight: x0 = rcBounds.x + rcBounds.Wdt; break;
	}
}

bool Label::OnHotkey(char cHotkey)
{
	// if hotkey matches and focus control is assigned, set focus
	if (this->cHotkey == cHotkey && pClickFocusControl)
	{
		GetDlg()->SetFocus(pClickFocusControl, false);
		return true;
	}
	else return false;
}

void Label::SetX0(int32_t iToX0)
{
	x0 = iToX0;
	// update x-startpos
	switch (iAlign)
	{
	case ALeft: rcBounds.x = x0; break;
	case ACenter: rcBounds.x = x0 - rcBounds.Wdt / 2; break;
	case ARight: rcBounds.x = x0 - rcBounds.Wdt; break;
	}
	// size might have changed
	UpdateSize();
}

void Label::SetHyperlink(const char *szURL)
{
	if (szURL && *szURL)
	{
		sHyperlink.Copy(szURL);
		SetColor(C4GUI_HyperlinkFontClr, false);
	}
	else
		sHyperlink.Clear();
}

// WoodenLabel

void WoodenLabel::DrawElement(C4FacetEx &cgo)
{
	// draw wood
	DrawBar(cgo, GetRes()->barCaption);
	// draw symbol
	if (fctIcon.Surface)
	{
		C4Facet cgoSymbol(cgo.Surface, cgo.TargetX + rcBounds.x + 1, cgo.TargetY + rcBounds.y + 1, rcBounds.Hgt - 2, rcBounds.Hgt - 2);
		fctIcon.Draw(cgoSymbol);
	}
	// calculations for automatic scrolling
	int32_t iXOff = 0;
	if (iAlign == ALeft) iXOff += 5;
	if (tAutoScrollDelay)
	{
		time_t tNow = timeGetTime();
		if (!tLastChangeTime)
			tLastChangeTime = tNow;
		else if (tNow - tLastChangeTime >= tAutoScrollDelay)
		{
			if (!iScrollDir) iScrollDir = 1;
			int32_t iMaxScroll = std::max<int32_t>(pFont->GetTextWidth(sText.getData(), true) + (x0 - rcBounds.x) + iXOff + GetRightIndent() - rcBounds.Wdt, 0);
			if (iMaxScroll)
			{
				iScrollPos += iScrollDir;
				if (iScrollPos >= iMaxScroll || iScrollPos < 0)
				{
					iScrollDir = -iScrollDir;
					iScrollPos += iScrollDir;
					tLastChangeTime = tNow;
				}
			}
		}
		iXOff -= iScrollPos;
	}
	// print out text; clipped
	int ClipX1, ClipY1, ClipX2, ClipY2;
	lpDDraw->GetPrimaryClipper(ClipX1, ClipY1, ClipX2, ClipY2);
	lpDDraw->SetPrimaryClipper(rcBounds.x + GetLeftIndent() + cgo.TargetX, rcBounds.y + cgo.TargetY, rcBounds.x + rcBounds.Wdt - GetRightIndent() + cgo.TargetX, rcBounds.y + rcBounds.Hgt + cgo.TargetY);
	lpDDraw->TextOut(sText.getData(), *pFont, 1.0f, cgo.Surface, x0 + cgo.TargetX + iXOff, rcBounds.y + cgo.TargetY + (rcBounds.Hgt - pFont->GetLineHeight()) / 2 - 1, dwFgClr, iAlign);
	lpDDraw->SetPrimaryClipper(ClipX1, ClipY1, ClipX2, ClipY2);
}

int32_t WoodenLabel::GetDefaultHeight(CStdFont *pUseFont)
{
	if (!pUseFont) pUseFont = &(GetRes()->TextFont);
	return std::max<int32_t>(pUseFont->GetLineHeight(), C4GUI_MinWoodBarHgt);
}

void WoodenLabel::SetIcon(const C4Facet &rfctIcon)
{
	// set icon
	fctIcon = rfctIcon;
	// realign text to left for set icons
	if (fctIcon.Surface)
		iAlign = ALeft;
	else
		iAlign = ACenter;
	UpdateOwnPos();
}

// MultilineLabel

MultilineLabel::MultilineLabel(const C4Rect &rcBounds, int32_t iMaxLines, int32_t iMaxBuf, const char *szIndentChars, bool fAutoGrow, bool fMarkup)
	: Element(), Lines(iMaxBuf, iMaxLines, rcBounds.Wdt, szIndentChars, fAutoGrow, fMarkup), fMarkup(fMarkup)
{
	// set bounds
	this->rcBounds = rcBounds;
	// update height (min height)
	UpdateOwnPos();
}

void MultilineLabel::DrawElement(C4FacetEx &cgo)
{
	// get clipping
	int iClipX, iClipY, iClipX2, iClipY2;
	lpDDraw->GetPrimaryClipper(iClipX, iClipY, iClipX2, iClipY2);
	// draw all lines
	int32_t iIndex = 0; const char *szLine;
	int32_t iY = rcBounds.y + cgo.TargetY;
	CStdFont *pLineFont; uint32_t dwLineClr; bool fNewParagraph;
	while ((szLine = Lines.GetLine(iIndex, &pLineFont, &dwLineClr, &fNewParagraph)))
	{
		int32_t iFontLineHeight = pLineFont->GetLineHeight();
		// indents between paragraphs
		if (fNewParagraph && iIndex) iY += iFontLineHeight / 3;
		// clip
		if (iY > iClipY2) break;
		if (iY >= iClipY - iFontLineHeight)
		{
			// draw line
			lpDDraw->TextOut(szLine, *pLineFont, 1.0f, cgo.Surface, rcBounds.x + cgo.TargetX, iY, dwLineClr, ALeft, fMarkup);
		}
		// advance line
		iY += iFontLineHeight;
		++iIndex;
	}
}

void MultilineLabel::UpdateSize()
{
	// forward change to line buffer
	Lines.SetLBWidth(rcBounds.Wdt);
	UpdateHeight();
}

void MultilineLabel::UpdateHeight()
{
	// size by line count
	int32_t iIndex = 0; int32_t iHgt = 0;
	CStdFont *pLineFont; bool fNewPar;
	while (Lines.GetLine(iIndex, &pLineFont, nullptr, &fNewPar))
	{
		int32_t iFontLineHeight = pLineFont->GetLineHeight();
		// indents between separate messages
		if (fNewPar && iIndex) iHgt += iFontLineHeight / 3;
		// text line height
		iHgt += iFontLineHeight;
		++iIndex;
	}
	rcBounds.Hgt = std::max<int32_t>(iHgt, 5);
	// update parent container
	Element::UpdateSize();
}

void MultilineLabel::AddLine(const char *szLine, CStdFont *pFont, uint32_t dwClr, bool fDoUpdate, bool fMakeReadableOnBlack, CStdFont *pCaptionFont)
{
	// make color readable
	if (fMakeReadableOnBlack) MakeColorReadableOnBlack(dwClr);
	// forward to line buffer
	if (szLine) Lines.AppendLines(szLine, pFont, dwClr, pCaptionFont);
	// adjust height
	if (fDoUpdate) UpdateSize();
}

void MultilineLabel::Clear(bool fDoUpdate)
{
	// forward to line buffer
	Lines.Clear();
	// adjust height
	if (fDoUpdate) UpdateSize();
}

// HorizontalLine

void HorizontalLine::DrawElement(C4FacetEx &cgo)
{
	// draw horizontal line
	int32_t iX1 = rcBounds.x + cgo.TargetX, iX2 = iX1 + rcBounds.Wdt,
		iY = rcBounds.y + cgo.TargetY;
	lpDDraw->DrawLineDw(cgo.Surface, static_cast<float>(iX1 + 1), static_cast<float>(iY + 1), static_cast<float>(iX2 - 1), static_cast<float>(iY + 1), dwShadowClr);
	lpDDraw->DrawLineDw(cgo.Surface, static_cast<float>(iX1), static_cast<float>(iY), static_cast<float>(iX2 - 2), static_cast<float>(iY), dwClr);
}

// ProgressBar

void ProgressBar::DrawElement(C4FacetEx &cgo)
{
	// do not draw in negative progress
	if (iProgress < 0) return;
	CStdFont &rFont = GetRes()->TextFont;
	// draw border
	Draw3DFrame(cgo);
	// calc progress width
	int32_t iProgressWdt = (rcBounds.Wdt - 4) * iProgress / iMax;
	// draw progress
	GetRes()->fctProgressBar.DrawX(cgo.Surface, cgo.TargetX + rcBounds.x + 2, cgo.TargetY + rcBounds.y + 2, iProgressWdt, rcBounds.Hgt - 2);
	// print out progress text
	lpDDraw->TextOut((std::to_string(100 * iProgress / iMax) + "%").c_str(), rFont, 1.0f, cgo.Surface, cgo.TargetX + rcBounds.GetMiddleX(), rcBounds.y + cgo.TargetY + (rcBounds.Hgt - rFont.GetLineHeight()) / 2 - 1, C4GUI_ProgressBarFontClr, ACenter);
}

// Picture

Picture::Picture(const C4Rect &rcBounds, bool fAspect) : fCustomDrawClr(false), fAnimate(false)
{
	// set values
	this->fAspect = fAspect;
	this->rcBounds = rcBounds;
	// no facet yet
}

void Picture::DrawElement(C4FacetEx &cgo)
{
	// animation?
	C4Facet *pDrawFacet, DrawFacet;
	if (fAnimate)
	{
		if (++iPhaseTime > iDelay)
		{
			int32_t iPhasesX = 1, iPhasesY = 1;
			Facet.GetPhaseNum(iPhasesX, iPhasesY);
			if (++iAnimationPhase >= iPhasesX) iAnimationPhase = 0;
			iPhaseTime = 0;
		}
		DrawFacet = Facet.GetPhase(iAnimationPhase);
		pDrawFacet = &DrawFacet;
	}
	else
		pDrawFacet = &Facet;
	// draw the facet
	C4Facet cgo2 = cgo;
	cgo2.X = rcBounds.x + cgo.TargetX;
	cgo2.Y = rcBounds.y + cgo.TargetY;
	cgo2.Wdt = rcBounds.Wdt;
	cgo2.Hgt = rcBounds.Hgt;
	if (fCustomDrawClr)
	{
		pDrawFacet->DrawClr(cgo2, fAspect, dwDrawClr);
	}
	else
		pDrawFacet->Draw(cgo2, fAspect);
}

bool Picture::EnsureOwnSurface()
{
	// no surface?
	if (!Facet.Surface) return false;
	// equals face already?
	if (Facet.Surface == &Facet.GetFace()) return true;
	// then create as a copy
	C4Facet cgo = Facet;
	if (!Facet.Create(cgo.Wdt, cgo.Hgt)) return false;
	cgo.Draw(Facet);
	return true;
}

void Picture::SetAnimated(bool fEnabled, int iDelay)
{
	if ((fAnimate = fEnabled))
	{
		// starts cycling through all phases of the specified facet
		iAnimationPhase = iPhaseTime = 0;
		this->iDelay = iDelay;
	}
}

OverlayFrameSpec::OverlayFrameSpec(const C4Facet &texture, std::int32_t horizontalFrameSize, std::int32_t verticalFrameSize) noexcept
{
	const C4Facet top{texture.Surface, texture.X, texture.Y, texture.Wdt, verticalFrameSize};
	const C4Facet bottom{texture.Surface, texture.X, texture.Y + texture.Hgt - verticalFrameSize, texture.Wdt, verticalFrameSize};

	const auto verticalTileY = texture.Y + verticalFrameSize;
	const auto verticalTileHgt = texture.Hgt - 2 * verticalFrameSize;
	LeftTile = {texture.Surface, texture.X, verticalTileY, horizontalFrameSize, verticalTileHgt};
	RightTile = {texture.Surface, texture.X + texture.Wdt - horizontalFrameSize, verticalTileY, horizontalFrameSize, verticalTileHgt};
	Top.SetHorizontal(top, horizontalFrameSize);
	Bottom.SetHorizontal(bottom, horizontalFrameSize);
}

// OverlayPicture

OverlayPicture::OverlayPicture(const C4Rect &rcBounds, bool fAspect, const OverlayFrameSpec &rOverlayImage, int iBorderSize)
	: Picture(rcBounds, fAspect), iBorderSize(iBorderSize), OverlayImage(rOverlayImage) {}

void OverlayPicture::DrawElement(C4FacetEx &cgo)
{
	const auto targetWidth = rcBounds.Wdt;
	const auto targetHeight = rcBounds.Hgt;

	const auto innerTargetWidth = targetWidth - 2 * iBorderSize;
	const auto innerTargetHeight = targetHeight - 2 * iBorderSize;

	std::int32_t drawWidth{targetWidth};
	std::int32_t drawHeight{targetHeight};

	C4FacetEx overlayCgo = cgo;

	//    w1 : h1 <=> w2 : h2
	// => w1 * h2 <=> w2 * h1
	const auto aspectRatioRelation = Facet.Wdt * innerTargetHeight <=> innerTargetWidth * Facet.Hgt;
	if (!fAspect || std::is_eq(aspectRatioRelation))
	{
		// draw outer image
		overlayCgo.X = rcBounds.x;
		overlayCgo.Y = rcBounds.y;
		overlayCgo.Wdt = rcBounds.Wdt;
		overlayCgo.Hgt = rcBounds.Hgt;
	}
	else if (std::is_gt(aspectRatioRelation))	 // Scale height
	{
		drawHeight = Facet.Hgt * innerTargetWidth / Facet.Wdt + 2 * iBorderSize;
		const auto drawOffsetY = (targetHeight - drawHeight) / 2;

		overlayCgo.X = rcBounds.x;
		overlayCgo.Y = rcBounds.y + drawOffsetY;
		overlayCgo.Wdt = rcBounds.Wdt;
		overlayCgo.Hgt = drawHeight;
	}
	else if (std::is_lt(aspectRatioRelation)) //  Scale width
	{
		drawWidth = Facet.Wdt * innerTargetHeight / Facet.Hgt + 2 * iBorderSize;
		const auto drawOffsetX = (targetWidth - drawWidth) / 2;

		overlayCgo.X = rcBounds.x + drawOffsetX;
		overlayCgo.Y = rcBounds.y;
		overlayCgo.Wdt = drawWidth;
		overlayCgo.Hgt = rcBounds.Hgt;
	}
	else
	{
		assert(!"Aspect ratio comparison logic failed");
	}

	const auto minUnscaledOverlayWdt = OverlayImage.Top.fctBegin.Wdt + OverlayImage.Top.fctEnd.Wdt;
	const auto minUnscaledOverlayHgt = OverlayImage.Top.fctBegin.Hgt + OverlayImage.Bottom.fctBegin.Hgt;
	const auto [overlayScaleNumerator, overlayScaleDenominator] = [minUnscaledOverlayWdt, minUnscaledOverlayHgt, &overlayCgo]{

		const auto hgtScaleDenominator = std::max(minUnscaledOverlayHgt, overlayCgo.Hgt);
		const auto wdtScaleDenominator = std::max(minUnscaledOverlayWdt, overlayCgo.Wdt);

		//    w1 : h1 <=> w2 : h2
		// => w1 * h2 <=> w2 * h1
		if (wdtScaleDenominator * overlayCgo.Hgt < hgtScaleDenominator * overlayCgo.Wdt)
		{
			return std::pair{overlayCgo.Hgt, hgtScaleDenominator};
		}
		else
		{
			return std::pair{overlayCgo.Wdt, wdtScaleDenominator};
		}
	}();
	const auto scaleOverlay = [overlayScaleNumerator, overlayScaleDenominator](std::int32_t value, bool ceil = false)
	{
		return (value * overlayScaleNumerator + ceil * (overlayScaleDenominator - 1)) / overlayScaleDenominator;
	};

	const auto scaledBorderSize = scaleOverlay(iBorderSize, true);

	// draw inner image
	C4Facet innerCgo = cgo;
	innerCgo.X = overlayCgo.X + cgo.TargetX + scaledBorderSize;
	innerCgo.Y = overlayCgo.Y + cgo.TargetY + scaledBorderSize;
	innerCgo.Wdt = overlayCgo.Wdt - 2 * scaledBorderSize;
	innerCgo.Hgt = overlayCgo.Hgt - 2 * scaledBorderSize;
	Facet.Draw(innerCgo, true);


	if (!fAspect || std::is_eq(aspectRatioRelation))
	{
		// draw outer image
		overlayCgo.X = rcBounds.x;
		overlayCgo.Y = rcBounds.y;
		overlayCgo.Wdt = rcBounds.Wdt;
		overlayCgo.Hgt = rcBounds.Hgt;
	}
	else if (std::is_gt(aspectRatioRelation))	 // Scale height
	{
		drawHeight = innerCgo.Wdt * Facet.Hgt / Facet.Wdt + 2 * scaledBorderSize;
		const auto drawOffsetY = (targetHeight - drawHeight) / 2;

		overlayCgo.X = rcBounds.x;
		overlayCgo.Y = rcBounds.y + drawOffsetY;
		overlayCgo.Wdt = rcBounds.Wdt;
		overlayCgo.Hgt = drawHeight;
	}
	else if (std::is_lt(aspectRatioRelation)) //  Scale width
	{
		drawWidth = innerCgo.Hgt * Facet.Wdt / Facet.Hgt + 2 * scaledBorderSize;
		const auto drawOffsetX = (targetWidth - drawWidth) / 2;

		overlayCgo.X = rcBounds.x + drawOffsetX;
		overlayCgo.Y = rcBounds.y;
		overlayCgo.Wdt = drawWidth;
		overlayCgo.Hgt = rcBounds.Hgt;
	}


	C4FacetEx topBarCgo = overlayCgo;
	topBarCgo.Hgt = scaleOverlay(OverlayImage.Top.fctBegin.Hgt);

	C4FacetEx bottomBarCgo = overlayCgo;
	bottomBarCgo.Hgt = scaleOverlay(OverlayImage.Bottom.fctBegin.Hgt);
	bottomBarCgo.Y += overlayCgo.Hgt - bottomBarCgo.Hgt;

	C4FacetEx leftBarCgo = overlayCgo;
	leftBarCgo.Wdt = scaleOverlay(OverlayImage.LeftTile.Wdt);
	leftBarCgo.Y += topBarCgo.Hgt;
	leftBarCgo.Hgt = overlayCgo.Hgt - topBarCgo.Hgt - bottomBarCgo.Hgt;

	C4FacetEx rightBarCgo = overlayCgo;
	rightBarCgo.Wdt = scaleOverlay(OverlayImage.RightTile.Wdt);
	rightBarCgo.X += overlayCgo.Wdt - rightBarCgo.Wdt;
	rightBarCgo.Y += topBarCgo.Hgt;
	rightBarCgo.Hgt = overlayCgo.Hgt - topBarCgo.Hgt - bottomBarCgo.Hgt;

	OverlayImage.Top.Draw(topBarCgo);
	OverlayImage.Bottom.Draw(bottomBarCgo);
	OverlayImage.LeftTile.DrawVTile(leftBarCgo);
	OverlayImage.RightTile.DrawVTile(rightBarCgo);
}

// Icon

Icon::Icon(const C4Rect &rcBounds, Icons icoIconIndex)
	: Picture(rcBounds, true)
{
	// set icon facet
	SetIcon(icoIconIndex);
}

void Icon::SetIcon(Icons icoNewIconIndex)
{
	// load icon
	SetFacet(GetIconFacet(icoNewIconIndex));
}

C4FacetEx Icon::GetIconFacet(Icons icoIconIndex)
{
	if (icoIconIndex == Ico_None) return C4FacetEx();
	C4FacetEx &rFacet = (icoIconIndex & Ico_Extended) ? GetRes()->fctIconsEx : GetRes()->fctIcons;
	icoIconIndex = Icons(icoIconIndex & (Ico_Extended - 1));
	int32_t iXMax, iYMax;
	rFacet.GetPhaseNum(iXMax, iYMax);
	if (!iXMax) iXMax = 6;
	return rFacet.GetPhase(icoIconIndex % iXMax, icoIconIndex / iXMax);
}

// TextWindow

TextWindow::TextWindow(const C4Rect &rtBounds, size_t iPicWdt, size_t iPicHgt, size_t iPicPadding, size_t iMaxLines, size_t iMaxTextLen, const char *szIndentChars, bool fAutoGrow, const OverlayFrameSpec *pOverlayPic, int iOverlayBorder, bool fMarkup, bool keepPictureAspectRatio)
	: Control(rtBounds), pLogBuffer(nullptr), fDrawBackground(true), fDrawFrame(true), iPicPadding(iPicPadding), iPicWdt(iPicWdt), iPicHgt(iPicHgt), keepPictureAspectRatio(keepPictureAspectRatio)
{
	// calc client rect
	UpdateOwnPos();
	// create content scroll window
	pClientWindow = new ScrollWindow(this);
	pClientWindow->SetBounds(GetContainedClientRect());
	// create content multiline label
	pLogBuffer = new MultilineLabel(pClientWindow->GetContainedClientRect(), iMaxLines, iMaxTextLen, szIndentChars, fAutoGrow, fMarkup);
	// add to scroll window
	pClientWindow->AddElement(pLogBuffer);
	// update scrolling (for empty buffer)
	pClientWindow->SetClientHeight(1);
	// create content picture, if desired
	C4Rect rcContentSize = pClientWindow->GetClientRect();
	if (iPicWdt && iPicHgt)
	{
		C4Rect rcImage;
		rcImage.x = 0;
		rcImage.y = 0;
		rcImage.Wdt = rcContentSize.Wdt;
		if (keepPictureAspectRatio)
		{
			rcImage.Hgt = iPicHgt;
		}
		else
		{
			rcImage.Hgt = iPicHgt * rcImage.Wdt / iPicWdt;
		}
		rcContentSize.y += rcImage.Hgt + iPicPadding;
		if (pOverlayPic)
			pTitlePicture = new OverlayPicture(rcImage, keepPictureAspectRatio, *pOverlayPic, iOverlayBorder);
		else
			pTitlePicture = new Picture(rcImage, keepPictureAspectRatio);
		pClientWindow->AddElement(pTitlePicture);
	}
	else pTitlePicture = nullptr;

	// update size
	UpdateSize();
}

void TextWindow::UpdateSize()
{
	Control::UpdateSize();
	pClientWindow->SetBounds(GetContainedClientRect());
	// resize log buffer pos to horizontal extents
	C4Rect rcChildBounds = pLogBuffer->GetBounds();
	rcChildBounds.x = 0;
	rcChildBounds.y = pTitlePicture ? pTitlePicture->GetBounds().Hgt + iPicPadding : 0;
	rcChildBounds.Wdt = pClientWindow->GetClientRect().Wdt;
	pLogBuffer->SetBounds(rcChildBounds);

	C4Rect rcContentSize = pClientWindow->GetClientRect();
	if (pTitlePicture != nullptr)
	{
		C4Rect rcImage;
		rcImage.x = 0;
		rcImage.y = 0;
		rcImage.Wdt = rcContentSize.Wdt;

		if (keepPictureAspectRatio)
		{
			rcImage.Hgt = iPicHgt;
		}
		else
		{
			rcImage.Hgt = iPicHgt * rcImage.Wdt / iPicWdt;
		}
		rcContentSize.y += rcImage.Hgt + iPicPadding;
		pTitlePicture->SetBounds(rcImage);
	}
}

void TextWindow::DrawElement(C4FacetEx &cgo)
{
	// draw background
	if (fDrawBackground) lpDDraw->DrawBoxDw(cgo.Surface, cgo.TargetX + rcBounds.x, cgo.TargetY + rcBounds.y, rcBounds.x + rcBounds.Wdt - 1 + cgo.TargetX, rcBounds.y + rcBounds.Hgt - 1 + cgo.TargetY, 0x7f000000);
	// draw frame
	if (fDrawFrame) Draw3DFrame(cgo);
}

void TextWindow::ElementSizeChanged(Element *pOfElement)
{
	// inherited
	if (pOfElement->GetParent() == this)
		Control::ElementSizeChanged(pOfElement);
	// update size of scroll control
	if (pClientWindow && pLogBuffer)
		pClientWindow->SetClientHeight(pLogBuffer->GetBounds().y + pLogBuffer->GetBounds().Hgt);
}

void TextWindow::ElementPosChanged(Element *pOfElement)
{
	// inherited
	if (pOfElement->GetParent() == this)
		Control::ElementSizeChanged(pOfElement);
	// update size of scroll control
	if (pClientWindow && pLogBuffer)
		pClientWindow->SetClientHeight(pLogBuffer->GetBounds().y + pLogBuffer->GetBounds().Hgt);
}

void TextWindow::SetPicture(const C4Facet &rNewPic)
{
	// update picture
	if (!pTitlePicture) return;
	pTitlePicture->SetFacet(rNewPic);
	// reposition multiline label below picture if any is assigned
	pLogBuffer->GetBounds().y = rNewPic.Surface ? pTitlePicture->GetBounds().Hgt + iPicPadding : 0;
	pLogBuffer->UpdateOwnPos();
	pTitlePicture->SetVisibility(!!rNewPic.Surface);
}

} // end of namespace
