
#include <C4Include.h>
#include <C4Client.h>
#include <C4Network2Client.h>
#include <C4Network2Players.h>
#include <C4Network2Res.h>
#include <C4Network2.h>
#include <C4Network2Reference.h>
#include <C4ChatDlg.h>
#include <C4GameControlNetwork.h>
#include <C4StartupNetDlg.h>
#include <C4UpdateDlg.h>
#include <C4Game.h>

const int C4NetIO::TO_INF = -1;

C4Network2Client *C4Network2ClientList::RegClient(C4Client *pClient) { return 0; }
void C4Network2ClientList::Clear() {}
void C4Network2ClientList::DeleteClient(C4Network2Client*) {}
void C4Network2Players::HandlePlayerInfo(C4ClientPlayerInfos const&) {}
C4Network2Res::Ref C4Network2ResList::getRefRes(int32_t) { return 0; }
C4Network2Res::Ref C4Network2ResList::AddByCore(C4Network2ResCore const&, bool) { return 0; }
void C4Network2Res::AddRef() {}
void C4Network2Res::DelRef() {}
void C4Network2::LeagueNotifyDisconnect(int32_t, C4LeagueDisconnectReason) {}
void C4Network2::Clear() {}
C4IDPacket * C4Network2::GetVote(int32_t, C4ControlVoteType, int32_t) { return 0; }
void C4Network2Players::OnClientPart(C4Client*) {}
void C4Network2Players::SendUpdatedPlayers() {}
C4Network2ResCore::C4Network2ResCore() {}
bool C4Network2::ToggleClientListDlg() { return 0; }
bool C4Network2::ToggleAllowJoin() { return 0; }
void C4Network2::Execute() {}
C4Network2Reference::~C4Network2Reference() {}
C4Network2::C4Network2(): Clients(&NetIO) {}
C4Network2ClientList::~C4Network2ClientList() {}
C4Network2ResList::~C4Network2ResList() {}
C4Network2IO::~C4Network2IO() {}
bool C4ChatDlg::IsChatActive() { return 0; }
C4ChatDlg * C4ChatDlg::ShowChat() { return 0; }
C4GameControlNetwork::~C4GameControlNetwork() {}
C4GameControlNetwork::C4GameControlNetwork(C4GameControl * p): pParent(p) {}
void C4GameControlNetwork::CalcPerformance(int32_t) {}
void C4GameControlNetwork::Clear() {}
int32_t C4GameControlNetwork::ClientPerfStat(int32_t) { return 0; }
bool C4GameControlNetwork::ClientReady(int32_t, int32_t) { return 0; }
bool C4GameControlNetwork::CtrlNeeded(int32_t) const { return 0; }
bool C4GameControlNetwork::CtrlReady(int32_t) { return 0; }
C4ControlDeliveryType C4GameControlNetwork::DecideControlDelivery() const { return CDT_Queue; }
void C4GameControlNetwork::DoInput(C4Control const&) {}
void C4GameControlNetwork::DoInput(C4PacketType, C4ControlPacket*, C4ControlDeliveryType) {}
void C4GameControlNetwork::Execute() {}
bool C4GameControlNetwork::GetControl(C4Control*, int32_t) { return 0; }
void C4GameControlNetwork::SetActivated(bool) {}
C4NetIOPacket::~C4NetIOPacket() {}
void C4Network2::AbortLobbyCountdown() {}
StdStrBuf C4Network2Address::toString() const { return StdStrBuf(""); }
void C4Network2::AllowJoin(bool) {}
void C4Network2Client::ClearGraphs() {}
void C4Network2Client::CreateGraphs() {}
bool C4Network2ClientList::BroadcastMsgToClients(C4NetIOPacket const&) { return 0; }
C4Network2ClientList::C4Network2ClientList(C4Network2IO*) {}
C4Network2Client * C4Network2ClientList::GetClientByID(int32_t) const { return 0; }
C4Network2Client * C4Network2ClientList::GetNextClient(C4Network2Client*) { return 0; }
void C4Network2::DrawStatus(C4FacetEx&) {}
bool C4Network2::FinishStreaming() { return 0; }
void C4Network2::InvalidateReference() {}
C4Network2IO::C4Network2IO() {}
void C4Network2IOConnection::Close() {}
int C4Network2IOConnection::getLag() const { return 0; }
bool C4Network2IO::Execute(int) { return 0; }
const char * C4Network2IO::getNetIOName(C4NetIO*) { return 0; }
int C4Network2IO::GetTimeout() { return 0; }
bool C4Network2IO::OnConn(sockaddr_in const&, sockaddr_in const&, sockaddr_in const*, C4NetIO*) { return 0; }
void C4Network2IO::OnDisconn(sockaddr_in const&, C4NetIO*, char const*) {}
void C4Network2IO::OnError(char const*, C4NetIO*) {}
void C4Network2IO::OnPacket(C4NetIOPacket const&, C4NetIO*) {}
void C4Network2IO::OnThreadEvent(C4InteractiveEventType, void*) {}
bool C4Network2::isStreaming() const { return 0; }
void C4Network2::LeagueSignupDisable() {}
bool C4Network2::LeagueSignupEnable() { return 0; }
C4Network2Players::C4Network2Players():rInfoList(Game.Parameters.PlayerInfos) {}
DWORD C4Network2Players::GetClientChatColor(int, bool) const { return 0; }
C4ClientPlayerInfos * C4Network2Players::GetLocalPlayerInfoPacket() const { return 0; }
bool C4Network2Players::JoinLocalPlayer(char const*, bool) { return 0; }
void C4Network2Players::RequestPlayerInfoUpdate(C4ClientPlayerInfos const&) {}
void C4Network2Reference::CompileFunc(StdCompiler*) {}
void C4Network2::RequestActivate() {}
bool C4Network2Res::CalculateSHA() { return 0; }
void C4Network2ResCore::Clear() {}
C4Network2Res::Ref C4Network2Res::Derive() { return 0; }
bool C4Network2Res::FinishDerive() { return 0; }
C4Network2Res::Ref C4Network2ResList::AddByFile(char const*, bool, C4Network2ResType, int32_t, char const*, bool) { return 0; }
C4Network2ResList::C4Network2ResList() {}
C4Network2Res::Ref C4Network2ResList::getRefNextRes(int32_t) { return 0; }
C4Network2Res::Ref C4Network2ResList::getRefRes(char const*, bool) { return 0; }
void C4Network2ResList::OnShareFree(CStdCSecEx*) {}
void C4Network2ResList::RemoveAtClient(int32_t) {}
C4Network2Res::Ref C4Network2::RetrieveRes(C4Network2ResCore const&, int32_t, char const*, bool) { return 0; }
void C4Network2::SetCtrlMode(int32_t) {}
void C4Network2::SetPassword(char const*) {}
bool C4Network2::Start() { return 0; }
void C4Network2::StartLobbyCountdown(int32_t) {}
bool C4Network2::StartStreaming(C4Record*) { return 0; }
C4Network2Status::C4Network2Status() {}
void C4Network2::Vote(C4ControlVoteType, bool, int32_t) {}
void C4Network2::AddVote(const C4ControlVote &Vote) {}
void C4Network2::EndVote(C4ControlVoteType,bool,int32_t) {}
bool C4Network2::FinalInit() { return false; }
bool C4Network2::Pause() { return false;}
void C4Network2::LeagueGameEvaluate(char const *, const BYTE *) {}
void C4Network2::OnGameSynchronized() {}
bool C4Network2::InitHost(bool) { return false; }
C4StartupNetDlg::C4StartupNetDlg(): C4StartupDlg("") {}
bool C4UpdateDlg::ApplyUpdate(char const*, bool, C4GUI::Screen*) { return 0; }
bool C4UpdateDlg::CheckForUpdates(C4GUI::Screen*, bool) { return 0; }
C4NetIOPacket::C4NetIOPacket() {}
C4NetIOPacket::C4NetIOPacket(StdBuf &Buf, const C4NetIO::addr_t &naddr)	: StdCopyBuf(Buf), addr(naddr) {}
void C4Network2ResCore::CompileFunc(StdCompiler *pComp) {}
void C4Network2Status::CompileFunc(StdCompiler *pComp) {}
C4StartupNetDlg::~C4StartupNetDlg() {}
bool C4StartupNetDlg::DoBack() { return 0; }
bool C4StartupNetDlg::DoOK() { return 0; }
const uint16_t C4NetIO::P_NONE = ~0;
void C4StartupNetDlg::DrawElement(C4FacetEx&) {}
C4GUI::Control * C4StartupNetDlg::GetDefaultControl() { return 0; }
void C4StartupNetDlg::OnClosed(bool) {}
void C4StartupNetDlg::OnShown() {}
void C4StartupNetDlg::OnThreadEvent(C4InteractiveEventType, void*) {}
bool C4ChatDlg::ToggleChat() { return 0; }
C4Network2::InitResult C4Network2::InitClient(C4Network2Reference const&, bool) { return IR_Fatal; }
void C4Network2Players::Init() {}
bool C4Network2::DoLobby() { return 0; }
bool C4Network2::RetrieveScenario(char*) { return 0; }
bool AcquireWinSock() { return 0; }
void ReleaseWinSock() {}
