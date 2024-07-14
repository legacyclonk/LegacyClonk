/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2001, Sven2
 * Copyright (c) 2017-2020, The LegacyClonk Team and contributors
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

// complex dynamic landscape creator

#pragma once

#include "C4ForwardDeclarations.h"
#include <C4Group.h>
#include <C4Scenario.h>
#include <C4Surface.h>

#include <format>
#include <memory>

#define C4MC_SizeRes 100 // positions in percent
#define C4MC_ZoomRes 100 // zoom resolution (-100 to +99)

// string consts
#define C4MC_Overlay "overlay" // overlay node
#define C4MC_Point   "point"   // polygon point
#define C4MC_Map     "map"     // map node

#define C4MC_DefAlgo "solid" // default overlay algorithm

// error messages
#define C4MCErr_404     "file not found"
#define C4MCErr_NoGroup "internal error: no group"

#define C4MCErr_EOF               "unexpected end of file"
#define C4MCErr_NoDirGlobal       "can't use directives in local scope"
#define C4MCErr_UnknownDir        "unknown directive: {}"
#define C4MCErr_MapNoGlobal       "can't declare map in local scope"
#define C4MCErr_OpTypeErr         "operator type mismatch"
#define C4MCErr_IdtfExp           "identifier expected"
#define C4MCErr_UnnamedNoGlbl     "unnamed objects not allowed in global scope"
#define C4MCErr_BlOpenExp         "'{{' expected"
#define C4MCErr_OpsNoGlobal       "operators not allowed in global scope"
#define C4MCErr_SColonOrOpExp     "';' or operator expected"
#define C4MCErr_Obj2Exp           "second operand expected"
#define C4MCErr_ReinstNoGlobal    "can't reinstanciate object '{}' in global scope"
#define C4MCErr_UnknownObj        "unknown object: {}"
#define C4MCErr_ReinstUnknown     "can't reinstanciate '{}'; object type is unknown"
#define C4MCErr_EqSColonBlOpenExp "'=', ';' or '{{' expected"
#define C4MCErr_FieldConstExp     "constant for field '{}' expected"
#define C4MCErr_SColonExp         "';' expected"
#define C4MCErr_Field404          "field '{}' not found"
#define C4MCErr_FieldValInvalid   "'{}' is not a valid value for this field"
#define C4MCErr_MatNotFound       "material '{}' not found"
#define C4MCErr_TexNotFound       "texture '{}' not found"
#define C4MCErr_AlgoNotFound      "algorithm '{}' not found"
#define C4MCErr_SFuncNotFound     "script func '{}' not found in scenario script"
#define C4MCErr_PointOnlyOvl      "point only allowed in overlays"

// predef
class C4MCCallbackArray;
class C4MCCallbackArrayList;
class C4MCNode;
class C4MCOverlay;
class C4MCPoint;
class C4MCMap;
class C4MapCreatorS2;
class C4MCParserErr;
class C4MCParser;

struct C4MCAlgorithm
{
	char Identifier[C4MaxName];
	bool(*Function)(C4MCOverlay *, int32_t, int32_t);
};

extern C4MCAlgorithm C4MCAlgoMap[];

// node type enum
enum C4MCNodeType { MCN_Node, MCN_Overlay, MCN_Point, MCN_Map };

// one token type
enum C4MCTokenType
{
	MCT_NONE,    // nothing
	MCT_DIR,     // directive (stored in CurrTokenIdtf)
	MCT_IDTF,    // identifier (stored in CurrTokenIdtf)
	MCT_INT,     // integer constant (stored in CurrTokenVal)
	MCT_EQ,      // =
	MCT_BLOPEN,  // {
	MCT_BLCLOSE, // }
	MCT_SCOLON,  // ;
	MCT_AND,     // &
	MCT_OR,      // |
	MCT_XOR,     // ^
	MCT_RANGE,   // -
	MCT_PERCENT, // integer constant (stored in CurrTokenVal) + %
	MCT_PX,      // integer constant (stored in CurrTokenVal) + px
	MCT_EOF      // end of file
};

// a callback array
// contains a script func, and a map to call the func for
class C4MCCallbackArray
{
public:
	C4MCCallbackArray(C4AulFunc *pSFunc, C4MapCreatorS2 *pMapCreator);
	~C4MCCallbackArray();

protected:
	C4MapCreatorS2 *pMapCreator; // map creator class to query current map of
	uint8_t *pMap; // bitmap whether or not to call the function for a map pixel
	int32_t iWdt, iHgt; // size of the bitmap, when created
	C4AulFunc *pSF; // script func to be called

