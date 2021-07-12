#include "stdafx.h"
#include "Resource.h"
#include "Framework.h" 
#include <fstream>
#include <string>
#pragma warning(disable  : 4996)    // mbstowcs unsafe###
#define MAX_GAME_LOOP 30
#define FPS 1 / 60.0f

//프레임을 따라잡기까지 최대 몇번 루프를 돌 것인지를 지정합니다.
#define MAX_LOOP_TIME 50

#define TITLE_LENGTH 50

// 캡션 FPS 출력 여부 -------------------
// 항상 캡션에 FPS를 출력		(0 : 비활성 | 1 : 활성)
#define USE_CAPTIONFPS_ALWAYS	 0

#if USE_CAPTIONFPS_ALWAYS
	#define SHOW_CAPTIONFPS 
#elif _DEBUG	// Debug에서는 항상 실행
	#define SHOW_CAPTIONFPS 
#endif

#if defined(SHOW_CAPTIONFPS)
	#define MAX_UPDATE_FPS_CAPTION 1.0f / 5.0f
#endif
 
CFramework::CFramework()
{
	m_ServerIp = "";
	bool isServerConnected = false; 

	m_LeftX = 1001;
	m_TopY = 1;
}

CFramework::~CFramework()
{ 
	//SaveMapData();
}

void CFramework::init(HWND hWnd, HINSTANCE hInst)
{
	ReadMapData();
	strcpy_s(m_PlayerName, "Test");

	for (int i = 0; i < WORLD_HEIGHT; ++i) {
		for (int j = 0; j < WORLD_WIDTH; ++j) {
			//m_TileDatas[j][i] = (MAP_TILE_DATA)(rand() % (int)(MAP_TILE_DATA::COUNT));
		}
	}
	m_hWnd = hWnd;
	m_hInst = hInst;

	::GetClientRect(hWnd, &m_rtClient);
	  
	LoadString(m_hInst, IDS_APP_TITLE, m_captionTitle, TITLE_LENGTH);
#if defined(SHOW_CAPTIONFPS)
	lstrcat(m_captionTitle, TEXT(" ("));
#endif
	m_titleLength = lstrlen(m_captionTitle);
	SetWindowText(m_hWnd, m_captionTitle);

	BuildScene();
	InitBuffers();
}

void CFramework::BuildScene()
{ 
	m_Background.Load(L"Resources/Background.png");
	m_TileImage.Load(L"Resources/GameTileX.png"); 
	m_ChatUi.Load(L"Resources/ChatUi.png");
}

void CFramework::InitBuffers()
{
	if (m_hdc) // 기존에 디바이스 컨테스트가 생성되어 있었다면 제거합니다.
	{
		::SelectObject(m_hdc, nullptr);
		::DeleteObject(m_hdc);
	}
	if (m_hbmp) //기존에 버퍼가 생성되어 있었다면 제거합니다.
	{
		::DeleteObject(m_hbmp);
	}

	HDC hdc = ::GetDC(m_hWnd);

	m_hdc = CreateCompatibleDC(hdc);
	m_hbmp = CreateCompatibleBitmap(hdc, m_rtClient.right, m_rtClient.bottom);
	::SelectObject(m_hdc, m_hbmp);
	::FillRect(m_hdc, &m_rtClient, (HBRUSH)WHITE_BRUSH);

	::ReleaseDC(m_hWnd, hdc);
}

void CFramework::preUpdate()
{
	m_timeElapsed = std::chrono::system_clock::now() - m_currentTime[0];//현재시간과 이전시간을 비교해서
	m_serverUpdated = std::chrono::system_clock::now() - m_currentTime[1];

	if (m_timeElapsed.count() > FPS)				//지정된 시간이 흘렀다면
	{
		m_currentTime[0] = std::chrono::system_clock::now();//현재시간 갱신

		if (m_timeElapsed.count() > 0.0)
		{
			m_fps = 1.0 / m_timeElapsed.count();
		}

		//게임 시간이 늦어진 경우 이를 따라잡을 때 까지 업데이트 시킵니다.
		m_dLag += m_timeElapsed.count();
		for (int i = 0; m_dLag > FPS && i < MAX_LOOP_TIME; ++i)
		{ 
			update(FPS);
			m_dLag -= FPS;
		}
	}
	// 최대 FPS 미만의 시간이 경과하면 진행 생략(Frame Per Second)
	else
		return; 
	// 업데이트가 종료되면 렌더링을 진행합니다.
	InitBuffers();
	InvalidateRect(m_hWnd, &m_rtClient, FALSE);

#if defined(SHOW_CAPTIONFPS)

	m_updateElapsed = std::chrono::system_clock::now() - m_lastUpdateTime;
	if (m_updateElapsed.count() > MAX_UPDATE_FPS_CAPTION)
		m_lastUpdateTime = std::chrono::system_clock::now();
	else
		return;

	_itow_s(m_fps + 0.1f, m_captionTitle + m_titleLength, TITLE_LENGTH - m_titleLength, 10);
	wcscat_s(m_captionTitle + m_titleLength, TITLE_LENGTH - m_titleLength, TEXT(" FPS)"));
	SetWindowText(m_hWnd, m_captionTitle);

#endif
}

