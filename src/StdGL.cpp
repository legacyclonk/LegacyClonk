/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2001, Sven2
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

/* OpenGL implementation of NewGfx */

#include <Standard.h>
#include <StdGL.h>
#include <StdSurface2.h>
#include <StdWindow.h>

#ifdef USE_GL

#include <stdio.h>
#include <math.h>
#include <limits.h>

void glColorDw(uint32_t dwClr)
{
	glColor4ub(GLubyte(dwClr >> 16), GLubyte(dwClr >> 8), GLubyte(dwClr), GLubyte(dwClr >> 24));
}

CStdGL::CStdGL()
{
	Default();
	// global ptr
	pGL = this;
	shader = 0;
	shaders[0] = 0;
}

CStdGL::~CStdGL()
{
	Clear();
	pGL = nullptr;
}

void CStdGL::Clear()
{
#ifndef USE_SDL_MAINLOOP
	CStdDDraw::Clear();
#endif
	NoPrimaryClipper();
	if (pTexMgr) pTexMgr->IntUnlock();
	DeleteDeviceObjects();
	MainCtx.Clear();
	pCurrCtx = nullptr;
#ifndef USE_SDL_MAINLOOP
	CStdDDraw::Clear();
#endif
}

bool CStdGL::PageFlip(RECT *pSrcRt, RECT *pDstRt, CStdWindow *pWindow)
{
	// call from gfx thread only!
	if (!pApp || !pApp->AssertMainThread()) return false;
	// safety
	if (!pCurrCtx) return false;
	// end the scene and present it
	if (!pCurrCtx->PageFlip()) return false;
	// success!
	return true;
}

