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

#pragma once

class C4Stat;

// *** main statistic class
// should only been constructed once per application
class C4MainStat
{
	friend class C4Stat;

public:
	C4MainStat();
	~C4MainStat();

	void Show();
	void ShowPart();

	void Reset();
	void ResetPart();

	void OpenStatFile();
	void CloseStatFile();

	FILE *StatFile;
	bool bStatFileOpen;

protected:
	C4Stat *pFirst;

	void RegisterStat(C4Stat *pStat);
	void UnRegStat(C4Stat *pStat);
};

// *** one statistic.
// Holds the data about one "checkpoint" in code
// registers himself to C4MainStat when Start() is called the first time
class C4Stat
{
	friend class C4MainStat;

public:
	C4Stat(const char *strName);
	~C4Stat();

	inline void Start()
	{
		if (!iStartCalled)
			iStartTick = timeGetTime();
		iCount++;
		iCountPart++;
		iStartCalled++;
	}

	inline void Stop()
	{
		assert(iStartCalled);
		iStartCalled--;
		if (!iStartCalled && iCount >= 100)
		{
			unsigned int iTime = timeGetTime() - iStartTick;

			iTimeSum += iTime;
			iTimeSumPart += iTime;
		}
	}

	void Reset();
	void ResetPart();

	static C4MainStat *getMainStat();

protected:
	// used by C4MainStat
	C4Stat *pNext;
	C4Stat *pPrev;

	// start tick
	unsigned int iStartTick;

	// start-call depth
	unsigned int iStartCalled;

	// ** statistic data

	// sum of times
	unsigned int iTimeSum;

	// number of starts called
	unsigned int iCount;

	// ** statistic data (partial stat)

	// sum of times
	unsigned int iTimeSumPart;

	// number of starts called
	unsigned int iCountPart;

	// name of statistic
	const char *strName;
};

// *** some directives
#ifdef USE_STAT

// used to create and start a new C4Stat object
#define C4ST_STARTNEW(StatName, strName) static C4Stat StatName(strName); StatName.Start();

// used to create a new C4Stat object
#define C4ST_NEW(StatName, strName) C4Stat StatName(strName);

// used to start an existing C4Stat object
#define C4ST_START(StatName) StatName.Start();

// used to stop an existing C4Stat object
#define C4ST_STOP(StatName) StatName.Stop();

// shows the statistic (to log)
#define C4ST_SHOWSTAT C4Stat::getMainStat()->Show();

// shows the statistic (to log)
#define C4ST_SHOWPARTSTAT C4Stat::getMainStat()->ShowPart();

// resets the whole statistic
#define C4ST_RESET C4Stat::getMainStat()->Reset();

// resets the partial statistic
#define C4ST_RESETPART C4Stat::getMainStat()->ResetPart();

#else

#define C4ST_STARTNEW(StatName, strName)
#define C4ST_NEW(StatName, strName)
#define C4ST_START(StatName)
#define C4ST_STOP(StatName)
#define C4ST_SHOWSTAT
#define C4ST_SHOWPARTSTAT
#define C4ST_RESET
#define C4ST_RESETPART

#endif
