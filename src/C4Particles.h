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

// newgfx particle system for smoke, sparks, ...
// - everything, that is not sync-relevant
// function pointers for drawing and executing are used
// instead of virtual classes and a hierarchy, because
// the latter ones couldn't be optimized using static
// chunks
// thus, more complex partivle behaviour should be solved via
// objects
// note: this particle system will always assume the owning def
//  object to be a static class named Game.Particles!

#pragma once

#include <C4FacetEx.h>
#include "C4ForwardDeclarations.h"
#include <C4Group.h>
#include <C4Shape.h>

// class predefs
class C4ParticleDefCore;
class C4ParticleDef;
class C4Particle;
class C4ParticleChunk;
class C4ParticleList;
class C4ParticleSystem;

typedef bool(*C4ParticleProc)(C4Particle *, C4Section &, C4Object *); // generic particle proc
typedef C4ParticleProc C4ParticleInitProc; // particle init proc - init and return whether particle could be created
typedef C4ParticleProc C4ParticleExecProc; // particle execution proc - returns whether particle died
typedef C4ParticleProc C4ParticleCollisionProc; // particle collision proc - returns whether particle died
typedef void(*C4ParticleDrawProc)(C4Particle *, C4FacetEx &, C4Object *); // particle drawing code

// core for particle defs
class C4ParticleDefCore
{
public:
	StdStrBuf Name; // name
	C4TargetRect GfxFace; // target rect for graphics; used because stup
	int32_t MaxCount; // maximum number of particles that may coexist of this type
	int32_t MinLifetime, MaxLifetime; // used by exec proc; number of frames this particle can exist
	int32_t YOff; // Y-Offset for Std-particles
	int32_t Delay; // frame delay between animation phases
	int32_t Repeats; // number of times the animation is repeated
	int32_t Reverse; // reverse action after it has been played
	int32_t FadeOutLen, FadeOutDelay; // phases used for letting the particle fade out
	int32_t RByV; // if set, rotation will be adjusted according to the movement; if 2, the particle does not move
	int32_t Placement; // when is the particle to be drawn?
	int32_t GravityAcc; // acceleration done by gravity
	int32_t WindDrift;
	int32_t VertexCount; // number of vertices - 0 or 1
	int32_t VertexY; // y-offset of vertex; 100 is object height
	int32_t Additive; // whether particle should be drawn additively
	int32_t Attach; // whether the particle moves relatively to the target
	int32_t AlphaFade; // fadeout in each frame
	int32_t Parallaxity[2]; // parallaxity

	StdStrBuf InitFn;      // proc to be used for initialization
	StdStrBuf ExecFn;      // proc to be used for frame-execution
	StdStrBuf DrawFn;      // proc to be used for drawing
	StdStrBuf CollisionFn; // proc to be called upon collision with the landscape; may be left out

	C4ParticleDefCore();
	void CompileFunc(StdCompiler *pComp);

	bool Compile(const char *szSource, const char *szName); // compile from def file
};

class C4LoadedParticleList;

// one particle definition
class C4ParticleDef : public C4ParticleDefCore
{
public:
	C4ParticleDef *pPrev, *pNext; // linked list members

	StdStrBuf Filename; // path to group this particle was loaded from (for EM reloading)

	C4FacetExSurface Gfx; // graphics
	int32_t Length; // number of phases in gfx
	float Aspect; // height:width

	C4ParticleInitProc      InitProc;      // procedure called once upon creation of the particle
	C4ParticleExecProc      ExecProc;      // procedure used for execution of one particle
	C4ParticleCollisionProc CollisionProc; // procedure called upon collision with the landscape; may be nullptr
	C4ParticleDrawProc      DrawProc;      // procedure used for drawing of one particle

	int32_t Count; // number of particles currently existent of this kind

	C4ParticleDef();
	~C4ParticleDef();

	void Clear(); // free mem associated with this class

	bool Load(C4Group &rGrp); // load particle from group; assume file to be accessed already
	bool Reload(); // reload particle from stored position
};

// one tiny little particle
// note: list management done by the chunk, which spares one ptr here
// the chunk initializes the values to zero here, too; so no ctors here...
class C4Particle
{
protected:
	C4Particle *pPrev, *pNext; // previous/next particle of the same list in the buffer

	void MoveList(C4ParticleList &rFrom, C4ParticleList &rTo); // move from one list to another

public:
	C4ParticleDef *pDef; // kind of particle
	float x, y, xdir, ydir; // position and movement
	int32_t life; // lifetime remaining for this particle
	float a; int32_t b; // all-purpose values

