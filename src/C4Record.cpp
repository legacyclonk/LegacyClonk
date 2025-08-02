/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2001, Sven2
 * Copyright (c) 2017-2021, The LegacyClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */

// scenario record functionality

#include <C4Include.h>
#include <C4Record.h>

#include <C4Console.h>
#include <C4PlayerInfo.h>
#include <C4GameSave.h>
#include <C4Log.h>
#include <C4Wrappers.h>
#include <C4Player.h>

#include <StdFile.h>

#include <format>

#define IMMEDIATEREC

//#define DEBUGREC_EXTFILE "DbgRec.c4b" // if defined, an external file is used for debugrec writing (replays only)
#define DEBUGREC_EXTFILE_WRITE        // if defined, the external file is used for debugrec writing. Otherwise read/check

#ifdef DEBUGREC

#ifdef DEBUGREC_EXTFILE
C4File DbgRecFile;
#endif
int DoNoDebugRec = 0; // debugrec disable counter

void AddDbgRec(C4RecordChunkType eType, const void *pData, int iSize)
{
	Game.Control.DbgRec(eType, static_cast<const uint8_t *>(pData), iSize);
}

#endif

C4DebugRecOff::C4DebugRecOff() : fDoOff(true)
{
	DEBUGREC_OFF;
}

C4DebugRecOff::C4DebugRecOff(bool fDoOff) : fDoOff(fDoOff)
{
	if (fDoOff) { DEBUGREC_OFF; }
}

C4DebugRecOff::~C4DebugRecOff()
{
	if (fDoOff) { DEBUGREC_ON; }
}

void C4DebugRecOff::Clear()
{
	DEBUGREC_ON;
	fDoOff = false;
}

void C4PktDebugRec::CompileFunc(StdCompiler *pComp)
{
	// type
	pComp->Value(mkNamingAdapt(mkIntAdapt(eType), "Type"));
	// data
	pComp->Value(mkNamingAdapt(Data,              "Data"));
}

C4RecordChunk::C4RecordChunk()
	: pCtrl(nullptr) {}

void C4RecordChunk::Delete()
{
	switch (Type)
	{
	case RCT_Ctrl:    delete pCtrl;     pCtrl = nullptr; break;
	case RCT_CtrlPkt: delete pPkt;      pPkt  = nullptr; break;
	case RCT_End:                                        break;
	case RCT_Frame:                                      break;
	case RCT_File:    delete pFileData;                  break;
	default:          delete pDbg;      pDbg  = nullptr; break;
	}
}

void C4RecordChunk::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(Frame,            "Frame"));
	pComp->Value(mkNamingAdapt(mkIntAdapt(Type), "Type"));
	switch (Type)
	{
	case RCT_Ctrl: pComp->Value(mkPtrAdaptNoNull(pCtrl)); break;
	case RCT_CtrlPkt: pComp->Value(mkPtrAdaptNoNull(pPkt)); break;
	case RCT_End: break;
	case RCT_Frame: break;
	case RCT_File: pComp->Value(Filename); pComp->Value(mkPtrAdaptNoNull(pFileData)); break;
	default: pComp->Value(mkPtrAdaptNoNull(pDbg)); break;
	}
}

C4Record::C4Record()
	: fRecording(false), fStreaming(false) {}

C4Record::~C4Record() {}

bool C4Record::Start(bool fInitial)
{
	// no double record
	if (fRecording) return false;

	// create demos folder
	if (!Config.General.CreateSaveFolder(Config.General.SaveDemoFolder.getData(), LoadResStr(C4ResStrTableKey::IDS_GAME_RECORDSTITLE)))
		return false;

	// various infos
	StdStrBuf sDemoFolder; sDemoFolder.Ref(Config.General.SaveDemoFolder);
	char sScenName[_MAX_FNAME + 1]; SCopy(GetFilenameOnly(Game.Parameters.Scenario.getFile()), sScenName, _MAX_FNAME);

	// remove trailing numbers from scenario name (e.g. from savegames) - could we perhaps use C4S.Head.Origin instead...?
	char *pScenNameEnd = sScenName + SLen(sScenName);
	while (Inside<char>(*--pScenNameEnd, '0', '9'))
		if (pScenNameEnd == sScenName)
			break;
	pScenNameEnd[1] = 0;

	// determine index (by total number of records)
	Index = 1;
	for (DirectoryIterator i(Config.General.SaveDemoFolder.getData()); *i; ++i)
		if (WildcardMatch(C4CFN_ScenarioFiles, *i))
			Index++;

	// compose record filename
	sFilename.Copy(std::format("{}" DirSep "{:03}-{}.c4s", sDemoFolder.getData(), Index, +sScenName).c_str());

	// log
	std::string log{LoadResStr(C4ResStrTableKey::IDS_PRC_RECORDINGTO, sFilename.getData())};
	if (Game.FrameCounter) log += std::format(" (Frame {})", Game.FrameCounter);
	LogNTr(log);

	// save game - this also saves player info list
	C4GameSaveRecord saveRec(fInitial, Index, Game.Parameters.isLeague());
	if (!saveRec.Save(sFilename.getData())) return false;
	saveRec.Close();

	// unpack group, if neccessary
	if (!DirectoryExists(sFilename.getData()) &&
		!C4Group_UnpackDirectory(sFilename.getData()))
		return false;

	// open control record file
	const std::string ctrlRecFilename{std::format("{}" DirSep C4CFN_CtrlRec, sFilename.getData())};
	if (!CtrlRec.Open(ctrlRecFilename, "wb")) return false;

	// open record group
	if (!RecordGrp.Open(sFilename.getData()))
		return false;

	// record go
	fStreaming = false;
	fRecording = true;
	iLastFrame = 0;
	return true;
}