void CStdGL::FillBG(uint32_t dwClr)
{
	if (!pCurrCtx) if (!MainCtx.Select()) return;
	glClearColor((float)GetBValue(dwClr) / 255.0f, (float)GetGValue(dwClr) / 255.0f, (float)GetRValue(dwClr) / 255.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

bool CStdGL::UpdateClipper()
{
	// no render target? do nothing
	if (!RenderTarget || !Active) return true;
	// negative/zero?
	int iWdt = Min(ClipX2, RenderTarget->Wdt - 1) - ClipX1 + 1;
	int iHgt = Min(ClipY2, RenderTarget->Hgt - 1) - ClipY1 + 1;
	int iX = ClipX1; if (iX < 0) { iWdt += iX; iX = 0; }
	int iY = ClipY1; if (iY < 0) { iHgt += iY; iY = 0; }
	if (iWdt <= 0 || iHgt <= 0)
	{
		ClipAll = true;
		return true;
	}
	ClipAll = false;
	// set it
	glViewport(iX, RenderTarget->Hgt - iY - iHgt, iWdt, iHgt);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D((GLdouble)iX, (GLdouble)(iX + iWdt), (GLdouble)(iY + iHgt), (GLdouble)iY);
	return true;
}

bool CStdGL::PrepareRendering(SURFACE sfcToSurface)
{
	// call from gfx thread only!
	if (!pApp || !pApp->AssertMainThread()) return false;
	// device?
	if (!pCurrCtx) if (!MainCtx.Select()) return false;
	// not ready?
	if (!Active)
		return false;
	// target?
	if (!sfcToSurface) return false;
	// target locked?
	if (sfcToSurface->Locked) return false;
	// target is already set as render target?
	if (sfcToSurface != RenderTarget)
	{
		// target is a render-target?
		if (!sfcToSurface->IsRenderTarget()) return false;
		// set target
		RenderTarget = sfcToSurface;
		// new target has different size; needs other clipping rect
		UpdateClipper();
	}
	// done
	return true;
}

void CStdGL::PerformBlt(CBltData &rBltData, CTexRef *pTex, uint32_t dwModClr, bool fMod2, bool fExact)
{
	// clipping
	if (DDrawCfg.ClipManually && rBltData.pTransform) ClipPoly(rBltData);
	// global modulation map
	int i;
	uint32_t dwModMask = 0;
	bool fAnyModNotBlack;
	bool fModClr = false;
	if (fUseClrModMap && dwModClr)
	{
		fAnyModNotBlack = false;
		for (i = 0; i < rBltData.byNumVertices; ++i)
		{
			float x = rBltData.vtVtx[i].ftx;
			float y = rBltData.vtVtx[i].fty;
			if (rBltData.pTransform)
			{
				rBltData.pTransform->TransformPoint(x, y);
			}
			rBltData.vtVtx[i].dwModClr = pClrModMap->GetModAt(x, y);
			if (rBltData.vtVtx[i].dwModClr >> 24) dwModMask = 0xff000000;
			ModulateClr((uint32_t &)rBltData.vtVtx[i].dwModClr, dwModClr);
			if (rBltData.vtVtx[i].dwModClr) fAnyModNotBlack = true;
			if (rBltData.vtVtx[i].dwModClr != 0xffffff) fModClr = true;
		}
	}
	else
	{
		fAnyModNotBlack = !!dwModClr;
		for (i = 0; i < rBltData.byNumVertices; ++i)
			rBltData.vtVtx[i].dwModClr = dwModClr;
		if (dwModClr != 0xffffff) fModClr = true;
	}
	// reset MOD2 for completely black modulations
	if (fMod2 && !fAnyModNotBlack) fMod2 = 0;
	if (shader)
	{
		glEnable(GL_FRAGMENT_SHADER_ATI);
		if (!fModClr)
			glColor4f(1.0f, 1.0f, 1.0f, 0.0f);
		dwModMask = 0;
		if (Saturation < 255)
		{
			glBindFragmentShaderATI(fMod2 ? shader + 3 : shader + 2);
			GLfloat bla[4] = { Saturation / 255.0f, Saturation / 255.0f, Saturation / 255.0f, 1.0f };
			glSetFragmentShaderConstantATI(GL_CON_1_ATI, bla);
		}
		else
		{
			glBindFragmentShaderATI(fMod2 ? shader + 1 : shader);
		}
	}
	else if (shaders[0])
	{
		glEnable(GL_FRAGMENT_PROGRAM_ARB);
		if (!fModClr)
			glColor4f(1.0f, 1.0f, 1.0f, 0.0f);
		dwModMask = 0;
		if (Saturation < 255)
		{
			glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, shaders[fMod2 ? 3 : 2]);
			GLfloat bla[4] = { Saturation / 255.0f, Saturation / 255.0f, Saturation / 255.0f, 1.0f };
			glProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 0, bla);
		}
		else
		{
			glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, shaders[fMod2 ? 1 : 0]);
		}
	}
	// modulated blit
	else if (fModClr)
	{
		if (fMod2 || ((dwModClr >> 24 || dwModMask) && !DDrawCfg.NoAlphaAdd))
		{
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB,      fMod2 ? GL_ADD_SIGNED : GL_MODULATE);
			glTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE,        fMod2 ? 2.0f : 1.0f);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA,    GL_ADD);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB,      GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB,      GL_PRIMARY_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA,    GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA,    GL_PRIMARY_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB,     GL_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB,     GL_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA,   GL_SRC_ALPHA);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA,   GL_SRC_ALPHA);
			dwModMask = 0;
		}
		else
		{
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			glTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE,        1.0f);
			dwModMask = 0xff000000;
		}
	}
	else
	{
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE, 1.0f);
	}
	// set texture+modes
	glShadeModel((fUseClrModMap && fModClr && !DDrawCfg.NoBoxFades) ? GL_SMOOTH : GL_FLAT);
	glBindTexture(GL_TEXTURE_2D, pTex->texName);
	if (!fExact && !DDrawCfg.PointFiltering)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}

	glMatrixMode(GL_TEXTURE);
	float matrix[16];
	matrix[0] = rBltData.TexPos.mat[0];  matrix[ 1] = rBltData.TexPos.mat[3];  matrix[ 2] = 0; matrix[ 3] = rBltData.TexPos.mat[6];
	matrix[4] = rBltData.TexPos.mat[1];  matrix[ 5] = rBltData.TexPos.mat[4];  matrix[ 6] = 0; matrix[ 7] = rBltData.TexPos.mat[7];
	matrix[8] = 0;                       matrix[ 9] = 0;                       matrix[10] = 1; matrix[11] = 0;
	matrix[12] = rBltData.TexPos.mat[2]; matrix[13] = rBltData.TexPos.mat[5];  matrix[14] = 0; matrix[15] = rBltData.TexPos.mat[8];
	glLoadMatrixf(matrix);

	glMatrixMode(GL_MODELVIEW);
	if (rBltData.pTransform)
	{
		float *mat = rBltData.pTransform->mat;
		matrix[ 0] = mat[0]; matrix[ 1] = mat[3]; matrix[ 2] = 0; matrix[ 3] = mat[6];
		matrix[ 4] = mat[1]; matrix[ 5] = mat[4]; matrix[ 6] = 0; matrix[ 7] = mat[7];
		matrix[ 8] = 0;      matrix[ 9] = 0;      matrix[10] = 1; matrix[11] = 0;
		matrix[12] = mat[2]; matrix[13] = mat[5]; matrix[14] = 0; matrix[15] = mat[8];
		glLoadMatrixf(matrix);
	}
	else
	{
		glLoadIdentity();
	}

	// draw polygon
	glBegin(GL_POLYGON);
	for (i = 0; i < rBltData.byNumVertices; ++i)
	{
		if (fModClr) glColorDw(rBltData.vtVtx[i].dwModClr | dwModMask);
		glTexCoord2f(rBltData.vtVtx[i].ftx, rBltData.vtVtx[i].fty);
		glVertex2f(rBltData.vtVtx[i].ftx, rBltData.vtVtx[i].fty);
	}
	glEnd();
	glLoadIdentity();
	if (shader)
	{
		glDisable(GL_FRAGMENT_SHADER_ATI);
	}
	else if (shaders[0])
	{
		glDisable(GL_FRAGMENT_PROGRAM_ARB);
	}
	if (!fExact && !DDrawCfg.PointFiltering)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}
}