	friend class C4ParticleChunk;
	friend class C4ParticleList;
	friend class C4ParticleSystem;
};

// one chunk of particles
// linked list is managed by owner
class C4ParticleChunk
{
protected:
	C4ParticleChunk *pNext; // single linked list
	C4Particle Data[C4Px_BufSize]; // the particles

public:
	C4ParticleChunk();
	~C4ParticleChunk();

	void Clear(); // clear all particles

	friend class C4ParticleSystem;
};

// a subset of particles
class C4ParticleList
{
public:
	C4Particle *pFirst; // first particle in list - others follow in linked list

	C4ParticleList() { pFirst = nullptr; }

	void Exec(C4Section &section, C4Object *pObj = nullptr); // execute all particles
	void Draw(C4FacetEx &cgo, C4Object *pObj = nullptr); // draw all particles
	void Clear(C4ParticleList &freeParticles); // remove all particles
	int32_t Remove(C4ParticleList &freeParticles, C4ParticleDef *pOfDef); // remove all particles of def

	operator bool() { return !!pFirst; } // checks whether list contains particles
};

class C4LoadedParticleList
{
public:
	~C4LoadedParticleList();

protected:
	C4ParticleDef *pDef0, *pDefL; // linked list for particle defs
	C4ParticleProc GetProc(const char *szName); // get init/exec proc for a particle type
	C4ParticleDrawProc GetDrawProc(const char *szName); // get draw proc for a particle type

public:
	C4ParticleDef *pSmoke{nullptr};  // default particle: smoke
	C4ParticleDef *pBlast{nullptr};  // default particle: blast
	C4ParticleDef *pFSpark{nullptr}; // default particle: firy spark
	C4ParticleDef *pFire1{nullptr};  // default particle: fire base
	C4ParticleDef *pFire2{nullptr};  // default particle: fire additive

	void Clear(); // remove all particle definitions and particles

	C4ParticleDef *GetDef(const char *szName, C4ParticleDef *pExclude = nullptr); // get particle def by name
	void SetDefParticles(); // seek and assign default particels (smoke, etc.)
	bool IsFireParticleLoaded() { return pFire1 && pFire2; }

	friend class C4ParticleDef;
	friend class C4Particle;
	friend class C4ParticleChunk;
};

// the main particle system
class C4ParticleSystem
{
protected:
	C4ParticleChunk Chunk; // linked list for particle chunks

	C4ParticleChunk *AddChunk(); // add a new chunk to the list


public:
	C4Section &section;
	C4ParticleList FreeParticles; // list of free particles
	C4ParticleList GlobalParticles; // list of free particles

	C4ParticleSystem(C4Section &section);
	~C4ParticleSystem();

	void ClearParticles(); // remove all particles

	C4Particle *Create(C4ParticleDef *pOfDef, // create one particle of given type
		float x, float y, float xdir = 0.0f, float ydir = 0.0f,
		float a = 0.0f, int32_t b = 0, C4ParticleList *pPxList = nullptr, C4Object *pObj = nullptr);
	bool Cast(C4ParticleDef *pOfDef, // create several particles with different speeds and params
		int32_t iAmount,
		float x, float y, int32_t level,
		float a0 = 0.0f, uint32_t b0 = 0, float a1 = 0.0f, uint32_t b1 = 0,
		C4ParticleList *pPxList = nullptr, C4Object *pObj = nullptr);


	int32_t Push(C4ParticleDef *pOfDef, float dxdir, float dydir); // add movement to all particles of type
};

// default particle execution/drawing functions
bool fxStdInit(C4Particle *pPrt, C4Section &section, C4Object *pTarget);
bool fxStdExec(C4Particle *pPrt, C4Section &section, C4Object *pTarget);
void fxStdDraw(C4Particle *pPrt, C4FacetEx &cgo, C4Object *pTarget);

// structures used for static function maps
struct C4ParticleProcRec
{
	char Name[C4Px_MaxIDLen + 1]; // name of procedure
	C4ParticleProc Proc; // procedure
};

struct C4ParticleDrawProcRec
{
	char Name[C4Px_MaxIDLen + 1]; // name of procedure
	C4ParticleDrawProc Proc; // procedure
};

extern C4ParticleProcRec C4ParticleProcMap[]; // particle init/execution function map
extern C4ParticleDrawProcRec C4ParticleDrawProcMap[]; // particle drawing function map