bool C4Record::Stop(StdStrBuf *pRecordName, uint8_t *pRecordSHA1)
{
	// safety
	if (!fRecording) return false;
	if (!DirectoryExists(sFilename.getData())) return false;

	// streaming finished
	StopStreaming();

	// save desc into record group
	C4GameSaveRecord saveRec(false, Index, Game.Parameters.isLeague());
	saveRec.SaveDesc(RecordGrp);

	// save end player infos into record group
	Game.PlayerInfos.Save(RecordGrp, C4CFN_RecPlayerInfos);
	RecordGrp.Close();

	// write last entry and close
	C4RecordChunkHead Head;
	Head.iFrm = Game.FrameCounter + 37;
	Head.Type = RCT_End;
	CtrlRec.WriteElement(Head);
	CtrlRec.Close();

	// pack group
#ifndef DEBUGREC
	if (!C4Group_PackDirectory(sFilename.getData())) return false;
#endif

	// return record data
	if (pRecordName)
		pRecordName->Copy(sFilename);
	if (pRecordSHA1)
		if (!C4Group_GetFileSHA1(sFilename.getData(), pRecordSHA1))
			return false;

	// ok
	fRecording = false;
	return true;
}

bool C4Record::Rec(const C4Control &Ctrl, int iFrame)
{
	if (!fRecording) return false;
	// don't record empty control
	if (!Ctrl.firstPkt()) return true;
	// create copy
	C4Control Cpy; Cpy.Copy(Ctrl);
	// prepare it for record
	Cpy.PreRec(this);
	// record it
	return Rec(iFrame, DecompileToBuf<StdCompilerBinWrite>(Cpy), RCT_Ctrl);
}

bool C4Record::Rec(C4PacketType eCtrlType, C4ControlPacket *pCtrl, int iFrame)
{
	if (!fRecording) return false;
	// create copy
	C4IDPacket Pkt = C4IDPacket(eCtrlType, pCtrl, false); if (!Pkt.getPkt()) return false;
	C4ControlPacket *pCtrlCpy = static_cast<C4ControlPacket *>(Pkt.getPkt());
	// prepare for recording
	pCtrlCpy->PreRec(this);
	// record it
	return Rec(iFrame, DecompileToBuf<StdCompilerBinWrite>(Pkt), RCT_CtrlPkt);
}

bool C4Record::Rec(uint32_t iFrame, const StdBuf &sBuf, C4RecordChunkType eType)
{
	// filler chunks (this should never be necessary, though)
	while (iFrame > iLastFrame + 0xff)
		Rec(iLastFrame + 0xff, StdBuf(), RCT_Frame);
	// get frame difference
	const uint32_t iFrameDiff = iLastFrame > iFrame ? 0 : iFrame - iLastFrame;
	iLastFrame += iFrameDiff;
	// create head
	C4RecordChunkHead Head = { static_cast<uint8_t>(iFrameDiff), static_cast<uint8_t>(eType) };
	// pack
	CtrlRec.WriteElement(Head);
	CtrlRec.WriteRaw(sBuf.getData(), sBuf.getSize());
#ifdef IMMEDIATEREC
	// immediate rec: always flush
	CtrlRec.Flush();
#endif
	// Stream
	if (fStreaming)
		Stream(Head, sBuf);
	return true;
}

void C4Record::Stream(const C4RecordChunkHead &Head, const StdBuf &sBuf)
{
	if (!fStreaming) return;
	StreamingData.Append(&Head, sizeof(Head));
	StreamingData.Append(sBuf.getData(), sBuf.getSize());
}

bool C4Record::AddFile(const char *szLocalFilename, const char *szAddAs, bool fDelete)
{
	if (!fRecording) return false;

	// Streaming?
	if (fStreaming)
	{
		// Special stripping for streaming
		StdStrBuf szFile(szLocalFilename);
		if (SEqualNoCase(GetExtension(szAddAs), "c4p"))
		{
			// Create a copy
			MakeTempFilename(&szFile);
			if (!CopyItem(szLocalFilename, szFile.getData()))
				return false;
			// Strip it
			if (!C4Player::Strip(szFile.getData(), true))
				return false;
		}

		// Add to stream
		if (!StreamFile(szFile.getData(), szAddAs))
			return false;

		// Remove temporary file
		if (szFile != szLocalFilename)
			EraseItem(szFile.getData());
	}

	// Add to record group
	if (fDelete)
	{
		if (!RecordGrp.Move(szLocalFilename, szAddAs))
			return false;
	}
	else
	{
		if (!RecordGrp.Add(szLocalFilename, szAddAs))
			return false;
	}

	return true;
}

