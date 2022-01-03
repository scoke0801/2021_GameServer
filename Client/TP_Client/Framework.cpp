#include "stdafx.h"
#include "Resource.h"
#include "Framework.h"
#include "TestScene.h"
#include "TitleScene.h"

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
	m_ServerIp = "127.0.0.1";
	bool isServerConnected = false;
}

CFramework::~CFramework()
{ 
}

void CFramework::init(HWND hWnd, HINSTANCE hInst)
{ 
	m_hWnd = hWnd;
	m_hInst = hInst;

	::GetClientRect(hWnd, &m_rtClient);
	  
	LoadString(m_hInst, IDS_APP_TITLE, m_captionTitle, TITLE_LENGTH);
#if defined(SHOW_CAPTIONFPS)
	lstrcat(m_captionTitle, TEXT(" ("));
#endif
	m_titleLength = lstrlen(TEXT("2021GameServer_MMORPG")); 
	SetWindowText(m_hWnd, TEXT("2021GameServer_MMORPG"));

	BuildScene();
	InitBuffers();
}

void CFramework::BuildScene()
{ 
#ifdef TEST_MODE
	ChangeScene<CTestScene>(); 
#else
	ChangeScene<CTitleScene>();
#endif 
	m_ServerIp = "127.0.0.1"; 
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
	SetBkMode(m_hdc, TRANSPARENT);
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
	m_CurScene->Update(timeElapsed);
}

void CFramework::draw(HDC hdc)
{ 
	m_CurScene->Draw(hdc);

	BitBlt(hdc, 0, 0, m_rtClient.right, m_rtClient.bottom,
		m_hdc, 0,0, SRCCOPY);
}  
LRESULT CFramework::ProcessWindowInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{ 
	switch (message)
	{
	case WM_CREATE: 
	case WM_IME_COMPOSITION: 
	case WM_CHAR: 
	case WM_KEYDOWN: 
	case WM_LBUTTONDOWN:
	
		m_CurScene->ProcessWindowInput(hWnd, message, wParam, lParam);
	break;
	
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);

		this->draw(hdc);

		EndPaint(hWnd, &ps);
		return 0;;
	}
	case WM_SOCKET:
		ProcessCommunication(wParam, lParam);
		return 0;;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
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

	// 채팅에 다시 쓸거니깐 삭제하진 말자..
	HideCaret(m_hWnd);
	DestroyCaret();

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
	m_CurScene->ProcessPacket(p_buf); 
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
