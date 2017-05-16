/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* A handy wrapper class to gzio files */

#ifndef INC_CSTDFILE
#define INC_CSTDFILE

#include <stdio.h>
#include <StdFile.h>
#include <zlib.h> // for gzFile

const int CStdFileBufSize = 4096;

class CStdStream
	{
	public:
		virtual bool Read(void *pBuffer, size_t iSize) = 0;
		virtual bool Advance(int iOffset) = 0;
		// Get size. compatible with c4group!
		virtual int AccessedEntrySize() = 0;
		virtual ~CStdStream() {}
	};

class CStdFile: public CStdStream
	{
	public:
		CStdFile();
		~CStdFile();
		bool Status;
		char Name[_MAX_PATH+1];
	protected:    
		FILE *hFile; 
		gzFile hgzFile;
		BYTE Buffer[CStdFileBufSize];
		int BufferLoad,BufferPtr;
		BOOL ModeWrite;
	public:   
		bool Create(const char *szFileName, bool fCompressed=false, bool fExecutable=false);
		bool Open(const char *szFileName, bool fCompressed=false);
		bool Append(const char *szFilename); // append (uncompressed only)
		bool Close();
		bool Default();
		bool Read(void *pBuffer, size_t iSize) { return Read(pBuffer, iSize, 0); }
		bool Read(void *pBuffer, size_t iSize, size_t *ipFSize);
		bool Write(const void *pBuffer, int iSize);
		bool WriteString(const char *szStr);
		bool Rewind();
		bool Advance(int iOffset);
		// Single line commands
		bool Load(const char *szFileName, BYTE **lpbpBuf,
		          int *ipSize=NULL, int iAppendZeros=0, 
		          bool fCompressed = false);
		bool Save(const char *szFileName, const BYTE *bpBuf,
		          int iSize, 
		          bool fCompressed = false);
		// flush contents to disk
		inline bool Flush() { if (ModeWrite && BufferLoad) return SaveBuffer(); else return TRUE; }
		int AccessedEntrySize();
	protected:
		void ClearBuffer();
		int LoadBuffer();
		bool SaveBuffer();
	};

int UncompressedFileSize(const char *szFileName);

#endif // INC_CSTDFILE