bool C4Record::StartStreaming(bool fInitial)
{
	if (!fRecording) return false;
	if (fStreaming) return false;

	// Get temporary file name
	StdStrBuf sTempFilename(sFilename);
	MakeTempFilename(&sTempFilename);

	// Save start state (without copy of scenario!)
	C4GameSaveRecord saveRec(fInitial, Index, Game.Parameters.isLeague(), false);
	if (!saveRec.Save(sTempFilename.getData())) return false;
	saveRec.Close();

	// Add file into stream, delete file
	fStreaming = true;
	if (!StreamFile(sTempFilename.getData(), sFilename.getData()))
	{
		fStreaming = false;
		return false;
	}

	// Okay
	EraseFile(sTempFilename.getData());
	iStreamingPos = 0;
	return true;
}

void C4Record::ClearStreamingBuf(unsigned int iAmount)
{
	iStreamingPos += iAmount;
	if (iAmount == StreamingData.getSize())
		StreamingData.Clear();
	else
	{
		StreamingData.Move(iAmount, StreamingData.getSize() - iAmount);
		StreamingData.SetSize(StreamingData.getSize() - iAmount);
	}
}

void C4Record::StopStreaming()
{
	fStreaming = false;
}

bool C4Record::StreamFile(const char *szLocalFilename, const char *szAddAs)
{
	// Load file into memory
	StdBuf FileData;
	if (!FileData.LoadFromFile(szLocalFilename))
		return false;

	// Prepend name
	StdBuf Packed = DecompileToBuf<StdCompilerBinWrite>(
		mkInsertAdapt(StdStrBuf::MakeRef(szAddAs), FileData, false));

	// Add to stream
	C4RecordChunkHead Head = { 0, RCT_File };
	Stream(Head, Packed);
	return true;
}

// set defaults
C4Playback::C4Playback(std::shared_ptr<spdlog::logger> logger) : logger{std::move(logger)}, Finished(true),fLoadSequential(false)
{
#ifdef DEBUGREC
	loggerDebugRec = logger->clone("DbgRec");
#endif
}

C4Playback::~C4Playback()
{
	Clear();
}

bool C4Playback::Open(C4Group &rGrp)
{
	// clean up
	Clear();
	fLoadSequential = false;
	iLastSequentialFrame = 0;
	bool fStrip = false;
	// get text record file
	StdStrBuf TextBuf;
	if (rGrp.LoadEntryString(C4CFN_CtrlRecText, TextBuf))
	{
		if (!ReadText(TextBuf))
			return false;
	}
	else
	{
		// open group? Then do some sequential reading for large files
		// Can't do this when a dump is forced, because the dump needs all data
		// Also can't do this when stripping is desired
		if (!rGrp.IsPacked()) if (!Game.RecordDumpFile.getLength()) if (!fStrip) fLoadSequential = true;
		// get record file
		if (fLoadSequential)
		{
			if (!rGrp.FindEntry(C4CFN_CtrlRec)) return false;
			if (!playbackFile.Open(std::format("{}" DirSep "{}", rGrp.GetFullName().getData(), C4CFN_CtrlRec), "rb")) return false;
			// forcing first chunk to be read; will call ReadBinary
			currChunk = chunks.end();
			if (!NextSequentialChunk())
			{
				// empty replay??!
				LogFatalNTr("Record: Binary read error.");
				return false;
			}
		}
		else
		{
			// non-sequential reading: Just read as a whole
			StdBuf BinaryBuf;
			if (rGrp.LoadEntry(C4CFN_CtrlRec, BinaryBuf))
			{
				if (!ReadBinary(BinaryBuf))
					return false;
			}
			else
			{
				// no control data?
				LogFatalNTr("Record: No control data found!");
				return false;
			}
		}
	}
	// rewrite record
	if (fStrip) Strip();
	if (Game.RecordDumpFile.getLength())
	{
		if (SEqualNoCase(GetExtension(Game.RecordDumpFile.getData()), "txt"))
		{
			const std::string text{ReWriteText()};
			StdStrBuf{text.c_str(), text.size(), false}.SaveToFile(Game.RecordDumpFile.getData());
		}
		else
		{
			ReWriteBinary().SaveToFile(Game.RecordDumpFile.getData());
		}
	}
	// reset status
	currChunk = chunks.begin();
	Finished = false;
	// external debugrec file
#if defined(DEBUGREC_EXTFILE) && defined(DEBUGREC)
#ifdef DEBUGREC_EXTFILE_WRITE
	if (!DbgRecFile.Open(DEBUGREC_EXTFILE, "wb"))
	{
		LogFatalNTr("DbgRec: Creation of external file \"" DEBUGREC_EXTFILE "\" failed!");
		return false;
	}
	else loggerDebugRec->info("Writing to \"" DEBUGREC_EXTFILE "\"...");
#else
	if (!DbgRecFile.Open(DEBUGREC_EXTFILE, "rb"))
	{
		LogFatalNTr("DbgRec: Opening of external file \"" DEBUGREC_EXTFILE "\" failed!");
		return false;
	}
	else loggerDebugRec->info("Checking against \"" DEBUGREC_EXTFILE "\"...");
#endif
#endif
	// ok
	return true;
}

