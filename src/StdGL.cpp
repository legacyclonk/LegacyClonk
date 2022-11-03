/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2001, Sven2
 * Copyright (c) 2017-2022, The LegacyClonk Team and contributors
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

#include "C4Config.h"

#include <Standard.h>
#include "StdApp.h"
#include <StdGL.h>
#include "C4Config.h"
#include <C4Surface.h>
#include <C4Log.h>
#include <StdWindow.h>

#ifndef USE_CONSOLE

#include <array>

#include <stdio.h>
#include <math.h>
#include <limits.h>

void CStdGLShader::Compile()
{
	if (shader) // recompiling?
	{
		glDeleteShader(shader);
	}

	GLenum t;
	switch (type)
	{
	case Type::Vertex:
		t = GL_VERTEX_SHADER;
		break;

	case Type::TesselationControl:
		t = GL_TESS_CONTROL_SHADER;
		break;

	case Type::TesselationEvaluation:
		t = GL_TESS_EVALUATION_SHADER;
		break;

	case Type::Geometry:
		t = GL_GEOMETRY_SHADER;
		break;

	case Type::Fragment:
		t = GL_FRAGMENT_SHADER;
		break;

	default:
		throw Exception{"Invalid shader type"};
	}

	shader = glCreateShader(t);
	if (!shader)
	{
		throw Exception{"Could not create shader"};
	}

	PrepareSource();

	GLint status = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (!status)
	{
		GLint size = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &size);
		if (size)
		{
			std::string errorMessage;
			errorMessage.resize(size);
			glGetShaderInfoLog(shader, size, nullptr, errorMessage.data());
			errorMessage = errorMessage.c_str();
			throw Exception{errorMessage};
		}

		throw Exception{"Compile failed"};
	}
}

void CStdGLShader::Clear()
{
	if (shader)
	{
		glDeleteShader(shader);
		shader = 0;
	}

	CStdShader::Clear();
}

void CStdGLShader::PrepareSource()
{
	size_t pos = source.find("#version");
	if (pos == std::string::npos)
	{
		glDeleteShader(shader);
		throw Exception{"Version directive must be first statement and may not be repeated"};
	}

	pos = source.find('\n', pos + 1);
	assert(pos != std::string::npos);

	std::string copy = source;
	std::string buffer = "";

	for (const auto &[key, value] : macros)
	{
		buffer.append("#define ");
		buffer.append(key);
		buffer.append(" ");
		buffer.append(value);
		buffer.append("\n");
	}

	buffer.append("#line 1\n");

	copy.insert(pos + 1, buffer);

	const char *s = copy.c_str();
	glShaderSource(shader, 1, &s, nullptr);
	glCompileShader(shader);
}

void CStdGLShaderProgram::Link()
{
	EnsureProgram();

	glLinkProgram(shaderProgram);

	GLint status = 0;
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &status);
	if (!status)
	{
		GLint size = 0;
		glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &size);
		assert(size);
		if (size)
		{
			std::string errorMessage;
			errorMessage.resize(size);
			glGetProgramInfoLog(shaderProgram, size, nullptr, errorMessage.data());
			errorMessage = errorMessage.c_str();
			throw Exception{errorMessage};
		}

		throw Exception{"Link failed"};
	}

	glValidateProgram(shaderProgram);
	glGetProgramiv(shaderProgram, GL_VALIDATE_STATUS, &status);
	if (!status)
	{
		GLint size = 0;
		glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &size);
		if (size)
		{
			std::string errorMessage;
			errorMessage.resize(size);
			glGetProgramInfoLog(shaderProgram, size, nullptr, errorMessage.data());
			errorMessage = errorMessage.c_str();
			throw Exception{errorMessage};
		}

		throw Exception{"Validation failed"};
	}

	for (const auto &shader : shaders)
	{
		glDetachShader(shaderProgram, dynamic_cast<CStdGLShader *>(shader)->GetHandle());
	}

	shaders.clear();
}

void CStdGLShaderProgram::Clear()
{
	for (const auto &shader : shaders)
	{
		glDetachShader(shaderProgram, dynamic_cast<CStdGLShader *>(shader)->GetHandle());
	}

	if (shaderProgram)
	{
		glDeleteProgram(shaderProgram);
		shaderProgram = 0;
	}

	attributeLocations.clear();
	uniformLocations.clear();

	CStdShaderProgram::Clear();
}

