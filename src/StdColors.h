/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2005, Sven2
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

// color calculation routines

#pragma once

#include <Standard.h>

#include <algorithm>

// color definitions
const int FTrans = -1, FWhite = 0, FBlack = 1, FPlayer = 2, FRed = 3;
const int CBlack = 0, CGray1 = 1, CGray2 = 2, CGray3 = 3, CGray4 = 4, CGray5 = 5, CWhite = 6,
	CDRed = 7, CDGreen = 8, CDBlue = 9, CRed = 10, CGreen = 11, CLBlue = 12, CYellow = 13, CBlue = 14;
extern const uint8_t FColors[];

// helper function
constexpr uint32_t RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) { return (a << 24) | (r << 16) | (g << 8) | b; }

namespace
{
	struct Color
	{
		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint8_t a;
	};

	constexpr Color SplitRGB(uint32_t color)
	{
		return
		{
			static_cast<uint8_t>(color >> 16),
			static_cast<uint8_t>(color >> 8),
			static_cast<uint8_t>(color),
			static_cast<uint8_t>(color >> 24)
		};
	}

	template<typename Func>
	constexpr uint32_t CombineColors(uint32_t dst, uint32_t src, Func func)
	{
		const auto [srcR, srcG, srcB, srcA] = SplitRGB(src);
		const auto [dstR, dstG, dstB, dstA] = SplitRGB(dst);

		return func(srcR, srcG, srcB, srcA, dstR, dstG, dstB, dstA);
	}

	template<typename RgbFunc, typename AFunc>
	constexpr uint32_t CombineColors(uint32_t dst, uint32_t src, RgbFunc rgbFunc, AFunc aFunc)
	{
		const auto [srcR, srcG, srcB, srcA] = SplitRGB(src);
		const auto [dstR, dstG, dstB, dstA] = SplitRGB(dst);

		return RGBA
		(
			rgbFunc(srcR, dstR),
			rgbFunc(srcG, dstG),
			rgbFunc(srcB, dstB),
			aFunc(srcA, dstA)
		);
	}

	template<typename RgbFunc, typename AFunc>
	constexpr uint32_t ModifyColor(uint32_t src, RgbFunc rgbFunc, AFunc aFunc)
	{
		const auto [srcR, srcG, srcB, srcA] = SplitRGB(src);

		return RGBA
		(
			rgbFunc(srcR),
			rgbFunc(srcG),
			rgbFunc(srcB),
			aFunc(srcA)
		);
	}

	template<typename RgbFunc>
	constexpr uint32_t ModifyColor(uint32_t src, RgbFunc rgbFunc)
	{
		return ModifyColor(src, rgbFunc, [](uint8_t src) { return src; });
	}
}

constexpr void BltAlpha(uint32_t &dst, uint32_t src)
{
	if (SplitRGB(dst).a == 0xff)
	{
		dst = src;
		return;
	}

	const auto byAlphaDst = SplitRGB(src).a;
	const auto byAlphaSrc = 0xff - byAlphaDst;
	dst = CombineColors(dst, src, [byAlphaDst, byAlphaSrc](uint16_t src, uint16_t dst)
	{
		return static_cast<uint8_t>(std::min((src * byAlphaSrc + dst * byAlphaDst) >> 8, 0xff));
	},
	[byAlphaSrc](uint8_t src, uint8_t dst)
	{
		return static_cast<uint8_t>(std::max(dst - byAlphaSrc, 0));
	});
}

constexpr void BltAlphaAdd(uint32_t &dst, uint32_t src)
{
	if (SplitRGB(dst).a == 0xff)
	{
		dst = src;
		return;
	}

	const auto byAlphaSrc = 0xff - SplitRGB(src).a;
	dst = CombineColors(dst, src, [byAlphaSrc](uint16_t src, uint16_t dst)
	{
		return static_cast<uint8_t>(std::min(dst + ((src * byAlphaSrc) >> 8), 0xff));
	},
	[byAlphaSrc](uint8_t src, uint8_t dst)
	{
		return static_cast<uint8_t>(std::max(dst - byAlphaSrc, 0));
	});
}

constexpr void ModulateClr(uint32_t &dst, uint32_t src)
{
	dst = CombineColors(dst, src, [](uint16_t src, uint16_t dst)
	{
		return static_cast<uint8_t>((src * dst) >> 8);
	},
	[](uint16_t src, uint16_t dst)
	{
		return static_cast<uint8_t>(std::min(src + dst - ((src * dst) >> 8), 0xff));
	});
}