bool C4Playback::ReadBinary(const StdBuf &Buf)
{
	// sequential reading: Take over rest from last buffer
	const StdBuf *pUseBuf; uint32_t iFrame = 0;
	if (fLoadSequential)
	{
		sequentialBuffer.Append(Buf);
		pUseBuf = &sequentialBuffer;
		iFrame = iLastSequentialFrame;
	}
	else
		pUseBuf = &Buf;
	// get buffer data
	size_t iPos = 0; bool fFinished = false;
	do
	{
		// unpack header
		if (pUseBuf->getSize() - iPos < sizeof(C4RecordChunkHead)) break;
		const C4RecordChunkHead *pHead = pUseBuf->getPtr<C4RecordChunkHead>(iPos);
		// get chunk
		iPos += sizeof(C4RecordChunkHead);
		StdBuf Chunk = pUseBuf->getPart(iPos, pUseBuf->getSize() - iPos);
		// Create entry
		C4RecordChunk c;
		c.Frame = (iFrame += pHead->iFrm);
		c.Type = pHead->Type;
		// Unpack data
		try
		{
			// Initialize compiler
			StdCompilerBinRead Compiler;
			Compiler.setInput(Chunk);
			Compiler.Begin();
			// Read chunk
			switch (pHead->Type)
			{
			case RCT_Ctrl:
				Compiler.Value(mkPtrAdaptNoNull(c.pCtrl));
				break;
			case RCT_CtrlPkt:
				Compiler.Value(mkPtrAdaptNoNull(c.pPkt));
				break;
			case RCT_End:
				fFinished = true;
				break;
			case RCT_File:
				Compiler.Value(c.Filename);
				Compiler.Value(mkPtrAdaptNoNull(c.pFileData));
				break;
			default:
				// debugrec
				if (pHead->Type >= 0x80)
					Compiler.Value(mkPtrAdaptNoNull(c.pDbg));
			}
			// Advance over data
			Compiler.End();
			iPos += Compiler.getPosition();
		}
		catch (const StdCompiler::EOFException &e)
		{
			// This is to be expected for sequential reading
			if (fLoadSequential)
			{
				iPos -= sizeof(C4RecordChunkHead);
				iFrame -= pHead->iFrm;
				break;
			}
			logger->error("Binary unpack error: {}", e.what());
			c.Delete();
			return false;
		}
		catch (const StdCompiler::Exception &e)
		{
			logger->error("Binary unpack error: {}", e.what());
			c.Delete();
			return false;
		}
		// Add to list
		chunks.push_back(c); c.pPkt = nullptr;
	} while (!fFinished);
	// erase everything but the trailing part from sequential buffer
	if (fLoadSequential)
	{
		if (iPos >= sequentialBuffer.getSize())
			sequentialBuffer.Clear();
		else if (iPos)
		{
			sequentialBuffer.Move(iPos, sequentialBuffer.getSize() - iPos);
			sequentialBuffer.Shrink(iPos);
		}
		iLastSequentialFrame = iFrame;
	}
	return true;
}

bool C4Playback::ReadText(const StdStrBuf &Buf)
{
	return CompileFromBuf_LogWarn<StdCompilerINIRead>(mkNamingAdapt(mkSTLContainerAdapt(chunks), "Rec"), Buf, C4CFN_CtrlRecText);
}

void C4Playback::NextChunk()
{
	assert(currChunk != chunks.end());
	++currChunk;
	if (currChunk != chunks.end()) return;
	// end of all chunks if not loading sequential here
	if (!fLoadSequential) return;
	// otherwise, get next few chunks
	for (chunks_t::iterator i = chunks.begin(); i != chunks.end(); i++) i->Delete();
	chunks.clear(); currChunk = chunks.end();
	NextSequentialChunk();
}

bool C4Playback::NextSequentialChunk()
{
	StdBuf BinaryBuf;
	bool success;
	size_t iRealSize;
	BinaryBuf.New(4096);
	// load data until a chunk could be filled
	for (;;)
	{
		std::tie(success, iRealSize) = playbackFile.ReadPartialRaw(BinaryBuf.getMData(), 4096);
		if (!success || !iRealSize) return false;
		BinaryBuf.SetSize(iRealSize);
		if (!ReadBinary(BinaryBuf)) return false;
		// okay, at least one chunk has been read!
		if (chunks.size())
		{
			currChunk = chunks.begin();
			return true;
		}
	}
	// playback file reading failed - looks like we're done
	return false;
}