void CStdGLShaderProgram::EnsureProgram()
{
	if (!shaderProgram)
	{
		shaderProgram = glCreateProgram();
	}
	assert(shaderProgram);
}

bool CStdGLShaderProgram::AddShaderInt(CStdShader *shader)
{
	if (auto *s = dynamic_cast<CStdGLShader *>(shader); s)
	{
		glAttachShader(shaderProgram, s->GetHandle());
		return true;
	}

	return false;
}

void CStdGLShaderProgram::OnSelect()
{
	assert(shaderProgram);
	glUseProgram(shaderProgram);
}

void CStdGLShaderProgram::OnDeselect()
{
	glUseProgram(GL_NONE);
}

CStdGLTexture::CStdGLTexture(const std::int32_t width, const std::int32_t height, const GLenum internalFormat, const GLenum format, const GLenum type)
	: width{width}, height{height}, internalFormat{internalFormat}, format{format}, type{type}
{
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	ThrowIfGLError();
	glGenTextures(1, &texture);
	ThrowIfGLError();
}

CStdGLTexture::~CStdGLTexture()
{
	Clear();
}

void CStdGLTexture::Bind(const GLenum target, const GLenum offset)
{
	glActiveTexture(GL_TEXTURE0 + offset);
	glBindTexture(target, texture);
}

void CStdGLTexture::SetData(const GLenum target, const void *const data)
{
	glTexImage2D(target, 0, internalFormat, width, height, 0, format, type, data);
	ThrowIfGLError();
}

void CStdGLTexture::UpdateData(const GLenum target, const void *const data)
{
	glTexSubImage2D(target, 0, 0, 0, width, height, format, type, data);
	ThrowIfGLError();
}

void CStdGLTexture::Clear()
{
	if (texture)
	{
		glDeleteTextures(1, &texture);
	}
}

void CStdGLTexture::ThrowIfGLError()
{
	if (const GLenum error{glGetError()}; error != GL_NO_ERROR)
	{
		throw Exception{reinterpret_cast<const char *>(gluErrorString(error))};
	}
}

static void glColorDw(const uint32_t dwClr)
{
	glColor4ub(
		static_cast<GLubyte>(dwClr >> 16),
		static_cast<GLubyte>(dwClr >> 8),
		static_cast<GLubyte>(dwClr),
		static_cast<GLubyte>(dwClr >> 24));
}

CStdGL::CStdGL()
{
	Default();
	// global ptr
	pGL = this;
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
	pCurrCtx = nullptr;
#ifndef USE_SDL_MAINLOOP
	CStdDDraw::Clear();
#endif
}

void CStdGL::PageFlip()
{
	// call from gfx thread only!
	if (!pApp || !pApp->AssertMainThread()) return;
	// safety
	if (!pCurrCtx) return;
	// end the scene and present it
	pCurrCtx->PageFlip();
}