	C4MCCallbackArray *pNext; // next array in linked list

public:
	void EnablePixel(int32_t iX, int32_t iY); // enable pixel in map; create map if necessary
	void Execute(C4Section &section, int32_t iMapZoom); // evaluate the array

	friend class C4MCCallbackArrayList;
};

// callback array list: contains all callbacks
class C4MCCallbackArrayList
{
public:
	C4MCCallbackArrayList() { pFirst = nullptr; }
	~C4MCCallbackArrayList() { Clear(); }

protected:
	C4MCCallbackArray *pFirst; // first array in list

public:
	void Add(C4MCCallbackArray *pNewArray); // add given array to list
	void Clear(); // clear the list
	void Execute(C4Section &section, int32_t iMapZoom); // execute all arrays
};

// generic map creator tree node
// the code has been STL-free so far, so keep the line
class C4MCNode
{
public:
	C4MCNode *Owner, *Child0, *ChildL, *Prev, *Next; // tree structure
	C4MapCreatorS2 *MapCreator; // owning map creator
	char Name[C4MaxName]; // name, if named

public:
	C4MCNode(C4MCNode *pOwner = nullptr);
	C4MCNode(C4MCNode *pOwner, C4MCNode &rTemplate, bool fClone); // constructor using template
	virtual ~C4MCNode();

	virtual C4MCNode *clone(C4MCNode *pToNode) { return new C4MCNode(pToNode, *this, true); }

	void Clear(); // clear all child nodes
	void Reg2Owner(C4MCNode *pOwner); // register into list

protected:
	virtual bool GlobalScope() { return false; } // whether node is a global scope
	virtual bool SetOp(C4MCTokenType eOp) { return false; } // set following operator
	C4MCNode *GetNodeByName(const char *szName); // search node by name

	virtual bool SetField(C4MCParser *pParser, const char *szField, const char *szSVal, int32_t iVal, C4MCTokenType ValType); // set field
	int32_t IntPar(C4MCParser *pParser, const char *szSVal, int32_t iVal, C4MCTokenType ValType); // ensure par is int32_t
	const char *StrPar(C4MCParser *pParser, const char *szSVal, int32_t iVal, C4MCTokenType ValType); // ensure par is string

	virtual void Evaluate() {} // called when all fields are initialized
	void ReEvaluate(); // evaluate everything again

	// For Percents and Pixels
	class int_bool
	{
	public:
		int32_t Evaluate(int32_t relative_to)
		{
			if (percent) return value * relative_to / C4MC_SizeRes; else return value;
		}

		void Set(int32_t value, bool percent)
		{
			this->value = value; this->percent = percent;
		}

	private:
		int32_t value;
		bool percent;
	};

public:
	virtual C4MCNodeType Type() { return MCN_Node; } // get node type
	virtual C4MCOverlay *Overlay() { return nullptr; } // return overlay, if this is one
	C4MCOverlay *OwnerOverlay(); // return an owner who is an overlay

	friend class C4MCParser;
	friend struct C4MCNodeAttr;
};

// node attribute entry for SetField search
enum C4MCValueType
{
	C4MCV_None,
	C4MCV_Integer,
	C4MCV_Percent,
	C4MCV_Pixels,
	C4MCV_Material,
	C4MCV_Texture,
	C4MCV_Algorithm,
	C4MCV_Boolean,
	C4MCV_Zoom,
	C4MCV_ScriptFunc,
};

struct C4MCNodeAttr
{
	const char *Name; // name of field
	C4MCValueType Type; // type of field
	union
	{
		int32_t C4MCOverlay:: *integer;
		C4MCNode::int_bool C4MCOverlay:: *intBool;
		char (C4MCOverlay:: *texture)[C4M_MaxName + 1];
		C4MCAlgorithm * C4MCOverlay:: *algorithm;
		bool C4MCOverlay:: *boolean;
		C4MCCallbackArray * C4MCOverlay:: *scriptFunc;
	}; // offset of field in overlay MCOverlay-class