void CFramework::update(float timeElapsed)
{
	for (auto& pObjects : m_ToDrawObjects)
	{
		pObjects->Update(timeElapsed);
	}
}

void CFramework::draw(HDC hdc)
{
	DrawMap(m_hdc);
	
	string pos = to_string(m_LeftX) + " " + to_string(m_TopY);
	wstring wpos;
	wpos.assign(pos.begin(), pos.end());
	TextOut(m_hdc, 0, 0, wpos.c_str(), lstrlen(wpos.c_str()));
	BitBlt(hdc, 0, 0, m_rtClient.right, m_rtClient.bottom,
		m_hdc, 0,0, SRCCOPY);
}

LRESULT CFramework::ProcessWindowInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	DWORD bytes_sent, bytes_recv, recv_flag = 0;
	move_packet p; 
	WSABUF s_wsabuf[1], r_wsabuf[1];   
	switch (message)
	{ 
	case WM_CREATE:
		//CreateCaret(hWnd, NULL, 5, 15);
		//ShowCaret(hWnd);
		//SetCaretPos(360 + 7.8 * m_ServerIp.length(), m_CaretYPos);
		break;
	case WM_CHAR:
		switch (wParam)
		{
			// 이미지 선택
		case VK_0:
			m_SelectedTileImage = m_SelectedTileImage = 0;
			break;
		case VK_1:
			m_SelectedTileImage = m_SelectedTileImage = 1;
			break;
		case VK_2:
			m_SelectedTileImage = m_SelectedTileImage = 2;
			break;
		case VK_3:
			m_SelectedTileImage = m_SelectedTileImage = 3;
			break;
		case VK_4:
			m_SelectedTileImage = m_SelectedTileImage = 4;
			break;
		case VK_5:
			m_SelectedTileImage = m_SelectedTileImage = 5;
			break;
		case VK_6:
			m_SelectedTileImage = m_SelectedTileImage = 6;
			break;
		case VK_7:
			m_SelectedTileImage = m_SelectedTileImage = 7;
			break;
		case VK_8:
			m_SelectedTileImage = m_SelectedTileImage = 8;
			break;
		case VK_9:
			m_SelectedTileImage = m_SelectedTileImage = 9;
			break;
		
		}
		break;
	case WM_KEYDOWN:  
	{ 
		switch (wParam)
		{
		case VK_LEFT:
			m_LeftX = m_LeftX - 10;
			break;
		case VK_RIGHT:
			m_LeftX = m_LeftX + 10;
			break;
		case VK_UP:
			m_TopY = m_TopY - 10;
			break;
		case VK_DOWN:
			m_TopY = m_TopY + 10;
			break;
		case VK_ADD:
			SaveMapData();
			break;
		case VK_SUBTRACT:
			ReadMapData();
			break;
		case VK_F1:
			m_SelectedTileImage = m_SelectedTileImage = 10;
			break;
		case VK_F2:
			m_SelectedTileImage = m_SelectedTileImage = 11;
			break;
		case VK_F3:
			m_SelectedTileImage = m_SelectedTileImage = 12;
			break;
		case VK_F4:
			m_SelectedTileImage = m_SelectedTileImage = 13;
			break;
		case VK_F5:
			m_SelectedTileImage = m_SelectedTileImage = 14;
			break;
		case VK_F6:
			m_SelectedTileImage = m_SelectedTileImage = 15;
			break;
		default: 
			break;
		}
		break;
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);

		this->draw(hdc);

		EndPaint(hWnd, &ps);
		break;
	}
	case WM_LBUTTONDOWN:
		m_IsMouseDown = true;
		break;
	case WM_LBUTTONUP:
		m_IsMouseDown = false;
		break;
	case WM_MOUSEMOVE:
	{
		if (false == m_IsMouseDown) {
			break;
		}
		int mx = LOWORD(lParam);
		int my = HIWORD(lParam);
		
		// 타일 좌표
		int tx = mx / TILE_WIDTH + m_LeftX;
		int ty = my / TILE_WIDTH + m_TopY;

		m_TileDatas[ty][tx] = (MAP_TILE_DATA)m_SelectedTileImage;
	}
		break;
	case WM_SOCKET:
		//ProcessCommunication(wParam, lParam);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

