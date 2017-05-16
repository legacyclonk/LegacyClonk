/* by Sven2, 2001 */
// an associated list
// used by C4AulScriptEngine, as virtual function table and include/dependency list

#ifndef INC_C4AList
#define INC_C4AList

#define C4AListChunkSize 128 // number of table entries for each chunk

// table entry
struct C4AListEntry
	{
	void *Var, *Val;
	void *get(void *pVar); // get value of var; start parsing here
	C4AListEntry *next(); // get entry after the given one
	C4AListEntry *nextVal(); // get next entry with var holding a value
	C4AListEntry *firstVal(); // get first entry with var holding a value
	};

// bunch of table entries
struct C4AListChunk
	{
	C4AListEntry Entries[C4AListChunkSize]; // table entries
	void *Stop; // stop entry; should always be NULL
	C4AListChunk *Next; // next chunk
	};

// associative list
class C4AList
	{
	protected:
		C4AListChunk *Table, *CurrC; // first/last table chunk
		int CCount; // number of entries in current chunk
		C4AListEntry *Curr; // next available entry to be used
		void Grow(); // append chunk

	public:
		C4AList(); // constructor
		~C4AList(); // destructor
		void Clear(); // clear the list

		C4AListEntry *push(void *pVar = NULL, void *pVal = NULL); // push var/value pair to end of list
		void *get(void *pVar, C4AListEntry *pOff = NULL); // get value of var
	};

#endif
