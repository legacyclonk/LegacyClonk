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

#pragma once

#ifndef USE_CONSOLE

#include <GL/glew.h>

#ifdef USE_X11
// Xmd.h typedefs bool to CARD8, but we want int
#define bool _BOOL
#include <X11/Xmd.h>
#undef bool
#define GLX_GLXEXT_PROTOTYPES
#include <GL/glx.h>
#endif

#if defined(__APPLE__)
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif
#include <StdDDraw2.h>

#include <array>
#include <unordered_map>
#include <utility>
#include <vector>

class CStdWindow;

// one GLSL shader
class CStdGLShader : public CStdShader
{
public:
	using CStdShader::CStdShader;

	bool Compile() override;
	void Clear() override;

	virtual int64_t GetHandle() const override { return shader; }

protected:
	virtual bool PrepareSource();

protected:
	GLuint shader = 0;
};

class CStdGLSPIRVShader : public CStdGLShader
{
public:
	using CStdGLShader::CStdGLShader;

protected:
	bool PrepareSource() override;
};

class CStdGLShaderProgram : public CStdShaderProgram
{
public:
	using CStdShaderProgram::CStdShaderProgram;

	explicit operator bool() const override { return /*glIsProgram(*/shaderProgram/*)*/; }

	bool Link() override;
	void Select() override;
	void Deselect() override;
	void Clear() override;

	void EnsureProgram() override;

	template<typename Func, typename... Args> bool SetUniform(const std::string &key, Func function, Args... args)
	{
		assert(shaderProgram);

		GLint location;
		if (auto it = uniformLocations.find(key); it != uniformLocations.end())
		{
			location = it->second;
			assert(location != -1);
		}
		else
		{
			location = glGetUniformLocation(shaderProgram, key.c_str());
			if (location == -1)
			{
				return false;
			}

			uniformLocations.emplace(key, location);
		}
		function(location, args...);

		return true;
	}

	virtual int64_t GetProgram() const override { return shaderProgram; }

protected:
	bool AddShaderInt(CStdShader *shader) override;

protected:
	GLuint shaderProgram = 0;
	std::unordered_map<std::string, GLint> uniformLocations;
};

// one OpenGL context
class CStdGLCtx
{
public:
	CStdGLCtx();
	~CStdGLCtx() { Clear(); };

	void Clear(); // clear objects
#ifdef _WIN32
	bool Init(CStdWindow *pWindow, CStdApp *pApp, HWND hWindow = nullptr);
#else
	bool Init(CStdWindow *pWindow, CStdApp *pApp);
#endif

	bool Select(bool verbose = false, bool selectOnly = false); // select this context
	void Deselect(bool secondary = false); // select this context
	void Destroy(); // destroy this context
	bool UpdateSize(); // get new size from hWnd

	bool PageFlip(); // present scene

protected:
	void DoDeselect();
	// this handles are declared as pointers to structs
	CStdWindow *pWindow; // window to draw in
#ifdef _WIN32
	HGLRC hrc; // rendering context
	HWND hWindow; // used if pWindow==nullptr
	HDC hDC; // device context handle
#elif defined(USE_X11)
	GLXContext ctx;
#endif
	int cx, cy; // context window size

	friend class CStdGL;
	friend class CSurface;
};

// OpenGL encapsulation
class CStdGL : public CStdDDraw
{
public:
	CStdGL();
	~CStdGL();
	virtual bool PageFlip(RECT *pSrcRt = nullptr, RECT *pDstRt = nullptr, CStdWindow *pWindow = nullptr);

protected:
	GLenum sfcFmt; // texture surface format
	CStdGLCtx MainCtx; // main GL context
	CStdGLCtx *pCurrCtx = nullptr; // current context

	CStdGLShaderProgram BlitShader;
	CStdGLShaderProgram BlitShaderMod2;
	CStdGLShaderProgram LandscapeShader;
	CStdGLShaderProgram DummyShader;

	struct
	{
		enum Index
		{
			Vertices = 0,
			TexCoords = 1,
			Color = 2,
			LiquidTexCoords = 3,
			ModelViewMatrixCol0 = 4,
			ModelViewMatrixCol1 = 5,
			ModelViewMatrixCol2 = 6,
			ModelViewMatrixCol3 = 7
		};