constexpr void ModulateClrA(uint32_t &dst, uint32_t src)
{
	dst = CombineColors(dst, src, [](uint16_t src, uint16_t dst)
	{
		return static_cast<uint8_t>((src * dst) >> 8);
	},
	[](uint16_t src, uint16_t dst)
	{
		return static_cast<uint8_t>(std::min(src + dst, 0xff));
	});
}

constexpr void ModulateClrMOD2(uint32_t &dst, uint32_t src)
{
	dst = CombineColors(dst, src, [](uint16_t src, uint16_t dst)
	{
		return static_cast<uint8_t>(std::clamp((src + dst - 0x7f) * 2, 0, 0xff));
	},
	[](uint16_t src, uint16_t dst)
	{
		return static_cast<uint8_t>(std::min(src + dst, 0xff));
	});
}

constexpr void ModulateClrMonoA(uint32_t &dst, uint8_t byMod, uint8_t byA)
{
	dst = ModifyColor(dst, [byMod](uint16_t dst)
	{
		return static_cast<uint8_t>((dst * byMod) >> 8);
	},
	[byA](uint16_t dst)
	{
		return static_cast<uint8_t>(std::min(dst + byA, 0xff));
	});
}

constexpr uint32_t LightenClr(uint32_t &dst)
{
	return dst = ModifyColor(dst, [](uint8_t dst)
	{
		const auto tmp = (dst & 0x80) | ((dst << 1) & 0xfe);
		return (dst & 0x80) ? tmp | 0xff : tmp;
	});
}

constexpr uint32_t LightenClrBy(uint32_t &dst, uint8_t by)
{
	// quite a desaturating method...
	return dst = ModifyColor(dst, [by](uint8_t dst)
	{
		return static_cast<uint8_t>(std::min(dst + by, 0xff));
	});
}

constexpr uint32_t DarkenClrBy(uint32_t &dst, uint8_t by)
{
	// quite a desaturating method...
	return dst = ModifyColor(dst, [by](uint8_t dst)
	{
		return static_cast<uint8_t>(std::max(dst - by, 0));
	});
}

[[nodiscard]] constexpr uint32_t PlrClr2TxtClr(uint32_t clr)
{
	// convert player color to text color, lightening up when necessary
	const auto [r, g, b, a] = SplitRGB(clr);
	const auto lgt = std::max({r, g, b});

	return ((lgt < 0x8f) ? LightenClrBy(clr, 0x8f - lgt) : clr) | 0xff000000;
}

// get modulation that is necessary to transform dwSrcClr to dwDstClr
// does not support alpha values in dwSrcClr and dwDstClr
[[nodiscard]] constexpr uint32_t GetClrModulation(uint32_t src, uint32_t dst, uint32_t &back)
{
	return CombineColors(dst, src, [&back](const int16_t srcR, const int16_t srcG, const int16_t srcB, const uint8_t, const int16_t dstR, const int16_t dstG, const int16_t dstB, const uint8_t)
	{
		const auto diffR = dstR - srcR;
		const auto diffG = dstG - srcG;
		const auto diffB = dstB - srcB;

		// get max enlightment
		const auto diff = static_cast<uint8_t>(std::max({0, diffR, diffG, diffB}));

		// is dest > src?
		if (diff > 0)
		{
			// so a back mask must be used
			auto backVal = [diff](int src, int diffComponent)
			{
				return static_cast<uint8_t>(src + (diffComponent * 0xff) / diff);
			};
			back = RGBA(backVal(srcR, diffR), backVal(dstG, diffG), backVal(srcB, diffB), 0);
		}

		auto combine = [](int16_t src, uint16_t dst)
		{
			return static_cast<uint8_t>(std::min(dst * 256 / std::max<int16_t>(src, 1), 0xff));
		};

		return RGBA
		(
			combine(srcR, dstR),
			combine(srcG, dstG),
			combine(srcB, dstB),
			diff
		);
	});
}

inline uint32_t NormalizeColors(uint32_t &dwClr1, uint32_t &dwClr2, uint32_t &dwClr3, uint32_t &dwClr4)
{
	// normalize the colors to a color in the middle
	// combine clr1 and clr2 to clr1
	ModulateClr(dwClr1, dwClr2); LightenClr(dwClr1);
	// combine clr3 and clr4 to clr3
	ModulateClr(dwClr3, dwClr4); LightenClr(dwClr3);
	// combine clr1 and clr3 to clr1
	ModulateClr(dwClr1, dwClr3); LightenClr(dwClr1);
	// set other colors, return combined color
	return dwClr2 = dwClr3 = dwClr4 = dwClr1;
}

inline uint32_t InvertRGBAAlpha(uint32_t dwFromClr)
{
	return (dwFromClr & 0xffffff) | (255 - (dwFromClr >> 24)) << 24;
}

