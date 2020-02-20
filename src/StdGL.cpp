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

#ifndef USE_CONSOLE

#include <stdio.h>
#include <math.h>
#include <limits.h>

namespace
{
	template<typename VertexType = GLfloat>
	struct Vertex
	{
		VertexType coordinates[2];
		GLuint color;
	};

	template<typename VertexType = GLfloat>
	struct TextureVertex
	{
		VertexType coordinates[2];
		GLuint color;
		GLfloat textureCoordinates[2];
	};

	template<typename VertexType = GLfloat>
	struct LiquidShadedTextureVertex
	{
		VertexType coordinates[2];
		GLuint color;
		GLfloat textureCoordinates[2];
		GLfloat liquidTextureCoordinates[2];
	};
}

[[deprecated("Immediate mode is deprecated")]]
static void glColorDw(const uint32_t dwClr)
{
	glColor4ub(
		static_cast<GLubyte>(dwClr >> 16),
		static_cast<GLubyte>(dwClr >> 8),
		static_cast<GLubyte>(dwClr),
		static_cast<GLubyte>(dwClr >> 24));
}

template<typename T>
static void SplitColor(const uint32_t &color, T &r, T&g, T&b, T&a)
{
	static_assert(std::is_floating_point_v<T>);
	r = ((color >> 16) & 0xff) / static_cast<T>(255.0f);
	g = ((color >> 8) & 0xff) / static_cast<T>(255.0f);
	b = (color & 0xff) / static_cast<T>(255.0f);
	a = ((color >> 24) & 0xff) / static_cast<T>(255.0f);
}

template<typename T>
static void SplitColor(const uint32_t &color, T (&c)[4])
{
	SplitColor(color, c[0], c[1], c[2], c[3]);
}

CStdGLShader::CStdGLShader(CStdGLShader &&s)
{
	shader = s.shader;
	source = std::move(s.source);

	errorMessage = std::move(s.errorMessage);
	s.Clear();
}

void CStdGLShader::SetSource(const std::string &source)
{
	this->source = source;
}

void CStdGLShader::Clear()
{
	source.clear();
	macros.clear();

	if (shader)
	{
		glDeleteShader(shader);
		shader = 0;
	}
	errorMessage.clear();
}

GLuint CStdGLShader::Compile()
{
	if (shader) // recompiling?
	{
		glDeleteShader(shader);
		errorMessage.clear();
	}

	shader = glCreateShader(GetType());
	if (!shader)
	{
		return SetError("Could not create shader");
	}

	size_t pos = source.find("#version");
	if (pos == std::string::npos)
	{
		glDeleteShader(shader);
		return SetError("Version directive must be first statement and may not be repeated");
	}

	pos = source.find('\n', pos + 1);
	assert(pos != std::string::npos);

	std::string buffer = "";
	for (const auto &macro : macros)
	{
		buffer.append("#define ");
		buffer.append(macro.first);
		buffer.append(" ");
		buffer.append(macro.second);
		buffer.append("\n");
	}
	source.insert(pos + 1, buffer);

	const char *s = source.c_str();
	glShaderSource(shader, 1, &s, nullptr);

	GLint status = 0;
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (!status)
	{
		GLint size = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &size);
		if (size)
		{
			errorMessage.resize(size);
			glGetShaderInfoLog(shader, size, NULL, errorMessage.data());
		}

		return 0;
	}

	return shader;
}

void CStdGLShaderProgram::AddShader(CStdGLShader *shader)
{
	EnsureProgram();
	if (std::find(shaders.cbegin(), shaders.cend(), shader) != shaders.cend())
	{
		return;
	}

	glAttachShader(shaderProgram, shader->GetShader());
	shaders.push_back(shader);
}

bool CStdGLShaderProgram::Link()
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
			errorMessage.resize(size);
			glGetProgramInfoLog(shaderProgram, size, NULL, errorMessage.data());
			LogF("Error linking shader program: %s", errorMessage.c_str());
		}

		return false;
	}

	glValidateProgram(shaderProgram);
	glGetProgramiv(shaderProgram, GL_VALIDATE_STATUS, &status);
	if (!status)
	{
		GLint size = 0;
		glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &size);
		if (size)
		{
			errorMessage.resize(size);
			glGetProgramInfoLog(shaderProgram, size, NULL, errorMessage.data());
		}

		return false;
	}

	for (const auto &shader : shaders)
	{
		glDetachShader(shaderProgram, shader->GetShader());
	}

	shaders.clear();

	return true;
}