std::string C4Playback::ReWriteText()
{
	std::string output;
	for (const auto &chunk : chunks)
	{
		output += DecompileToBuf<StdCompilerINIWrite>(mkNamingAdapt(mkDecompileAdapt(chunk), "Rec"));
		output += "\n\n";
	}
	return output;
}

StdBuf C4Playback::ReWriteBinary()
{
	const int OUTPUT_GROW = 16 * 1024;
	StdBuf Output; int iPos = 0;
	bool fFinished = false;
	uint32_t iFrame = 0;
	for (chunks_t::const_iterator i = chunks.begin(); !fFinished && i != chunks.end(); i++)
	{
		// Check frame difference
		if (i->Frame - iFrame < 0 || i->Frame - iFrame > 0xff)
			logger->error("Invalid frame difference between chunks (0-255 allowed)! Data will be invalid!");
		// Pack data
		StdBuf Chunk;
		try
		{
			switch (i->Type)
			{
			case RCT_Ctrl:
				Chunk = DecompileToBuf<StdCompilerBinWrite>(*i->pCtrl);
				break;
			case RCT_CtrlPkt:
				Chunk = DecompileToBuf<StdCompilerBinWrite>(*i->pPkt);
				break;
			case RCT_End:
				fFinished = true;
				break;
			default: // debugrec
				if (i->pDbg)
					Chunk = DecompileToBuf<StdCompilerBinWrite>(*i->pDbg);
				break;
			}
		}
		catch (const StdCompiler::Exception &e)
		{
			logger->error("Binary unpack error: {}", e.what());
			return StdBuf();
		}
		// Grow output
		while (Output.getSize() - iPos < sizeof(C4RecordChunkHead) + Chunk.getSize())
			Output.Grow(OUTPUT_GROW);
		// Write header
		C4RecordChunkHead *pHead = Output.getMPtr<C4RecordChunkHead>(iPos);
		pHead->Type = i->Type;
		pHead->iFrm = i->Frame - iFrame;
		iPos += sizeof(C4RecordChunkHead);
		iFrame = i->Frame;
		// Write chunk
		Output.Write(Chunk, iPos);
		iPos += Chunk.getSize();
	}
	Output.SetSize(iPos);
	return Output;
}

void C4Playback::Strip()
{
	// Strip what?
	const bool fStripPlayers = false;
	const bool fStripSyncChecks = false;
	const bool fStripDebugRec = true;
	const bool fCheckCheat = false;
	const bool fStripMessages = true;
	const int32_t iEndFrame = -1;
	// Iterate over chunk list
	for (chunks_t::iterator i = chunks.begin(); i != chunks.end();)
	{
		// Strip rest of record?
		if (iEndFrame >= 0 && i->Frame > iEndFrame)
		{
			// Remove this and all remaining chunks
			while (i != chunks.end())
			{
				i->Delete();
				i = chunks.erase(i);
			}
			// Push new End-Chunk
			C4RecordChunk EndChunk;
			EndChunk.Frame = iEndFrame;
			EndChunk.Type = RCT_End;
			chunks.push_back(EndChunk);
			// Done
			break;
		}
		switch (i->Type)
		{
		case RCT_Ctrl:
		{
			// Iterate over controls
			C4Control *pCtrl = i->pCtrl;
			for (C4IDPacket *pPkt = pCtrl->firstPkt(), *pNext; pPkt; pPkt = pNext)
			{
				pNext = pCtrl->nextPkt(pPkt);
				switch (pPkt->getPktType())
				{
				// Player join: Strip player file (if possible)
				case CID_JoinPlr:
					if (fStripPlayers)
					{
						C4ControlJoinPlayer *pJoinPlr = static_cast<C4ControlJoinPlayer *>(pPkt->getPkt());
						pJoinPlr->Strip();
					}
					break;
				// EM commands: May be cheats, so log them
				case CID_Script:
				case CID_EMMoveObj:
				case CID_EMDrawTool:
					if (fCheckCheat) LogNTr(DecompileToBuf<StdCompilerINIWrite>(mkNamingAdapt(*pPkt, std::format("Frame {}", i->Frame).c_str())));
					break;
				// Strip sync check
				case CID_SyncCheck:
					if (fStripSyncChecks)
					{
						i->pCtrl->Remove(pPkt);
					}
					break;
				default:
					// don't strip anything else
					break;
				}
			}
			// Strip empty control lists (always)
			if (!pCtrl->firstPkt())
			{
				i->Delete();
				i = chunks.erase(i);
			}
			else
				i++;
		}
		break;
		case RCT_CtrlPkt:
		{
			bool fStripThis = false;
			switch (i->pPkt->getPktType())
			{
			// EM commands: May be cheats, so log them
			case CID_Script:
			case CID_EMMoveObj:
			case CID_EMDrawTool:
				if (fCheckCheat) LogNTr(DecompileToBuf<StdCompilerINIWrite>(mkNamingAdapt(*i->pPkt, std::format("Frame {}", i->Frame).c_str())));
				break;
			// Strip some stuff
			case CID_SyncCheck:
				if (fStripSyncChecks) fStripThis = true;
				break;
			case CID_Message:
				if (fStripMessages) fStripThis = true;
				break;
			default:
				// don't strip anything else
				break;
			}
			if (fStripThis)
			{
				i->Delete();
				i = chunks.erase(i);
			}
			else i++;
		}
		break;
		case RCT_End:
			i++;
			break;
		default:
			// Strip debugrec
			if (fStripDebugRec)
			{
				i->Delete();
				i = chunks.erase(i);
			}
			else
				i++;
		}
	}
}

