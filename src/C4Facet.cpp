/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
 * Copyright (c) 2017-2019, The LegacyClonk Team and contributors
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

/* A piece of a DirectDraw surface */

#include <C4Include.h>
#include <C4Facet.h>
#include <C4Game.h>

void C4Facet::Default()
{
	Set(nullptr, 0, 0, 0, 0);
}

void C4Facet::Set(C4Surface *nsfc, int32_t nx, int32_t ny, int32_t nwdt, int32_t nhgt)
{
	Surface = nsfc; X = nx; Y = ny; Wdt = nwdt; Hgt = nhgt;
}

C4Facet::C4Facet()
{
	Default();
}

int32_t C4Facet::GetSectionCount()
{
	if (Hgt == 0) return 0;
	return Wdt / Hgt;
}

C4Facet C4Facet::GetSection(int32_t iSection)
{
	C4Facet rval;
	rval.Set(Surface, X + Hgt * iSection, Y, Hgt, Hgt);
	return rval;
}

void C4Facet::Draw(C4Surface *sfcTarget, int32_t iX, int32_t iY, int32_t iPhaseX, int32_t iPhaseY, const float scale)
{
#ifdef C4ENGINE
	if (!lpDDraw || !Surface || !sfcTarget || !Wdt || !Hgt) return;

	lpDDraw->Blit(Surface,
		float(X + Wdt * iPhaseX) * scale, float(Y + Hgt * iPhaseY) * scale, float(Wdt) * scale, float(Hgt) * scale,
		sfcTarget,
		iX, iY, Wdt, Hgt, true);
#endif
}

#ifdef C4ENGINE

void C4Facet::DrawT(C4Surface *sfcTarget, int32_t iX, int32_t iY, int32_t iPhaseX, int32_t iPhaseY, C4DrawTransform *pTransform, bool noScalingCorrection, const float scale)
{
	if (!lpDDraw || !Surface || !sfcTarget || !Wdt || !Hgt) return;

	lpDDraw->Blit(Surface,
		float(X + Wdt * iPhaseX) * scale, float(Y + Hgt * iPhaseY) * scale, float(Wdt) * scale, float(Hgt) * scale,
		sfcTarget,
		iX, iY, Wdt, Hgt, true, pTransform, noScalingCorrection);
}

void C4Facet::DrawT(C4Facet &cgo, bool fAspect, int32_t iPhaseX, int32_t iPhaseY, C4DrawTransform *pTransform, bool noScalingCorrection, const float scale)
{
	if (!lpDDraw || !Surface || !cgo.Surface || !Wdt || !Hgt) return;

	// Drawing area
	C4Facet ccgo = cgo;
	// Adjust for fixed aspect ratio
	if (fAspect)
	{
		// By height
		if (100 * cgo.Wdt / Wdt < 100 * cgo.Hgt / Hgt)
		{
			ccgo.Hgt = Hgt * cgo.Wdt / Wdt;
			ccgo.Y += (cgo.Hgt - ccgo.Hgt) / 2;
		}
		// By width
		else if (100 * cgo.Hgt / Hgt < 100 * cgo.Wdt / Wdt)
		{
			ccgo.Wdt = Wdt * cgo.Hgt / Hgt;
			ccgo.X += (cgo.Wdt - ccgo.Wdt) / 2;
		}
	}

	lpDDraw->Blit(Surface,
		float(X + Wdt * iPhaseX) * scale, float(Y + Hgt * iPhaseY) * scale, float(Wdt) * scale, float(Hgt) * scale,
		ccgo.Surface, ccgo.X, ccgo.Y, ccgo.Wdt, ccgo.Hgt,
		true, pTransform, noScalingCorrection);
}

#endif // C4ENGINE

void C4Facet::Draw(C4Facet &cgo, bool fAspect, int32_t iPhaseX, int32_t iPhaseY, bool fTransparent, const float scale)
{
#ifdef C4ENGINE
	// Valid parameter check
	if (!lpDDraw || !Surface || !cgo.Surface || !Wdt || !Hgt) return;
	// Drawing area
	C4Facet ccgo = cgo;
	// Adjust for fixed aspect ratio
	if (fAspect)
	{
		// By height
		if (100 * cgo.Wdt / Wdt < 100 * cgo.Hgt / Hgt)
		{
			ccgo.Hgt = Hgt * cgo.Wdt / Wdt;
			ccgo.Y += (cgo.Hgt - ccgo.Hgt) / 2;
		}
		// By width
		else if (100 * cgo.Hgt / Hgt < 100 * cgo.Wdt / Wdt)
		{
			ccgo.Wdt = Wdt * cgo.Hgt / Hgt;
			ccgo.X += (cgo.Wdt - ccgo.Wdt) / 2;
		}
	}
	// Blit
	lpDDraw->Blit(Surface,
		float(X + Wdt * iPhaseX) * scale, float(Y + Hgt * iPhaseY) * scale, float(Wdt) * scale, float(Hgt) * scale,
		ccgo.Surface,
		ccgo.X, ccgo.Y, ccgo.Wdt, ccgo.Hgt,
		fTransparent);
#endif
}