	C4MCNodeAttr() : Name(""), Type(C4MCV_None), integer(nullptr) {}
	C4MCNodeAttr(const char *Name, int32_t C4MCOverlay:: *integer, C4MCValueType Type = C4MCV_Integer) : Name(Name), Type(Type), integer(integer) {}
	C4MCNodeAttr(const char *Name, C4MCNode::int_bool C4MCOverlay:: *intBool, C4MCValueType Type = C4MCV_Percent) : Name(Name), Type(Type), intBool(intBool) {}
	C4MCNodeAttr(const char *Name, char (C4MCOverlay:: *texture)[C4M_MaxName + 1]) : Name(Name), Type(C4MCV_Texture), texture(texture) {}
	C4MCNodeAttr(const char *Name, C4MCAlgorithm * C4MCOverlay:: *algorithm) : Name(Name), Type(C4MCV_Algorithm), algorithm(algorithm) {}
	C4MCNodeAttr(const char *Name, bool C4MCOverlay:: *boolean) : Name(Name), Type(C4MCV_Boolean), boolean(boolean) {}
	C4MCNodeAttr(const char *Name, C4MCCallbackArray * C4MCOverlay:: *scriptFunc) : Name(Name), Type(C4MCV_ScriptFunc), scriptFunc(scriptFunc) {}
};
extern C4MCNodeAttr C4MCOvrlMap[];

// overlay node
class C4MCOverlay : public C4MCNode
{
public:
	C4MCOverlay(C4MCNode *pOwner = nullptr);
	C4MCOverlay(C4MCNode *pOwner, C4MCOverlay &rTemplate, bool fClone); // construct of template

	C4MCNode *clone(C4MCNode *pToNode) override { return new C4MCOverlay(pToNode, *this, true); }

protected:
	void Default(); // set default values for default presets

public:
	int32_t Seed; // random seed
	int32_t FixedSeed; // fixed random seed set in def
	int32_t X, Y, Wdt, Hgt, OffX, OffY; // extends/offset
	int_bool RX, RY, RWdt, RHgt, ROffX, ROffY; // extends/offset relatively to owner
	int32_t Material; // material index
	bool Sub; // tunnel bg?
	char Texture[C4M_MaxName + 1]; // texture name
	uint8_t MatClr; // resolved mat-tex color
	C4MCTokenType Op; // following operator
	C4MCAlgorithm *Algorithm; // algorithm to calc whether filled or not
	int32_t Turbulence, Lambda, Rotate; // turbulence factors; rotation angle
	int_bool Alpha, Beta; // extra params
	int32_t ZoomX, ZoomY; // zoom factor for algorithm
	bool Invert, LooseBounds, Group, Mask; // extra algo behaviour
	C4MCCallbackArray *pEvaluateFunc; // function called for nodes being evaluated and fulfilled
	C4MCCallbackArray *pDrawFunc; // function called when this node is drawn - pass drawcolor as first param, return color to be actually used

	bool SetOp(C4MCTokenType eOp) override { Op = eOp; return true; } // set following operator

	C4MCAlgorithm *GetAlgo(const char *szName);

	bool SetField(C4MCParser *pParser, const char *szField, const char *szSVal, int32_t iVal, C4MCTokenType ValType) override; // set field

	void Evaluate() override; // called when all fields are initialized

	C4MCOverlay *Overlay() override { return this; } // this is an overlay
	C4MCOverlay *FirstOfChain(); // go backwards in op chain until first overlay of chain

	bool CheckMask(int32_t iX, int32_t iY); // check whether algorithms succeeds at iX/iY
	bool RenderPix(int32_t iX, int32_t iY, uint8_t &rPix, C4MCTokenType eLastOp = MCT_NONE, bool fLastSet = false, bool fDraw = true, C4MCOverlay **ppPixelSetOverlay = nullptr); // render this pixel
	bool PeekPix(int32_t iX, int32_t iY); // check mask; regard operator chain
	bool InBounds(int32_t iX, int32_t iY) { return iX >= X && iY >= Y && iX < X + Wdt && iY < Y + Hgt; } // return whether point iX/iY is inside bounds

public:
	C4MCNodeType Type() override { return MCN_Overlay; } // get node type

	friend class C4MapCreatorS2;
	friend class C4MCParser;
};

// point of polygon node
class C4MCPoint : public C4MCNode
{
public:
	C4MCPoint(C4MCNode *pOwner = nullptr);
	C4MCPoint(C4MCNode *pOwner, C4MCPoint &rTemplate, bool fClone); // construct of template