bool C4Playback::ExecuteControl(C4Control *pCtrl, int iFrame)
{
	// still playbacking?
	if (currChunk == chunks.end()) return false;
	if (Finished) { Finish(); return false; }
#ifdef DEBUGREC
	if (DebugRec.firstPkt())
		DebugRecError("Debug rec overflow!");
	DebugRec.Clear();
#endif
	// return all control until this frame
	while (currChunk != chunks.end() && currChunk->Frame <= iFrame)
	{
		switch (currChunk->Type)
		{
		case RCT_Ctrl:
			pCtrl->Append(*currChunk->pCtrl);
			break;

		case RCT_CtrlPkt:
		{
			C4IDPacket Packet(*currChunk->pPkt);
			pCtrl->Add(Packet.getPktType(), static_cast<C4ControlPacket *>(Packet.getPkt()));
			Packet.Default();
			break;
		}

		case RCT_End:
			// end of playback; stop it!
			Finished = true;
			break;

#ifdef DEBUGREC
		default: // expect it to be debug rec
			// append to debug rec buffer
			if (currChunk->pDbg)
			{
				DebugRec.Add(CID_DebugRec, currChunk->pDbg);
				// the debugrec buffer is now responsible for deleting the packet
				currChunk->pDbg = nullptr;
			}
			break;
#endif
		}
		// next chunk
		NextChunk();
	}
	return true;
}

void C4Playback::Finish()
{
	Clear();
	// finished playback: end game
	if (Console.Active)
	{
		++Game.HaltCount;
		Console.UpdateHaltCtrls(!!Game.HaltCount);
	}
	else
	{
		Game.DoGameOver();
	}
	// finish playback: enable controls
	Game.Control.ChangeToLocal();
}

void C4Playback::Clear()
{
	// free stuff
	for (chunks_t::iterator i = chunks.begin(); i != chunks.end(); i++) i->Delete();
	chunks.clear(); currChunk = chunks.end();
	playbackFile.Close();
	sequentialBuffer.Clear();
	fLoadSequential = false;
#ifdef DEBUGREC
	C4IDPacket *pkt;
	while (pkt = DebugRec.firstPkt()) DebugRec.Delete(pkt);
#ifdef DEBUGREC_EXTFILE
	DbgRecFile.Close();
#endif
#endif
	// done
	Finished = true;
}

const char *GetRecordChunkTypeName(C4RecordChunkType eType)
{
	switch (eType)
	{
	case RCT_Ctrl:    return "Ctrl"; // control
	case RCT_CtrlPkt: return "CtrlPkt"; // control packet
	case RCT_Frame:   return "Frame"; // beginning frame
	case RCT_End:     return "End"; // --- the end ---
	case RCT_Log:     return "Log"; // log message
	case RCT_File:    return "File"; // file data
	// DEBUGREC
	case RCT_DbgFrame:   return "DbgFrame";
	case RCT_Block:      return "Block";      // point in Game::Execute
	case RCT_SetPix:     return "SetPix";     // set landscape pixel
	case RCT_ExecObj:    return "ExecObj";    // exec object
	case RCT_Random:     return "Random";     // Random()-call
	case RCT_Rn3:        return "Rn3";        // Rn3()-call
	case RCT_MMC:        return "MMC";        // create MassMover
	case RCT_MMD:        return "MMD";        // destroy MassMover
	case RCT_CrObj:      return "CrObj";      // create object
	case RCT_DsObj:      return "DsObj";      // remove object
	case RCT_GetPix:     return "GetPix";     // get landscape pixel; let the Gigas flow!
	case RCT_RotVtx1:    return "RotVtx1";    // before shape is rotated
	case RCT_RotVtx2:    return "RotVtx2";    // after shape is rotated
	case RCT_ExecPXS:    return "ExecPXS";    // execute pxs system
	case RCT_Sin:        return "Sin";        // sin by Shape-Rotation
	case RCT_Cos:        return "Cos";        // cos by Shape-Rotation
	case RCT_Map:        return "Map";        // map dump
	case RCT_Ls:         return "Ls";         // complete landscape dump!
	case RCT_MCT1:       return "MCT1";       // MapCreatorS2: before transformation
	case RCT_MCT2:       return "MCT2";       // MapCreatorS2: after transformation
	case RCT_AulFunc:    return "AulFunc";    // script function call
	case RCT_ObjCom:     return "ObjCom";     // object com
	case RCT_PlrCom:     return "PlrCom";     // player com
	case RCT_PlrInCom:   return "PlrInCom";   // player InCom
	case RCT_MatScan:    return "MatScan";    // landscape scan execute
	case RCT_MatScanDo:  return "MatScanDo";  // landscape scan mat change
	case RCT_Area:       return "Area";       // object area change
	case RCT_MenuAdd:    return "MenuAdd";    // add menu item
	case RCT_MenuAddC:   return "MenuAddC";   // add menu item: Following commands
	case RCT_OCF:        return "OCF";        // OCF setting of updating
	case RCT_DirectExec: return "DirectExec"; // a DirectExec-script

	case RCT_Custom: return "Custom"; // varies

	case RCT_Undefined: ; // fallthrough
	};
	return "Undefined";
}

