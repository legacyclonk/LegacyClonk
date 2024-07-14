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

// newgfx particle system for smoke, sparks, ...

#include <C4Include.h>
#include <C4Particles.h>

#include <C4Physics.h>
#include <C4Object.h>
#include <C4Random.h>
#include <C4Game.h>
#include <C4Components.h>
#include <C4Wrappers.h>

void C4ParticleDefCore::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(toC4CStrBuf(Name),                "Name",         ""));
	pComp->Value(mkNamingAdapt(MaxCount,                         "MaxCount",     C4Px_MaxParticle));
	pComp->Value(mkNamingAdapt(MinLifetime,                      "MinLifetime",  0));
	pComp->Value(mkNamingAdapt(MaxLifetime,                      "MaxLifetime",  0));
	pComp->Value(mkNamingAdapt(toC4CStrBuf(InitFn),              "InitFn",       ""));
	pComp->Value(mkNamingAdapt(toC4CStrBuf(ExecFn),              "ExecFn",       ""));
	pComp->Value(mkNamingAdapt(toC4CStrBuf(CollisionFn),         "CollisionFn",  ""));
	pComp->Value(mkNamingAdapt(toC4CStrBuf(DrawFn),              "DrawFn",       ""));
	pComp->Value(mkNamingAdapt(GfxFace,                          "Face"));
	pComp->Value(mkNamingAdapt(YOff,                             "YOff",         0));
	pComp->Value(mkNamingAdapt(Delay,                            "Delay",        0));
	pComp->Value(mkNamingAdapt(Repeats,                          "Repeats",      0));
	pComp->Value(mkNamingAdapt(Reverse,                          "Reverse",      0));
	pComp->Value(mkNamingAdapt(FadeOutLen,                       "FadeOutLen",   0));
	pComp->Value(mkNamingAdapt(FadeOutDelay,                     "FadeOutDelay", 0));
	pComp->Value(mkNamingAdapt(RByV,                             "RByV",         0));
	pComp->Value(mkNamingAdapt(GravityAcc,                       "GravityAcc",   0));
	pComp->Value(mkNamingAdapt(WindDrift,                        "WindDrift",    0));
	pComp->Value(mkNamingAdapt(VertexCount,                      "VertexCount",  0));
	pComp->Value(mkNamingAdapt(VertexY,                          "VertexY",      0));
	pComp->Value(mkNamingAdapt(Additive,                         "Additive",     0));
	pComp->Value(mkNamingAdapt(AlphaFade,                        "AlphaFade",    0));
	pComp->Value(mkNamingAdapt(mkArrayAdapt(Parallaxity, 100),   "Parallaxity"));
	pComp->Value(mkNamingAdapt(Attach,                           "Attach",       0));
}

C4ParticleDefCore::C4ParticleDefCore() :
	MaxCount(C4Px_MaxParticle),
	MinLifetime(0), MaxLifetime(0),
	YOff(0),
	Reverse(0), Repeats(0), Delay(0),
	FadeOutLen(0), FadeOutDelay(0),
	RByV(0),
	Placement(0),
	GravityAcc(0),
	VertexCount(0), VertexY(0),
	Additive(0),
	Attach(0),
	AlphaFade(0)
{
	GfxFace.Default();
	Parallaxity[0] = Parallaxity[1] = 100;
}

bool C4ParticleDefCore::Compile(const char *szSource, const char *szName)
{
	return CompileFromBuf_LogWarn<StdCompilerINIRead>(mkNamingAdapt(*this, "Particle"),
		StdStrBuf::MakeRef(szSource), szName);
}

C4ParticleDef::C4ParticleDef() :
	C4ParticleDefCore(),
	InitProc(&fxStdInit),
	ExecProc(&fxStdExec),
	DrawProc(&fxStdDraw),
	CollisionProc(nullptr),
	Count(0)
{
	// zero fields
	Gfx.Default();
	// link into list
	if (!Game.Particles.pDef0)
	{
		pPrev = nullptr;
		Game.Particles.pDef0 = this;
	}
	else
		(pPrev = Game.Particles.pDefL)->pNext = this;
	Game.Particles.pDefL = this;
	pNext = nullptr;
}

