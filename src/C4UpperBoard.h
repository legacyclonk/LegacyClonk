#ifndef INC_C4UpperBoard
#define INC_C4UpperBoard

#include <C4Facet.h>

class C4UpperBoard
{
	friend class C4GraphicsSystem;

public:
	C4UpperBoard();
	~C4UpperBoard();
	void Default();
	void Clear();
	void Init(C4Facet &cgo);
	void Execute();

protected:
	void Draw(C4Facet &cgo);
	C4Facet Output;
	char cTimeString[64];
	char cTimeString2[64];
	int TextWidth;
	int TextYPosition;
};

#endif
