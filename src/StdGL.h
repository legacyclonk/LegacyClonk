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
#include <GL/glx.h>
#endif

#if defined(__APPLE__)
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif
#include <StdDDraw2.h>

#include <utility>

class CStdWindow;

// one GLSL shader
class CStdGLShader
{
public:
	using Macro = std::pair<std::string, std::string>;

public:
	CStdGLShader() = default;
	explicit CStdGLShader(const std::string &source) : source{source} {}
	CStdGLShader(const CStdGLShader &) = delete;
	CStdGLShader(CStdGLShader &&shader);

	virtual ~CStdGLShader() { Clear(); }

	bool operator==(const CStdGLShader &other) { return other.shader == shader; }

	template<typename T> void SetMacro(const std::string &key, const T &value)
	{
		auto it = std::find_if(macros.begin(), macros.end(), [&key](const auto &macro) { return macro.first == key; });
		if constexpr (std::is_null_pointer_v<T>)
		{
			if (it != macros.end())
			{
				macros.erase(it);
			}
		}

		else
		{
			if (it != macros.end())
			{
				it->second = std::string{value};
			}
			else
			{
				macros.emplace_back(key, value);
			}
		}
	}

	void SetSource(const std::string &source);

	GLuint Compile();
	void Clear();

	GLuint GetShader() const { return shader; }
	std::string GetSource() const { return source; }
	std::vector<Macro> GetMacros() const { return macros; }
	std::string GetErrorMessage() const { return errorMessage; }
	virtual GLenum GetType() const = 0;

protected:
	GLuint SetError(std::string_view message) { errorMessage = message; return 0; }

	GLuint shader = 0;
	std::string source;
	std::vector<Macro> macros;
	std::string errorMessage;
};

class CStdGLVertexShader : public CStdGLShader
{
public:
	CStdGLVertexShader() = default;
	explicit CStdGLVertexShader(const std::string &source) : CStdGLShader{source} {}

	virtual GLenum GetType() const override { return GL_VERTEX_SHADER; }
};

class CStdGLFragmentShader : public CStdGLShader
{
public:
	CStdGLFragmentShader() = default;
	explicit CStdGLFragmentShader(const std::string &source) : CStdGLShader{source} {}

	virtual GLenum GetType() const override { return GL_FRAGMENT_SHADER; }
};

class CStdGLShaderProgram
{
public:
	CStdGLShaderProgram() = default;
	CStdGLShaderProgram(const CStdGLShaderProgram &) = delete;
	~CStdGLShaderProgram() { Clear(); }

	explicit operator bool() const { return /*glIsProgram(*/shaderProgram/*)*/; }

	void AddShader(CStdGLShader *shader);

	bool Link();
	void Select();
	void Deselect();
	void Clear();

	void EnsureProgram();

	template<typename... Args> bool SetUniform(const std::string &key, void (*function)(GLint, Args...), Args... args)
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

	GLuint GetShaderProgram() const { return shaderProgram; }
	std::string GetErrorMessage() const { return errorMessage; }

private:
	bool SetError(std::string_view message) { errorMessage = message; return false; }
	GLuint shaderProgram = 0;
	std::string errorMessage;
	std::vector<CStdGLShader *> shaders;
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
	CStdGLCtx *pCurrCtx; // current context

	CStdGLShaderProgram BlitShader;
	CStdGLShaderProgram BlitShaderMod2;
	CStdGLShaderProgram LandscapeShader;

	struct
	{
		GLuint VAO = 0;
		GLuint VBO[4] {};
		enum Index
		{
			Vertices = 0,
			TexCoords = 1,
			Color = 2,
			LiquidTexCoords = 3,
		};
	} VertexArray;

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