void CStdGL::BlitLandscape(SURFACE sfcSource, SURFACE sfcSource2, SURFACE sfcLiquidAnimation, int fx, int fy,
	SURFACE sfcTarget, int tx, int ty, int wdt, int hgt)
{
	// safety
	if (!sfcSource || !sfcTarget || !wdt || !hgt) return;
	assert(sfcTarget->IsRenderTarget());
	assert(!(dwBlitMode & C4GFXBLIT_MOD2));
	// bound
	if (ClipAll) return;
	// check exact
	bool fExact = true;
	// manual clipping? (primary surface only)
	if (DDrawCfg.ClipManuallyE)
	{
		int iOver;
		// Left
		iOver = tx - ClipX1;
		if (iOver < 0)
		{
			wdt += iOver;
			fx -= iOver;
			tx = ClipX1;
		}
		// Top
		iOver = ty - ClipY1;
		if (iOver < 0)
		{
			hgt += iOver;
			fy -= iOver;
			ty = ClipY1;
		}
		// Right
		iOver = ClipX2 + 1 - (tx + wdt);
		if (iOver < 0)
		{
			wdt += iOver;
		}
		// Bottom
		iOver = ClipY2 + 1 - (ty + hgt);
		if (iOver < 0)
		{
			hgt += iOver;
		}
	}
	// inside screen?
	if (wdt <= 0 || hgt <= 0) return;
	// prepare rendering to surface
	if (!PrepareRendering(sfcTarget)) return;
	// texture present?
	if (!sfcSource->ppTex)
	{
		return;
	}
	// blit with basesfc?
	bool fBaseSfc = false;
	// get involved texture offsets
	int iTexSize = sfcSource->iTexSize;
	int iTexX = Max(int(fx / iTexSize), 0);
	int iTexY = Max(int(fy / iTexSize), 0);
	int iTexX2 = Min((int)(fx + wdt - 1) / iTexSize + 1, sfcSource->iTexX);
	int iTexY2 = Min((int)(fy + hgt - 1) / iTexSize + 1, sfcSource->iTexY);
	// blit from all these textures
	SetTexture();
	if (sfcSource2)
	{
		glActiveTexture(GL_TEXTURE1);
		glEnable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE2);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, (*sfcLiquidAnimation->ppTex)->texName);
		glActiveTexture(GL_TEXTURE0);
	}
	uint32_t dwModMask = 0;
	if (shader)
	{
		glEnable(GL_FRAGMENT_SHADER_ATI);
		if (sfcSource2)
		{
			glBindFragmentShaderATI(shader + 4);
			static GLfloat bla[4] = { -0.6f / 3, 0.0f, 0.6f / 3, 0.0f };
			bla[0] += 0.05f; bla[1] += 0.05f; bla[2] += 0.05f;
			GLfloat mod[4];
			for (int i = 0; i < 3; ++i)
			{
				if (bla[i] > 0.9f) bla[i] = -0.3f;
				mod[i] = (bla[i] > 0.3f ? 0.6f - bla[i] : bla[i]) / 3.0f;
			}
			mod[3] = 0;
			glSetFragmentShaderConstantATI(GL_CON_2_ATI, mod);
		}
		else if (Saturation < 255)
		{
			glBindFragmentShaderATI(shader + 2);
			GLfloat bla[4] = { Saturation / 255.0f, Saturation / 255.0f, Saturation / 255.0f, 1.0f };
			glSetFragmentShaderConstantATI(GL_CON_1_ATI, bla);
		}
		else
		{
			glBindFragmentShaderATI(shader);
		}
		dwModMask = 0;
	}
	else if (shaders[0])
	{
		glEnable(GL_FRAGMENT_PROGRAM_ARB);
		if (sfcSource2)
		{
			glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, shaders[4]);
			static GLfloat bla[4] = { -0.6f / 3, 0.0f, 0.6f / 3, 0.0f };
			bla[0] += 0.05f; bla[1] += 0.05f; bla[2] += 0.05f;
			GLfloat mod[4];
			for (int i = 0; i < 3; ++i)
			{
				if (bla[i] > 0.9f) bla[i] = -0.3f;
				mod[i] = (bla[i] > 0.3f ? 0.6f - bla[i] : bla[i]) / 3.0f;
			}
			mod[3] = 0;
			glProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, mod);
		}
		else if (Saturation < 255)
		{
			glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, shaders[2]);
			GLfloat bla[4] = { Saturation / 255.0f, Saturation / 255.0f, Saturation / 255.0f, 1.0f };
			glProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 0, bla);
		}
		else
		{
			glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, shaders[0]);
		}
		dwModMask = 0;
	}
	// texture environment
	else
	{
		if (DDrawCfg.NoAlphaAdd)
		{
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			glTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE,        1.0f);
			dwModMask = 0xff000000;
		}
		else
		{
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB,      GL_MODULATE);
			glTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE,        1.0f);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA,    GL_ADD);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB,      GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB,      GL_PRIMARY_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA,    GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA,    GL_PRIMARY_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB,     GL_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB,     GL_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA,   GL_SRC_ALPHA);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA,   GL_SRC_ALPHA);
			dwModMask = 0;
		}
	}
	// set texture+modes
	glShadeModel((fUseClrModMap && !DDrawCfg.NoBoxFades) ? GL_SMOOTH : GL_FLAT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	for (int iY = iTexY; iY < iTexY2; ++iY)
	{
		for (int iX = iTexX; iX < iTexX2; ++iX)
		{
			// blit
			uint32_t dwModClr = BlitModulated ? BlitModulateClr : 0xffffff;

			if (sfcSource2)
				glActiveTexture(GL_TEXTURE0);
			CTexRef *pTex = *(sfcSource->ppTex + iY * sfcSource->iTexX + iX);
			glBindTexture(GL_TEXTURE_2D, pTex->texName);
			if (sfcSource2)
			{
				CTexRef *pTex = *(sfcSource2->ppTex + iY * sfcSource2->iTexX + iX);
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, pTex->texName);
			}

			// draw polygon
			glBegin(GL_POLYGON);
			// get current blitting offset in texture (beforing any last-tex-size-changes)
			int iBlitX = iTexSize * iX;
			int iBlitY = iTexSize * iY;
			// size changed? recalc dependant, relevant (!) values
			if (iTexSize != pTex->iSize)
			{
				iTexSize = pTex->iSize;
			}
			// get new texture source bounds
			FLOAT_RECT fTexBlt;
			// get new dest bounds
			FLOAT_RECT tTexBlt;
			// set up blit data as rect
			fTexBlt.left = Max<float>((float)(fx - iBlitX), 0.0f);
			tTexBlt.left = (float)((fTexBlt.left + iBlitX - fx) + tx);
			fTexBlt.top  = Max<float>((float)(fy - iBlitY), 0.0f);
			tTexBlt.top  = (float)((fTexBlt.top + iBlitY - fy) + ty);
			fTexBlt.right  = Min<float>((float)(fx + wdt - iBlitX), (float)iTexSize);
			tTexBlt.right  = (float)((fTexBlt.right + iBlitX - fx) + tx);
			fTexBlt.bottom = Min<float>((float)(fy + hgt - iBlitY), (float)iTexSize);
			tTexBlt.bottom = (float)((fTexBlt.bottom + iBlitY - fy) + ty);
			float ftx[4]; float fty[4]; // blit positions
			ftx[0] = tTexBlt.left;  fty[0] = tTexBlt.top;
			ftx[1] = tTexBlt.right; fty[1] = tTexBlt.top;
			ftx[2] = tTexBlt.right; fty[2] = tTexBlt.bottom;
			ftx[3] = tTexBlt.left;  fty[3] = tTexBlt.bottom;
			float tcx[4]; float tcy[4]; // blit positions
			tcx[0] = fTexBlt.left;  tcy[0] = fTexBlt.top;
			tcx[1] = fTexBlt.right; tcy[1] = fTexBlt.top;
			tcx[2] = fTexBlt.right; tcy[2] = fTexBlt.bottom;
			tcx[3] = fTexBlt.left;  tcy[3] = fTexBlt.bottom;

			uint32_t fdwModClr[4]; // color modulation
			// global modulation map
			int i;
			if (fUseClrModMap && dwModClr)
			{
				for (i = 0; i < 4; ++i)
				{
					fdwModClr[i] = pClrModMap->GetModAt(int(ftx[i]), int(fty[i]));
					ModulateClr((uint32_t &)fdwModClr[i], dwModClr);
				}
			}
			else
			{
				for (i = 0; i < 4; ++i)
					fdwModClr[i] = dwModClr;
			}

			for (i = 0; i < 4; ++i)
			{
				glColorDw(fdwModClr[i] | dwModMask);
				glTexCoord2f((tcx[i] + DDrawCfg.fTexIndent) / iTexSize, (tcy[i] + DDrawCfg.fTexIndent) / iTexSize);
				if (sfcSource2)
					glMultiTexCoord2f(GL_TEXTURE1_ARB, (tcx[i] + DDrawCfg.fTexIndent) / iTexSize, (tcy[i] + DDrawCfg.fTexIndent) / iTexSize);
				glVertex2f(ftx[i] + DDrawCfg.fBlitOff, fty[i] + DDrawCfg.fBlitOff);
			}

			glEnd();
		}
	}
	if (shader)
	{
		glDisable(GL_FRAGMENT_SHADER_ATI);
	}
	else if (shaders[0])
	{
		glDisable(GL_FRAGMENT_PROGRAM_ARB);
	}
	if (sfcSource2)
	{
		glActiveTexture(GL_TEXTURE1);
		glDisable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE2);
		glDisable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
	}
	// reset texture
	ResetTexture();
}