inline uint16_t ClrDw2W(uint32_t dwClr)
{
	return
		static_cast<uint16_t>((dwClr & 0x000000f0) >>  4) |
		static_cast<uint16_t>((dwClr & 0x0000f000) >>  8) |
		static_cast<uint16_t>((dwClr & 0x00f00000) >> 12) |
		static_cast<uint16_t>((dwClr & 0xf0000000) >> 16);
}

inline bool rgb2xyY(double r, double g, double b, double *px, double *py, double *pY) // linear rgb to CIE xyY
{
	double X = 0.412453 * r + 0.357580 * g + 0.180423 * b;
	double Y = 0.212671 * r + 0.715160 * g + 0.072169 * b;
	double Z = 0.019334 * r + 0.119193 * g + 0.950227 * b;
	double XYZ = X + Y + Z;
	if (!XYZ)
	{
		*px = *py = 0.3; // assume grey cromaticity for black
	}
	else
	{
		*px = X / XYZ; *py = Y / XYZ;
	}
	*pY = Y;
	return true;
}

inline bool xy2upvp(double x, double y, double *pu, double *pv) // CIE xy to u'v'
{
	double n = -2.0 * x + 12.0 * y + 3.0;
	if (!n) return false;
	*pu = 4.0 * x / n;
	*pv = 9.0 * y / n;
	return true;
}

inline bool RGB2rgb(int R, int G, int B, double *pr, double *pg, double *pb, double gamma = 2.2) // monitor RGB (0 to 255) to linear rgb (0.0 to 1.0) assuming default gamma 2.2
{
	*pr = pow(static_cast<double>(R) / 255.0, 1.0 / gamma);
	*pg = pow(static_cast<double>(G) / 255.0, 1.0 / gamma);
	*pb = pow(static_cast<double>(B) / 255.0, 1.0 / gamma);
	return true;
}

// a standard pal
struct CStdPalette
{
	uint8_t Colors[3 * 256];
	uint8_t Alpha[3 * 256];

	uint32_t GetClr(uint8_t byCol)
	{
		return RGB(Colors[byCol * 3 + 2], Colors[byCol * 3 + 1], Colors[byCol * 3]) + (Alpha[byCol] << 24);
	}

	void EnforceC0Transparency()
	{
		Colors[0] = Colors[1] = Colors[2] = 0; Alpha[0] = 255;
	}
};

// color mod+add infostructure
struct CClrModAdd
{
	uint32_t dwModClr;
	uint32_t dwAddClr;
};

// clrmod-add-map to cover a drawing range in which all draws shall be adjusted by the map
class CClrModAddMap
{
private:
	CClrModAdd *pMap; size_t iMapSize;
	int iWdt, iHgt; // number of sections in the map
	int iOffX, iOffY; // offset to add to drawing positions before applying the map
	bool fFadeTransparent; // if set, ReduceModulation and AddModulation fade transparent instead of black
	int iResolutionX, iResolutionY;

public:
	enum { iDefResolutionX = 64, iDefResolutionY = 64 };

public:
	CClrModAddMap() : pMap(nullptr), iMapSize(0), iWdt(0), iHgt(0), iOffX(0), iOffY(0), fFadeTransparent(false), iResolutionX(iDefResolutionX), iResolutionY(iDefResolutionY) {}
	~CClrModAddMap() { delete[] pMap; }

	void Reset(int iResX, int iResY, int iWdtPx, int iHgtPx, int iOffX, int iOffY, uint32_t dwModClr, uint32_t dwAddClr, int x0, int y0, uint32_t dwBackClr = 0, class C4Surface *backsfc = nullptr); // reset all of map to given values; uses transparent mode and clears rect if a back color is given
	void ReduceModulation(int cx, int cy, int iRadius1, int iRadius2); // reveal all within iRadius1; fade off until iRadius2
	void AddModulation(int cx, int cy, int iRadius1, int iRadius2, uint8_t byTransparency); // hide all within iRadius1; fade off until iRadius2

	uint32_t GetModAt(int x, int y) const;
	int GetResolutionX() const { return iResolutionX; }
	int GetResolutionY() const { return iResolutionY; }
};

// used to calc intermediate points of color fades
class CColorFadeMatrix
{
private:
	int ox, oy, w, h; // offset of x/y
	struct { int c0, cx, cy, ce; } clrs[4];

public:
	CColorFadeMatrix(int iX, int iY, int iWdt, int iHgt, uint32_t dwClr1, uint32_t dwClr2, uint32_t dwClr3, uint32_t dwClr4);
	uint32_t GetColorAt(int iX, int iY);
};