void C4Facet::DrawFullScreen(C4Facet &cgo)
{
#ifdef C4ENGINE
	// stretched fullscreen blit: make sure right and lower side are cleared, because this may be missed due to stretching
	if (cgo.Wdt > Wdt + 2 || cgo.Hgt > Wdt + 2)
	{
		lpDDraw->DrawBoxDw(cgo.Surface, cgo.X, cgo.Y + cgo.Hgt - 1, cgo.X + cgo.Wdt + 2, cgo.Y + cgo.Hgt + 2, 0x00000000);
		lpDDraw->DrawBoxDw(cgo.Surface, cgo.X + cgo.Wdt - 1, cgo.Y, cgo.X + cgo.Wdt + 2, cgo.Y + cgo.Hgt + 2, 0x00000000);
	}
	// normal blit OK
	Draw(cgo, false);
#endif
}

void C4Facet::DrawClr(C4Facet &cgo, bool fAspect, uint32_t dwClr)
{
	if (!Surface) return;
	// set ColorByOwner-color
	Surface->SetClr(dwClr);
	// draw
	Draw(cgo, fAspect);
}

void C4Facet::DrawValue2Clr(C4Facet &cgo, int32_t iValue1, int32_t iValue2, uint32_t dwClr)
{
	// set ColorByOwner-color
	Surface->SetClr(dwClr);
	// draw
	DrawValue2(cgo, iValue1, iValue2);
}

void C4Facet::DrawXR(C4Surface *sfcTarget, int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, int32_t iSectionX, int32_t iSectionY, int32_t r)
{
#ifdef C4ENGINE
	if (!lpDDraw || !Surface || !sfcTarget || !Wdt || !Hgt) return;
	CBltTransform rot;
	rot.SetRotate(r, (iX + iX + iWdt) / 2.f, (iY + iY + iHgt) / 2.f);
	lpDDraw->Blit(Surface,
		float(X + Wdt * iSectionX), float(Y + Hgt * iSectionY), float(Wdt), float(Hgt),
		sfcTarget,
		iX, iY, iWdt, iHgt,
		true, &rot);
#endif
}

void C4Facet::DrawClrMod(C4Surface *sfcTarget, int32_t iX, int32_t iY, int32_t iPhaseX, int32_t iPhaseY, uint32_t dwModClr)
{
#ifdef C4ENGINE
	// set color
	if (!lpDDraw) return;
	lpDDraw->ActivateBlitModulation(dwModClr);
	// blit
	Draw(sfcTarget, iX, iY, iPhaseX, iPhaseY);
	// reset color
	lpDDraw->DeactivateBlitModulation();
#endif
}

C4Facet C4Facet::TruncateSection(int32_t iAlign)
{
	C4Facet fctResult; fctResult.Set(Surface, 0, 0, 0, 0);
	// Calculate section size
	int32_t iWdt = Wdt, iHgt = Hgt;
	switch (iAlign & C4FCT_Alignment)
	{
	case C4FCT_Left: case C4FCT_Right:
		iWdt = Hgt;
		if (iAlign & C4FCT_Triple) iWdt *= 3;
		if (iAlign & C4FCT_Double) iWdt *= 2;
		if (iAlign & C4FCT_Half) iWdt /= 2;
		break;
	case C4FCT_Top: case C4FCT_Bottom:
		iHgt = Wdt;
		if (iAlign & C4FCT_Triple) iHgt *= 3;
		if (iAlign & C4FCT_Double) iHgt *= 2;
		if (iAlign & C4FCT_Half) iHgt /= 2;
		break;
	}
	// Size safety
	if ((iWdt > Wdt) || (iHgt > Hgt)) return fctResult;
	// Truncate
	switch (iAlign & C4FCT_Alignment)
	{
	case C4FCT_Left:   fctResult.Set(Surface, X,              Y,              iWdt, iHgt); X   += iWdt; Wdt -= iWdt; break;
	case C4FCT_Right:  fctResult.Set(Surface, X + Wdt - iWdt, Y,              iWdt, iHgt); Wdt -= iWdt;              break;
	case C4FCT_Top:    fctResult.Set(Surface, X,              Y,              iWdt, iHgt); Y   += iHgt; Hgt -= iHgt; break;
	case C4FCT_Bottom: fctResult.Set(Surface, X,              Y + Hgt - iHgt, iWdt, iHgt); Hgt -= iHgt;              break;
	}
	// Done
	return fctResult;
}

