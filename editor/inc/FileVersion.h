/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Loads a version info resource from executable files */

class CFileVersion
	{ 
	public: 
		CFileVersion(LPCTSTR ModuleName = NULL);
		~CFileVersion(); 
		BOOL    Open(LPCTSTR ModuleName);
		void    Close();
		CString QueryValue(LPCTSTR ValueName, DWORD LangCharset = 0);
		CString GetFileDescription()  {return QueryValue(_T("FileDescription")); };
		CString GetFileVersion()      {return QueryValue(_T("FileVersion"));     };
		CString GetInternalName()     {return QueryValue(_T("InternalName"));    };
		CString GetCompanyName()      {return QueryValue(_T("CompanyName"));     }; 
		CString GetLegalCopyright()   {return QueryValue(_T("LegalCopyright"));  };
		CString GetOriginalFilename() {return QueryValue(_T("OriginalFilename"));};
		CString GetProductName()      {return QueryValue(_T("ProductName"));     };
		CString GetProductVersion()   {return QueryValue(_T("ProductVersion"));  };
		BOOL    GetFixedInfo(VS_FIXEDFILEINFO& vsffi);
		CString GetFixedFileVersion();
		CString GetFixedProductVersion();
	protected:
		LPBYTE  pVersionData; 
		DWORD   LangCharset; 
	};