bool CStdGL::CreateDirectDraw()
{
	Log("  Using OpenGL...");
	return true;
}

CStdGLCtx *CStdGL::CreateContext(CStdWindow *pWindow, CStdApp *pApp)
{
	// safety
	if (!pWindow) return nullptr;
	// create it
	CStdGLCtx *pCtx = new CStdGLCtx();
	if (!pCtx->Init(pWindow, pApp))
	{
		delete pCtx; Error("  gl: Error creating secondary context!"); return nullptr;
	}
	// done
	return pCtx;
}

#ifdef _WIN32
CStdGLCtx *CStdGL::CreateContext(HWND hWindow, CStdApp *pApp)
{
	// safety
	if (!hWindow) return nullptr;
	// create it
	CStdGLCtx *pCtx = new CStdGLCtx();
	if (!pCtx->Init(nullptr, pApp, hWindow))
	{
		delete pCtx; Error("  gl: Error creating secondary context!"); return nullptr;
	}
	// done
	return pCtx;
}
#endif

bool CStdGL::CreatePrimarySurfaces(bool Playermode, int iColorDepth, unsigned int iMonitor)
{
	// remember fullscreen setting
	fFullscreen = Playermode && !DDrawCfg.Windowed;

	// Set window size only in playermode
	if (Playermode)
	{
		// Always search for display mode, in case the user decides to activate fullscreen later
		if (!pApp->FindDisplayMode(pApp->ScreenWidth(), pApp->ScreenHeight(), iColorDepth, iMonitor))
		{
			Error("  gl: No Display mode found; leaving current!");
			fFullscreen = false;
		}
		if (fFullscreen)
		{
			// change resolution
			if (!pApp->SetFullScreen(true))
			{
				Error("  gl: No Display cannot be set; leaving current!");
				fFullscreen = false;
			}
		}
		if (!fFullscreen)
		{
			pApp->pWindow->SetSize(pApp->ScreenWidth(), pApp->ScreenHeight());
		}
	}

	// store options
	byByteCnt = 4; iClrDpt = iColorDepth;

	// create lpPrimary and lpBack (used in first context selection)
	lpPrimary = lpBack = new CSurface();
	lpPrimary->fPrimary = true;
	lpPrimary->AttachSfc(nullptr);
	lpPrimary->byBytesPP = byByteCnt;

	// create+select gl context
	if (!MainCtx.Init(pApp->pWindow, pApp)) return Error("  gl: Error initializing context");

	// done, init device stuff
	return InitDeviceObjects();
}

