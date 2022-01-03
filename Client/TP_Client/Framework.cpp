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
	m_PlayerName = "Test";
	for (int i = 0; i < WORLD_HEIGHT; ++i) {
		for (int j = 0; j < WORLD_WIDTH; ++j) {
			m_TileDatas[j][i] = (MAP_TILE_DATA)(rand() % (int)(MAP_TILE_DATA::COUNT));
		}
	}
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
	ReadMapData();
	m_Background.Load(L"Resources/Background.png");
	m_TileImage.Load(L"Resources/GameTileX.png"); 
	m_ChatUi.Load(L"Resources/ChatUi.png");
	m_PlayerUiBoard.Load(L"Resources/PlayerUiBoard.png");
	m_ItemUIImage.Load(L"Resources/item_ui.png");
	m_Items.Load(L"Resources/Items.png");
	m_SellerUI.Load(L"Resources/SellerUI.png");
	m_TitleImage.Load(L"Resources/Title.png");
	int idx = 0;
	for (int x = 100; x < 2000; x += 100) {
		for (int y = 101; y < 2000; y += 100) {
			m_Npcs[idx].MoveTo({ x, y });
			m_Npcs[idx].Show();
			m_Npcs[idx].SetName("--NPC--");
			m_Npcs[idx].SetType(ObjectType::Seller);
			++idx;
		}
	}
	cout << "idx : " << idx;
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
	for (auto& pObjects : m_ToDrawObjects)
	{
		pObjects->Update(timeElapsed);
	}
	for (auto& pObjects : m_Npcs)
	{
		pObjects.Update(timeElapsed);
	}
}