C4ParticleDef::~C4ParticleDef()
{
	// clear
	Clear();
	// unlink from list
	if (pPrev) pPrev->pNext = pNext; else Game.Particles.pDef0 = pNext;
	if (pNext) pNext->pPrev = pPrev; else Game.Particles.pDefL = pPrev;
}

void C4ParticleDef::Clear()
{
	Name.Clear();
}

bool C4ParticleDef::Load(C4Group &rGrp)
{
	// store file
	Filename.Copy(rGrp.GetFullName());
	// load
	char *pSource;
	if (rGrp.LoadEntry(C4CFN_ParticleCore, &pSource, nullptr, 1))
	{
		if (!Compile(pSource, Filename.getData()))
		{
			DebugLog(spdlog::level::err, "invalid particle def at '{}'", rGrp.GetFullName().getData());
			delete[] pSource; return false;
		}
		delete[] pSource;
		// load graphics
		if (!Gfx.Load(rGrp, C4CFN_DefGraphicsPNG))
		{
			DebugLog(spdlog::level::err, "particle {} has no valid graphics defined", Name.getData());
			return false;
		}
		// set facet, if assigned - otherwise, assume full surface
		if (GfxFace.Wdt) Gfx.Set(Gfx.Surface, GfxFace.x, GfxFace.y, GfxFace.Wdt, GfxFace.Hgt, GfxFace.tx, GfxFace.ty);
		// set phase num
		int32_t Q; Gfx.GetPhaseNum(Length, Q);
		if (!Length)
		{
			DebugLog(spdlog::level::err, "invalid facet for particle '{}'", Name.getData());
			return false;
		}
		// case fadeout from length
		if (FadeOutLen)
		{
			Length = std::max<int32_t>(Length - FadeOutLen, 1);
			if (!FadeOutDelay) FadeOutDelay = 1;
		}
		// if phase num is 1, no reverse is allowed
		if (Length == 1) Reverse = 0;
		// calc aspect
		Aspect = static_cast<float>(Gfx.Wdt) / Gfx.Hgt;
		// get proc pointers
		if (!(InitProc = Game.Particles.GetProc(InitFn.getData())))
		{
			DebugLog(spdlog::level::err, "init proc for particle '{}' not found: '{}'", Name.getData(), InitFn.getData());
			return false;
		}
		if (!(ExecProc = Game.Particles.GetProc(ExecFn.getData())))
		{
			DebugLog(spdlog::level::err, "exec proc for particle '{}' not found: '{}'", Name.getData(), ExecFn.getData());
			return false;
		}
		if (CollisionFn && CollisionFn[0]) if (!(CollisionProc = Game.Particles.GetProc(CollisionFn.getData())))
		{
			DebugLog(spdlog::level::err, "collision proc for particle '{}' not found: '{}'", Name.getData(), CollisionFn.getData());
			return false;
		}
		if (!(DrawProc = Game.Particles.GetDrawProc(DrawFn.getData())))
		{
			DebugLog(spdlog::level::err, "draw proc for particle '{}' not found: '{}'", Name.getData(), DrawFn.getData());
			return false;
		}
		// particle overloading
		C4ParticleDef *pDefOverload;
		if (pDefOverload = Game.Particles.GetDef(Name.getData(), this))
		{
			if (Config.Graphics.VerboseObjectLoading >= 1)
			{
				Log(C4ResStrTableKey::IDS_PRC_DEFOVERLOAD, pDefOverload->Name.getData(), "<particle>");
			}
			delete pDefOverload;
		}
		// success
		return true;
	}
	return false;
}

bool C4ParticleDef::Reload()
{
	// no file?
	if (!Filename[0]) return false;
	// open group
	C4Group hGroup;
	if (!hGroup.Open(Filename.getData())) return false;
	// reset class
	Clear();
	// load
	return Load(hGroup);
}