void CStdGL::DrawQuadDw(SURFACE sfcTarget, int *ipVtx, uint32_t dwClr1, uint32_t dwClr2, uint32_t dwClr3, uint32_t dwClr4)
{
	// prepare rendering to target
	if (!PrepareRendering(sfcTarget)) return;
	// apply global modulation
	ClrByCurrentBlitMod(dwClr1);
	ClrByCurrentBlitMod(dwClr2);
	ClrByCurrentBlitMod(dwClr3);
	ClrByCurrentBlitMod(dwClr4);
	// apply modulation map
	if (fUseClrModMap)
	{
		ModulateClr(dwClr1, pClrModMap->GetModAt(ipVtx[0], ipVtx[1]));
		ModulateClr(dwClr2, pClrModMap->GetModAt(ipVtx[2], ipVtx[3]));
		ModulateClr(dwClr3, pClrModMap->GetModAt(ipVtx[4], ipVtx[5]));
		ModulateClr(dwClr4, pClrModMap->GetModAt(ipVtx[6], ipVtx[7]));
	}
	// no clr fading supported
	if (DDrawCfg.NoBoxFades)
	{
		NormalizeColors(dwClr1, dwClr2, dwClr3, dwClr4);
		glShadeModel(GL_FLAT);
	}
	else
		glShadeModel((dwClr1 == dwClr2 && dwClr1 == dwClr3 && dwClr1 == dwClr4) ? GL_FLAT : GL_SMOOTH);
	// set blitting state
	int iAdditive = dwBlitMode & C4GFXBLIT_ADDITIVE;
	glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, iAdditive ? GL_ONE : GL_SRC_ALPHA);
	// draw two triangles
	glBegin(GL_POLYGON);
	glColorDw(dwClr1); glVertex2f(ipVtx[0] + DDrawCfg.fBlitOff, ipVtx[1] + DDrawCfg.fBlitOff);
	glColorDw(dwClr2); glVertex2f(ipVtx[2] + DDrawCfg.fBlitOff, ipVtx[3] + DDrawCfg.fBlitOff);
	glColorDw(dwClr3); glVertex2f(ipVtx[4] + DDrawCfg.fBlitOff, ipVtx[5] + DDrawCfg.fBlitOff);
	glColorDw(dwClr4); glVertex2f(ipVtx[6] + DDrawCfg.fBlitOff, ipVtx[7] + DDrawCfg.fBlitOff);
	glEnd();
	glShadeModel(GL_FLAT);
}

void CStdGL::DrawLineDw(SURFACE sfcTarget, float x1, float y1, float x2, float y2, uint32_t dwClr)
{
	float i;
	// manual clipping?
	if (DDrawCfg.ClipManuallyE)
	{
		// sort left/right
		if (x1 > x2) { i = x1; x1 = x2; x2 = i; i = y1; y1 = y2; y2 = i; }
		// clip horizontally
		if (x1 < ClipX1)
			if (x2 < ClipX1)
				return; // left out
			else
			{
				y1 += (y2 - y1) * ((float)ClipX1 - x1) / (x2 - x1); x1 = (float)ClipX1;
			} // clip left
		else if (x2 > ClipX2)
			if (x1 > ClipX2)
				return; // right out
			else
			{
				y2 -= (y2 - y1) * (x2 - (float)ClipX2) / (x2 - x1); x2 = (float)ClipX2;
			} // clip right
		// sort top/bottom
		if (y1 > y2) { i = x1; x1 = x2; x2 = i; i = y1; y1 = y2; y2 = i; }
		// clip vertically
		if (y1 < ClipY1)
			if (y2 < ClipY1)
				return; // top out
			else
			{
				x1 += (x2 - x1) * ((float)ClipY1 - y1) / (y2 - y1); y1 = (float)ClipY1;
			} // clip top
		else if (y2 > ClipY2)
			if (y1 > ClipY2)
				return; // bottom out
			else
			{
				x2 -= (x2 - x1) * (y2 - (float)ClipY2) / (y2 - y1); y2 = (float)ClipY2;
			} // clip bottom
	}
	// apply color modulation
	ClrByCurrentBlitMod(dwClr);
	// render target?
	if (sfcTarget->IsRenderTarget())
	{
		// prepare rendering to target
		if (!PrepareRendering(sfcTarget)) return;
		// set blitting state
		int iAdditive = dwBlitMode & C4GFXBLIT_ADDITIVE;
		// use a different blendfunc here, because GL_LINE_SMOOTH expects this one
		glBlendFunc(GL_SRC_ALPHA, iAdditive ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA);
		// draw one line
		glBegin(GL_LINES);
		// global clr modulation map
		uint32_t dwClr1 = dwClr;
		if (fUseClrModMap)
		{
			ModulateClr(dwClr1, pClrModMap->GetModAt((int)x1, (int)y1));
		}
		// convert from clonk-alpha to GL_LINE_SMOOTH alpha
		glColorDw(InvertRGBAAlpha(dwClr1));
		glVertex2f(x1 + 0.5f, y1 + 0.5f);
		if (fUseClrModMap)
		{
			ModulateClr(dwClr, pClrModMap->GetModAt((int)x2, (int)y2));
			glColorDw(InvertRGBAAlpha(dwClr));
		}
		glVertex2f(x2 + 0.5f, y2 + 0.5f);
		glEnd();
	}
	else
	{
		// emulate
		if (!LockSurfaceGlobal(sfcTarget)) return;
		ForLine((int32_t)x1, (int32_t)y1, (int32_t)x2, (int32_t)y2, &DLineSPix, (int)dwClr);
		UnLockSurfaceGlobal(sfcTarget);
	}
}

void CStdGL::DrawPixInt(SURFACE sfcTarget, float tx, float ty, uint32_t dwClr)
{
	// render target?
	if (sfcTarget->IsRenderTarget())
	{
		if (!PrepareRendering(sfcTarget)) return;
		int iAdditive = dwBlitMode & C4GFXBLIT_ADDITIVE;
		// use a different blendfunc here because of GL_POINT_SMOOTH
		glBlendFunc(GL_SRC_ALPHA, iAdditive ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA);
		// convert the alpha value for that blendfunc
		glBegin(GL_POINTS);
		glColorDw(InvertRGBAAlpha(dwClr));
		glVertex2f(tx + 0.5f, ty + 0.5f);
		glEnd();
	}
	else
	{
		// emulate
		sfcTarget->SetPixDw((int)tx, (int)ty, dwClr);
	}
}

bool CStdGL::InitDeviceObjects()
{
	bool fSuccess = true;
	// restore everything
	return fSuccess && RestoreDeviceObjects();
}

