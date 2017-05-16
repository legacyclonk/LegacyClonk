

const int C4MsgBox_Ok = 0,
					C4MsgBox_OkCancel = 1,
					C4MsgBox_YesNo = 2,
					C4MsgBox_Retry = 3;

int C4MessageBox(const char *szText, int iType = C4MsgBox_Ok, const char *szCheckPrompt = NULL, BOOL *fpCheck = NULL);

template <class CompT, class StructT>
	bool CompileFromBuf_Log(StructT &TargetStruct, const typename CompT::InT &SrcBuf, const char *szName)
	{
		try
		{
			CompileFromBuf<CompT>(TargetStruct, SrcBuf);
			return TRUE;
		}
		catch(StdCompiler::Exception *pExc)
		{
			//C4MessageBox(FormatString("ERROR: %s: %s", szName, pExc->Msg.getData()));
			delete pExc;
			return FALSE;
		}
	}

#define CompileFromBuf_LogWarn CompileFromBuf_Log

template <class CompT, class StructT>
	bool DecompileToBuf_Log(StructT &TargetStruct, typename CompT::OutT *pOut, const char *szName)
	{
		if(!pOut) return false;
		try
		{
			pOut->Take(DecompileToBuf<CompT>(TargetStruct));
			return TRUE;
		}
		catch(StdCompiler::Exception *pExc)
		{
			//C4MessageBox(FormatString("ERROR: %s: %s", szName, pExc->Msg.getData()));
			delete pExc;
			return FALSE;
		}
	}