void CStdGLShaderProgram::Select()
{
	assert(shaderProgram);
	glUseProgram(shaderProgram);
}

void CStdGLShaderProgram::Deselect()
{
	glUseProgram(0);
}

void CStdGLShaderProgram::Clear()
{
	shaders.clear();
	uniformLocations.clear();
	glDeleteProgram(shaderProgram);
	shaderProgram = 0;
}

void CStdGLShaderProgram::EnsureProgram()
{
	if (!shaderProgram)
	{
		shaderProgram = glCreateProgram();
	}
	assert(shaderProgram);
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

bool CStdGL::PageFlip(RECT *, RECT *, CStdWindow *)
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
	const auto left = static_cast<GLfloat>(iX);
	const auto right = static_cast<GLfloat>(iX + iWdt);
	const auto bottom = static_cast<GLfloat>(iY + iHgt);
	const auto top = static_cast<GLfloat>(iY);

	GLfloat ortho2D[16] { // column major!
		2.f / (right - left), 0, 0, 0,
		0, 2.f / (top - bottom), 0, 0,
		0, 0, -1, 0,
		-(right + left) / (right - left), -(top + bottom) / (top - bottom), 0, 1
	};
	glBufferSubData(GL_UNIFORM_BUFFER, StandardUniforms.Offset[StandardUniforms.ProjectionMatrix], sizeof(ortho2D), ortho2D);
	return true;
}

bool CStdGL::PrepareRendering(CSurface *const sfcToSurface)
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

static constexpr GLfloat IDENTITY_MATRIX[16]
{
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1
};

static std::array<Vertex<>, std::tuple_size_v<decltype(CBltData::vtVtx)>> PerformBltVertexData;

void CStdGL::PerformBlt(CBltData &rBltData, CTexRef *const pTex,
	const uint32_t dwModClr, bool fMod2, const bool fExact)
{
	// global modulation map
	uint32_t dwModMask = 0;
	bool fAnyModNotBlack;
	bool fModClr = false;
	if (fUseClrModMap && dwModClr)
	{
		fAnyModNotBlack = false;
		for (size_t i = 0; i < PerformBltVertexData.size(); ++i)
		{
			float x = rBltData.vtVtx[i].ftx;
			float y = rBltData.vtVtx[i].fty;
			if (rBltData.pTransform)
			{
				rBltData.pTransform->TransformPoint(x, y);
			}
			rBltData.vtVtx[i].dwModClr = pClrModMap->GetModAt(static_cast<int>(x), static_cast<int>(y));
			if (rBltData.vtVtx[i].dwModClr >> 24) dwModMask = 0xff000000;
			ModulateClr(rBltData.vtVtx[i].dwModClr, dwModClr);
			if (rBltData.vtVtx[i].dwModClr) fAnyModNotBlack = true;
			if (rBltData.vtVtx[i].dwModClr != 0xffffff) fModClr = true;
		}
	}
	else
	{
		fAnyModNotBlack = !!dwModClr;
		for (size_t i = 0; i < PerformBltVertexData.size(); ++i)
			rBltData.vtVtx[i].dwModClr = dwModClr;
		if (dwModClr != 0xffffff) fModClr = true;
	}
	// reset MOD2 for completely black modulations
	if (fMod2 && !fAnyModNotBlack) fMod2 = 0;

	CStdGLShaderProgram &shader = fMod2 ? BlitShaderMod2 : BlitShader;
	shader.Select();
	shader.SetUniform("textureSampler", glUniform1i, 0);

	// set texture+modes
	//glShadeModel((fUseClrModMap && fModClr && !DDrawCfg.NoBoxFades) ? GL_SMOOTH : GL_FLAT);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, pTex->texName);
	if (pApp->GetScale() != 1.f || (!fExact && !DDrawCfg.PointFiltering))
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}

	GLfloat matrix[16];
	matrix[0] = rBltData.TexPos.mat[0];  matrix[ 1] = rBltData.TexPos.mat[3];  matrix[ 2] = 0; matrix[ 3] = rBltData.TexPos.mat[6];
	matrix[4] = rBltData.TexPos.mat[1];  matrix[ 5] = rBltData.TexPos.mat[4];  matrix[ 6] = 0; matrix[ 7] = rBltData.TexPos.mat[7];
	matrix[8] = 0;                       matrix[ 9] = 0;                       matrix[10] = 1; matrix[11] = 0;
	matrix[12] = rBltData.TexPos.mat[2]; matrix[13] = rBltData.TexPos.mat[5];  matrix[14] = 0; matrix[15] = rBltData.TexPos.mat[8];

	glBufferSubData(GL_UNIFORM_BUFFER, StandardUniforms.Offset[StandardUniforms.TextureMatrix], sizeof(matrix), matrix);

	if (rBltData.pTransform)
	{
		const float *const mat = rBltData.pTransform->mat;
		matrix[ 0] = mat[0]; matrix[ 1] = mat[3]; matrix[ 2] = 0; matrix[ 3] = mat[6];
		matrix[ 4] = mat[1]; matrix[ 5] = mat[4]; matrix[ 6] = 0; matrix[ 7] = mat[7];
		matrix[ 8] = 0;      matrix[ 9] = 0;      matrix[10] = 1; matrix[11] = 0;
		matrix[12] = mat[2]; matrix[13] = mat[5]; matrix[14] = 0; matrix[15] = mat[8];

		glBufferSubData(GL_UNIFORM_BUFFER, StandardUniforms.Offset[StandardUniforms.ModelViewMatrix], sizeof(matrix), matrix);
	}
	else
	{
		glBufferSubData(GL_UNIFORM_BUFFER, StandardUniforms.Offset[StandardUniforms.ModelViewMatrix], sizeof(IDENTITY_MATRIX), IDENTITY_MATRIX);
	}

	// draw triangle fan for speed
	static_assert(PerformBltVertexData.size() == 4, "GL_TRIANGLE_STRIP cannot be used with more than 4 vertices; you need to add the necessary code.");

	std::array<CBltVertex, PerformBltVertexData.size()> vtx {rBltData.vtVtx[0], rBltData.vtVtx[1], rBltData.vtVtx[3], rBltData.vtVtx[2]};

	for (size_t i = 0; i < vtx.size(); ++i)
	{
		PerformBltVertexData[i].coordinates[0] = vtx[i].ftx;
		PerformBltVertexData[i].coordinates[1] = vtx[i].fty;
		PerformBltVertexData[i].color = vtx[i].dwModClr;
	}

	SelectVAO(VertexArray.PerformBlt);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(PerformBltVertexData), PerformBltVertexData.data());

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	shader.Deselect();

	if (pApp->GetScale() != 1.f || (!fExact && !DDrawCfg.PointFiltering))
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}
}