void C4Particle::MoveList(C4ParticleList &rFrom, C4ParticleList &rTo)
{
	// remove from current list
	if (pPrev)
		pPrev->pNext = pNext;
	else
		// is it the first item in the list? then set this to the next one
		if (rFrom.pFirst == this) rFrom.pFirst = pNext;
	if (pNext) pNext->pPrev = pPrev;
	// add to the other list - insert before first
	if (pNext = rTo.pFirst) pNext->pPrev = this;
	rTo.pFirst = this; pPrev = nullptr;
}

C4ParticleChunk::C4ParticleChunk()
{
	// zero linked list
	pNext = nullptr;
	// zero buffer
	Clear();
}

C4ParticleChunk::~C4ParticleChunk()
{
	// list stuff done by C4ParticleSystem
}

void C4ParticleChunk::Clear()
{
	// note that this method is called in ctor with uninitialized data!
	// simply clear mem - this won't adjust any counts!
	std::memset(Data, 0, sizeof(Data));
	// init list
	C4Particle *pPrt = Data;
	for (int32_t i = 0; i < C4Px_BufSize; ++i)
	{
		pPrt->pPrev = pPrt - 1;
		pPrt->pNext = pPrt + 1;
		++pPrt;
	}
	Data[0].pPrev = Data[C4Px_BufSize - 1].pNext = nullptr;
}

void C4ParticleList::Exec(C4Section &section, C4Object *pObj)
{
	// execute all particles
	C4Particle *pPrtNext = pFirst, *pPrt;
	while (pPrt = pPrtNext)
	{
		// get next now, because destruction could corrupt the list
		pPrtNext = pPrt->pNext;
		// execute it
		if (!pPrt->pDef->ExecProc(pPrt, section, pObj))
		{
			// sorry, life is over for you :P
			--pPrt->pDef->Count;
			pPrt->MoveList(*this, section.Particles.FreeParticles);
		}
	}
	// done
}

void C4ParticleList::Draw(C4FacetEx &cgo, C4Object *pObj)
{
	// draw all particles
	for (C4Particle *pPrt = pFirst; pPrt; pPrt = pPrt->pNext)
		pPrt->pDef->DrawProc(pPrt, cgo, pObj);
	// done
}

void C4ParticleList::Clear(C4ParticleList &freeParticles)
{
	// remove all particles
	C4Particle *pPrtNext = pFirst, *pPrt;
	while (pPrt = pPrtNext)
	{
		// get next now, because destruction could corrupt the list
		pPrtNext = pPrt->pNext;
		// sorry, life is over for you :P
		--pPrt->pDef->Count;
		pPrt->MoveList(*this, freeParticles);
	}
}

int32_t C4ParticleList::Remove(C4ParticleList &freeParticles, C4ParticleDef *pOfDef)
{
	int32_t iNumRemoved = 0;
	// check all particles for def
	C4Particle *pPrtNext = pFirst, *pPrt;
	while (pPrt = pPrtNext)
	{
		// get next now, because destruction could corrupt the list
		pPrtNext = pPrt->pNext;
		// execute it
		if (!pOfDef || pPrt->pDef == pOfDef)
		{
			// sorry, life is over for you :P
			--pPrt->pDef->Count;
			pPrt->MoveList(*this, freeParticles);
		}
	}
	// done
	return iNumRemoved;
}

C4LoadedParticleList::~C4LoadedParticleList()
{
	Clear();
}

void C4LoadedParticleList::Clear()
{
	// clear defs
	while (pDef0) delete pDef0;
	// clear system particles
	pSmoke = pBlast = pFSpark = pFire1 = pFire2 = nullptr;
	// done
}

C4ParticleProc C4LoadedParticleList::GetProc(const char *szName)
{
	// seek in map
	for (int32_t i = 0; C4ParticleProcMap[i].Name[0]; ++i)
		if (SEqual(C4ParticleProcMap[i].Name, szName))
			return C4ParticleProcMap[i].Proc;
	// nothing found...
	return nullptr;
}

