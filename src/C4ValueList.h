
// Stellt eine einfache dynamische Integer-Liste bereit.

#ifndef INC_C4ValueList
#define INC_C4ValueList

#include "C4Value.h"

class C4ValueList
{
public:
	enum { MaxSize = 1000000, }; // ye shalt not create arrays larger than that!

	C4ValueList();
	C4ValueList(int32_t inSize);
	C4ValueList(const C4ValueList &ValueList2);
	~C4ValueList();

	C4ValueList &operator =(const C4ValueList& ValueList2);

protected:
	int32_t iSize;
	C4Value* pData;
	
public:
	int32_t GetSize() const { return iSize; }

	void Sort(class C4SortObject &rSort);

	const C4Value &GetItem(int32_t iElem) const { return Inside<int32_t>(iElem, 0, iSize-1) ? pData[iElem] : C4VNull; }
	C4Value &GetItem(int32_t iElem);
	
	C4Value operator[](int32_t iElem) const { return GetItem(iElem); }
	C4Value &operator[](int32_t iElem)	 { return GetItem(iElem); }

	void Reset();
	void SetSize(int32_t inSize); // (enlarge only!)

	void DenumeratePointers();

	// comparison
	bool operator==(const C4ValueList& IntList2) const;

	// Compilation
	void CompileFunc(class StdCompiler *pComp);
};

// value list with reference count, used for arrays
class C4ValueArray : public C4ValueList
{
public:
	C4ValueArray();
	C4ValueArray(int32_t inSize);

	~C4ValueArray();
		
	// Add Reference, return self or new copy if necessary
	C4ValueArray * IncRef();
	C4ValueArray * IncElementRef();
	// Change length, return self or new copy if necessary
	C4ValueArray * SetLength(int32_t size);
	void DecRef();
	void DecElementRef();

private:
	// Only for IncRef/AddElementRef
	C4ValueArray(const C4ValueArray &Array2);
	// Reference counter
	unsigned int iRefCnt;
	unsigned int iElementReferences;
};

#endif

