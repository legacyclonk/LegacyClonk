/* by Sven2, 2005 */
// color calculation routines

#ifndef INC_StdColors
#define INC_StdColors

#include <Standard.h>

// color definitions
const int FTrans=-1,FWhite=0,FBlack=1,FPlayer=2,FRed=3;
const int CBlack=0,CGray1=1,CGray2=2,CGray3=3,CGray4=4,CGray5=5,CWhite=6,
					CDRed=7,CDGreen=8,CDBlue=9,CRed=10,CGreen=11,CLBlue=12,CYellow=13,CBlue=14;
extern const BYTE FColors [];

// helper function
#define RGBA(r, g, b, a) (((DWORD)(a)<<24)|(((DWORD)(r)&0xff)<<16)|(((DWORD)(g)&0xff)<<8)|((b)&0xff))

inline void BltAlpha(DWORD &dwDst, DWORD dwSrc)
	{
	// blit one color value w/alpha on another
	if (dwDst>>24 == 0xff) { dwDst=dwSrc; return; }
	BYTE byAlphaDst=BYTE(dwSrc>>24); BYTE byAlphaSrc=255-byAlphaDst;
	dwDst = Min<uint32_t>((int(dwDst    & 0xff ) * byAlphaDst + int(dwSrc & 0xff    ) * byAlphaSrc) >>8,            0xff)      | // blue
	        Min<uint32_t>((int(dwDst   & 0xff00) * byAlphaDst + int(dwSrc & 0xff00  ) * byAlphaSrc) >>8 & 0xff00,   0xff00)    | // green
					Min<uint32_t>((int(dwDst & 0xff0000) * byAlphaDst + int(dwSrc & 0xff0000) * byAlphaSrc) >>8 & 0xff0000, 0xff0000)  | // red
				  uint32_t(Max((int) (dwDst >> 24) - byAlphaSrc, 0)) << 24; // alpha
	}

inline void BltAlphaAdd(DWORD &dwDst, DWORD dwSrc)
	{
	// blit one color value w/alpha on another in additive mode
	if (dwDst>>24 == 0xff) { dwDst=dwSrc; return; }
	BYTE byAlphaSrc=255-BYTE(dwSrc>>24);
	dwDst = Min<uint32_t>((dwDst    & 0xff ) + ((int(dwSrc    & 0xff  ) * byAlphaSrc) >>8)       , 0xff)      | // blue
	        Min<uint32_t>((dwDst   & 0xff00) + (int(dwSrc>>8 & 0xff  ) * byAlphaSrc) & 0x00ffff00, 0xff00)    | // green
					Min<uint32_t>((dwDst & 0xff0000) + (int(dwSrc>>8 & 0xff00) * byAlphaSrc) & 0xffff0000, 0xff0000)  | // red
				  uint32_t(Max((int) (dwDst >> 24) - byAlphaSrc, 0)) << 24; // alpha
	}

inline void ModulateClr(DWORD &dwDst, DWORD dwMod) // modulate two color values
	{
	// modulate two color values
	// get alpha
	int iA1=dwDst>>24, iA2=dwMod>>24;
	// modulate color values; mod alpha upwards
	dwDst = ((dwDst     & 0xff) * (dwMod    &   0xff))    >>  8   | // blue
	        ((dwDst>> 8 & 0xff) * (dwMod>>8 &   0xff)) &   0xff00 | // green
				  ((dwDst>>16 & 0xff) * (dwMod>>8 & 0xff00)) & 0xff0000 | // red
				  uint32_t(Min(iA1+iA2 - ((iA1*iA2)>>8), 255))  << 24   ; // alpha (=255-(255*iA1)(255*iA2)/255)
	}

inline void ModulateClrA(DWORD &dwDst, DWORD dwMod) // modulate two color values and add alpha value
	{
	// modulate two color values and add alpha value
	dwDst = (((dwDst&0xff)*(dwMod&0xff))>>8) |  // B
		      (((dwDst>>8&0xff)*(dwMod>>8&0xff))&0xff00) | // G
					(((dwDst>>16&0xff)*(dwMod>>8&0xff00))&0xff0000) | // R
					Min<uint32_t>((dwDst>>24)+(dwMod>>24), 0xff)<<24;
	}

inline void ModulateClrMOD2(DWORD &dwDst, DWORD dwMod) // clr1+clr2-0.5
	{
	// signed color addition
	dwDst = (BoundBy<int>(((int)(dwDst&0xff)+(dwMod&0xff)-0x7f)<<1,0,0xff)&0xff) |  // B
		      (BoundBy<int>(((int)(dwDst&0xff00)+(dwMod&0xff00)-0x7f00)<<1,0,0xff00)&0xff00) | // G
					(BoundBy<int>(((int)(dwDst&0xff0000)+(dwMod&0xff0000)-0x7f0000)<<1,0,0xff0000)&0xff0000) | // R
					Min<uint32_t>((dwDst>>24)+(dwMod>>24), 0xff)<<24;
	}