static void DefineShaderARB(const char *p, GLuint &s)
{
	glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, s);
	glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, strlen(p), p);
	if (GL_INVALID_OPERATION == glGetError())
	{
		GLint errPos; glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errPos);
		fprintf(stderr, "ARB program%d:%d: Error: %s\n", s, errPos, glGetString(GL_PROGRAM_ERROR_STRING_ARB));
		s = 0;
	}
}

bool CStdGL::RestoreDeviceObjects()
{
	// safety
	if (!lpPrimary) return false;
	// delete any previous objects
	InvalidateDeviceObjects();
	bool fSuccess = true;
	// restore primary/back
	RenderTarget = lpPrimary;
	lpPrimary->AttachSfc(nullptr);
	lpPrimary->byBytesPP = byByteCnt;

	// set states
	fSuccess = pCurrCtx ? (pCurrCtx->Select()) : MainCtx.Select();
	// activate if successful
	Active = fSuccess;
	// restore gamma if active
	if (Active)
		EnableGamma();
	// reset blit states
	dwBlitMode = 0;

	if (!DDrawCfg.Shader) {}
	else if (!shader && GLEW_ATI_fragment_shader)
	{
		shader = glGenFragmentShadersATI(6);
		if (!shader) return Active;

		// Standard color modulation and alpha addition
		glBindFragmentShaderATI(shader);
		glBeginFragmentShaderATI();
		// Load the contents of the texture into a register
		glSampleMapATI(GL_REG_0_ATI, GL_TEXTURE0, GL_SWIZZLE_STR_ATI);
		// X(d) = X(a1) * X(a2)
		glColorFragmentOp2ATI(GL_MUL_ATI,
			GL_REG_0_ATI, GL_NONE, GL_NONE,
			GL_PRIMARY_COLOR_ARB, GL_NONE, GL_NONE,
			GL_REG_0_ATI, GL_NONE, GL_NONE);
		// A(d) = BoundBy(A(a1) + A(a2), 0, 1)
		glAlphaFragmentOp2ATI(GL_ADD_ATI,
			GL_REG_0_ATI, GL_SATURATE_BIT_ATI,
			GL_PRIMARY_COLOR_ARB, GL_NONE, GL_NONE,
			GL_REG_0_ATI, GL_NONE, GL_NONE);
		glEndFragmentShaderATI();

		// Spezial "mod2" color addition and alpha addition
		glBindFragmentShaderATI(shader + 1);
		glBeginFragmentShaderATI();
		// Load the contents of the texture into a register
		glSampleMapATI(GL_REG_0_ATI, GL_TEXTURE0, GL_SWIZZLE_STR_ATI);
		// X(d) = BoundBy((X(a1) + X(a2) - 0.5) * 2, 0, 1)
		glColorFragmentOp2ATI(GL_ADD_ATI,
			GL_REG_0_ATI, GL_NONE, GL_2X_BIT_ATI | GL_SATURATE_BIT_ATI,
			GL_REG_0_ATI, GL_NONE, GL_NONE,
			GL_PRIMARY_COLOR_ARB, GL_NONE, GL_BIAS_BIT_ATI);
		// A(d) = BoundBy(A(a1) + A(a2), 0, 1)
		glAlphaFragmentOp2ATI(GL_ADD_ATI,
			GL_REG_0_ATI, GL_SATURATE_BIT_ATI,
			GL_REG_0_ATI, GL_NONE, GL_NONE,
			GL_PRIMARY_COLOR_ARB, GL_NONE, GL_NONE);
		glEndFragmentShaderATI();

		// Standard color modulation and alpha addition, with additional greying
		glBindFragmentShaderATI(shader + 2);
		glBeginFragmentShaderATI();
		// Load the contents of the texture into a register
		glSampleMapATI(GL_REG_0_ATI, GL_TEXTURE0, GL_SWIZZLE_STR_ATI);
		// Define the factors for each color
		GLfloat grey[4] = { 0.299f, 0.587f, 0.114f, 1.0f };
		glSetFragmentShaderConstantATI(GL_CON_0_ATI, grey);
		// X(d) = X(a1) * X(a2)
		glColorFragmentOp2ATI(GL_MUL_ATI,
			GL_REG_0_ATI, GL_NONE, GL_NONE,
			GL_PRIMARY_COLOR_ARB, GL_NONE, GL_NONE,
			GL_REG_0_ATI, GL_NONE, GL_NONE);
		// A(d) = BoundBy(A(a1) + A(a2), 0, 1)
		glAlphaFragmentOp2ATI(GL_ADD_ATI,
			GL_REG_0_ATI, GL_SATURATE_BIT_ATI,
			GL_PRIMARY_COLOR_ARB, GL_NONE, GL_NONE,
			GL_REG_0_ATI, GL_NONE, GL_NONE);
		// X(d) = R(a1) * R(a2) + G(a1) * G(a2) + B(a1) * B(a2)
		glColorFragmentOp2ATI(GL_DOT3_ATI,
			GL_REG_1_ATI, GL_NONE, GL_NONE,
			GL_REG_0_ATI, GL_NONE, GL_NONE,
			GL_CON_0_ATI, GL_NONE, GL_NONE);
		// X(d) = X(a1) * X(a2) + (1 - X(a1)) * X(a3)
		// CON_1 is defined in PerformBlt.
		glColorFragmentOp3ATI(GL_LERP_ATI,
			GL_REG_0_ATI, GL_NONE, GL_SATURATE_BIT_ATI,
			GL_CON_1_ATI, GL_NONE, GL_NONE,
			GL_REG_0_ATI, GL_NONE, GL_NONE,
			GL_REG_1_ATI, GL_NONE, GL_NONE);
		glEndFragmentShaderATI();

		// Spezial "mod2" color addition and alpha addition
		glBindFragmentShaderATI(shader + 3);
		glBeginFragmentShaderATI();
		// Load the contents of the texture into a register
		glSampleMapATI(GL_REG_0_ATI, GL_TEXTURE0, GL_SWIZZLE_STR_ATI);
		// Define the factors for each color
		glSetFragmentShaderConstantATI(GL_CON_0_ATI, grey);
		// X(d) = BoundBy((X(a1) + X(a2) - 0.5) * 2, 0, 1)
		glColorFragmentOp2ATI(GL_ADD_ATI,
			GL_REG_0_ATI, GL_NONE, GL_2X_BIT_ATI | GL_SATURATE_BIT_ATI,
			GL_REG_0_ATI, GL_NONE, GL_NONE,
			GL_PRIMARY_COLOR_ARB, GL_NONE, GL_BIAS_BIT_ATI);
		// A(d) = BoundBy(A(a1) + A(a2), 0, 1)
		glAlphaFragmentOp2ATI(GL_ADD_ATI,
			GL_REG_0_ATI, GL_SATURATE_BIT_ATI,
			GL_REG_0_ATI, GL_NONE, GL_NONE,
			GL_PRIMARY_COLOR_ARB, GL_NONE, GL_NONE);
		// X(d) = R(a1) * R(a2) + G(a1) * G(a2) + B(a1) * B(a2)
		glColorFragmentOp2ATI(GL_DOT3_ATI,
			GL_REG_1_ATI, GL_NONE, GL_NONE,
			GL_REG_0_ATI, GL_NONE, GL_NONE,
			GL_CON_0_ATI, GL_NONE, GL_NONE);
		// X(d) = X(a1) * X(a2) + (1 - X(a1)) * X(a3)
		glColorFragmentOp3ATI(GL_LERP_ATI,
			GL_REG_0_ATI, GL_NONE, GL_SATURATE_BIT_ATI,
			GL_CON_1_ATI, GL_NONE, GL_NONE,
			GL_REG_1_ATI, GL_NONE, GL_NONE,
			GL_REG_0_ATI, GL_NONE, GL_NONE);
		glEndFragmentShaderATI();

		// Spezial animated landscape shader
		glBindFragmentShaderATI(shader + 4);
		glBeginFragmentShaderATI();
		// Load the contents of the textures into two registers
		glSampleMapATI(GL_REG_0_ATI, GL_TEXTURE0, GL_SWIZZLE_STR_ATI);
		glSampleMapATI(GL_REG_1_ATI, GL_TEXTURE1, GL_SWIZZLE_STR_ATI);
		glSampleMapATI(GL_REG_2_ATI, GL_TEXTURE0, GL_SWIZZLE_STR_ATI);
		// to test: load both textures at the same coord glSampleMapATI (GL_REG_1_ATI, GL_TEXTURE0, GL_SWIZZLE_STR_ATI);
		// X(d) = X(a1) * X(a2)
		glColorFragmentOp2ATI(GL_MUL_ATI,
			GL_REG_0_ATI, GL_NONE, GL_NONE,
			GL_PRIMARY_COLOR_ARB, GL_NONE, GL_NONE,
			GL_REG_0_ATI, GL_NONE, GL_NONE);
		// A(d) = BoundBy(A(a1) + A(a2), 0, 1)
		glAlphaFragmentOp2ATI(GL_ADD_ATI,
			GL_REG_0_ATI, GL_SATURATE_BIT_ATI,
			GL_PRIMARY_COLOR_ARB, GL_NONE, GL_NONE,
			GL_REG_0_ATI, GL_NONE, GL_NONE);
		// R(d) = R(a1) * R(a2) + R(a3)
		glColorFragmentOp2ATI(GL_DOT3_ATI,
			GL_REG_2_ATI, GL_NONE, GL_NONE,
			GL_CON_2_ATI, GL_NONE, GL_NONE,
			GL_REG_2_ATI, GL_NONE, GL_BIAS_BIT_ATI);
		glColorFragmentOp3ATI(GL_MAD_ATI,
			GL_REG_0_ATI, GL_NONE, GL_SATURATE_BIT_ATI,
			GL_REG_2_ATI, GL_NONE, GL_NONE,
			GL_REG_1_ATI, GL_ALPHA, GL_NONE,
			GL_REG_0_ATI, GL_NONE, GL_NONE);
		glEndFragmentShaderATI();
	}
	else if (!shaders[0] && GLEW_ARB_fragment_program)
	{
		glGenProgramsARB(6, shaders);

		DefineShaderARB("!!ARBfp1.0\n"
			"TEMP tmp;\n"
			// sample the texture
			"TXP tmp, fragment.texcoord[0], texture, 2D;\n"
			// perform the modulation
			"MUL tmp.rgb, tmp, fragment.color.primary;\n"
			// Apparently, it is not possible to directly add and mul into the same register or something.
			"ADD_SAT result.color.rgb, tmp, {0,0,0,0};\n"
			"ADD_SAT result.color.a, tmp, fragment.color.primary;\n"
			"END\n", shaders[0]);

		DefineShaderARB("!!ARBfp1.0\n"
			"ATTRIB tex = fragment.texcoord;\n"
			"ATTRIB col = fragment.color.primary;\n"
			"OUTPUT outColor = result.color;\n"
			"TEMP tmp;\n"
			// sample the texture
			"TXP tmp, tex, texture, 2D;\n"
			// perform the modulation
			"ADD tmp, tmp, col;\n"
			"MAD_SAT outColor, tmp, { 2.0, 2.0, 2.0, 1.0 }, { -1.0, -1.0, -1.0, 0.0 };\n"
			"END\n", shaders[1]);

		DefineShaderARB("!!ARBfp1.0\n"
			"ATTRIB tex = fragment.texcoord;\n"
			"ATTRIB col = fragment.color.primary;\n"
			"OUTPUT outColor = result.color;\n"
			"TEMP tmp, grey;\n"
			// sample the texture
			"TXP tmp, tex, texture, 2D;\n"
			// perform the modulation
			"MUL tmp.rgb, tmp, col;\n"
			// grey
			"DP3 grey, tmp, { 0.299, 0.587, 0.114, 1.0 };\n"
			"LRP tmp.rgb, program.local[0], tmp, grey;"
			"ADD_SAT tmp.a, tmp, col;\n"
			"MOV outColor, tmp;\n"
			"END\n", shaders[2]);

		DefineShaderARB("!!ARBfp1.0\n"
			"ATTRIB tex = fragment.texcoord;"
			"ATTRIB col = fragment.color.primary;"
			"OUTPUT outColor = result.color;"
			"TEMP tmp, grey;"
			// sample the texture
			"TXP tmp, tex, texture, 2D;"
			// perform the modulation
			"ADD tmp, tmp, col;\n"
			"MAD_SAT tmp, tmp, { 2.0, 2.0, 2.0, 1.0 }, { -1.0, -1.0, -1.0, 0.0 };\n"
			// grey
			"DP3 grey, tmp, { 0.299, 0.587, 0.114, 1.0 };\n"
			"LRP tmp.rgb, program.local[0], tmp, grey;"
			"MOV outColor, tmp;\n"
			"END", shaders[3]);

		DefineShaderARB("!!ARBfp1.0\n"
			"TEMP tmp;\n"
			"TEMP mask;\n"
			"TEMP liquid;\n"
			// sample the texture
			"TXP tmp, fragment.texcoord, texture[0], 2D;\n"
			"TXP mask, fragment.texcoord, texture[1], 2D;\n"
			"TXP liquid, fragment.texcoord, texture[2], 2D;\n"
			// animation
			"SUB liquid.rgb, liquid, {0.5, 0.5, 0.5, 0};\n"
			"DP3 liquid.rgb, liquid, program.local[1];\n"
			"MUL liquid.rgb, mask.aaaa, liquid;\n"
			"ADD_SAT tmp.rgb, liquid, tmp;\n"
			// perform the modulation
			"MUL tmp.rgb, tmp, fragment.color.primary;\n"
			"ADD_SAT tmp.a, tmp, fragment.color.primary;\n"
			"MOV result.color, tmp;\n"
			"END\n", shaders[4]);
	}
	// done
	return Active;
}