bool CFramework::ConnectToServer()
{ 
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 0), &WSAData);

	m_SocketServer = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, 0);
	
	SOCKADDR_IN serverAddr;
	memset(&serverAddr, 0, sizeof(SOCKADDR_IN));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);  
	inet_pton(AF_INET, m_ServerIp.c_str(), &serverAddr.sin_addr);
	int ret = WSAConnect(m_SocketServer, (sockaddr*)&serverAddr, sizeof(serverAddr), 0, 0, 0, 0);

	WSAAsyncSelect(m_SocketServer, m_hWnd, WM_SOCKET, FD_CLOSE | FD_READ);

	if (ret == SOCKET_ERROR) {
		display_error("잘못된 ip주소 연결 시도 -- ", WSAGetLastError());
		//exit(-1);
		return false;
	}
	
	m_Client.m_socket = m_SocketServer;
	m_CaretYPos = m_rtClient.bottom - 20;

	// 채팅에 다시 쓸거니깐 삭제하진 말자..
	//HideCaret(m_hWnd);
	//DestroyCaret();

	return (m_isServerConnected = true);
}

void CFramework::ProcessCommunication(WPARAM wParam, LPARAM lParam)
{
	if (WSAGETSELECTERROR(lParam)) {
		closesocket((SOCKET)wParam);
		error_display("WSAGETSELECTERROR");
	}
	switch (WSAGETSELECTEVENT(lParam))
	{
	case FD_READ:
		DoRecv();
		break;
	case FD_CLOSE:
		closesocket((SOCKET)wParam);
		error_display("FD_CLOSE");
	}
}

void CFramework::DoRecv()
{
	m_Client.m_recv_over.m_wsabuf.buf = reinterpret_cast<char*>(m_Client.m_recv_over.m_packetbuf) + m_Client.m_prev_size;
	m_Client.m_recv_over.m_wsabuf.len = MAX_BUFFER - m_Client.m_prev_size;

	memset(&m_Client.m_recv_over.m_over, 0, sizeof(m_Client.m_recv_over.m_over));

	DWORD iobyte, ioflag = 0;
	int ret = WSARecv(m_Client.m_socket, &m_Client.m_recv_over.m_wsabuf, 1,
		&iobyte, &ioflag, NULL, NULL);
	if (0 != ret) {
		auto err_no = WSAGetLastError();
		if (WSA_IO_PENDING != err_no)
			error_display("Error in RecvPacket: ");
	}

	unsigned char* packet_ptr = m_Client.m_recv_over.m_packetbuf;
	int num_data = iobyte + m_Client.m_prev_size;
	int packet_size = packet_ptr[0];

	while (num_data >= packet_size) {
		ProcessPacket(packet_ptr);
		num_data -= packet_size;
		packet_ptr += packet_size;
		if (0 >= num_data) break;
		packet_size = packet_ptr[0];
	}
	m_Client.m_prev_size = num_data;
	if (0 != num_data) {
		memcpy(m_Client.m_recv_over.m_packetbuf, packet_ptr, num_data);
	}
}

