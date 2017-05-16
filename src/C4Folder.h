/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Core component of a folder */

#pragma once

const int C4MaxFolderSort = 4096;

class C4FolderHead
{
public:
	int32_t Index; // Folder index in scenario selection dialog
	char Sort[C4MaxFolderSort + 1]; // Folder-defined group sort list (to be used for folder maps)

public:
	void Default();
	void CompileFunc(StdCompiler *pComp);
};

class C4Folder
{
public:
	C4Folder();

public:
	C4FolderHead Head;

public:
	void Default();
	void CompileFunc(StdCompiler *pComp);
};