static std::array<LiquidShadedTextureVertex<>, 4> BlitLandscapeVertexData;

void CStdGL::BlitLandscape(CSurface *const sfcSource, CSurface *const sfcSource2,
	CSurface *const sfcLiquidAnimation, const int fx, const int fy,
	CSurface *const sfcTarget, const int tx, const int ty, const int wdt, const int hgt)
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
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, (*sfcLiquidAnimation->ppTex)->texName);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glActiveTexture(GL_TEXTURE0);
	}

	LandscapeShader.Select();
	LandscapeShader.SetUniform("textureSampler", glUniform1i, 0);

	if (sfcSource2)
	{
		LandscapeShader.SetUniform("maskSampler", glUniform1i, 1);
		LandscapeShader.SetUniform("liquidSampler", glUniform1i, 2);

		static GLfloat value[4] = { -0.6f / 3, 0.0f, 0.6f / 3, 0.0f };
		value[0] += 0.05f; value[1] += 0.05f; value[2] += 0.05f;
		GLfloat mod[4];
		for (int i = 0; i < 3; ++i)
		{
			if (value[i] > 0.9f) value[i] = -0.3f;
			mod[i] = (value[i] > 0.3f ? 0.6f - value[i] : value[i]) / 3.0f;
		}

		LandscapeShader.SetUniform("modulation", glUniform4f, mod[0], mod[1], mod[2], mod[3]);
	}

	// set texture+modes
	//glShadeModel((fUseClrModMap && !DDrawCfg.NoBoxFades) ? GL_SMOOTH : GL_FLAT);

	glBufferSubData(GL_UNIFORM_BUFFER, StandardUniforms.Offset[StandardUniforms.TextureMatrix], sizeof(IDENTITY_MATRIX), IDENTITY_MATRIX);
	glBufferSubData(GL_UNIFORM_BUFFER, StandardUniforms.Offset[StandardUniforms.ModelViewMatrix], sizeof(IDENTITY_MATRIX), IDENTITY_MATRIX);

	const uint32_t dwModClr = BlitModulated ? BlitModulateClr : 0xffffff;
	int chunkSize = iTexSize;
	if (fUseClrModMap && dwModClr)
	{
		chunkSize = std::min(iTexSize, 64);
	}

	SelectVAO(VertexArray.BlitLandscape);

	if (DDrawCfg.ColorAnimation)
	{
		glEnableVertexAttribArray(VertexArray.LiquidTexCoords);
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

					// get new texture source bounds
					FLOAT_RECT fTexBlt;
					// get new dest bounds
					FLOAT_RECT tTexBlt;
					// set up blit data as rect

					fTexBlt.left = std::max<float>(xOffset, fx - iBlitX);
					fTexBlt.top  = std::max<float>(yOffset, fy - iBlitY);
					fTexBlt.right  = std::min<float>(fx + wdt - iBlitX, xOffset + chunkSize);
					fTexBlt.bottom = std::min<float>(fy + hgt - iBlitY, yOffset + chunkSize);

					tTexBlt.left = fTexBlt.left + iBlitX - fx + tx;
					tTexBlt.top  = fTexBlt.top + iBlitY - fy + ty;
					tTexBlt.right  = fTexBlt.right + iBlitX - fx + tx;
					tTexBlt.bottom = fTexBlt.bottom + iBlitY - fy + ty;

					const int texSize = sfcLiquidAnimation->iTexSize;

					new(BlitLandscapeVertexData.data()) decltype(BlitLandscapeVertexData)::value_type[BlitLandscapeVertexData.size()]
					{
						{
							{tTexBlt.left, tTexBlt.top},
							{},
							{fTexBlt.left / iTexSize, fTexBlt.top / iTexSize},
							{fTexBlt.left / texSize,  fTexBlt.top / texSize}
						},
						{
							{tTexBlt.right, tTexBlt.top},
							{},
							{fTexBlt.right / iTexSize, fTexBlt.top / iTexSize},
							{fTexBlt.right / texSize, fTexBlt.top / texSize},
						},
						{
							{tTexBlt.left, tTexBlt.bottom},
							{},
							{fTexBlt.left / iTexSize, fTexBlt.bottom / iTexSize},
							{fTexBlt.left / texSize, fTexBlt.bottom / texSize},
						},
						{
							{tTexBlt.right, tTexBlt.bottom},
							{},
							{fTexBlt.right / iTexSize, fTexBlt.bottom / iTexSize},
							{fTexBlt.right / texSize, fTexBlt.bottom / texSize}
						}
					};


					for (size_t i = 0; i < BlitLandscapeVertexData.size(); ++i)
					{
						if (fUseClrModMap && dwModClr)
						{
							BlitLandscapeVertexData[i].color = pClrModMap->GetModAt(
								static_cast<int>(BlitLandscapeVertexData[i].coordinates[0]), static_cast<int>(BlitLandscapeVertexData[i].coordinates[1]));

							ModulateClr(BlitLandscapeVertexData[i].color, dwModClr);
						}
						else
						{
							BlitLandscapeVertexData[i].color = dwModClr;
						}
					}

					glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(BlitLandscapeVertexData), BlitLandscapeVertexData.data());

					glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
				}
			}
		}
	}

	if (DDrawCfg.ColorAnimation)
	{
		glDisableVertexAttribArray(VertexArray.LiquidTexCoords);
	}

	// reset texture
	ResetTexture();
}