void CFramework::ProcessPacket(unsigned char* p_buf)
{
	char buf[10000];
	///unsigned char p_size = reinterpret_cast<unsigned char*>(p_buf)[0];
	unsigned char p_type = reinterpret_cast<unsigned char*>(p_buf)[1];
	switch (p_type)
	{
	case SC_LOGIN_OK:
	{ 
		sc_packet_login_ok* packet = reinterpret_cast<sc_packet_login_ok*>(p_buf);

		//cout << "LoginOk - x : " << packet->x << " y : " << packet->y << "\n";
		m_ClientId = packet->id;
		m_Player = &m_Objects[m_ClientId];
		m_Player->SetName(m_PlayerName);
		m_Player->MoveTo({ packet->x, packet->y }); 

		//avatar.SetName(packet->name);
		m_LeftX = packet->x - SCREEN_WIDTH * 0.5f;
		m_TopY = packet->y  - SCREEN_HEIGHT *0.5f;
		m_Player->Show();

		m_ToDrawObjects.emplace_back(m_Player);
		break;
	}
	case SC_LOGIN_FAIL:
		break;
	case SC_POSITION:
	{		
		sc_packet_position* packet = reinterpret_cast<sc_packet_position*>(p_buf);
		//cout << "SC_POSITION - x : " << packet->x << " y : " << packet->y << "\n";
		int other_id = packet->id;
		if (other_id == m_ClientId) {
			m_Player->MoveTo({ packet->x, packet->y });
			m_LeftX = packet->x - SCREEN_WIDTH * 0.5f;
			m_TopY = packet->y - SCREEN_HEIGHT * 0.5f;
		}
		else if (other_id < MAX_USER) {
			m_Objects[other_id].MoveTo({ packet->x, packet->y });
		}
		else {
			//npc[other_id - NPC_START].x = my_packet->x;
			//npc[other_id - NPC_START].y = my_packet->y;
		}

		break;
	}
	case SC_CHAT:
	{
		cout << "SC_CHAT\n";

	}
		break;
	case SC_STAT_CHANGE:
		break;
	case SC_REMOVE_OBJECT:
	{
		cout << "SC_REMOVE_OBJECT\n";
		sc_packet_remove_object* packet = reinterpret_cast<sc_packet_remove_object*>(p_buf);
		int other_id = packet->id;
		if (other_id == m_ClientId) {
			m_Player->Hide();
		}
		else if (other_id < MAX_USER) {
			m_Objects[other_id].Hide();
		}
		else {
			//		npc[other_id - NPC_START].attr &= ~BOB_ATTR_VISIBLE;
		}
		auto res = std::find(m_ToDrawObjects.begin(), m_ToDrawObjects.end(), &m_Objects[other_id]);
		if (res != m_ToDrawObjects.end()) {
			m_ToDrawObjects.erase(res);
		}
		break;
	}
		break;
	case SC_ADD_OBJECT:
	{
		cout << "SC_ADD_OBJECT\n";
		sc_packet_add_object* packet = reinterpret_cast<sc_packet_add_object*>(p_buf);
		int id = packet->id;
		 
		if (id < MAX_USER) { 
			m_Objects[id].SetName(packet->name);
			m_Objects[id].MoveTo({ packet->x, packet->y });
			m_Objects[id].Show();
			m_ToDrawObjects.push_back(&m_Objects[id]);
		}
		else {
			//npc[id - NPC_START].x = my_packet->x;
			//npc[id - NPC_START].y = my_packet->y;
			//npc[id - NPC_START].attr |= BOB_ATTR_VISIBLE;
		}
		break;
	}
	default:
		cout << "잘못된 패킷이 수신되었습니다";
		cout << ":: <올바르지 못한 패킷 형식" << p_buf[1] << ">\n";
		while (true);
		break;
	}
}

bool CFramework::SendPacket(void* p)
{
	unsigned char p_size = reinterpret_cast<unsigned char*>(p)[0];
	unsigned char p_type = reinterpret_cast<unsigned char*>(p)[1];

	EX_OVER* s_over = new EX_OVER;
	memset(&s_over->m_over, 0, sizeof(s_over->m_over));
	memcpy(s_over->m_packetbuf, p, p_size);
	s_over->m_wsabuf.buf = reinterpret_cast<CHAR*>(s_over->m_packetbuf);
	s_over->m_wsabuf.len = p_size;

	auto ret = WSASend(m_Client.m_socket, &s_over->m_wsabuf, 1, NULL, 0, &s_over->m_over, NULL);
	if (0 != ret) {
		auto err_no = WSAGetLastError();
		if (WSA_IO_PENDING != err_no) {
			cout << "로그아웃\n";
			return false;
			//disconnect(p_id);
		}
	}
	return true;
}

void CFramework::SendLoginPacket()
{ 
	cs_packet_login packet;
	packet.size = sizeof(packet);
	packet.type = CS_LOGIN;
	strcpy_s(packet.player_id, m_PlayerName); 

	SendPacket(&packet);
}