C4Facet C4Facet::Truncate(int32_t iAlign, int32_t iSize)
{
	C4Facet fctResult; fctResult.Set(Surface, 0, 0, 0, 0);
	// Calculate section size
	int32_t iWdt = Wdt, iHgt = Hgt;
	switch (iAlign)
	{
	case C4FCT_Left: case C4FCT_Right: iWdt = iSize;  break;
	case C4FCT_Top: case C4FCT_Bottom: iHgt = iSize;  break;
	}
	// Size safety
	if ((iWdt > Wdt) || (iHgt > Hgt)) return fctResult;
	// Truncate
	switch (iAlign)
	{
	case C4FCT_Left:   fctResult.Set(Surface, X,              Y,              iWdt, iHgt); X   += iWdt; Wdt -= iWdt; break;
	case C4FCT_Right:  fctResult.Set(Surface, X + Wdt - iWdt, Y,              iWdt, iHgt); Wdt -= iWdt;              break;
	case C4FCT_Top:    fctResult.Set(Surface, X,              Y,              iWdt, iHgt); Y   += iHgt; Hgt -= iHgt; break;
	case C4FCT_Bottom: fctResult.Set(Surface, X,              Y + Hgt - iHgt, iWdt, iHgt); Hgt -= iHgt;              break;
	}
	// Done
	return fctResult;
}

void C4Facet::DrawValue(C4Facet &cgo, int32_t iValue, int32_t iSectionX, int32_t iSectionY, int32_t iAlign)
{
#ifdef C4ENGINE
	if (!lpDDraw) return;
	char ostr[25]; sprintf(ostr, "%i", iValue);
	switch (iAlign)
	{
	case C4FCT_Center:
		Draw(cgo, true, iSectionX, iSectionY);
		lpDDraw->TextOut(ostr, Game.GraphicsResource.FontRegular, 1.0, cgo.Surface,
			cgo.X + cgo.Wdt - 1, cgo.Y + cgo.Hgt - 1, CStdDDraw::DEFAULT_MESSAGE_COLOR, ARight);
		break;
	case C4FCT_Right:
	{
		int32_t textwdt, texthgt;
		Game.GraphicsResource.FontRegular.GetTextExtent(ostr, textwdt, texthgt, false);
		lpDDraw->TextOut(ostr, Game.GraphicsResource.FontRegular, 1.0, cgo.Surface,
			cgo.X + cgo.Wdt - 1, cgo.Y, CStdDDraw::DEFAULT_MESSAGE_COLOR, ARight);
		cgo.Set(cgo.Surface, cgo.X + cgo.Wdt - 1 - textwdt - 2 * cgo.Hgt, cgo.Y, 2 * cgo.Hgt, cgo.Hgt);
		Draw(cgo, true, iSectionX, iSectionY);
		break;
	}
	}
#endif
}

void C4Facet::DrawValue2(C4Facet &cgo, int32_t iValue1, int32_t iValue2, int32_t iSectionX, int32_t iSectionY, int32_t iAlign, int32_t *piUsedWidth)
{
#ifdef C4ENGINE
	if (!lpDDraw) return;
	char ostr[25]; sprintf(ostr, "%i/%i", iValue1, iValue2);
	switch (iAlign)
	{
	case C4FCT_Center:
		Draw(cgo, true, iSectionX, iSectionY);
		lpDDraw->TextOut(ostr, Game.GraphicsResource.FontRegular, 1.0, cgo.Surface,
			cgo.X + cgo.Wdt - 1, cgo.Y + cgo.Hgt - 1, CStdDDraw::DEFAULT_MESSAGE_COLOR, ARight);
		break;
	case C4FCT_Right:
	{
		int32_t textwdt, texthgt;
		Game.GraphicsResource.FontRegular.GetTextExtent(ostr, textwdt, texthgt, false);
		textwdt += Wdt + 3;
		lpDDraw->TextOut(ostr, Game.GraphicsResource.FontRegular, 1.0, cgo.Surface,
			cgo.X + cgo.Wdt - 1, cgo.Y, CStdDDraw::DEFAULT_MESSAGE_COLOR, ARight);
		cgo.Set(cgo.Surface, cgo.X + cgo.Wdt - textwdt, cgo.Y, 2 * cgo.Hgt, cgo.Hgt);
		Draw(cgo, true, iSectionX, iSectionY);
		if (piUsedWidth) *piUsedWidth = textwdt;
	}
	break;
	}
#endif
}