bool CStdGL::CreateDirectDraw()
{
	Log("  Using OpenGL...");
	return true;
}

CStdGLCtx *CStdGL::CreateContext(CStdWindow *const pWindow, CStdApp *const pApp)
{
	// safety
	if (!pWindow) return nullptr;
	// create it
	const auto pCtx = new CStdGLCtx();
	if (!pCtx->Init(pWindow, pApp))
	{
		delete pCtx; Error("  gl: Error creating secondary context!"); return nullptr;
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
		delete pCtx; Error("  gl: Error creating secondary context!"); return nullptr;
	}
	// done
	return pCtx;
}
#endif

bool CStdGL::CreatePrimarySurfaces()
{
	// create lpPrimary and lpBack (used in first context selection)
	lpPrimary = lpBack = new CSurface();
	lpPrimary->fPrimary = true;
	lpPrimary->AttachSfc(nullptr);

	// create+select gl context
	if (!MainCtx.Init(pApp->pWindow, pApp)) return Error("  gl: Error initializing context");

	// done, init device stuff
	return RestoreDeviceObjects();
}

static std::array<Vertex<GLint>, 4> DrawQuadVertexData;

void CStdGL::DrawQuadDw(CSurface *const sfcTarget, int *const ipVtx,
	uint32_t dwClr1, uint32_t dwClr2, uint32_t dwClr3, uint32_t dwClr4)
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
		//glShadeModel(GL_FLAT);
	}
	/*else
		glShadeModel((dwClr1 == dwClr2 && dwClr1 == dwClr3 && dwClr1 == dwClr4) ? GL_FLAT : GL_SMOOTH);*/
	// set blitting state
	glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, (dwBlitMode & C4GFXBLIT_ADDITIVE) ? GL_ONE : GL_SRC_ALPHA);

	DummyShader.Select();

	glBufferSubData(GL_UNIFORM_BUFFER, StandardUniforms.Offset[StandardUniforms.ModelViewMatrix], sizeof(IDENTITY_MATRIX), IDENTITY_MATRIX);

	new(DrawQuadVertexData.data()) decltype(DrawQuadVertexData)::value_type[DrawQuadVertexData.size()]
	{
		{
			{ipVtx[0], ipVtx[1]},
			dwClr1
		},
		{
			{ipVtx[2], ipVtx[3]},
			dwClr2
		},
		{
			{ipVtx[6], ipVtx[7]},
			dwClr3
		},
		{
			{ipVtx[4], ipVtx[5]},
			dwClr4
		}
	};

	SelectVAO(VertexArray.DrawQuadDw);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(DrawQuadVertexData), DrawQuadVertexData.data());

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

