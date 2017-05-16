/* by Sven2, 2001 */
// a buffer holding a log history

#ifndef INC_C4LogBuf
#define INC_C4LogBuf

// circular log buffer to holding line-wise log data
class C4LogBuffer
{
private:
	struct LineData
	{
		CStdFont *pFont; // line font
		DWORD dwClr; // line clr
		bool fNewParagraph; // if set, this line marks a new paragraph (and is not the wrapped line of a previous par)
	};

	char *szBuf; // string buffer
	LineData *pLineDataBuf; // line data buffer
	int iBufSize; // size of string buffer
	int iFirstLinePos, iAfterLastLinePos; // current string buffer positions
	int iLineDataPos, iNextLineDataPos; // current line data buffer positions
	int iMaxLineCount; // max number of valid lines - size of line data buffer
	int iLineCount; // number of valid lines in buffer
	int iLineBreakWidth; // line breaking width
	char *szIndent; // chars inserted as indent space
	bool fDynamicGrow; // if true, lines are always added to the buffer. If false, the buffer is used circular and old lines removed
	bool fMarkup; // if set, '|' is treated as linebreak

	void GrowLineCountBuffer(size_t iGrowBy);
	void GrowTextBuffer(size_t iGrowBy);
	void DiscardFirstLine(); // discard oldest line in buffer
	void AppendSingleLine(const char *szLine, int iLineLength, const char *szIndent, CStdFont *pFont, DWORD dwClr, bool fNewParagraph); // append given string as single line

public:
	C4LogBuffer(int iSize, int iMaxLines, int iLBWidth, const char *szIndentChars = "    ", bool fDynamicGrow = false, bool fMarkup = true);
	~C4LogBuffer();

	void AppendLines(const char *szLine, CStdFont *pFont, DWORD dwClr, CStdFont *pFirstLineFont = NULL); // append message line to buffer; overwriting old lines if necessary
	const char *GetLine(int iLineIndex, CStdFont **ppFont, DWORD *pdwClr, bool *pNewParagraph) const; // get indexed line - negative indices -n return last-n'th-line
	void Clear(); // clear all lines

	int GetCount() const { return iLineCount; } // retrieve number of valid lines in buffer
	void SetLBWidth(int iToWidth);
};

#endif // C4LogBuf