C4ParticleDrawProc C4LoadedParticleList::GetDrawProc(const char *szName)
{
	// seek in map
	for (int32_t i = 0; C4ParticleDrawProcMap[i].Name[0]; ++i)
		if (SEqual(C4ParticleDrawProcMap[i].Name, szName))
			return C4ParticleDrawProcMap[i].Proc;
	// nothing found...
	return nullptr;
}

C4ParticleDef *C4LoadedParticleList::GetDef(const char *szName, C4ParticleDef *pExclude)
{
	// seek list
	for (C4ParticleDef *pDef = pDef0; pDef; pDef = pDef->pNext)
		if (pDef != pExclude && pDef->Name == szName)
			return pDef;
	// nothing found
	return nullptr;
}

void C4LoadedParticleList::SetDefParticles()
{
	// get smoke
	pSmoke = GetDef("Smoke");
	// get blast
	pBlast = GetDef("Blast");
	pFSpark = GetDef("FSpark");
	// get fire, if fire particles are desired
	if (Config.Graphics.FireParticles)
	{
		pFire1 = GetDef("Fire");
		pFire2 = GetDef("Fire2");
	}
	else
		pFire1 = pFire2 = nullptr;
	// if fire is drawn w/o background fct: unload fire face if both fire particles are assigned
	// but this is not done here
}

C4ParticleSystem::~C4ParticleSystem()
{
	// clean up
	ClearParticles();
}

C4ParticleChunk *C4ParticleSystem::AddChunk()
{
	// add another chunk
	C4ParticleChunk *pNewChnk = new C4ParticleChunk();
	pNewChnk->pNext = Chunk.pNext;
	Chunk.pNext = pNewChnk;
	// register into free-particle-list
	if (pNewChnk->Data[C4Px_BufSize - 1].pNext = FreeParticles.pFirst)
		FreeParticles.pFirst->pPrev = &pNewChnk->Data[C4Px_BufSize - 1];
	FreeParticles.pFirst = &pNewChnk->Data[0];
	// return it
	return pNewChnk;
}

C4ParticleSystem::C4ParticleSystem(C4Section &section)
	: section{section}
{

}
void C4ParticleSystem::ClearParticles()
{
	// clear particle lists
	C4ObjectLink *pLnk;
	for (pLnk = section.Objects.First; pLnk; pLnk = pLnk->Next)
		pLnk->Obj->FrontParticles.pFirst = pLnk->Obj->BackParticles.pFirst = nullptr;
	for (pLnk = section.Objects.InactiveObjects.First; pLnk; pLnk = pLnk->Next)
		pLnk->Obj->FrontParticles.pFirst = pLnk->Obj->BackParticles.pFirst = nullptr;
	GlobalParticles.pFirst = nullptr;
	// reset chunks
	C4ParticleChunk *pNextChnk = Chunk.pNext, *pChnk;
	while (pChnk = pNextChnk)
	{
		pNextChnk = pChnk->pNext;
		delete pChnk;
	}
	Chunk.pNext = nullptr;
	Chunk.Clear();
	FreeParticles.pFirst = Chunk.Data;
	// adjust counts
	// FIXME
	/*for (C4ParticleDef *pDef = pDef0; pDef; pDef = pDef->pNext)
		pDef->Count = 0;
	*/
}

C4Particle *C4ParticleSystem::Create(C4ParticleDef *pOfDef,
	float x, float y,
	float xdir, float ydir,
	float a, int32_t b, C4ParticleList *pPxList,
	C4Object *pObj)
{
	// safety
	if (!pOfDef) return nullptr;
	// default to global list
	if (!pPxList) pPxList = &GlobalParticles;
	// check count
	int32_t MaxCount = pOfDef->MaxCount * (Config.Graphics.SmokeLevel + 20) / 150;
	int32_t iRoom = MaxCount - pOfDef->Count;
	if (iRoom <= 0) return nullptr;
	// reduce creation if limit is nearly reached
	if (iRoom < (MaxCount >> 1))
		if (SafeRandom(iRoom) < SafeRandom(MaxCount)) return nullptr;
	// get free particle
	if (!FreeParticles.pFirst) AddChunk();
	C4Particle *pPrt = FreeParticles.pFirst;
	if (!pPrt) return nullptr;
	// set values
	pPrt->x = x; pPrt->y = y;
	pPrt->xdir = xdir; pPrt->ydir = ydir;
	pPrt->a = a; pPrt->b = b;
	pPrt->pDef = pOfDef;
	if (pPrt->pDef->Attach && pObj != nullptr)
	{
		pPrt->x -= pObj->x;
		pPrt->y -= pObj->y;
	}
	// call initialization
	if (!pOfDef->InitProc(pPrt, section, pObj))
		// failed :(
		return nullptr;
	// count particle
	++pOfDef->Count;
	// more to desired list
	pPrt->MoveList(FreeParticles, *pPxList);
	// return newly created particle
	return pPrt;
}