inline void ModulateClrMonoA(DWORD &dwDst, BYTE byMod, BYTE byA)
	{
	// darken a color value by constant modulation and add an alpha value
	dwDst = ((dwDst     & 0xff) * byMod)    >>  8   | // blue
	        ((dwDst>> 8 & 0xff) * byMod) &   0xff00 | // green
				  ((dwDst>> 8 & 0xff00) * byMod) & 0xff0000 | // red
				  Min<uint32_t>((dwDst>>24) + byA, 255) << 24;        // alpha
	}


inline DWORD LightenClr(DWORD &dwDst) // enlight a color
	{
	// enlight a color
	DWORD dw = dwDst;
	dwDst=(dw&0xff808080)|((dw<<1)&0xfefefe);
	if (dw & 0x80) dwDst |= 0xff;
	if (dw & 0x8000) dwDst |= 0xff00;
	if (dw & 0x800000) dwDst |= 0xff0000;
	return dwDst;
	}

inline DWORD LightenClrBy(DWORD &dwDst, int iBy) // enlight a color
	{
	// enlight a color
	// quite a desaturating method...
	dwDst = Min<int>((dwDst     & 0xff) + iBy, 255)       | // blue
	        Min<int>((dwDst>> 8 & 0xff) + iBy, 255) <<  8 | // green
				  Min<int>((dwDst>>16 & 0xff) + iBy, 255) << 16 | // red
				  (dwDst & 0xff000000);                     // alpha
	return dwDst;
	}

inline DWORD DarkenClrBy(DWORD &dwDst, int iBy) // darken a color
	{
	// darken a color
	// quite a desaturating method...
	dwDst = Max<int>(int(dwDst     & 0xff) - iBy, 0)       | // blue
	        Max<int>(int(dwDst>> 8 & 0xff) - iBy, 0) <<  8 | // green
				  Max<int>(int(dwDst>>16 & 0xff) - iBy, 0) << 16 | // red
				  (dwDst & 0xff000000);                     // alpha
	return dwDst;
	}

inline DWORD PlrClr2TxtClr(DWORD dwClr)
	{
	// convert player color to text color, lightening up when necessary
	int lgt=Max(Max(GetRValue(dwClr), GetGValue(dwClr)), GetBValue(dwClr));
	if (lgt<0x8f) LightenClrBy(dwClr, 0x8f-lgt);
	return dwClr|0xff000000;
	}

inline DWORD GetClrModulation(DWORD dwSrcClr, DWORD dwDstClr, DWORD &dwBack)
	{
	// get modulation that is necessary to transform dwSrcClr to dwDstClr
	// does not support alpha values in dwSrcClr and dwDstClr
	// get source color
	BYTE sB=BYTE(dwSrcClr); dwSrcClr=dwSrcClr>>8;
	BYTE sG=BYTE(dwSrcClr); dwSrcClr=dwSrcClr>>8;
	BYTE sR=BYTE(dwSrcClr); dwSrcClr=dwSrcClr>>8;
	// get dest color
	BYTE dB=BYTE(dwDstClr); dwDstClr=dwDstClr>>8;
	BYTE dG=BYTE(dwDstClr); dwDstClr=dwDstClr>>8;
	BYTE dR=BYTE(dwDstClr); dwDstClr=dwDstClr>>8;
	// get difference
	int cR=(int) dR-sR;
	int cG=(int) dG-sG;
	int cB=(int) dB-sB;
	// get max enlightment
	int diffN=0;
	if (cR>0) diffN=cR; 
	if (cG>0) diffN=Max(diffN, cG);
	if (cB>0) diffN=Max(diffN, cB);
	// is dest > src?
	if (diffN)
		{
		// so a back mask must be used
		int bR=sR+(cR*255)/diffN;
		int bG=sG+(cG*255)/diffN;
		int bB=sB+(cB*255)/diffN;
		dwBack=RGBA(bR, bG, bB, 0);
		}
	if (!sR) sR=1; if (!sG) sG=1; if (!sB) sB=1;
	return RGBA(Min((int)dR*256/sR, 255), Min((int)dG*256/sG, 255), Min((int)dB*256/sB, 255), diffN);
	}

inline DWORD NormalizeColors(DWORD &dwClr1, DWORD &dwClr2, DWORD &dwClr3, DWORD &dwClr4)
	{
	// normalize the colors to a color in the middle
	// combine clr1 and clr2 to clr1
	ModulateClr(dwClr1, dwClr2); LightenClr(dwClr1);
	// combine clr3 and clr4 to clr3
	ModulateClr(dwClr3, dwClr4); LightenClr(dwClr3);
	// combine clr1 and clr3 to clr1
	ModulateClr(dwClr1, dwClr3); LightenClr(dwClr1);
	// set other colors, return combined color
	return dwClr2=dwClr3=dwClr4=dwClr1;
	}