		enum ArrayIndex
		{
			PerformBlt = 0,
			BlitLandscape,
			DrawQuadDw,
			DrawLineDw,
			DrawPixInt,
			NumVAO
		};
		GLuint VAO[NumVAO];
		GLuint VBO[NumVAO];
	} VertexArray;

	struct
	{
		enum
		{
			TexIndent = 0,
			BlitOffset,
			ProjectionMatrix,
		};

		GLuint VBO;
		const char * const Names[3]
		{
			"texIndent",
			"blitOffset",
			"projectionMatrix"
		};

		std::array<GLint, std::extent_v<decltype(Names)>> Offset;
	} StandardUniforms;

public:
	// General
	void Clear();
	void Default();
	virtual int GetEngine() { return 1; } // get indexed engine
	virtual bool OnResolutionChanged(); // reinit OpenGL and window for new resolution

	// Clipper
	bool UpdateClipper(); // set current clipper to render target

	// Surface
	bool PrepareRendering(CSurface *sfcToSurface); // check if/make rendering possible to given surface
	CStdGLCtx &GetMainCtx() { return MainCtx; }
	virtual CStdGLCtx *CreateContext(CStdWindow *pWindow, CStdApp *pApp);
#ifdef _WIN32
	virtual CStdGLCtx *CreateContext(HWND hWindow, CStdApp *pApp);
#endif

	// Blit
	void PerformBlt(CBltData &rBltData, CTexRef *pTex, uint32_t dwModClr, bool fMod2, bool fExact);
	virtual void BlitLandscape(CSurface *sfcSource, CSurface *sfcSource2, CSurface *sfcLiquidAnimation, int fx, int fy,
		CSurface *sfcTarget, int tx, int ty, int wdt, int hgt);
	void FillBG(uint32_t dwClr = 0);

	// Drawing
	void DrawQuadDw(CSurface *sfcTarget, int *ipVtx, uint32_t dwClr1, uint32_t dwClr2, uint32_t dwClr3, uint32_t dwClr4);
	void DrawLineDw(CSurface *sfcTarget, float x1, float y1, float x2, float y2, uint32_t dwClr);
	void DrawPixInt(CSurface *sfcDest, float tx, float ty, uint32_t dwCol);

	// Gamma
	virtual bool ApplyGammaRamp(CGammaControl &ramp, bool fForce);
	virtual bool SaveDefaultGammaRamp(CStdWindow *pWindow);

	// device objects
	bool RestoreDeviceObjects(); // init/restore device dependent objects
	bool InvalidateDeviceObjects(); // free device dependent objects
	void SetTexture();
	void ResetTexture();
#ifdef _WIN32
	bool DeviceReady() { return !!MainCtx.hrc; }
#elif defined(USE_X11)
	bool DeviceReady() { return !!MainCtx.ctx; }
#else
	bool DeviceReady() { return true; } // SDL
#endif

protected:
	bool CreatePrimarySurfaces();
	bool CreateDirectDraw();

	template<size_t index, typename Struct, typename... Args>
	void InitializeVAO(Args&&... args)
	{
		glBindVertexArray(VertexArray.VAO[index]);
		glBindBuffer(GL_ARRAY_BUFFER, VertexArray.VBO[index]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Struct), nullptr, GL_DYNAMIC_DRAW);

		(glEnableVertexAttribArray(args), ...);
	}

	void SelectVAO(decltype(VertexArray)::ArrayIndex index);
	virtual void DrawModeChanged(DrawMode oldMode, DrawMode newMode) override;
	virtual CStdShader *CreateShader(CStdShader::Type type, ShaderLanguage language, const std::string &source) override;
	virtual CStdShaderProgram *CreateShaderProgram() override;
	virtual void ShaderProgramSet(DrawMode mode, CStdShaderProgram *shaderProgram) override;

#ifdef USE_X11
	// Size of gamma ramps
	int gammasize;
#endif

	friend class CSurface;
	friend class CTexRef;
	friend class CPattern;
	friend class CStdGLCtx;
	friend class C4StartupOptionsDlg;
	friend class C4FullScreen;
};

// Global access pointer
extern CStdGL *pGL;

#endif