bool C4ParticleSystem::Cast(C4ParticleDef *pOfDef, int32_t iAmount,
	float x, float y, int32_t level,
	float a0, uint32_t b0, float a1, uint32_t b1, C4ParticleList *pPxList, C4Object *pObj)
{
	// safety
	if (!pOfDef) return false;
	// get range for a and b
	int32_t iA0 = static_cast<int32_t>(a0 * 100), iA1 = static_cast<int32_t>(a1 * 100);
	if (iA1 < iA0) std::swap(iA0, iA1);
	int32_t iAd = iA1 - iA0 + 1;
	if (b1 < b0) { uint32_t dwX = b0; b0 = b1; b1 = dwX; }
	uint32_t db = b1 - b0;
	uint8_t db1 = uint8_t(db >> 24), db2 = uint8_t(db >> 16), db3 = uint8_t(db >> 8), db4 = uint8_t(db);
	// create them
	for (int32_t i = iAmount; i > 0; --i)
		Create(pOfDef, x, y,
		static_cast<int32_t>(SafeRandom(level + 1) - level / 2) / 10.0f,
			static_cast<int32_t>(SafeRandom(level + 1) - level / 2) / 10.0f,
			static_cast<int32_t>(iA0 + SafeRandom(iAd)) / 100.0f,
			b0 + (SafeRandom(db1) << 24) + (SafeRandom(db2) << 16) + (SafeRandom(db3) << 8) + SafeRandom(db4), pPxList, pObj);
	// success
	return true;
}

int32_t C4ParticleSystem::Push(C4ParticleDef *pOfDef, float dxdir, float dydir)
{
	int32_t iNumPushed = 0;
	// go through all particle chunks
	for (C4ParticleChunk *pChnk = &Chunk; pChnk; pChnk = pChnk->pNext)
	{
		// go through all particles
		C4Particle *pPrt = pChnk->Data; int32_t i = C4Px_BufSize;
		while (i--)
		{
			// def fits?
			if (!pOfDef || pPrt->pDef == pOfDef)
			{
				// push it!
				pPrt->xdir += dxdir;
				pPrt->ydir += dydir;
				// count pushed
				++iNumPushed;
			}
			// next particle
			++pPrt;
		}
	}
	// done
	return iNumPushed;
}

bool fxSmokeInit(C4Particle *pPrt, C4Section &section, C4Object *pTarget)
{
	// init lifetime
	pPrt->life = pPrt->pDef->MinLifetime;
	int32_t iLD = pPrt->pDef->MaxLifetime - pPrt->pDef->MinLifetime;
	if (iLD) pPrt->life += SafeRandom(iLD);
	// use high-word of life to store init-status
	pPrt->life |= (pPrt->life / 17) << 16;
	// set kind - ydir is unused anyway; set last kind reeeaaally seldom
	pPrt->ydir = static_cast<float>(SafeRandom(15)) + SafeRandom(300) / 299;
	// set color
	if (!pPrt->b) pPrt->b = 0xff4b4b4b; else pPrt->b |= 0xff000000;
	// always OK
	return true;
}