inline DWORD InvertRGBAAlpha(DWORD dwFromClr)
	{
	return (dwFromClr&0xffffff) | (255-(dwFromClr>>24))<<24;
	}

inline WORD ClrDw2W(DWORD dwClr)
	{
	return
		  WORD((dwClr & 0x000000f0) >> 4)
		| WORD((dwClr & 0x0000f000) >> 8)
		| WORD((dwClr & 0x00f00000) >> 12)
		| WORD((dwClr & 0xf0000000) >> 16);
	}

inline DWORD ClrW2Dw(WORD wClr)
	{
	return (DWORD)
		  ((wClr & 0x000f) << 4)
		| ((wClr & 0x00f0) << 8)
		| ((wClr & 0x0f00) << 12)
		| ((wClr & 0xf000) << 16);
	}

inline bool rgb2xyY(double r, double g, double b, double *px, double *py, double *pY) // linear rgb to CIE xyY
	{
	double X = 0.412453*r + 0.357580*g + 0.180423*b;
	double Y = 0.212671*r + 0.715160*g + 0.072169*b;
	double Z = 0.019334*r + 0.119193*g + 0.950227*b;
	double XYZ=X+Y+Z;
	if (!XYZ)
		{
		*px=*py=0.3; // assume grey cromaticity for black
		}
	else
		{
		*px = X/XYZ; *py = Y/XYZ;
		}
	*pY = Y;
	return true;
	}

inline bool xy2upvp(double x, double y, double *pu, double *pv) // CIE xy to u'v'
	{
	double n = -2.0*x+12.0*y+3.0;
	if (!n) return false;
	*pu = 4.0*x / n;
	*pv = 9.0*y / n;
	return true;
	}

inline bool RGB2rgb(int R, int G, int B, double *pr, double *pg, double *pb, double gamma=2.2) // monitor RGB (0 to 255) to linear rgb (0.0 to 1.0) assuming default gamma 2.2
	{
	*pr = pow((double) R / 255.0, 1.0/gamma);
	*pg = pow((double) G / 255.0, 1.0/gamma);
	*pb = pow((double) B / 255.0, 1.0/gamma);
	return true;
	}

// a standard pal
struct CStdPalette
	{
	BYTE Colors[3*256];
	BYTE Alpha[3*256];

	DWORD GetClr(BYTE byCol)
		{ return RGB(Colors[byCol*3+2], Colors[byCol*3+1], Colors[byCol*3])+(Alpha[byCol]<<24); }

	void EnforceC0Transparency()
		{
		Colors[0]=Colors[1]=Colors[2]=0; Alpha[0]=255;
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
		int iWdt, iHgt;   // number of sections in the map
		int iOffX, iOffY; // offset to add to drawing positions before applying the map
		bool fFadeTransparent; // if set, ReduceModulation and AddModulation fade transparent instead of black
		int iResolutionX, iResolutionY;

	public:
		enum { iDefResolutionX = 64, iDefResolutionY = 64 };

	public:
		CClrModAddMap() : pMap(NULL), iMapSize(0), iWdt(0), iHgt(0), iOffX(0), iOffY(0), fFadeTransparent(false), iResolutionX(iDefResolutionX), iResolutionY(iDefResolutionY) {}
		~CClrModAddMap() { delete [] pMap; }

		void Reset(int iResX, int iResY, int iWdtPx, int iHgtPx, int iOffX, int iOffY, uint32_t dwModClr, uint32_t dwAddClr, int x0, int y0, uint32_t dwBackClr=0, class CSurface *backsfc=NULL); // reset all of map to given values; uses transparent mode and clears rect if a back color is given
		void ReduceModulation(int cx, int cy, int iRadius1, int iRadius2);                              // reveal all within iRadius1; fade off until iRadius2
		void AddModulation(int cx, int cy, int iRadius1, int iRadius2, uint8_t byTransparency);                                 // hide all within iRadius1; fade off until iRadius2

		uint32_t GetModAt(int x, int y) const;
		int GetResolutionX() const { return iResolutionX; }
		int GetResolutionY() const { return iResolutionY; }
	};

// used to calc intermediate points of color fades
class CColorFadeMatrix
	{
	private:
		int ox,oy,w,h; // offset of x/y
		struct { int c0,cx,cy,ce; } clrs[4];

	public:
		CColorFadeMatrix(int iX, int iY, int iWdt, int iHgt, uint32_t dwClr1, uint32_t dwClr2, uint32_t dwClr3, uint32_t dwClr4);
		uint32_t GetColorAt(int iX, int iY);
	};

#endif