void C4Facet::DrawX(C4Surface *sfcTarget, int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, int32_t iSectionX, int32_t iSectionY, const float scale) const
{
#ifdef C4ENGINE
	if (!lpDDraw || !Surface || !sfcTarget || !Wdt || !Hgt) return;
	lpDDraw->Blit(Surface,
		float(X + Wdt * iSectionX) * scale, float(Y + Hgt * iSectionY) * scale, float(Wdt) * scale, float(Hgt) * scale,
		sfcTarget,
		iX, iY, iWdt, iHgt,
		true);
#endif
}

void C4Facet::DrawXFloat(C4Surface *sfcTarget, float fX, float fY, float fWdt, float fHgt) const
{
#ifdef C4ENGINE
	if (!lpDDraw || !Surface || !sfcTarget || !Wdt || !Hgt || fWdt <= 0 || fHgt <= 0) return;
	// Since only source coordinates are available as floats for blitting, go inwards into this facet to match blit
	// for closest integer target coordinates
	float zx = fWdt / float(Wdt), zy = fHgt / float(Hgt);
	int32_t iX = static_cast<int32_t>(ceilf(fX)), iY = static_cast<int32_t>(ceilf(fY)), iX2 = static_cast<int32_t>(floorf(fX + fWdt)), iY2 = static_cast<int32_t>(floorf(fY + fHgt));
	float ox = (-fX + iX) / zx, oy = (-fY + iY) / zy;
	float oxs = (+fX + fWdt - iX2) / zx, oys = (+fY + fHgt - iY2) / zy;
	lpDDraw->Blit(Surface,
		float(X) + ox, float(Y) + oy, float(Wdt) - ox - oxs, float(Hgt) - oy - oys,
		sfcTarget,
		iX, iY, iX2 - iX, iY2 - iY,
		true);
	zx = (iX2 - iX) / (float(Wdt) - ox - oxs);
	zy = (iY2 - iY) / (float(Hgt) - oy - oys);
#endif
}

#ifdef C4ENGINE
void C4Facet::DrawXT(C4Surface *sfcTarget, int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, int32_t iPhaseX, int32_t iPhaseY, C4DrawTransform *pTransform, bool noScalingCorrection, const float scale)
{
	if (!lpDDraw || !Surface || !sfcTarget || !Wdt || !Hgt) return;
	lpDDraw->Blit(Surface,
		float(X + Wdt * iPhaseX) * scale, float(Y + Hgt * iPhaseY) * scale, float(Wdt) * scale, float(Hgt) * scale,
		sfcTarget,
		iX, iY, iWdt, iHgt,
		true, pTransform, noScalingCorrection);
}
#endif // C4ENGINE

void C4Facet::DrawEnergyLevelEx(int32_t iLevel, int32_t iRange, const C4Facet &gfx, int32_t bar_idx)
{
#ifdef C4ENGINE
	// draw energy level using graphics
	if (!lpDDraw || !gfx.Surface) return;
	int32_t y0 = Y, x0 = X, h = gfx.Hgt;
	int32_t yBar = Hgt - BoundBy<int32_t>(iLevel, 0, iRange) * Hgt / std::max<int32_t>(iRange, 1);
	int32_t iY = 0, vidx = 0;
	C4Facet gfx_draw = gfx;
	bool filled = false;
	while (iY < Hgt)
	{
		int32_t dy = iY % h;
		int32_t dh = (iY >= Hgt - h) ? Hgt - iY : h - dy;
		if (!filled)
		{
			if (iY >= yBar)
			{
				filled = true; // fully filled
			}
			else if (iY + h >= yBar)
				dh = yBar - iY; // partially filled
		}
		if (!vidx && iY && iY + dh > h)
		{
			if (iY < h)
			{
				// had a break within top section of bar; finish top section
				dh = h - iY;
			}
			else
			{
				// top section finished
				++vidx;
			}
		}
		if (iY + dh >= Hgt - h)
		{
			if (iY >= Hgt - h)
			{
				// within bottom section
				vidx = 2;
				dy = iY + h - Hgt;
			}
			else
			{
				// finish middle section
				dh = Hgt - h - iY;
			}
		}
		// draw it; partially if necessary
		gfx_draw.Y = gfx.Y + vidx * h + dy;
		gfx_draw.Hgt = dh;
		gfx_draw.Draw(Surface, X, Y + iY, bar_idx + bar_idx + !filled);
		iY += dh;
	}
#endif
}