bool fxSmokeExec(C4Particle *pPrt, C4Section &section, C4Object *pTarget)
{
	// lifetime
	if (!--pPrt->life) return false;
	bool fBuilding = !!(pPrt->life & 0x7fff0000);
	// still building?
	if (fBuilding)
	{
		// decrease init-time
		pPrt->life -= 0x010000;
		// increase color value
		pPrt->b -= 0x10000000;
		// if full-grown, adjust to lifetime
		if (!(pPrt->life & 0x7fff0000))
			pPrt->b = (pPrt->b & 0xffffff) | ((255 - pPrt->life) << 24);
	}
	// color change
	uint32_t dwClr = pPrt->b;
	pPrt->b = (LightenClrBy(dwClr, 1) & 0xffffff) | std::min<int32_t>((dwClr >> 24) + 1, 255) << 24;
	// wind to float
	if (!(pPrt->b % 12) || fBuilding)
	{
		pPrt->xdir = 0.025f * section.Weather.GetWind(int32_t(pPrt->x), int32_t(pPrt->y));
		if (pPrt->xdir < -2.0f) pPrt->xdir = -2.0f; else if (pPrt->xdir > 2.0f) pPrt->xdir = 2.0f;
		pPrt->xdir += 0.1f * SafeRandom(41) - 2.0f;
	}
	// float
	if (section.Landscape.GBackSolid(int32_t(pPrt->x), int32_t(pPrt->y - pPrt->a)))
	{
		// if stuck, decay; otherwise, move down
		if (!section.Landscape.GBackSolid(int32_t(pPrt->x), int32_t(pPrt->y))) pPrt->y += 0.4f; else pPrt->a -= 2;
	}
	else
		--pPrt->y;
	pPrt->x += pPrt->xdir;
	// increase in size
	pPrt->a *= 1.01f;
	// done, keep
	return true;
}

void fxSmokeDraw(C4Particle *pPrt, C4FacetEx &cgo, C4Object *pTarget)
{
	C4ParticleDef *pDef = pPrt->pDef;
	// apply parallaxity to target pos
	int32_t tx = cgo.TargetX * pDef->Parallaxity[0] / 100;
	int32_t ty = cgo.TargetY * pDef->Parallaxity[1] / 100;
	// check if it's in screen range
	if (!Inside(pPrt->x, tx - pPrt->a, tx + cgo.Wdt + pPrt->a)) return;
	if (!Inside(pPrt->y, ty - pPrt->a, ty + cgo.Hgt + pPrt->a)) return;
	// get pos
	int32_t cx = int32_t(pPrt->x) + cgo.X - tx;
	int32_t cy = int32_t(pPrt->y) + cgo.Y - ty;
	// get phase by particle index
	int32_t i = static_cast<int32_t>(pPrt->ydir);
	int32_t ipx = i / 4;
	int32_t ipy = i % 4;
	// draw at pos
	Application.DDraw->ActivateBlitModulation(pPrt->b);
	pDef->Gfx.DrawX(cgo.Surface, int32_t(cx - pPrt->a), int32_t(cy - pPrt->a), int32_t(pPrt->a * 2), int32_t(pPrt->a * 2), ipx, ipy);
	Application.DDraw->DeactivateBlitModulation();
}

bool fxStdInit(C4Particle *pPrt, C4Section &section, C4Object *pTarget)
{
	if (pPrt->pDef->Delay)
		// delay given: lifetime starts at zero
		pPrt->life = 0;
	else
		// init lifetime as phase
		pPrt->life = SafeRandom(pPrt->pDef->Length);
	// default color
	if (!pPrt->b) pPrt->b = 0xffffff;
	// always OK
	return true;
}