void CFramework::SendMovePacket(DIRECTION dir)
{
	cs_packet_move packet;
	packet.size = sizeof(packet);
	packet.type = CS_MOVE;
	packet.direction = dir;
	SendPacket(&packet); 
}
void CFramework::SendChatPacket()
{
	if (m_ChatData.size() == 0) {
		// 채팅 데이터가 없을 때는 반환
		return;
	}
	cs_packet_chat packet;
	packet.size = sizeof(packet);
	packet.type = CS_CHAT;
	strcpy_s(packet.message, m_ChatData.c_str());
	SendPacket(&packet);
}
bool CFramework::SendPacket(SOCKET& sock, char* packet, int packetSize, int& retVal)
{
	retVal = send(sock, packet, packetSize, 0);
	if (retVal == SOCKET_ERROR)
	{
		error_display("send()");
		return false;
	}
	return true;
}

void CFramework::DrawMap(HDC hdc)
{ 
	for (int i = 0; i < 20; ++i) {
		if (m_TopY + i >= WORLD_HEIGHT || m_TopY + i < 0) {
			continue;
		}
		for (int j = 0; j < 20; ++j) { 
			if (m_LeftX + j >= WORLD_WIDTH || m_LeftX + j < 0) {
				continue;
			}
			m_TileImage.StretchBlt(m_hdc,
				j * TILE_WIDTH, i * TILE_WIDTH, TILE_WIDTH, TILE_WIDTH, 
				(int)m_TileDatas[m_TopY + i][m_LeftX + j] * TILE_WIDTH, 0, TILE_WIDTH, TILE_WIDTH);
		}
	}
}

RECT rt = { 0, WINDOW_HEIGHT / 2, WINDOW_WIDTH, WINDOW_HEIGHT };
void CFramework::DrawChatUi(HDC hdc)
{
	//m_ChatUi.StretchBlt(hdc, 0, m_rtClient.bottom * 0.75f, m_rtClient.right, m_rtClient.bottom * 0.25f, NULL);
	m_ChatUi.TransparentBlt(hdc, 0, m_rtClient.bottom * 0.75f, m_rtClient.right, m_rtClient.bottom * 0.25f,
		0, 0, m_ChatUi.GetWidth(), m_ChatUi.GetHeight(), NULL);

	wstring wide_string = wstring(m_ChatData.begin(), m_ChatData.end()); 
	TextOut(m_hdc, 10, m_CaretYPos, wide_string.c_str(), lstrlen(wide_string.c_str()));
}

void CFramework::SaveMapData()
{
	ofstream fileOut("MapInfos.txt");
	int indexX, indexY;
	for (int i = 0; i < WORLD_HEIGHT; ++i)
	{
		for (int j = 0; j < WORLD_WIDTH; ++j)
		{
			fileOut << (int)m_TileDatas[i][j] << " "; 
		}
		fileOut << endl;  
	}
	cout << "파일 저장 완료!\n";
}

void CFramework::ReadMapData()
{ 
	ifstream fileIn("MapInfos.txt");
	for (int i = 0; i < WORLD_HEIGHT; ++i)
	{
		for (int j = 0; j < WORLD_WIDTH; ++j)
		{
			string text; fileIn >> text; 
			m_TileDatas[i][j] = (MAP_TILE_DATA)stoi(text); 

			//if (j < 1000) {
			//	if (i < 1000) { // 얼음
			//		if (j == 0) {
			//			m_TileDatas[i][j] = (MAP_TILE_DATA)11;
			//		}
			//		else {
			//			m_TileDatas[i][j] = (MAP_TILE_DATA)6;
			//		}
			//	}
			//	else { // 사막1
			//		if (j == 0) {
			//			m_TileDatas[i][j] = (MAP_TILE_DATA)10;
			//		}
			//		else {
			//			m_TileDatas[i][j] = (MAP_TILE_DATA)2;
			//		}
			//	}
			//}
			//else if (j >= 1000) {
			//	if (i < 1000) { // 사막2
			//		if (j == 1999) {
			//			m_TileDatas[i][j] = (MAP_TILE_DATA)10;
			//		}
			//		else { 
			//			m_TileDatas[i][j] = (MAP_TILE_DATA)3;
			//		}
			//	}
			//	else {// 잔디
			//		if (j == 1999) {
			//			m_TileDatas[i][j] = (MAP_TILE_DATA)8;
			//		}
			//		else {
			//			m_TileDatas[i][j] = (MAP_TILE_DATA)0;
			//		}
			//	}
			//}
		}
	}
	 
	cout << "맵 데이터 읽기 완료!\n";
}