void CStdGL::FillBG(const uint32_t dwClr)
{
	if (!pCurrCtx && !MainCtx.Select()) return;
	glClearColor(
		GetBValue(dwClr) / 255.0f,
		GetGValue(dwClr) / 255.0f,
		GetRValue(dwClr) / 255.0f,
		1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

bool CStdGL::UpdateClipper()
{
	int iX, iY, iWdt, iHgt;
	// no render target or clip all? do nothing
	if (!CalculateClipper(&iX, &iY, &iWdt, &iHgt)) return true;
	const auto scale = pApp->GetScale();
	glLineWidth(scale);
	glPointSize(scale);
	// set it
	glViewport(static_cast<int32_t>(floorf(iX * scale)), static_cast<int32_t>(floorf((RenderTarget->Hgt - iY - iHgt) * scale)), static_cast<int32_t>(ceilf(iWdt * scale)), static_cast<int32_t>(ceilf(iHgt * scale)));
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(
		static_cast<GLdouble>(iX),        static_cast<GLdouble>(iX + iWdt),
		static_cast<GLdouble>(iY + iHgt), static_cast<GLdouble>(iY));
	return true;
}

bool CStdGL::PrepareRendering(C4Surface *const sfcToSurface)
{
	// call from gfx thread only!
	if (!pApp || !pApp->AssertMainThread()) return false;
	// device?
	if (!pCurrCtx && !MainCtx.Select()) return false;
	// not ready?
	if (!Active) return false;
	// target?
	if (!sfcToSurface) return false;
	// target locked?
	assert(!sfcToSurface->Locked);
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

void CStdGL::PerformBlt(CBltData &rBltData, C4TexRef *const pTex,
	const uint32_t dwModClr, bool fMod2, const bool fExact)
{
	// global modulation map
	uint32_t dwModMask = 0;
	bool fAnyModNotBlack;
	bool fModClr = false;
	if (fUseClrModMap && dwModClr)
	{
		fAnyModNotBlack = false;
		for (auto &vertex : rBltData.vtVtx)
		{
			float x{vertex.ftx};
			float y{vertex.fty};
			if (rBltData.pTransform)
			{
				rBltData.pTransform->TransformPoint(x, y);
			}
			vertex.dwModClr = pClrModMap->GetModAt(static_cast<int>(x), static_cast<int>(y));
			if (vertex.dwModClr >> 24) dwModMask = 0xff000000;
			ModulateClr(vertex.dwModClr, dwModClr);
			if (vertex.dwModClr) fAnyModNotBlack = true;
			if (vertex.dwModClr != 0xffffff) fModClr = true;
		}
	}
	else
	{
		fAnyModNotBlack = !!dwModClr;
		for (auto &vertex : rBltData.vtVtx)
		{
			vertex.dwModClr = dwModClr;
		}
		if (dwModClr != 0xffffff) fModClr = true;
	}
	// reset MOD2 for completely black modulations
	if (fMod2 && !fAnyModNotBlack) fMod2 = 0;
	if (BlitShader)
	{
		dwModMask = 0;
		if (fMod2 && BlitShaderMod2)
		{
			BlitShaderMod2.Select();
		}
		else
		{
			BlitShader.Select();
		}

		if (!fModClr) glColor4f(1.0f, 1.0f, 1.0f, 0.0f);
	}
	// modulated blit
	else if (fModClr)
	{
		if (fMod2 || ((dwModClr >> 24 || dwModMask) && !Config.Graphics.NoAlphaAdd))
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
	glShadeModel((fUseClrModMap && fModClr && !Config.Graphics.NoBoxFades) ? GL_SMOOTH : GL_FLAT);
	glBindTexture(GL_TEXTURE_2D, pTex->texName);

	const auto enableTextureFiltering = (pApp->GetScale() != 1.f || (!fExact && !Config.Graphics.PointFiltering));
	if (enableTextureFiltering)
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
		const float *const mat = rBltData.pTransform->mat;
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

	glBegin(GL_TRIANGLE_STRIP);
	for (const auto vertex : rBltData.vtVtx)
	{
		if (fModClr) glColorDw(vertex.dwModClr | dwModMask);
		glTexCoord2f(vertex.ftx, vertex.fty);
		glVertex2f(vertex.ftx, vertex.fty);
	}
	glEnd();
	glLoadIdentity();
	if (BlitShader)
	{
		CStdShaderProgram::Deselect();
	}

	if (enableTextureFiltering)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}
}

void CStdGL::BlitLandscape(C4Surface *const sfcSource, C4Surface *const sfcSource2,
	C4Surface *const sfcLiquidAnimation, const int fx, const int fy,
	C4Surface *const sfcTarget, const int tx, const int ty, const int wdt, const int hgt)
{
	// safety
	if (!sfcSource || !sfcTarget || !wdt || !hgt) return;
	assert(sfcTarget->IsRenderTarget());
	assert(!(dwBlitMode & C4GFXBLIT_MOD2));
	// bound
	if (ClipAll) return;
	// inside screen?
	if (wdt <= 0 || hgt <= 0) return;
	// prepare rendering to surface
	if (!PrepareRendering(sfcTarget)) return;
	// texture present?
	if (!sfcSource->ppTex) return;
	// blit with basesfc?
	// get involved texture offsets
	int iTexSize = sfcSource->iTexSize;
	const int iTexX = (std::max)(fx / iTexSize, 0);
	const int iTexY = (std::max)(fy / iTexSize, 0);
	const int iTexX2 = (std::min)((fx + wdt - 1) / iTexSize + 1, sfcSource->iTexX);
	const int iTexY2 = (std::min)((fy + hgt - 1) / iTexSize + 1, sfcSource->iTexY);
	// blit from all these textures
	SetTexture();
	if (sfcSource2)
	{
		glActiveTexture(GL_TEXTURE1);
		glEnable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE2);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, (*sfcLiquidAnimation->ppTex)->texName);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glActiveTexture(GL_TEXTURE0);
	}
	uint32_t dwModMask = 0;
	if (LandscapeShader)
	{
		LandscapeShader.Select();

		if (sfcSource2)
		{
			static GLfloat value[4] = { -0.6f / 3, 0.0f, 0.6f / 3, 0.0f };
			value[0] += 0.05f; value[1] += 0.05f; value[2] += 0.05f;
			GLfloat mod[4];
			for (int i = 0; i < 3; ++i)
			{
				if (value[i] > 0.9f) value[i] = -0.3f;
				mod[i] = (value[i] > 0.3f ? 0.6f - value[i] : value[i]) / 3.0f;
			}
			mod[3] = 0;

			LandscapeShader.SetUniform("modulation", glUniform4fv, 1, mod);
		}
	}
	// texture environment
	else
	{
		if (Config.Graphics.NoAlphaAdd)
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
	glShadeModel((fUseClrModMap && !Config.Graphics.NoBoxFades) ? GL_SMOOTH : GL_FLAT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();

	const uint32_t dwModClr = BlitModulated ? BlitModulateClr : 0xffffff;
	int chunkSize = iTexSize;
	if (fUseClrModMap && dwModClr)
	{
		chunkSize = std::min(iTexSize, 64);
	}

	for (int iY = iTexY; iY < iTexY2; ++iY)
	{
		for (int iX = iTexX; iX < iTexX2; ++iX)
		{
			// blit

			if (sfcSource2) glActiveTexture(GL_TEXTURE0);
			const auto *const pTex = *(sfcSource->ppTex + iY * sfcSource->iTexX + iX);
			// get current blitting offset in texture (beforing any last-tex-size-changes)
			const int iBlitX = iTexSize * iX;
			const int iBlitY = iTexSize * iY;
			glBindTexture(GL_TEXTURE_2D, pTex->texName);
			if (sfcSource2)
			{
				const auto *const pTex = *(sfcSource2->ppTex + iY * sfcSource2->iTexX + iX);
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, pTex->texName);
			}

			int maxXChunk = std::min<int>((fx + wdt - iBlitX - 1) / chunkSize + 1, iTexSize / chunkSize);
			int maxYChunk = std::min<int>((fy + hgt - iBlitY - 1) / chunkSize + 1, iTexSize / chunkSize);

			// size changed? recalc dependent, relevant (!) values
			if (iTexSize != pTex->iSize) iTexSize = pTex->iSize;
			for (int yChunk = std::max<int>((fy - iBlitY) / chunkSize, 0); yChunk < maxYChunk; ++yChunk)
			{
				for (int xChunk = std::max<int>((fx - iBlitX) / chunkSize, 0); xChunk < maxXChunk; ++xChunk)
				{
					int xOffset = xChunk * chunkSize;
					int yOffset = yChunk * chunkSize;
					// draw triangle strip
					glBegin(GL_TRIANGLE_STRIP);
					// Get new texture source bounds
					const auto fTexBltLeft = static_cast<float>(std::max(xOffset, fx - iBlitX));
					const auto fTexBltTop  = static_cast<float>(std::max(yOffset, fy - iBlitY));
					const auto fTexBltRight  = static_cast<float>(std::min(fx + wdt - iBlitX, xOffset + chunkSize));
					const auto fTexBltBottom = static_cast<float>(std::min(fy + hgt - iBlitY, yOffset + chunkSize));
					// Get new dest bounds
					const float tTexBltLeft  {fTexBltLeft   + iBlitX - fx + tx};
					const float tTexBltTop   {fTexBltTop    + iBlitY - fy + ty};
					const float tTexBltRight {fTexBltRight  + iBlitX - fx + tx};
					const float tTexBltBottom{fTexBltBottom + iBlitY - fy + ty};

					// Get blit positions
					const std::array<float, 4>
						ftx{tTexBltLeft, tTexBltRight, tTexBltLeft  , tTexBltRight },
						fty{tTexBltTop , tTexBltTop  , tTexBltBottom, tTexBltBottom},
						tcx{fTexBltLeft, fTexBltRight, fTexBltLeft  , fTexBltRight },
						tcy{fTexBltTop , fTexBltTop  , fTexBltBottom, fTexBltBottom};

					uint32_t fdwModClr[4]; // color modulation
					// global modulation map
					if (fUseClrModMap && dwModClr)
					{
						for (int i = 0; i < 4; ++i)
						{
							fdwModClr[i] = pClrModMap->GetModAt(
								static_cast<int>(ftx[i]), static_cast<int>(fty[i]));
							ModulateClr(fdwModClr[i], dwModClr);
						}
					}
					else
					{
						std::fill(fdwModClr, std::end(fdwModClr), dwModClr);
					}

					for (int i = 0; i < 4; ++i)
					{
						glColorDw(fdwModClr[i] | dwModMask);
						glTexCoord2f((tcx[i] + texIndent) / iTexSize,
							(tcy[i] + texIndent) / iTexSize);
						if (sfcSource2)
						{
							glMultiTexCoord2f(GL_TEXTURE1_ARB,
								(tcx[i] + texIndent) / iTexSize,
								(tcy[i] + texIndent) / iTexSize);
							glMultiTexCoord2f(GL_TEXTURE2_ARB,
								(tcx[i] + texIndent) / sfcLiquidAnimation->iTexSize,
								(tcy[i] + texIndent) / sfcLiquidAnimation->iTexSize);
						}
						glVertex2f(ftx[i] + blitOffset, fty[i] + blitOffset);
					}

					glEnd();
				}
			}
		}
	}
	if (LandscapeShader)
	{
		CStdShaderProgram::Deselect();
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
	logger->info("Using OpenGL...");
	return true;
}

CStdGLCtx *CStdGL::CreateContext(CStdWindow *const pWindow, CStdApp *const pApp)
{
	// safety
	if (!pWindow) return nullptr;

#ifdef USE_SDL_MAINLOOP
	if (SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1) != 0)
	{
		logger->error("SDL: Enabling context sharing failed: {}", SDL_GetError());
	}
#endif

	// create it
	const auto pCtx = new CStdGLCtx();
	if (!pCtx->Init(pWindow, pApp))
	{
		delete pCtx; logger->error("Error creating secondary context!"); return nullptr;
	}
	// done
	return pCtx;
}

#ifdef _WIN32
CStdGLCtx *CStdGL::CreateContext(const HWND hWindow, CStdApp *const pApp)
{
	// safety
	if (!hWindow) return nullptr;
	// create it
	const auto pCtx = new CStdGLCtx();
	if (!pCtx->Init(nullptr, pApp, hWindow))
	{
		delete pCtx; logger->error("Error creating secondary context!"); return nullptr;
	}
	// done
	return pCtx;
}
#endif

bool CStdGL::CreatePrimarySurfaces()
{
	// create lpPrimary and lpBack (used in first context selection)
	lpPrimary = lpBack = new C4Surface();
	lpPrimary->fPrimary = true;
	lpPrimary->AttachSfc(nullptr);

	// create+select gl context
	if (!MainCtx.Init(pApp->pWindow, pApp))
	{
		logger->error("Error initializing context");
		return false;
	}

	// done, init device stuff
	return RestoreDeviceObjects();
}

void CStdGL::DrawQuadDw(C4Surface *const sfcTarget, int *const ipVtx,
	uint32_t dwClr1, uint32_t dwClr2, uint32_t dwClr3, uint32_t dwClr4)
{
	// prepare rendering to target
	if (!PrepareRendering(sfcTarget)) return;

	CStdGLShaderProgram::Deselect();

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
	if (Config.Graphics.NoBoxFades)
	{
		NormalizeColors(dwClr1, dwClr2, dwClr3, dwClr4);
		glShadeModel(GL_FLAT);
	}
	else
		glShadeModel((dwClr1 == dwClr2 && dwClr1 == dwClr3 && dwClr1 == dwClr4) ? GL_FLAT : GL_SMOOTH);
	// set blitting state
	const int iAdditive = dwBlitMode & C4GFXBLIT_ADDITIVE;
	glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, iAdditive ? GL_ONE : GL_SRC_ALPHA);
	// draw two triangles
	if (DummyShader)
	{
		DummyShader.Select();
	}
	glBegin(GL_TRIANGLE_STRIP);
	glColorDw(dwClr1); glVertex2f(ipVtx[0] + blitOffset, ipVtx[1] + blitOffset);
	glColorDw(dwClr2); glVertex2f(ipVtx[2] + blitOffset, ipVtx[3] + blitOffset);
	glColorDw(dwClr4); glVertex2f(ipVtx[6] + blitOffset, ipVtx[7] + blitOffset);
	glColorDw(dwClr3); glVertex2f(ipVtx[4] + blitOffset, ipVtx[5] + blitOffset);
	glEnd();
	glShadeModel(GL_FLAT);
}

void CStdGL::DrawLineDw(C4Surface *const sfcTarget,
	const float x1, const float y1, const float x2, const float y2, uint32_t dwClr)
{
	// apply color modulation
	ClrByCurrentBlitMod(dwClr);
	// render target?
	assert(sfcTarget->IsRenderTarget());
	// prepare rendering to target
	if (!PrepareRendering(sfcTarget)) return;

	CStdGLShaderProgram::Deselect();

	// set blitting state
	const int iAdditive = dwBlitMode & C4GFXBLIT_ADDITIVE;
	// use a different blendfunc here, because GL_LINE_SMOOTH expects this one
	glBlendFunc(GL_SRC_ALPHA, iAdditive ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA);
	// draw one line
	if (DummyShader)
	{
		DummyShader.Select();
	}
	glBegin(GL_LINES);
	// global clr modulation map
	uint32_t dwClr1 = dwClr;
	if (fUseClrModMap)
	{
		ModulateClr(dwClr1, pClrModMap->GetModAt(
			static_cast<int>(x1), static_cast<int>(y1)));
	}
	// convert from clonk-alpha to GL_LINE_SMOOTH alpha
	glColorDw(InvertRGBAAlpha(dwClr1));
	glVertex2f(x1 + 0.5f, y1 + 0.5f);
	if (fUseClrModMap)
	{
		ModulateClr(dwClr, pClrModMap->GetModAt(
			static_cast<int>(x2), static_cast<int>(y2)));
		glColorDw(InvertRGBAAlpha(dwClr));
	}
	glVertex2f(x2 + 0.5f, y2 + 0.5f);
	glEnd();
}

void CStdGL::DrawPixInt(C4Surface *const sfcTarget,
	const float tx, const float ty, const uint32_t dwClr)
{
	// render target?
	assert(sfcTarget->IsRenderTarget());

	if (!PrepareRendering(sfcTarget)) return;

	CStdGLShaderProgram::Deselect();

	const int iAdditive = dwBlitMode & C4GFXBLIT_ADDITIVE;
	// use a different blendfunc here because of GL_POINT_SMOOTH
	glBlendFunc(GL_SRC_ALPHA, iAdditive ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA);
	// convert the alpha value for that blendfunc
	if (DummyShader)
	{
		DummyShader.Select();
	}
	glBegin(GL_POINTS);
	glColorDw(InvertRGBAAlpha(dwClr));
	glVertex2f(tx + 0.5f, ty + 0.5f);
	glEnd();
}

void CStdGL::DisableGamma()
{
	if (Config.Graphics.DisableGamma)
	{
		return;
	}
	else if (Config.Graphics.Shader)
	{
		GammaTexture.Clear();
		glActiveTexture(GL_TEXTURE3);
		glDisable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
	}
	else
	{
		CStdDDraw::DisableGamma();
	}
}

void CStdGL::EnableGamma()
{
	if (Config.Graphics.DisableGamma)
	{
		return;
	}
	else if (Config.Graphics.Shader)
	{
		assert(GammaTexture);
	}

	CStdDDraw::EnableGamma();
}

bool CStdGL::ApplyGammaRamp(CGammaControl &ramp, bool force)
{
	if (Config.Graphics.DisableGamma) return true;

	else if (GammaTexture)
	{
		glActiveTexture(GL_TEXTURE3);
		GammaTexture.UpdateData(GL_TEXTURE_2D, ramp.red);
		glActiveTexture(GL_TEXTURE0);
		return true;
	}

	return ApplyGammaRampToMonitor(ramp, force);
}

bool CStdGL::SaveDefaultGammaRamp(CStdWindow *window)
{
	if (GammaTexture || Config.Graphics.DisableGamma)
	{
		DefRamp.Default();
		Gamma.Set(0x000000, 0x808080, 0xffffff, 256, &DefRamp);
		return true;
	}

	return SaveDefaultGammaRampToMonitor(window);
}

bool CStdGL::RestoreDeviceObjects()
{
	// safety
	if (!lpPrimary) return false;
	// delete any previous objects
	InvalidateDeviceObjects();
	// restore primary/back
	RenderTarget = lpPrimary;
	lpPrimary->AttachSfc(nullptr);

	// set states
	const bool fSuccess = pCurrCtx ? pCurrCtx->Select() : MainCtx.Select();
	// activate if successful
	Active = fSuccess;
	// reset blit states
	dwBlitMode = 0;

	blitOffset = static_cast<float>(Config.Graphics.BlitOffset) / 100;
	texIndent = static_cast<float>(Config.Graphics.TexIndent) / 1000;

	if (Config.Graphics.Shader && !BlitShader)
	{
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback([](auto, auto, auto, auto, auto, const GLchar *const message, auto)
		{
			LogF("%s", message);
		}, nullptr);

		try
		{
			CStdGLShader vertexShader{CStdShader::Type::Vertex,
				R"(
				#version 120

				void main()
				{
					gl_Position = ftransform();
					gl_FrontColor = gl_Color;
					gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
				#ifdef LC_COLOR_ANIMATION
					gl_TexCoord[1] = gl_TextureMatrix[0] * gl_MultiTexCoord1;
					gl_TexCoord[2] = gl_TextureMatrix[0] * gl_MultiTexCoord2;
				#endif
				}
				)"
			};

			vertexShader.Compile();

			CStdGLShader blitFragmentShader{CStdShader::Type::Fragment,
				R"(
				#version 120

				uniform sampler2D textureSampler;

				#ifdef LC_GAMMA
				uniform sampler2D gamma;
				#endif

				void main()
				{
					vec4 fragColor = texture2D(textureSampler, gl_TexCoord[0].st);

				#ifdef LC_MOD2
					fragColor.rgb += gl_Color.rgb;
					fragColor.rgb = clamp(fragColor.rgb * 2.0 - 1.0, 0.0, 1.0);
				#else
					fragColor.rgb *= gl_Color.rgb;
					fragColor.rgb = clamp(fragColor.rgb, 0.0, 1.0);
					fragColor.a = clamp(fragColor.a + gl_Color.a, 0.0, 1.0);
				#endif

				#ifdef LC_GAMMA
					fragColor.r = texture2D(gamma, vec2(fragColor.r, 0)).r;
					fragColor.g = texture2D(gamma, vec2(fragColor.g, 1)).r;
					fragColor.b = texture2D(gamma, vec2(fragColor.b, 2)).r;
				#endif

					gl_FragColor = fragColor;
				}
				)"
			};

			if (!Config.Graphics.DisableGamma)
			{
				blitFragmentShader.SetMacro("LC_GAMMA", "1");
			}

			blitFragmentShader.Compile();

			BlitShader.AddShader(&vertexShader);
			BlitShader.AddShader(&blitFragmentShader);
			BlitShader.Link();

			blitFragmentShader.SetMacro("LC_MOD2", "1");

			blitFragmentShader.Compile();

			BlitShaderMod2.AddShader(&vertexShader);
			BlitShaderMod2.AddShader(&blitFragmentShader);
			BlitShaderMod2.Link();

			CStdGLShader landscapeFragmentShader{CStdShader::Type::Fragment,
				R"(
				#version 120

				uniform sampler2D textureSampler;
				#ifdef LC_COLOR_ANIMATION
				uniform sampler2D maskSampler;
				uniform sampler2D liquidSampler;
				uniform vec4 modulation;
				#endif

				#ifdef LC_GAMMA
				uniform sampler2D gamma;
				#endif

				void main()
				{
					vec4 fragColor = texture2D(textureSampler,  gl_TexCoord[0].st);
				#ifdef LC_COLOR_ANIMATION
					float mask = texture2D(maskSampler,  gl_TexCoord[1].st).a;
					vec3 liquid = texture2D(liquidSampler,  gl_TexCoord[2].st).rgb;
					liquid -= vec3(0.5, 0.5, 0.5);
					liquid = vec3(dot(liquid, modulation.rgb));
					liquid *= mask;
					fragColor.rgb = fragColor.rgb + liquid;
				#endif
					fragColor.rgb = clamp(fragColor.rgb, 0.0, 1.0) * gl_Color.rgb;
					fragColor.a = clamp(fragColor.a + gl_Color.a, 0.0, 1.0);

				#ifdef LC_GAMMA
					fragColor.r = texture2D(gamma, vec2(fragColor.r, 0)).r;
					fragColor.g = texture2D(gamma, vec2(fragColor.g, 1)).r;
					fragColor.b = texture2D(gamma, vec2(fragColor.b, 2)).r;
				#endif

					gl_FragColor = fragColor;
				}
				)"};

			if (Config.Graphics.ColorAnimation)
			{
				vertexShader.SetMacro("LC_COLOR_ANIMATION", "1");
				vertexShader.Compile();
				landscapeFragmentShader.SetMacro("LC_COLOR_ANIMATION", "1");
			}

			if (!Config.Graphics.DisableGamma)
			{
				landscapeFragmentShader.SetMacro("LC_GAMMA", "1");
			}

			landscapeFragmentShader.Compile();

			LandscapeShader.AddShader(&vertexShader);
			LandscapeShader.AddShader(&landscapeFragmentShader);
			LandscapeShader.Link();

			if (!Config.Graphics.DisableGamma)
			{
				CStdGLShader dummyVertexShader{CStdShader::Type::Vertex,
				R"(
					#version 120

					void main()
					{
						gl_Position = ftransform();
						gl_FrontColor = gl_Color;
					}
				)"};

				dummyVertexShader.Compile();

				CStdGLShader dummyFragmentShader{CStdShader::Type::Fragment,
					R"(
					#version 120

					uniform sampler2D gamma;

					void main()
					{
						gl_FragColor.r = texture2D(gamma, vec2(gl_Color.r, 0)).r;
						gl_FragColor.g = texture2D(gamma, vec2(gl_Color.g, 0)).r;
						gl_FragColor.b = texture2D(gamma, vec2(gl_Color.b, 0)).r;
						gl_FragColor.a = gl_Color.a;
					}
					)"
				};

				dummyFragmentShader.Compile();

				DummyShader.AddShader(&dummyVertexShader);
				DummyShader.AddShader(&dummyFragmentShader);
				DummyShader.Link();
			}

			const auto setUniforms = [this](CStdGLShaderProgram &program)
			{
				program.Select();
				program.SetUniform("texIndent", texIndent);
				program.SetUniform("blitOffset", blitOffset);
				program.SetUniform("textureSampler", glUniform1i, 0);

				if (!Config.Graphics.DisableGamma)
				{
					program.SetUniform("gamma", glUniform1i, 3);
				}
			};

			setUniforms(BlitShader);
			setUniforms(BlitShaderMod2);

			if (DummyShader)
			{
				setUniforms(DummyShader);
			}

			setUniforms(LandscapeShader); // Last so that the shader is selected for the subsequent calls

			LandscapeShader.SetUniform("maskSampler", glUniform1i, 1);
			LandscapeShader.SetUniform("liquidSampler", glUniform1i, 2);

			CStdShaderProgram::Deselect();

			if (!Config.Graphics.DisableGamma)
			{
				GammaTexture = {Gamma.GetSize(), 3, GL_R16, GL_RED, GL_UNSIGNED_SHORT};
				GammaTexture.Bind(GL_TEXTURE_2D, 3);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

				GammaTexture.SetData(GL_TEXTURE_2D, nullptr);

				glActiveTexture(GL_TEXTURE3);
				glEnable(GL_TEXTURE_2D);
				// Don't switch back to GL_TEXTURE0 - EnableGamma does this
			}
		}
		catch (const CStdRenderException &e)
		{
			LogFatalNTr(e.what());
			return Active = false;
		}
	}
	// restore gamma if active
	if (Active) EnableGamma();
	// done
	return Active;
}