static std::array<Vertex<>, 2> DrawLineVertexData;

void CStdGL::DrawLineDw(CSurface *const sfcTarget,
	const float x1, const float y1, const float x2, const float y2, uint32_t dwClr)
{
	// apply color modulation
	ClrByCurrentBlitMod(dwClr);
	// render target?
	assert(sfcTarget->IsRenderTarget());
	// prepare rendering to target
	if (!PrepareRendering(sfcTarget)) return;
	// set blitting state
	glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, (dwBlitMode & C4GFXBLIT_ADDITIVE) ? GL_ONE : GL_SRC_ALPHA);
	// global clr modulation map
	if (fUseClrModMap)
	{
		ModulateClr(dwClr, pClrModMap->GetModAt(
			static_cast<int>(x1), static_cast<int>(y1)));
	}

	DummyShader.Select();
	glBufferSubData(GL_UNIFORM_BUFFER, StandardUniforms.Offset[StandardUniforms.ModelViewMatrix], sizeof(IDENTITY_MATRIX), IDENTITY_MATRIX);

	DrawLineVertexData[0] = Vertex<>{{x1 + 0.5f, y1 + 0.5f}, dwClr};
	DrawLineVertexData[1] = Vertex<>{{x2 + 0.5f, y2 + 0.5f}, dwClr};

	SelectVAO(VertexArray.DrawLineDw);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(DrawLineVertexData), DrawLineVertexData.data());

	glDrawArrays(GL_LINE_STRIP, 0, 2);
}

static Vertex<> DrawPixVertexData;

void CStdGL::DrawPixInt(CSurface *const sfcTarget,
	const float tx, const float ty, const uint32_t dwClr)
{
	// render target?
	assert(sfcTarget->IsRenderTarget());

	if (!PrepareRendering(sfcTarget)) return;

	glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, (dwBlitMode & C4GFXBLIT_ADDITIVE) ? GL_ONE : GL_SRC_ALPHA);

	DummyShader.Select();
	glBufferSubData(GL_UNIFORM_BUFFER, StandardUniforms.Offset[StandardUniforms.ModelViewMatrix], sizeof(IDENTITY_MATRIX), IDENTITY_MATRIX);

	DrawPixVertexData = Vertex<>{{tx, ty}, dwClr};

	SelectVAO(VertexArray.DrawPixInt);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(DrawPixVertexData), &DrawPixVertexData);

	glDrawArrays(GL_POINT_SMOOTH, 0, 1);
}