	C4MCNode *clone(C4MCNode *pToNode) override { return new C4MCPoint(pToNode, *this, true); }

protected:
	void Default(); // set default values for default presets

public:
	int32_t X, Y;
	int_bool RX, RY;

	virtual void Evaluate() override; // called when all fields are initialized
	bool SetField(C4MCParser *pParser, const char *szField, const char *szSVal, int32_t iVal, C4MCTokenType ValType) override; // set field

public:
	C4MCNodeType Type() override { return MCN_Point; } // get node type

	friend class C4MapCreatorS2;
	friend class C4MCParser;
};

// simply an overlay that can be rendered
class C4MCMap : public C4MCOverlay
{
public:
	C4MCMap(C4MCNode *pOwner = nullptr);
	C4MCMap(C4MCNode *pOwner, C4MCMap &rTemplate, bool fClone); // construct of template

	C4MCNode *clone(C4MCNode *pToNode) override { return new C4MCMap(pToNode, *this, true); }

protected:
	void Default(); // set default values for default presets

public:
	bool RenderTo(uint8_t *pToBuf, int32_t iPitch); // render to buffer
	void SetSize(int32_t iWdt, int32_t iHgt);

public:
	C4MCNodeType Type() override { return MCN_Map; } // get node type

	friend class C4MapCreatorS2;
	friend class C4MCParser;
};

// main map creator class
class C4MapCreatorS2 : public C4MCNode
{
public:
	C4MapCreatorS2(C4Section &section, C4SLandscape *pLandscape, C4TextureMap *pTexMap, C4MaterialMap *pMatMap, int iPlayerCount);
	~C4MapCreatorS2();

	void Default(); // set default data
	void Clear(); // clear any data
	bool ReadFile(const char *szFilename, C4Group *pGrp); // read defs of file
	bool ReadScript(const char *szScript); // reads def directly from mem

public:
	C4MCMap *GetMap(const char *szMapName); // get map by name

public:
	std::unique_ptr<CSurface8> Render(const char *szMapName); // create map surface
	C4Section &GetSection() const noexcept { return section; }

protected:
	C4Section &section;
	C4SLandscape  *Landscape; // landsape presets
	C4TextureMap  *TexMap; // texture map
	C4MaterialMap *MatMap; // material map
	C4MCMap DefaultMap; // default template: landscape
	C4MCOverlay DefaultOverlay; // default template: overlay
	C4MCPoint DefaultPoint; // default template: point
	C4MCMap *pCurrentMap; // map currently rendered
	C4MCCallbackArrayList CallbackArrays; // list of callback arrays
	int PlayerCount; // player count for MapPlayerExtend

	bool GlobalScope() override { return true; } // it's the global node

public:
	void ExecuteCallbacks(int32_t iMapZoom) { CallbackArrays.Execute(section, iMapZoom); }

	friend class C4MCOverlay;
	friend class C4MCMap;
	friend class C4MCParser;
	friend class C4MCCallbackArray;
};

// file parser for map creator

// parser error
class C4MCParserErr
{
public:
	std::string Msg; // message string

	C4MCParserErr(C4MCParser *pParser, std::string_view msg); // construct setting error msg

	template<typename... Args>
	C4MCParserErr(C4MCParser *pParser, std::format_string<Args...> fmt, Args &&...args) // construct setting error msg
		: C4MCParserErr{pParser, std::format(fmt, std::forward<Args>(args)...)}
	{
	}

	void show() const; // log error
};

// the parser
class C4MCParser
{
private:
	C4MapCreatorS2 *MapCreator; // map creator parsing into
	char *Code; // loaded code
	const char *CPos; // current parser pos in code
	C4MCTokenType CurrToken; // last token read
	char CurrTokenIdtf[C4MaxName]; // current token string
	int32_t CurrTokenVal; // current token value
	char Filename[C4MaxName]; // filename

	bool AdvanceSpaces(); // advance to next token char; return whether EOF is reached
	bool GetNextToken(); // get token, store in fields and advance to next; return whether not EOF
	void ParseTo(C4MCNode *pToNode); // parse stuff into
	void ParseValue(C4MCNode *pToNode, const char *szFieldName); // Set Field

public:
	C4MCParser(C4MapCreatorS2 *pMapCreator);
	~C4MCParser();

	void Clear(); // clear stuff

	void ParseFile(const char *szFilename, C4Group *pGrp); // load and parse file
	void Parse(const char *szScript); // load and parse from mem

	friend class C4MCParserErr;
};