bool CStdGL::InvalidateDeviceObjects()
{
	// clear gamma
#ifdef USE_SDL_MAINLOOP
	if (Config.Graphics.Shader)
#endif
		CStdGL::DisableGamma();
	// deactivate
	Active = false;
	// invalidate font objects
	// invalidate primary surfaces
	if (lpPrimary) lpPrimary->Clear();
	if (BlitShader)
	{
		BlitShader.Clear();
		BlitShaderMod2.Clear();
		LandscapeShader.Clear();
		DummyShader.Clear();
	}
	return true;
}

void CStdGL::SetTexture()
{
	glBlendFunc(GL_ONE_MINUS_SRC_ALPHA,
		(dwBlitMode & C4GFXBLIT_ADDITIVE) ? GL_ONE : GL_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);
}

void CStdGL::ResetTexture()
{
	// disable texturing
	glDisable(GL_TEXTURE_2D);
}

CStdGL *pGL = nullptr;

bool CStdGL::OnResolutionChanged()
{
	InvalidateDeviceObjects();
	RestoreDeviceObjects();
	// Re-create primary clipper to adapt to new size.
	CreatePrimaryClipper();
	return true;
}

void CStdGL::Default()
{
	CStdDDraw::Default();
	sfcFmt = 0;
	MainCtx.Clear();
}

#endif