void CFramework::draw(HDC hdc)
{ 
	if (m_isServerConnected)	
	{ 
		DrawMap(m_hdc);
		for (auto& pObjects : m_ToDrawObjects)
		{
			pObjects->Draw(m_hdc, 
				m_LeftX * TILE_WIDTH,
				m_TopY * TILE_WIDTH);
		}
		for (auto& pObjects : m_Npcs)
		{
			pObjects.Draw(m_hdc,
				m_LeftX * TILE_WIDTH,
				m_TopY * TILE_WIDTH);
		}
		DrawPlayerInfo(m_hdc);
		// 가려지도록
		DrawChatUi(m_hdc);

		if (m_IsSellerClicked) {
			m_SellerUI.TransparentBlt(m_hdc, m_rtClient.right * 0.1f, m_rtClient.bottom * 0.15f,
				m_rtClient.right * 0.8f, m_rtClient.bottom * 0.55f,
				0, 0, m_SellerUI.GetWidth(), m_SellerUI.GetHeight(), RGB(255,0,255));
		}
	}
	else 
	{
		m_TitleImage.TransparentBlt(m_hdc, 0,0, m_rtClient.right, m_rtClient.bottom,
			0, 0, m_TitleImage.GetWidth(), m_TitleImage.GetHeight(), NULL);

		Rectangle(m_hdc, 0, 280, m_rtClient.right, 330); 
		Rectangle(m_hdc, 0, 560, m_rtClient.right, 610);
		wstring wide_string = wstring(m_ServerIp.begin(), m_ServerIp.end());
		TextOut(m_hdc, 250, 300, L"Enter ServerIP : ", lstrlen(L"Enter ServerIP : "));
		TextOut(m_hdc, 250 + 110, 300, wide_string.c_str(), lstrlen(wide_string.c_str()));

		wide_string = wstring(m_PlayerName.begin(), m_PlayerName.end());
		TextOut(m_hdc, 250, 580, L"Enter PlayerID : ", lstrlen(L"Enter PlayerID : "));
		TextOut(m_hdc, 250 + 110, 580, wide_string.c_str(), lstrlen(wide_string.c_str()));
	}
	BitBlt(hdc, 0, 0, m_rtClient.right, m_rtClient.bottom,
		m_hdc, 0,0, SRCCOPY);
}  
LRESULT CFramework::ProcessWindowInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	DWORD bytes_sent, bytes_recv, recv_flag = 0;
	WSABUF s_wsabuf[1], r_wsabuf[1];

	HIMC m_hIMC = NULL;   // IME 핸들
	 
	TCHAR Text[255];     // 텍스트를 저장하기위한 변수
	TCHAR  Cstr[10];      // 조합중인 문자!! 
	int len;
	switch (message)
	{
	case WM_CREATE: 
		CreateCaret(hWnd, NULL, 5, 15);
		ShowCaret(hWnd);
		SetCaretPos(360 + 7.8 * m_ServerIp.length(), m_CaretYPos);
		return 0;
	case WM_IME_COMPOSITION:
		if (false == m_isServerConnected) {
			break;
		}
		if (false == m_IsOnChatting) {
			break;
		}
		m_hIMC = ImmGetContext(hWnd);	// ime핸들을 얻는것

		if (lParam & GCS_RESULTSTR)
		{
			if ((len = ImmGetCompositionString(m_hIMC, GCS_RESULTSTR, NULL, 0)) > 0)
			{
				// 완성된 글자가 있다.
				ImmGetCompositionString(m_hIMC, GCS_RESULTSTR, Cstr, len);
				Cstr[len] = 0;
				if (m_ChatData.size() + len >= MAX_STR_LEN) {
					break;
				}

				m_ChatData += TCHARToString(Cstr);

				lstrcpy(Text + lstrlen(Text), Cstr);
				memset(Cstr, 0, 10);
				 
				{
					char szTemp[256] = "";
					sprintf(szTemp, "완성된 글자 : %s\r\n", Text);
					//OutputDebugString(_T(szTemp));
				}

			}

		}
		else if (lParam & GCS_COMPSTR)
		{
			// 현재 글자를 조합 중이다.

			// 조합중인 길이를 얻는다.
			// str에  조합중인 문자를 얻는다.
			len = ImmGetCompositionString(m_hIMC, GCS_COMPSTR, NULL, 0);
			ImmGetCompositionString(m_hIMC, GCS_COMPSTR, Cstr, len);
			Cstr[len] = 0; 
			{
				char szTemp[256] = "";
				sprintf(szTemp, "조합중인 글자 : %s\r\n", Cstr);
				//OutputDebugString(_T(szTemp));
			}
		}

		ImmReleaseContext(hWnd, m_hIMC);	// IME 핸들 반환!!
		return 0;  
	case WM_CHAR:
		switch (wParam)
		{
		case VK_a:
		case VK_A:
			if (false == m_IsOnChatting) {
				cout << "공격 코드는 여기!\n";
				SendAttackPacket();
				break;
			}
			//break;
		case VK_0:
		case VK_1:
		case VK_2:
		case VK_3:
		case VK_4:
		case VK_5:
		case VK_6:
		case VK_7:
		case VK_8:
		case VK_9:
		case VK_DOT:

			if (false == m_isServerConnected) {
				if (m_IsOnTypingID) { 
					m_PlayerName += static_cast<char>(wParam);
					SetCaretPos(360 + 7.8 * m_PlayerName.length(), 580);
				}
				else {
					m_ServerIp += static_cast<char>(wParam);
					SetCaretPos(360 + 7.8 * m_ServerIp.length(), 300);
				}
			}
			else {
				if (false == m_IsOnChatting) {
					if (wParam >= VK_1 && wParam <= VK_4) {
						int itemIdx = wParam - VK_1;
						SendUseItem(itemIdx);
					}
					break;
				}
				if (m_ChatData.size() + 1 >= MAX_STR_LEN) {
					break;
				}
				m_ChatData += static_cast<char>(wParam);
				//SetCaretPos(m_rtClient.right * 0.1f + 10 + 7.8 * m_ChatData.length(), m_CaretYPos);
			}

			break;
		case VK_BACK: 
			if (false == m_isServerConnected) {
				if (m_IsOnTypingID) {  
					if (m_PlayerName.length() <= 0)
					{
						return 0;
					}
					m_PlayerName = m_PlayerName.substr(
						0, m_PlayerName.length() - 1);
					SetCaretPos(360 + 7.8 * m_PlayerName.length(), 580);
				}
				else { 
					if (m_ServerIp.length() <= 0)
					{
						return 0;
					}
					m_ServerIp = m_ServerIp.substr(
						0, m_ServerIp.length() - 1);
					SetCaretPos(360 + 7.8 * m_ServerIp.length(), 300); 
				}
				 
			}
			else {
				if (false == m_IsOnChatting) {
					return 0;
				}
				if (m_ChatData.length() <= 0)
				{
					return 0;
				}
				m_ChatData = m_ChatData.substr(
					0, m_ChatData.length() - 1);
				//SetCaretPos(m_rtClient.right * 0.1f + 10 + 7.8 * m_ChatData.length(), m_CaretYPos);
			}
			return 0;
		case VK_RETURN:
			if (!m_isServerConnected)
			{
				if (m_PlayerName.size() == 0) {
					return 0;
				}
				if (ConnectToServer())
				{
					//SetCaretPos(m_rtClient.right * 0.1f + 10 + 7.8 * m_ChatData.length(), m_CaretYPos);
					SendLoginPacket();
				}
			}
			else {
				if (false == m_IsOnChatting) {
					return 0;
				}
				SendChatPacket();
				m_ChatData.clear();
				//SetCaretPos(m_rtClient.right * 0.1f + 10 + 7.8 * m_ChatData.length(), m_CaretYPos);
			}
			return 0;

		default:
			if (true == m_isServerConnected) {
				if (false == m_IsOnChatting) {
					return 0;
				}
				if (m_ChatData.size() + 1 >= MAX_STR_LEN) {
					return 0;
				}
				m_ChatData += static_cast<char>(wParam);
				//SetCaretPos(m_rtClient.right * 0.1f + 16 + 7.8 * m_ChatData.length(), m_CaretYPos);
				return 0;
			}
			else {
				if (m_IsOnTypingID) {
					m_PlayerName += static_cast<char>(wParam);
					SetCaretPos(360 + 7.8 * m_PlayerName.length(), 580);
				}
			}
		} 
		return 0;
	case WM_KEYDOWN:
		if (m_isServerConnected) {
			DIRECTION dir;
			switch (wParam)
			{
			case VK_LEFT:
				dir = DIRECTION::D_W;
				break;
			case VK_RIGHT:
				dir = DIRECTION::D_E;
				break;
			case VK_UP:
				dir = DIRECTION::D_N;
				break;
			case VK_DOWN:
				dir = DIRECTION::D_S;
				break;
			default:
				dir = DIRECTION::D_NO;
			}
			if (dir != DIRECTION::D_NO) {
				SendMovePacket(dir);
			}
		}
		return 0;;
	case WM_LBUTTONDOWN:
	{
		int mx = LOWORD(lParam);
		int my = HIWORD(lParam);

		if (m_isServerConnected) {
			m_IsOnChatting = false;
			if (mx >= m_rtClient.right * 0.1f && mx <= m_rtClient.right * 0.9f) {
				if (my >= m_rtClient.bottom * 0.75f && my <= m_rtClient.bottom) {
					m_IsOnChatting = true;
				}
			}
			int m_tile_x = mx / TILE_WIDTH;
			int m_tile_y = my / TILE_WIDTH;

			if (false == m_IsSellerClicked) { 
				if (((m_LeftX + m_tile_x) % 100) == 0 &&
					((m_TopY + m_tile_y) % 100) == 1) {
					cout << "상인 선택!!!!\n";
					m_IsSellerClicked = true;
				}
				else { 
					m_IsSellerClicked = false;
				}
			}
			else {
				cout << " mx - " << mx << " my - " << my << "\n"; 
				if (mx > m_rtClient.right * 0.15f && mx < m_rtClient.right * 0.85f)
				{
					int height = m_rtClient.bottom * 0.15f;
					if (my > m_rtClient.bottom * 0.15f && my < m_rtClient.bottom * 0.25f) {
						m_SelectedItem = ITEM_TYPE::I_POTION;
					}
					else if(my > m_rtClient.bottom * 0.26f && my < m_rtClient.bottom * 0.35f){
						m_SelectedItem = ITEM_TYPE::I_POWERUP;
					}
					else if (my > m_rtClient.bottom * 0.36f && my < m_rtClient.bottom * 0.45f) {
						m_SelectedItem = ITEM_TYPE::I_BOMB;
					}
					else if (my > m_rtClient.bottom * 0.46f && my < m_rtClient.bottom * 0.55f) {
						m_SelectedItem = ITEM_TYPE::I_LEVELUP;
					}
				}
				if (my > m_rtClient.bottom * 0.56f && my < m_rtClient.bottom * 0.7f)
				{ 
					if (mx > m_rtClient.right * 0.20f && mx < m_rtClient.right * 0.42f) {
						// 구매
						if (ITEM_TYPE::I_NOT != m_SelectedItem) {
							cs_packet_buy_item packet;
							packet.size = sizeof(packet);
							packet.type = CS_BUY_ITEM;
							packet.itemType = m_SelectedItem;
							SendPacket(&packet);
						}
					}
					else if (mx > m_rtClient.right * 0.58f && mx < m_rtClient.right * 0.75f) {
						// 취소
						m_IsSellerClicked = false; 
						m_SelectedItem = ITEM_TYPE::I_NOT;
					}
				}
			}
			cout << boolalpha << m_IsOnChatting << " mx - " << mx << " my - " << my << "\n";
		}
		else {
			//m_IsOnTypingID = false; 
			if (my >= 280 && my <= 330) {
				m_IsOnTypingID = false;
				SetCaretPos(360 + 7.8 * m_ServerIp.length(), 300); 
			}
			if (my >= 560 && my <= 610) {
				m_IsOnTypingID = true;
				SetCaretPos(360 + 7.8 * m_PlayerName.length(), 580);
			}  
		}

	}
	return 0;;
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
	m_CaretYPos = m_rtClient.bottom - 20;

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
		m_Player->SetHP(packet->HP);
		m_Player->SetLevel(packet->LEVEL);
		m_Player->SetExp(packet->EXP);
		m_Player->SetName(m_PlayerName.c_str());
		m_Player->SetGold(packet->GOLD);
		m_Player->MoveTo({ packet->x, packet->y }); 
		m_Player->SetType(ObjectType::Player);
		
		for (int i = 0; i < packet->itemCount; ++i) {
			m_Player->AddItem(packet->items[i]);
		}
		//avatar.SetName(packet->name);
		m_LeftX = packet->x - SCREEN_WIDTH * 0.5f;
		m_TopY = packet->y  - SCREEN_HEIGHT *0.5f;
		m_Player->Show();

		m_ToDrawObjects.emplace_back(m_Player);
		break;
	}
	case SC_LOGIN_FAIL:
		MessageBox(m_hWnd, L"이미 등록된 ID입니다!", L"로그인 실패!" , MB_OK);
		exit(1);
		break;
	case SC_POSITION:
	{		
		sc_packet_position* packet = reinterpret_cast<sc_packet_position*>(p_buf);
		//cout << "SC_POSITION - x : " << packet->x << " y : " << packet->y << "\n";
		int other_id = packet->id;
		m_Objects[other_id].SetDirection(packet->direction);
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

		m_Objects[packet->id].SetDirection(packet->direction);
		break;
	}
	case SC_CHAT:
	{
		//cout << "SC_CHAT\n";
		sc_packet_chat* packet = reinterpret_cast<sc_packet_chat*>(p_buf);
		cout << packet->message << endl;
		string chatMessage = "<";
		{
			if (packet->id == 0) {
				chatMessage += "SYSTEM";
			}
			else {
				chatMessage += m_Objects[packet->id].GetName();
			}
			chatMessage += "> : ";
			chatMessage += packet->message;
		}
		//m_ChatDatas.push_back(packet->message); 
		m_ChatDatas.insert(m_ChatDatas.begin(), chatMessage); 
		m_ChatDatasT.insert(m_ChatDatasT.begin(), StringToTCHAR(chatMessage));

		//m_ChatDatas.insert(m_ChatDatas.begin(), packet->message);
		if (m_ChatDatas.size() > 11) {
			m_ChatDatas.erase(m_ChatDatas.end() - 1);
			m_ChatDatasT.erase(m_ChatDatasT.end() - 1);
		}
		break;
	}
	case SC_STAT_CHANGE:
	{
		sc_packet_stat_change* packet = reinterpret_cast<sc_packet_stat_change*>(p_buf);
		 
		m_Player = &m_Objects[m_ClientId];
		m_Objects[packet->id].SetHP(packet->HP);
		m_Objects[packet->id].SetLevel(packet->LEVEL);
		m_Objects[packet->id].SetExp(packet->EXP); 
		m_Objects[packet->id].SetGold(packet->GOLD);
	}
		break;
	case SC_REMOVE_OBJECT:
	{
		//cout << "SC_REMOVE_OBJECT\n";
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
		sc_packet_add_object* packet = reinterpret_cast<sc_packet_add_object*>(p_buf);
		int id = packet->id;

		//cout << "SC_ADD_OBJECT - " << id <<"\n";
		if (id < MAX_USER) { 
			m_Objects[id].SetName(packet->name);
			m_Objects[id].SetHP(packet->HP);
			m_Objects[id].SetLevel(packet->LEVEL);
			m_Objects[id].SetExp(packet->EXP);
			m_Objects[id].SetType((ObjectType)packet->obj_class);
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

	case SC_ADD_ITEM:
	{
		sc_packet_add_item* packet = reinterpret_cast<sc_packet_add_item*>(p_buf);
		m_Player->AddItem(packet->itemType);
		cout << (int)packet->itemType;
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
	strcpy_s(packet.player_id, m_PlayerName.c_str()); 

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
void CFramework::SendAttackPacket()
{
	cs_packet_attack packet;
	packet.size = sizeof(packet);
	packet.type = CS_ATTACK;
	 
	SendPacket(&packet);
}
void CFramework::SendUseItem(int idx)
{
	cout << m_Player->GetItemCount()<< ", " << idx <<  " 아이템 사용하라고 \n";
	if (m_Player->GetItemCount() <= idx) {
		return;
	}
	m_Player->UseItem(idx);
	cs_packet_use_item packet;
	packet.size = sizeof(packet);
	packet.type = CS_USE_ITEM;
	packet.itemIndex = idx;
	SendPacket(&packet);
}
void CFramework::ReadMapData()
{
	ifstream fileIn("Resources/MapInfos.txt");
	for (int i = 0; i < WORLD_HEIGHT; ++i)
	{
		for (int j = 0; j < WORLD_WIDTH; ++j)
		{
			string text; fileIn >> text;
			m_TileDatas[i][j] = (MAP_TILE_DATA)stoi(text);
		}
	}

	cout << "맵 데이터 읽기 완료!\n";
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
 

void CFramework::DrawTransparent(HDC hdc,
	 BYTE alphaValue, void(CFramework::*drawFunc)(HDC))
{
	HDC LayDC;
	HBITMAP Lay;
	BLENDFUNCTION bf;

	bf.BlendOp = AC_SRC_OVER;
	bf.BlendFlags = 0;
	bf.AlphaFormat = 0;
	bf.SourceConstantAlpha = alphaValue;

	Lay = CreateCompatibleBitmap(hdc, m_rtClient.right, m_rtClient.bottom);
	LayDC = CreateCompatibleDC(hdc);
	SelectObject(LayDC, Lay);
	TransparentBlt(LayDC, 0, 0, m_rtClient.right, m_rtClient.bottom
		, hdc, m_rtClient.left, m_rtClient.top, m_rtClient.right, m_rtClient.bottom, NULL);
	
	(this->*drawFunc)(hdc); 

	m_ChatUi.TransparentBlt(hdc, 0, m_rtClient.bottom * 0.75f, m_rtClient.right, m_rtClient.bottom * 0.25f,
		0, 0, m_ChatUi.GetWidth(), m_ChatUi.GetHeight(), NULL);

	AlphaBlend(hdc, m_rtClient.left, m_rtClient.top, m_rtClient.right, m_rtClient.bottom
		, LayDC, 0, 0, m_rtClient.right, m_rtClient.bottom, bf);

	DeleteDC(LayDC);
	DeleteObject(Lay);
}

void CFramework::DrawTransparent(HDC hdc, int startX, int startY, int sizeX, int sizeY, BYTE alphaValue, const CImage& targetImage)
{
	HDC LayDC;
	HBITMAP Lay;
	BLENDFUNCTION bf;

	bf.BlendOp = AC_SRC_OVER;
	bf.BlendFlags = 0;
	bf.AlphaFormat = 0;
	bf.SourceConstantAlpha = alphaValue;

	Lay = CreateCompatibleBitmap(hdc, m_rtClient.right, m_rtClient.bottom);
	LayDC = CreateCompatibleDC(hdc);
	SelectObject(LayDC, Lay);
	TransparentBlt(LayDC, 0, 0, m_rtClient.right, m_rtClient.bottom
		, hdc, m_rtClient.left, m_rtClient.top, m_rtClient.right, m_rtClient.bottom, NULL);

	targetImage.TransparentBlt(hdc, startX, startY, sizeX, sizeY,
		0, 0, targetImage.GetWidth(), targetImage.GetHeight(), NULL); 

	AlphaBlend(hdc, m_rtClient.left, m_rtClient.top, m_rtClient.right, m_rtClient.bottom
		, LayDC, 0, 0, m_rtClient.right, m_rtClient.bottom, bf);
	 
	DeleteDC(LayDC);
	DeleteObject(Lay);
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
 
void CFramework::DrawChatUi(HDC hdc)
{
	DrawTransparent(hdc, m_rtClient.right * 0.1f, m_rtClient.bottom * 0.75f,
		m_rtClient.right * 0.8f, m_rtClient.bottom * 0.25f, 55, m_ChatUi);

	wstring wide_string = wstring(m_ChatData.begin(), m_ChatData.end());
	 
	m_ChatDataT = StringToTCHAR(m_ChatData);
	TextOut(m_hdc, m_rtClient.right * 0.1f + 10, m_CaretYPos,
		m_ChatDataT, _tcslen(m_ChatDataT));
		 
	for (int i = 0; i < m_ChatDatasT.size(); ++i) { 
		TextOut(m_hdc, m_rtClient.right * 0.1f + 10, m_CaretYPos - 18 - 15 * (i + 1),
			m_ChatDatasT[i], _tcslen(m_ChatDatasT[i]));
	}
}

void CFramework::DrawPlayerInfo(HDC hdc)
{
	if (m_Player == nullptr) {
		return;
	}	
	
	DrawTransparent(hdc, 0, 0, 256, 92, 55, m_PlayerUiBoard);

	DrawTransparent(hdc, m_rtClient.right - 256, 0, 256, 92, 55, m_ItemUIImage);
	m_Player->DrawItems(hdc);

	wstring wGold = to_wstring(m_Player->GetGold());
	TextOut(hdc, m_rtClient.right - 220, 62, wGold.c_str(), lstrlen(wGold.c_str()));

	wstring wLV = L"LV - ";
	wLV += to_wstring(m_Player->GetLevel());
	wstring wHP = L"HP - ";
	wHP += to_wstring(m_Player->GetHP());
	wstring wEXP = L"EXP - ";
	wEXP += to_wstring(m_Player->GetExp());
	TextOut(hdc, 70, 10, wLV.c_str(), lstrlen(wLV.c_str()));
	TextOut(hdc, 70, 38, wHP.c_str(), lstrlen(wHP.c_str()));
	TextOut(hdc, 70, 65, wEXP.c_str(), lstrlen(wEXP.c_str()));

	string name = m_Player->GetName();
	auto tileIdx = m_Player->GetTileIndex();

	wstring wName = L"ID - " + wstring(name.begin(), name.end());
	wstring posX = L"X - " + to_wstring(tileIdx.x); 
	wstring posY = L"Y - " + to_wstring(tileIdx.y); 
	TextOut(hdc, 150, 10, wName.c_str(), lstrlen(wName.c_str()));
	TextOut(hdc, 150, 38, posX.c_str(), lstrlen(posX.c_str()));
	TextOut(hdc, 150, 65, posY.c_str(), lstrlen(posY.c_str()));
}