bool CStdGL::InvalidateDeviceObjects()
{
	bool fSuccess = true;
	// clear gamma
#ifndef USE_SDL_MAINLOOP
	DisableGamma();
#endif
	// deactivate
	Active = false;
	// invalidate font objects
	// invalidate primary surfaces
	if (lpPrimary) lpPrimary->Clear();
	if (shader)
	{
		glDeleteFragmentShaderATI(shader);
		shader = 0;
	}
	return fSuccess;
}

bool CStdGL::StoreStateBlock()
{
	return true;
}

void CStdGL::SetTexture()
{
	glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, (dwBlitMode & C4GFXBLIT_ADDITIVE) ? GL_ONE : GL_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);
}

void CStdGL::ResetTexture()
{
	// disable texturing
	glDisable(GL_TEXTURE_2D);
}

bool CStdGL::RestoreStateBlock()
{
	return true;
}

CStdGL *pGL = nullptr;

bool CStdGL::DeleteDeviceObjects()
{
	InvalidateDeviceObjects();
	NoPrimaryClipper();
	// del font objects
	// del main surfaces
	delete lpPrimary;
	lpPrimary = lpBack = nullptr;
	RenderTarget = nullptr;
	// clear context
	if (pCurrCtx) pCurrCtx->Deselect();
	MainCtx.Clear();
	// restore display mode
	// when using the SDL, better let it do this when calling SDL_Cleanup!
#ifndef USE_SDL_MAINLOOP
	pApp->SetFullScreen(false, false);
#endif
	return true;
}

