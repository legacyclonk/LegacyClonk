/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
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

// statistics
//  by peter

#include <C4Include.h>
#include <C4Stat.h>

#include <C4Game.h>

// ** implemetation of C4MainStat

C4MainStat::C4MainStat()
	: bStatFileOpen(false), pFirst(nullptr) {}

C4MainStat::~C4MainStat()
{
	CloseStatFile();
}

void C4MainStat::RegisterStat(C4Stat *pStat)
{
	// add to list
	if (!pFirst)
	{
		pFirst = pStat;
		pStat->pNext = nullptr;
		pStat->pPrev = nullptr;
	}
	else
	{
		pStat->pNext = pFirst;
		pFirst->pPrev = pStat;
		pStat->pPrev = nullptr;
		pFirst = pStat;
	}
}

void C4MainStat::UnRegStat(C4Stat *pStat)
{
	// first item?
	if (!pStat->pPrev)
	{
		pFirst = pStat->pNext;
		pStat->pNext = nullptr;
	}
	// last item?
	else if (!pStat->pNext)
	{
		pStat->pPrev->pNext = nullptr;
		pStat->pPrev = nullptr;
	}
	else
	{
		pStat->pNext->pPrev = pStat->pPrev;
		pStat->pPrev->pNext = pStat->pNext;
		pStat->pNext = nullptr;
		pStat->pPrev = nullptr;
	}
}

void C4MainStat::Reset()
{
	CloseStatFile();
	for (C4Stat *pAkt = pFirst; pAkt; pAkt = pAkt->pNext)
		pAkt->Reset();
}

void C4MainStat::ResetPart()
{
	for (C4Stat *pAkt = pFirst; pAkt; pAkt = pAkt->pNext)
		pAkt->ResetPart();
}

void C4MainStat::Show()
{
	// output the whole statistic (to stat.txt)

	// open file
	if (!bStatFileOpen)
		OpenStatFile();

	// count stats
	unsigned int iCnt = 0;
	C4Stat *pAkt;
	for (pAkt = pFirst; pAkt; pAkt = pAkt->pNext)
		iCnt++;

	// create array
	C4Stat **StatArray = new C4Stat*[iCnt];
	bool *bHS = new bool[iCnt];

	// sort it
	unsigned int i, ii;
	for (ii = 0; ii < iCnt; ii++) bHS[ii] = false;
	for (i = 0; i < iCnt; i++)
	{
		C4Stat *pBestStat;
		unsigned int iBestNr = ~0u;

		for (ii = 0, pAkt = pFirst; ii < iCnt; ii++, pAkt = pAkt->pNext)
			if (!bHS[ii])
				if (iBestNr == ~0u)
				{
					iBestNr = ii;
					pBestStat = pAkt;
				}
				else if (stricmp(pBestStat->strName, pAkt->strName) > 0)
				{
					iBestNr = ii;
					pBestStat = pAkt;
				}

		if (iBestNr == ~0u)
			break;
		bHS[iBestNr] = true;

		StatArray[i] = pBestStat;
	}

	delete bHS;

	fprintf(StatFile, "** Stat\n");

	// output in order
	for (i = 0; i < iCnt; i++)
	{
		pAkt = StatArray[i];

		// output it!
		if (pAkt->iCount)
			fprintf(StatFile, "%s: n = %d, t = %d, td = %.2f\n",
				pAkt->strName, pAkt->iCount, pAkt->iTimeSum,
				double(pAkt->iTimeSum) / std::max<int>(1, pAkt->iCount - 100) * 1000);
	}

	// delete...
	delete[] StatArray;

	// ok. job done
	fputs("** Stat end\n", StatFile);
	fflush(StatFile);
}

void C4MainStat::ShowPart()
{
	C4Stat *pAkt;

	// open file
	if (!bStatFileOpen)
		OpenStatFile();

	// insert tick nr
	fprintf(StatFile, "** PartStat begin %d\n", Game.FrameCounter);

	// insert all stats
	for (pAkt = pFirst; pAkt; pAkt = pAkt->pNext)
		fprintf(StatFile, "%s: n=%d, t=%d\n", pAkt->strName, pAkt->iCountPart, pAkt->iTimeSumPart);

	// insert part stat end idtf
	fprintf(StatFile, "** PartStat end\n");
	fflush(StatFile);
}

// stat file handling
void C4MainStat::OpenStatFile()
{
	if (bStatFileOpen) return;

	// open & reset file
	StatFile = fopen("stat.txt", "w");

	// success?
	if (!StatFile)
		return;

	bStatFileOpen = true;
}

void C4MainStat::CloseStatFile()
{
	if (!bStatFileOpen) return;

	// open & reset file
	fclose(StatFile);

	bStatFileOpen = false;
}

// ** implemetation of C4Stat

C4Stat::C4Stat(const char *strnName)
	: strName(strnName)
{
	Reset();
	getMainStat()->RegisterStat(this);
}

C4Stat::~C4Stat()
{
	getMainStat()->UnRegStat(this);
}

void C4Stat::Reset()
{
	iStartCalled = 0;

	iTimeSum = 0;
	iCount = 0;

	ResetPart();
}

void C4Stat::ResetPart()
{
	iTimeSumPart = 0;
	iCountPart = 0;
}

C4MainStat *C4Stat::getMainStat()
{
	static C4MainStat *pMainStat = new C4MainStat();
	return pMainStat;
}