bool fxStdExec(C4Particle *pPrt, C4Section &section, C4Object *pTarget)
{
	float dx = pPrt->x, dy = pPrt->y;
	float dxdir = pPrt->xdir, dydir = pPrt->ydir;
	// rel. position & movement
	if (pPrt->pDef->Attach && pTarget != nullptr)
	{
		dx += pTarget->x;
		dy += pTarget->y;
		dxdir += fixtof(pTarget->xdir);
		dydir += fixtof(pTarget->ydir);
	}

	// move
	if (pPrt->xdir || pPrt->ydir)
	{
		if (pPrt->pDef->VertexCount && section.Landscape.GBackSolid(int32_t(dx + pPrt->xdir), int32_t(dy + pPrt->ydir + pPrt->pDef->VertexY * pPrt->a / 100.0f)))
		{
			// collision
			if (pPrt->pDef->CollisionProc)
				if (!pPrt->pDef->CollisionProc(pPrt, section, pTarget)) return false;
		}
		else if (pPrt->pDef->RByV != 2)
		{
			pPrt->x += pPrt->xdir;
			pPrt->y += pPrt->ydir;
		}
		else
		{
			// With RByV=2, the V is only used for rotation, not for movement
		}
	}
	// apply gravity
	if (pPrt->pDef->GravityAcc) pPrt->ydir += fixtof(section.Landscape.Gravity * pPrt->pDef->GravityAcc) / 100.0f;
	// apply WindDrift
	if (pPrt->pDef->WindDrift && !section.Landscape.GBackSolid(int32_t(dx), int32_t(dy)))
	{
		// Air speed: Wind plus some random
		int32_t iWind = section.GBackWind(int32_t(dx), int32_t(dy));
		float txdir = iWind / 15.0f;
		float tydir = 0;

		// Air friction, based on WindDrift.
		int32_t iWindDrift = (std::max)(pPrt->pDef->WindDrift - 20, 0);
		pPrt->xdir += ((txdir - dxdir) * iWindDrift) / 800;
		pPrt->ydir += ((tydir - dydir) * iWindDrift) / 800;
	}
	// fade out
	int32_t iFade = pPrt->pDef->AlphaFade;
	if (iFade < 0) if (Game.FrameCounter % -iFade == 0) iFade = 1; else iFade = 0;
	if (iFade)
	{
		uint32_t dwClr = pPrt->b;
		int32_t iAlpha = dwClr >> 24;
		iAlpha += pPrt->pDef->AlphaFade;
		if (iAlpha >= 0xff) return false;
		pPrt->b = (dwClr & 0xffffff) | (iAlpha << 24);
	}
	// if delay is given, advance lifetime
	if (pPrt->pDef->Delay)
	{
		if (pPrt->life < 0)
		{
			// decay
			return pPrt->life-- >= -pPrt->pDef->FadeOutLen * pPrt->pDef->FadeOutDelay;
		}
		++pPrt->life;
		// check if still alive
		int32_t iPhase = pPrt->life / pPrt->pDef->Delay;
		int32_t length = pPrt->pDef->Length - pPrt->pDef->Reverse;
		if (iPhase >= length * pPrt->pDef->Repeats + pPrt->pDef->Reverse)
		{
			// do fadeout, if assigned
			if (!pPrt->pDef->FadeOutLen) return false;
			pPrt->life = -1;
		}
		return true;
	}
	// outside landscape range?
	bool kp;
	if (dxdir > 0) kp =       (dx - pPrt->a < section.Landscape.Width); else kp =       (dx + pPrt->a > 0);
	if (dydir > 0) kp = kp && (dy - pPrt->a < section.Landscape.Height); else kp = kp && (dy + pPrt->a > pPrt->pDef->YOff);
	return kp;
}

bool fxBounce(C4Particle *pPrt, C4Section &section, C4Object *pTarget)
{
	// reverse xdir/ydir
	pPrt->xdir = -pPrt->xdir;
	pPrt->ydir = -pPrt->ydir;
	return true;
}

bool fxBounceY(C4Particle *pPrt, C4Section &section, C4Object *pTarget)
{
	// reverse ydir only
	pPrt->ydir = -pPrt->ydir;
	return true;
}

bool fxStop(C4Particle *pPrt, C4Section &section, C4Object *pTarget)
{
	// zero xdir/ydir
	pPrt->xdir = pPrt->ydir = 0;
	return true;
}

bool fxDie(C4Particle *pPrt, C4Section &section, C4Object *pTarget)
{
	// DIEEEEEE
	return false;
}