bool CStdGL::SetOutputAdapter(unsigned int iMonitor)
{
	return pApp->SetOutputAdapter(iMonitor);
}

void CStdGL::TaskOut()
{
	// deactivate
	// backup textures
	if (pTexMgr && fFullscreen) pTexMgr->IntLock();
	// shotdown gl
	InvalidateDeviceObjects();
	if (pCurrCtx) pCurrCtx->Deselect();
	// restore original resolution
	if (fFullscreen && !DDrawCfg.GLKeepRes)
	{
		pApp->SetFullScreen(false);
	}
}

void CStdGL::TaskIn()
{
	// restore resolution
	if (fFullscreen && !DDrawCfg.GLKeepRes)
	{
		// change resolution
		pApp->SetFullScreen(true);
	}
	// restore textures
	if (pTexMgr && fFullscreen) pTexMgr->IntUnlock();
	// restore device stuff
	InitDeviceObjects();
}

bool CStdGL::OnResolutionChanged()
{
	InvalidateDeviceObjects();
	if (fFullscreen && !DDrawCfg.GLKeepRes)
	{
		pApp->SetFullScreen(false, false);
		pApp->FindDisplayMode(pApp->ScreenWidth(), pApp->ScreenHeight(), iClrDpt, 0);
		pApp->SetFullScreen(true, false);
	}
	else
	{
		pApp->pWindow->SetSize(pApp->ScreenWidth(), pApp->ScreenHeight());
	}
	RestoreDeviceObjects();
	// Re-create primary clipper to adapt to new size.
	CreatePrimaryClipper();
	return true;
}

void CStdGL::Default()
{
	CStdDDraw::Default();
	iPixelFormat = 0;
	sfcFmt = 0;
	iClrDpt = 0;
	MainCtx.Clear();
}

#endif // USE_GL
