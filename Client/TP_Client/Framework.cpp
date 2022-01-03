#include "stdafx.h"
#include "Resource.h"
#include "Framework.h"
#include "TestScene.h"
#include "TitleScene.h"

#pragma warning(disable  : 4996)    // mbstowcs unsafe###
#define MAX_GAME_LOOP 30
#define FPS 1 / 60.0f

//�������� ���������� �ִ� ��� ������ �� �������� �����մϴ�.
#define MAX_LOOP_TIME 50

#define TITLE_LENGTH 50

// ĸ�� FPS ��� ���� -------------------
// �׻� ĸ�ǿ� FPS�� ���		(0 : ��Ȱ�� | 1 : Ȱ��)
#define USE_CAPTIONFPS_ALWAYS	 0

#if USE_CAPTIONFPS_ALWAYS
	#define SHOW_CAPTIONFPS 
#elif _DEBUG	// Debug������ �׻� ����
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
	if (m_hdc) // ������ ����̽� ���׽�Ʈ�� �����Ǿ� �־��ٸ� �����մϴ�.
	{
		::SelectObject(m_hdc, nullptr);

		::DeleteObject(m_hdc);
	}
	if (m_hbmp) //������ ���۰� �����Ǿ� �־��ٸ� �����մϴ�.
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
	m_timeElapsed = std::chrono::system_clock::now() - m_currentTime[0];//����ð��� �����ð��� ���ؼ�
	m_serverUpdated = std::chrono::system_clock::now() - m_currentTime[1];

	if (m_timeElapsed.count() > FPS)				//������ �ð��� �귶�ٸ�
	{
		m_currentTime[0] = std::chrono::system_clock::now();//����ð� ����

		if (m_timeElapsed.count() > 0.0)
		{
			m_fps = 1.0 / m_timeElapsed.count();
		}

		//���� �ð��� �ʾ��� ��� �̸� �������� �� ���� ������Ʈ ��ŵ�ϴ�.
		m_dLag += m_timeElapsed.count();
		for (int i = 0; m_dLag > FPS && i < MAX_LOOP_TIME; ++i)
		{ 
			update(FPS);
			m_dLag -= FPS;
		}
	}
	// �ִ� FPS �̸��� �ð��� ����ϸ� ���� ����(Frame Per Second)
	else
		return; 
	// ������Ʈ�� ����Ǹ� �������� �����մϴ�.
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
		display_error("�߸��� ip�ּ� ���� �õ� -- ", WSAGetLastError());
		//exit(-1);
		return false;
	}
	
	m_Client.m_socket = m_SocketServer;

	// ä�ÿ� �ٽ� ���Ŵϱ� �������� ����..
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
			cout << "�α׾ƿ�\n";
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