void fxStdDraw(C4Particle *pPrt, C4FacetEx &cgo, C4Object *pTarget)
{
	// get def
	C4ParticleDef *pDef = pPrt->pDef;
	// apply parallaxity to target pos
	int32_t tx = cgo.TargetX * pDef->Parallaxity[0] / 100;
	int32_t ty = cgo.TargetY * pDef->Parallaxity[1] / 100;

	float dx = pPrt->x, dy = pPrt->y;
	float dxdir = pPrt->xdir, dydir = pPrt->ydir;
	// relative position & movement
	if (pPrt->pDef->Attach && pTarget != nullptr)
	{
		dx += pTarget->x;
		dy += pTarget->y;
		dxdir += fixtof(pTarget->xdir);
		dydir += fixtof(pTarget->ydir);
	}

	// check if it's in screen range
	if (!Inside(dx, tx - pPrt->a, tx + cgo.Wdt + pPrt->a)) return;
	if (!Inside(dy, ty - pPrt->a, ty + cgo.Hgt + pPrt->a)) return;
	// get pos
	int32_t cgox = cgo.X - tx, cgoy = cgo.Y - ty;
	int32_t cx = int32_t(dx + cgox);
	int32_t cy = int32_t(dy + cgoy);
	// get phase
	int32_t iPhase = pPrt->life;
	if (pDef->Delay)
		if (iPhase >= 0)
		{
			iPhase /= pDef->Delay;
			int32_t length = pDef->Length;
			if (pDef->Reverse)
			{
				--length; iPhase %= length * 2;
				if (iPhase > length) iPhase = length * 2 + 1 - iPhase;
			}
			else iPhase %= length;
		}
		else iPhase = (iPhase + 1) / -pDef->FadeOutDelay + pDef->Length;
		// get rotation
		int32_t r = 0;
		if ((pDef->RByV == 1) || (pDef->RByV == 2)) // rotation by direction
			r = Angle(0, 0, static_cast<int32_t>(dxdir * 10.0f), static_cast<int32_t>(dydir * 10.0f)) * 100;
		if (pDef->RByV == 3) // random rotation - currently a pseudo random rotation by x/y position
			r = (static_cast<int32_t>(pPrt->x * 23 + pPrt->y * 12) % 360) * 100;
		// draw at pos
		Application.DDraw->ActivateBlitModulation(pPrt->b);
		Application.DDraw->StorePrimaryClipper();
		Application.DDraw->SubPrimaryClipper(cgox, cgoy + pDef->YOff, 100000, 100000);
		if (pDef->Additive) lpDDraw->SetBlitMode(C4GFXBLIT_ADDITIVE);
		int32_t iDrawWdt = int32_t(pPrt->a);
		int32_t iDrawHgt = int32_t(pDef->Aspect * iDrawWdt);
		if (r)
			pDef->Gfx.DrawXR(cgo.Surface, cx - iDrawWdt, cy - iDrawHgt, iDrawWdt * 2, iDrawHgt * 2, iPhase, 0, r);
		else
			pDef->Gfx.DrawX(cgo.Surface, cx - iDrawWdt, cy - iDrawHgt, iDrawWdt * 2, iDrawHgt * 2, iPhase, 0);
		Application.DDraw->ResetBlitMode();
		Application.DDraw->RestorePrimaryClipper();
		Application.DDraw->DeactivateBlitModulation();
}

C4ParticleProcRec C4ParticleProcMap[] =
{
	{ "SmokeInit", fxSmokeInit },
	{ "SmokeExec", fxSmokeExec },
	{ "StdInit",   fxStdInit },
	{ "StdExec",   fxStdExec },
	{ "Bounce",    fxBounce },
	{ "BounceY",   fxBounceY },
	{ "Stop",      fxStop },
	{ "Die",       fxDie },
	{ "",          nullptr }
};

C4ParticleDrawProcRec C4ParticleDrawProcMap[] =
{
	{ "Smoke", fxSmokeDraw },
	{ "Std",   fxStdDraw },
	{ "",      nullptr }
};
