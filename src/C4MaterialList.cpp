/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* A primitive list to store one amount value per mapped material */

#include <C4Include.h>
#include <C4MaterialList.h>

C4MaterialList::C4MaterialList()
	{
	Default();
	}

C4MaterialList::~C4MaterialList()
	{
	Clear();
	}

void C4MaterialList::Default()
	{
	Reset();
	}

void C4MaterialList::Clear()
	{

	}

void C4MaterialList::Reset()
	{
	for (int cnt=0; cnt<C4MaxMaterial; cnt++)
		Amount[cnt]=0;
	}

void C4MaterialList::Add(int32_t iMaterial, int32_t iAmount)
	{
	if (!Inside<int32_t>(iMaterial,0,C4MaxMaterial)) return;
	Amount[iMaterial]+=iAmount;
	}
