/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* A primitive list to store one amount value per mapped material */

#ifndef INC_C4MaterialList
#define INC_C4MaterialList

#include <C4Landscape.h>

class C4MaterialList  
	{
	public:
		C4MaterialList();
		~C4MaterialList();
	public:
		int32_t Amount[C4MaxMaterial];
	public:
		void Default();
		void Clear();
		void Reset();
		void Add(int32_t iMaterial, int32_t iAmount);
	};

#endif // INC_C4MaterialList