static void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
	std::printf("source: %d, type: %d, id: %ul, severity: %d, message: %s\n", source, type, id, severity, message);
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
	// restore gamma if active
	if (Active) EnableGamma();
	// reset blit states
	dwBlitMode = 0;

	if (!BlitShader)
	{
		assert(!LandscapeShader);
		glDebugMessageCallback(MessageCallback, nullptr);

		CStdGLVertexShader vertexShader{
					R"(
					#version 140
					#extension GL_ARB_explicit_attrib_location : enable

					layout (std140) uniform StandardUniforms
					{
						uniform float texIndent;
						uniform float blitOffset;
						uniform mat4 textureMatrix;
						uniform mat4 modelViewMatrix;
						uniform mat4 projectionMatrix;
					};

					// standard
					layout (location = 0) in vec2 vertexCoord;
					#ifdef LC_TEXTURE
					layout (location = 1) in vec2 texCoord;
					#endif
					layout (location = 2) in vec4 vertexColor;

					// color animation
					#ifdef LC_COLOR_ANIMATION
					layout (location = 3) in vec2 liquidTexCoord;
					#endif

					out vec2 vPosition;
					#ifdef LC_TEXTURE
					out vec2 vTexCoord;
					#endif
					#ifdef LC_COLOR_ANIMATION
					out vec2 vLiquidTexCoord;
					#endif
					out vec4 vFragColor;

					void main()
					{
					#ifdef LC_TEXTURE
						vTexCoord = (textureMatrix * vec4(texCoord + texIndent, 0.0, 1.0)).xy;
					#endif
					#ifdef LC_COLOR_ANIMATION
						vLiquidTexCoord = liquidTexCoord + texIndent;
					#endif
						vPosition = vertexCoord + blitOffset;
						vFragColor = vertexColor;
						gl_Position = projectionMatrix * modelViewMatrix * vec4(vPosition, 0.0, 1.0);
					}
					)"
				};

		vertexShader.SetMacro("LC_TEXTURE", "1");
		vertexShader.Compile();

		CStdGLFragmentShader blitFragmentShader{
			R"(
			#version 140
			#extension GL_ARB_explicit_attrib_location : enable

			uniform sampler2D textureSampler;

			in vec2 vPosition;
			in vec2 vTexCoord;
			in vec4 vFragColor;
			out vec4 fragColor;

			void main()
			{
				fragColor = texture(textureSampler, vTexCoord);
			#ifdef LC_MOD2
				fragColor.rgb += vFragColor.rgb;
				fragColor.rgb = clamp(fragColor.rgb * 2.0 - 1.0, 0.0, 1.0);
			#else
				fragColor.rgb *= vFragColor.rgb;
				fragColor.rgb = clamp(fragColor.rgb, 0.0, 1.0);
				fragColor.a = clamp(fragColor.a + vFragColor.a, 0.0, 1.0);
			#endif
			}
			)"
		};

		blitFragmentShader.Compile();

		BlitShader.AddShader(&vertexShader);
		BlitShader.AddShader(&blitFragmentShader);
		BlitShader.Link();

		blitFragmentShader.SetMacro("LC_MOD2", "1");
		blitFragmentShader.Compile();

		BlitShaderMod2.AddShader(&vertexShader);
		BlitShaderMod2.AddShader(&blitFragmentShader);
		BlitShaderMod2.Link();

		CStdGLFragmentShader landscapeFragmentShader{
					R"(
					#version 140
					#extension GL_ARB_explicit_attrib_location : enable

					uniform sampler2D textureSampler;
					#ifdef LC_COLOR_ANIMATION
					uniform sampler2D maskSampler;
					uniform sampler2D liquidSampler;
					uniform vec4 modulation;

					in vec2 vLiquidTexCoord;
					#endif

					in vec2 vPosition;
					in vec2 vTexCoord;
					in vec4 vFragColor;
					out vec4 fragColor;

					void main()
					{
						fragColor = texture(textureSampler, vTexCoord);
					#ifdef LC_COLOR_ANIMATION
						float mask = texture(maskSampler, vTexCoord).a;
						vec3 liquid = texture(liquidSampler, vLiquidTexCoord).rgb;

						liquid -= vec3(0.5, 0.5, 0.5);
						liquid = vec3(dot(liquid, modulation.rgb));
						liquid *= mask;
						fragColor.rgb = fragColor.rgb + liquid;
					#endif

						fragColor.rgb = clamp(fragColor.rgb, 0.0, 1.0) * vFragColor.rgb;
						fragColor.a = clamp(fragColor.a + vFragColor.a, 0.0, 1.0);
					}
					)"};

		if (DDrawCfg.ColorAnimation)
		{
			vertexShader.SetMacro("LC_COLOR_ANIMATION", "1");
			vertexShader.Compile();
			landscapeFragmentShader.SetMacro("LC_COLOR_ANIMATION", "1");
		}
		landscapeFragmentShader.Compile();

		LandscapeShader.AddShader(&vertexShader);
		LandscapeShader.AddShader(&landscapeFragmentShader);
		LandscapeShader.Link();

		vertexShader.SetMacro("LC_TEXTURE", nullptr);
		vertexShader.SetMacro("LC_COLOR_ANIMATION", nullptr);
		vertexShader.Compile();

		CStdGLFragmentShader dummyFragmentShader{
			R"(
			#version 140
			#extension GL_ARB_explicit_attrib_location : enable

			in vec2 vPosition;
			in vec4 vFragColor;
			out vec4 fragColor;

			void main()
			{
				fragColor = vFragColor;
			}
			)"
		};

		dummyFragmentShader.Compile();

		DummyShader.AddShader(&vertexShader);
		DummyShader.AddShader(&dummyFragmentShader);
		DummyShader.Link();

		glGenVertexArrays(std::size(VertexArray.VAO), VertexArray.VAO);
		glGenBuffers(std::size(VertexArray.VBO), VertexArray.VBO);

		InitializeVAO<decltype(VertexArray)::PerformBlt, decltype(PerformBltVertexData)>(VertexArray.Vertices, VertexArray.TexCoords, VertexArray.Color);
		glVertexAttribPointer(VertexArray.Vertices, 2, GL_FLOAT, GL_FALSE, sizeof(decltype(PerformBltVertexData)::value_type), nullptr);
		glVertexAttribPointer(VertexArray.TexCoords, 2, GL_FLOAT, GL_FALSE, sizeof(decltype(PerformBltVertexData)::value_type), nullptr);
		glVertexAttribPointer(VertexArray.Color, GL_BGRA, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(decltype(PerformBltVertexData)::value_type), reinterpret_cast<const void*>(offsetof(Vertex<>, color)));

		InitializeVAO<decltype(VertexArray)::BlitLandscape, decltype(BlitLandscapeVertexData)>(VertexArray.Vertices, VertexArray.TexCoords, VertexArray.Color);
		glVertexAttribPointer(VertexArray.Vertices, 2, GL_FLOAT, GL_FALSE, sizeof(decltype(BlitLandscapeVertexData)::value_type), reinterpret_cast<const void*>(offsetof(decltype(BlitLandscapeVertexData)::value_type, coordinates)));
		glVertexAttribPointer(VertexArray.TexCoords, 2, GL_FLOAT, GL_FALSE, sizeof(decltype(BlitLandscapeVertexData)::value_type), reinterpret_cast<const void*>(offsetof(decltype(BlitLandscapeVertexData)::value_type, textureCoordinates)));
		glVertexAttribPointer(VertexArray.Color, GL_BGRA, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(decltype(BlitLandscapeVertexData)::value_type), reinterpret_cast<const void*>(offsetof(decltype(BlitLandscapeVertexData)::value_type, color)));
		glVertexAttribPointer(VertexArray.LiquidTexCoords, 2, GL_FLOAT, GL_FALSE, sizeof(decltype(BlitLandscapeVertexData)::value_type), reinterpret_cast<const void*>(offsetof(decltype(BlitLandscapeVertexData)::value_type, liquidTextureCoordinates)));

		InitializeVAO<decltype(VertexArray)::DrawQuadDw, decltype(DrawQuadVertexData)>(VertexArray.Vertices, VertexArray.Color);
		glVertexAttribPointer(VertexArray.Vertices, 2, GL_INT, GL_FALSE, sizeof(decltype(DrawQuadVertexData)::value_type), reinterpret_cast<const void *>(offsetof(decltype(DrawQuadVertexData)::value_type, coordinates)));
		glVertexAttribPointer(VertexArray.Color, GL_BGRA, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(decltype(DrawQuadVertexData)::value_type), reinterpret_cast<const void *>(offsetof(decltype(DrawQuadVertexData)::value_type, color)));

		InitializeVAO<decltype(VertexArray)::DrawLineDw, decltype(DrawLineVertexData)>(VertexArray.Vertices, VertexArray.Color);
		glVertexAttribPointer(VertexArray.Vertices, 2, GL_FLOAT, GL_FALSE, sizeof(decltype(DrawLineVertexData)::value_type), reinterpret_cast<const void *>(offsetof(decltype(DrawLineVertexData)::value_type, coordinates)));
		glVertexAttribPointer(VertexArray.Color, GL_BGRA, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(decltype(DrawLineVertexData)::value_type), reinterpret_cast<const void *>(offsetof(decltype(DrawLineVertexData)::value_type, color)));

		InitializeVAO<decltype(VertexArray)::DrawLineDw, decltype(DrawPixVertexData)>(VertexArray.Vertices, VertexArray.Color);
		glVertexAttribPointer(VertexArray.Vertices, 2, GL_FLOAT, GL_FALSE, sizeof(decltype(DrawPixVertexData)), reinterpret_cast<const void *>(offsetof(decltype(DrawPixVertexData), coordinates)));
		glVertexAttribPointer(VertexArray.Color, GL_BGRA, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(decltype(DrawPixVertexData)), reinterpret_cast<const void *>(offsetof(decltype(DrawPixVertexData), color)));


		// StandardUniforms

		GLuint indices[std::tuple_size_v<decltype(StandardUniforms.Offset)>]; // std::array::size() or std::size() won't work as they'd require a this pointer
		glGetUniformIndices(BlitShader.GetShaderProgram(), std::size(indices), StandardUniforms.Names, indices);
		glGetActiveUniformsiv(BlitShader.GetShaderProgram(), StandardUniforms.Offset.size(), indices, GL_UNIFORM_OFFSET, StandardUniforms.Offset.data());

		GLuint blockIndex = glGetUniformBlockIndex(BlitShader.GetShaderProgram(), "StandardUniforms");

		GLint blockSize;
		glGetActiveUniformBlockiv(BlitShader.GetShaderProgram(), blockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &blockSize);

		glGenBuffers(1, &StandardUniforms.VBO);
		glBindBuffer(GL_UNIFORM_BUFFER, StandardUniforms.VBO);
		glBufferData(GL_UNIFORM_BUFFER, blockSize, nullptr, GL_DYNAMIC_DRAW);
		glBufferSubData(GL_UNIFORM_BUFFER, StandardUniforms.Offset[StandardUniforms.TexIndent], sizeof(float), &DDrawCfg.fTexIndent);
		glBufferSubData(GL_UNIFORM_BUFFER, StandardUniforms.Offset[StandardUniforms.BlitOffset], sizeof(float), &DDrawCfg.fBlitOff);

		for (const auto *shader : {&BlitShader, &BlitShaderMod2, &LandscapeShader, &DummyShader})
		{
			glUniformBlockBinding(shader->GetShaderProgram(), blockIndex, 0);
		}

		glBindBufferBase(GL_UNIFORM_BUFFER, 0, StandardUniforms.VBO);
	}
	// done
	return Active;
}

