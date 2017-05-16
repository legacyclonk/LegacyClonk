/* by Sven2, 2001 */
// user-customizable multimedia package Extra.c4g

#ifndef INC_C4Extra
#define INC_C4Extra

#include <C4Group.h>

class C4Extra
	{
	public:
		C4Extra() { Default(); };			// ctor
		~C4Extra() { Clear(); };			// dtor
		void Default(); // zero fields
		void Clear();		// free class members

		bool Init();		  // init extra group, using scneario presets
		bool InitGroup(); // open extra group

		C4Group ExtraGrp; // extra.c4g root folder

	protected:
		bool LoadDef(C4Group &hGroup, const char *szName); // load preset for definition
	};

#endif