std::string GetDbgRecPktData(C4RecordChunkType eType, const StdBuf &RawData)
{
	std::string r;
	switch (eType)
	{
	case RCT_AulFunc: r.assign(reinterpret_cast<const char *>(RawData.getData()), RawData.getSize() - 1);
		break;
	default:
		for (std::size_t i = 0; i < RawData.getSize(); ++i)
			r += std::format("{:02x} ", reinterpret_cast<const uint8_t *>(RawData.getData())[i]);
		break;
	}
	return r;
}

#ifdef DEBUGREC

void C4Playback::Check(C4RecordChunkType eType, const uint8_t *pData, int iSize)
{
	// only if enabled
	if (DoNoDebugRec > 0) return;
	if (Game.FrameCounter < DEBUGREC_START_FRAME) return;

	C4PktDebugRec PktInReplay;
	bool fHasPacketFromHead = false;
#ifdef DEBUGREC_EXTFILE
#ifdef DEBUGREC_EXTFILE_WRITE
	// writing of external debugrec file
	DbgRecFile.WriteElement(eType);
	int32_t iSize32 = iSize;
	DbgRecFile.WriteElement(iSize32);
	DbgRecFile.WriteRaw(pData, iSize);
	return;
#else
	int32_t iSize32 = 0;
	C4RecordChunkType eTypeRec = RCT_Undefined;
	DbgRecFile.ReadElement(eTypeRec);
	DbgRecFile.ReadElement(iSize32);
	if (iSize32)
	{
		StdBuf buf;
		buf.SetSize(iSize32);
		DbgRecFile.ReadRaw(buf.getMData(), iSize32);
		PktInReplay = C4PktDebugRec(eTypeRec, buf);
	}
#endif
#else
	// check debug rec in list
	C4IDPacket *pkt;
	if (pkt = DebugRec.firstPkt())
	{
		// copy from list
		PktInReplay = *static_cast<C4PktDebugRec *>(pkt->getPkt());
		DebugRec.Delete(pkt);
	}
	else
	{
		// special sync check skip...
		while (currChunk != chunks.end() && currChunk->Type == RCT_CtrlPkt)
		{
			C4IDPacket Packet(*currChunk->pPkt);
			C4ControlPacket *pCtrlPck = static_cast<C4ControlPacket *>(Packet.getPkt());
			assert(!pCtrlPck->Sync());
			Game.Control.ExecControlPacket(Packet.getPktType(), pCtrlPck);
			NextChunk();
		}
		// record end?
		if (currChunk == chunks.end() || currChunk->Type == RCT_End || Finished)
		{
			loggerDebugRec->info("end: All in sync!");
			++DoNoDebugRec;
			return;
		}
		// unpack directly from head
		if (currChunk->Type != eType)
		{
			DebugRecError(std::format("Playback type {:x}, this type {:x}", currChunk->Type, std::to_underlying(eType)));
			return;
		}
		PktInReplay = *currChunk->pDbg;
		fHasPacketFromHead = true;
	}
#endif // DEBUGREC_EXTFILE
	// record end?
	if (PktInReplay.getType() == RCT_End)
	{
		loggerDebugRec->info("end: All in sync (2)!");
		++DoNoDebugRec;
		return;
	}
	// replay packet is unpacked to PktInReplay now; check it
	if (PktInReplay.getType() != eType)
	{
		DebugRecError(std::format("Type {} != {}", GetRecordChunkTypeName(PktInReplay.getType()), GetRecordChunkTypeName(eType)));
		return;
	}
	if (PktInReplay.getSize() != iSize)
	{
		DebugRecError(std::format("Size {} != {}", PktInReplay.getSize(), iSize));
	}
	// check packet data
	if (memcmp(PktInReplay.getData(), pData, iSize))
	{
		const std::string error{std::format(
						"DbgRectPkt Type {}, size {} Replay: {} Here: {}",
						GetRecordChunkTypeName(eType),
						iSize,
						GetDbgRecPktData(eType, StdBuf{PktInReplay.getData(), PktInReplay.getSize(), false}),
						GetDbgRecPktData(eType, StdBuf{pData, static_cast<std::size_t>(iSize), false})
						)};
		DebugRecError(error);
	}
	// packet is fine, jump over it
	if (fHasPacketFromHead)
		NextChunk();
}