bool CStdGL::InvalidateDeviceObjects()
{
	// clear gamma
#ifndef USE_SDL_MAINLOOP
	DisableGamma();
#endif
	// deactivate
	Active = false;

	glUseProgram(GL_NONE);
	BlitShader.Clear();
	BlitShaderMod2.Clear();
	LandscapeShader.Clear();
	DummyShader.Clear();

	glDeleteBuffers(std::size(VertexArray.VBO), VertexArray.VBO);
	glDeleteVertexArrays(std::size(VertexArray.VAO), VertexArray.VAO);

	glDeleteBuffers(1, &StandardUniforms.VBO);

	// invalidate font objects
	// invalidate primary surfaces
	if (lpPrimary) lpPrimary->Clear();
	return true;
}

void CStdGL::SetTexture()
{
	glBlendFunc(GL_ONE_MINUS_SRC_ALPHA,
		(dwBlitMode & C4GFXBLIT_ADDITIVE) ? GL_ONE : GL_SRC_ALPHA);
}

void CStdGL::ResetTexture()
{
}

CStdGL *pGL = nullptr;

bool CStdGL::OnResolutionChanged()
{
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

void CStdGL::SelectVAO(decltype(VertexArray)::ArrayIndex index)
{
	assert(index < VertexArray.NumVAO);
	glBindVertexArray(VertexArray.VAO[index]);
	glBindBuffer(GL_ARRAY_BUFFER, VertexArray.VBO[index]);
}

#endif
