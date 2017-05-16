/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Rank list for players or crew members */

#ifndef INC_C4RankSystem
#define INC_C4RankSystem

class C4RankSystem
  {
  public:
    C4RankSystem();
		~C4RankSystem() { Clear(); }

		enum { EXP_NoPromotion = -1 }; // experience value for NextRankExp: No more promotion possible

  protected:
		char Register[256+1];
		char RankName[C4MaxName+1];
		int RankBase;
		char **pszRankNames;			// loaded rank names for non-registry ranks
		char *szRankNames;				// loaded rank-name buffer
		int iRankNum;							// number of ranks for loaded rank-names
		char **pszRankExtensions; // rank extensions (e.g. "%s First class") for even more ranks!
		int iRankExtNum;          // number of rank extensions
  public:    
	  void Default();
	  void Clear();
    int Init(const char *szRegister, const char *szDefRanks, int iRankBase);
		bool Load(C4Group &hGroup, const char *szFilenames, int DefRankBase, const char *szLanguage); // init based on nk file in group
    int Experience(int iRank);
		int RankByExperience(int iExp);  // get rank by experience
    StdStrBuf GetRankName(int iRank, bool fReturnLastIfOver);
		BOOL Check(int iRank, const char  *szDefRankName);
		int32_t GetExtendedRankNum() const { return iRankExtNum; }
		//void Reset(const char *szDefRanks);
#ifdef C4ENGINE
		static bool DrawRankSymbol(C4FacetExSurface *fctSymbol, int32_t iRank, C4FacetEx *pfctRankSymbols, int32_t iRankSymbolCount, bool fOwnSurface, int32_t iXOff=0, C4Facet *cgoDrawDirect=NULL); // create facet from rank symbol for definition - use custom rank facets if present
#endif
  };

#endif