void C4Playback::DebugRecError(const std::string_view error)
{
	loggerDebugRec->error("Playback error: {}", error);
	BREAKPOINT_HERE;
}

#endif

bool C4Playback::StreamToRecord(const char *szStream, StdStrBuf *pRecordFile)
{
	auto logger = Application.LogSystem.CreateLogger(Config.Logging.Playback);
	// Load data
	StdBuf CompressedData;
	logger->info("Reading stream...");
	if (!CompressedData.LoadFromFile(szStream))
		return false;

	// Decompress
	unsigned long iStreamSize = CompressedData.getSize() * 5;
	StdBuf StreamData; StreamData.New(iStreamSize);
	while (true)
	{
		// Initialize stream
		z_stream strm{};
		strm.next_in = CompressedData.getMPtr<uint8_t>();
		strm.avail_in = CompressedData.getSize();
		strm.next_out = StreamData.getMPtr<uint8_t>();
		strm.avail_out = StreamData.getSize();

		// Decompress
		if (inflateInit(&strm) != Z_OK)
			return false;
		int ret = inflate(&strm, Z_FINISH);
		if (ret == Z_STREAM_END)
		{
			inflateEnd(&strm);
			break;
		}
		if (ret != Z_BUF_ERROR)
			return false;

		// All input consumed?
		iStreamSize = strm.total_out;
		if (strm.avail_in == 0)
		{
			logger->error("Stream data incomplete, using as much data as possible");
			break;
		}

		// Larger buffer needed
		StreamData.Grow(CompressedData.getSize());
		iStreamSize = StreamData.getSize();
	}
	StreamData.SetSize(iStreamSize);

	// Parse
	C4Playback Playback{logger};
	Playback.ReadBinary(StreamData);
	logger->info("Got {} chunks from stream", Playback.chunks.size());

	// Get first chunk, which must contain the initial
	chunks_t::iterator chunkIter = Playback.chunks.begin();
	if (chunkIter == Playback.chunks.end() || chunkIter->Type != RCT_File)
		return false;

	// Get initial chunk, go over file name
	StdBuf InitialData = StdBuf::TakeOrRef(*chunkIter->pFileData);

	// Put to temporary file and unpack
	char szInitial[_MAX_PATH + 1] = "~initial.tmp";
	MakeTempFilename(szInitial);
	if (!InitialData.SaveToFile(szInitial) ||
		!C4Group_UnpackDirectory(szInitial))
		return false;

	// Load Scenario.txt from Initial
	C4Group Grp; C4Scenario Initial;
	if (!Grp.Open(szInitial) ||
		!Initial.Load(Grp) ||
		!Grp.Close())
		return false;

	// Copy original scenario
	const char *szOrigin = Initial.Head.Origin.getData();
	char szRecord[_MAX_PATH + 1];
	SCopy(szStream, szRecord, _MAX_PATH);
	if (GetExtension(szRecord))
		*(GetExtension(szRecord) - 1) = 0;
	SAppend(".c4s", szRecord, _MAX_PATH);
	logger->info("Original scenario is {}, creating {}.", szOrigin, szRecord);
	if (!C4Group_CopyItem(szOrigin, szRecord, false, false))
		return false;

	// Merge initial
	if (!Grp.Open(szRecord) ||
		!Grp.Merge(szInitial))
		return false;

	// Process other files in stream
	chunkIter->Delete();
	chunkIter = Playback.chunks.erase(chunkIter);
	while (chunkIter != Playback.chunks.end())
		if (chunkIter->Type == RCT_File)
		{
			logger->info("Inserting {}...", chunkIter->Filename.getData());
			StdStrBuf Temp; Temp.Copy(chunkIter->Filename);
			MakeTempFilename(&Temp);
			if (!chunkIter->pFileData->SaveToFile(Temp.getData()))
				return false;
			if (!Grp.Move(Temp.getData(), chunkIter->Filename.getData()))
				return false;
			chunkIter = Playback.chunks.erase(chunkIter);
		}
		else
			chunkIter++;

	// Write record data
	StdBuf RecordData = Playback.ReWriteBinary();
	if (!Grp.Add(C4CFN_CtrlRec, RecordData, false, true))
		return false;

	// Done
	logger->info("Writing record file...");
	Grp.Close();
	pRecordFile->Copy(szRecord);
	return true;
}