bool C4Facet::GetPhaseNum(int32_t &rX, int32_t &rY)
{
#ifdef C4ENGINE
	// safety
	if (!Surface) return false;
	// newgfx: use locally stored size
	rX = Surface->Wdt / Wdt; rY = Surface->Hgt / Hgt;
	// success
	return true;
#else
	// no surface sizes in frontend...sorry
	return false;
#endif
}

#ifdef C4ENGINE
void C4DrawTransform::CompileFunc(StdCompiler *pComp)
{
	bool fCompiler = pComp->isCompiler();
	int i;
	// hacky. StdCompiler doesn't allow floats to be safed directly.
	for (i = 0; i < 6; i++)
	{
		if (i) pComp->Separator();
		StdStrBuf val; if (!fCompiler) val.Format("%g", mat[i]);
		pComp->Value(mkParAdapt(val, StdCompiler::RCT_Idtf));
		if (fCompiler && pComp->hasNaming())
			if (pComp->Separator(StdCompiler::SEP_PART))
			{
				StdStrBuf val2;
				pComp->Value(mkParAdapt(val2, StdCompiler::RCT_Idtf));
				val.AppendChar('.'); val.Append(val2);
			}
		if (fCompiler) sscanf(val.getData(), "%g", &mat[i]);
	}
	pComp->Separator();
	pComp->Value(FlipDir);
	if (!fCompiler && mat[6] == 0 && mat[7] == 0 && mat[8] == 1) return;
	// because of backwards-compatibility, the last row comes after flipdir
	for (i = 6; i < 9; ++i)
	{
		if (!pComp->Separator())
		{
			mat[i] = (i == 8) ? 1.0f : 0.0f;
		}
		else
		{
			StdStrBuf val; if (!fCompiler) val.Format("%g", mat[i]);
			pComp->Value(mkParAdapt(val, StdCompiler::RCT_Idtf));
			if (fCompiler && pComp->hasNaming())
				if (pComp->Separator(StdCompiler::SEP_PART))
				{
					StdStrBuf val2;
					pComp->Value(mkParAdapt(val2, StdCompiler::RCT_Idtf));
					val.AppendChar('.'); val.Append(val2);
				}
			if (fCompiler) sscanf(val.getData(), "%g", &mat[i]);
		}
	}
}

void C4DrawTransform::SetTransformAt(C4DrawTransform &r, float iOffX, float iOffY)
{
	// Set matrix, so that this*(x,y,1)-(x,y,1)==this*(x+iOffX,y+iOffY,1)-(x+iOffX,y+iOffY,1)
	float A = r.mat[0] + r.mat[6] * iOffX;
	float B = r.mat[1] + r.mat[7] * iOffX;
	float D = r.mat[3] + r.mat[6] * iOffY;
	float E = r.mat[4] + r.mat[7] * iOffY;
	CBltTransform::Set(
		A, B, r.mat[2] - A       * iOffX - B       * iOffY + r.mat[8] * iOffX,
		D, E, r.mat[5] - D       * iOffX - E       * iOffY + r.mat[8] * iOffY,
		r.mat[6], r.mat[7], r.mat[8] - r.mat[6] * iOffX - r.mat[7] * iOffY);
}

#endif

C4Facet C4Facet::GetFraction(int32_t percentWdt, int32_t percentHgt, int32_t alignX, int32_t alignY)
{
	C4Facet rval;
	// Simple spec for square fractions
	if (percentHgt == 0) percentHgt = percentWdt;
	// Alignment
	int iX = X, iY = Y, iWdt = (std::max)(Wdt * percentWdt / 100, 1), iHgt = (std::max)(Hgt * percentHgt / 100, 1);
	if (alignX & C4FCT_Right)  iX += Wdt     - iWdt;
	if (alignX & C4FCT_Center) iX += Wdt / 2 - iWdt / 2;
	if (alignY & C4FCT_Bottom) iY += Hgt     - iHgt;
	if (alignY & C4FCT_Center) iY += Hgt / 2 - iHgt / 2;
	// Set resulting facet
	rval.Set(Surface, iX, iY, iWdt, iHgt);
	return rval;
}
