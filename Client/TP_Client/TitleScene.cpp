#include "stdafx.h"
#include "TitleScene.h"

 
CTitleScene::CTitleScene() : CScene()
{ 
	m_Type = SceneType::TitleScene;

	m_TitleImage.Load(L"Resources/Title.png");

	m_PlayerName = "Test";
}

CTitleScene::~CTitleScene()
{ 
}

void CTitleScene::Update(float timeElapsed)
{ 
}

void CTitleScene::Draw(HDC hdc)
{
	m_TitleImage.TransparentBlt(hdc, 0, 0, m_rtClient.right, m_rtClient.bottom,
		0, 0, m_TitleImage.GetWidth(), m_TitleImage.GetHeight(), NULL);

	Rectangle(hdc, 0, 280, m_rtClient.right, 330);
	Rectangle(hdc, 0, 560, m_rtClient.right, 610);
	wstring wide_string = wstring(m_ServerIp.begin(), m_ServerIp.end());
	TextOut(hdc, 250, 300, L"Enter ServerIP : ", lstrlen(L"Enter ServerIP : "));
	TextOut(hdc, 250 + 110, 300, wide_string.c_str(), lstrlen(wide_string.c_str()));

	wide_string = wstring(m_PlayerName.begin(), m_PlayerName.end());
	TextOut(hdc, 250, 580, L"Enter PlayerID : ", lstrlen(L"Enter PlayerID : "));
	TextOut(hdc, 250 + 110, 580, wide_string.c_str(), lstrlen(wide_string.c_str()));
}

LRESULT CTitleScene::ProcessWindowInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	DWORD bytes_sent, bytes_recv, recv_flag = 0;
	WSABUF s_wsabuf[1], r_wsabuf[1];

	HIMC m_hIMC = NULL;   // IME �ڵ�

	TCHAR Text[255];     // �ؽ�Ʈ�� �����ϱ����� ����
	TCHAR  Cstr[10];      // �������� ����!! 
	int len;
	switch (message)
	{
	case WM_CREATE:
		CreateCaret(hWnd, NULL, 5, 15);
		ShowCaret(hWnd);
		SetCaretPos(360 + 7.8 * m_ServerIp.length(), m_CaretYPos);
		return 0;
	case WM_CHAR:
		switch (wParam)
		{
		case VK_a:
		case VK_A:
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
			return 0;
		case VK_RETURN:
				if (!m_isServerConnected)
				{
					if (m_PlayerName.size() == 0) {
						return 0;
					}

					if (CFramework::GetInstance().ConnectToServer())
					{
						//SetCaretPos(m_rtClient.right * 0.1f + 10 + 7.8 * m_ChatData.length(), m_CaretYPos);
						SendLoginPacket();
					}
				}
			return 0;

		default:
			if (m_IsOnTypingID) {
				m_PlayerName += static_cast<char>(wParam);
				SetCaretPos(360 + 7.8 * m_PlayerName.length(), 580); 
			}
		}
		return 0; 
	case WM_LBUTTONDOWN:
	{
		int mx = LOWORD(lParam);
		int my = HIWORD(lParam);

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
	return 0;
	}
}

void CTitleScene::ProcessPacket(unsigned char* p_buf)
{
	//char buf[10000];
	/////unsigned char p_size = reinterpret_cast<unsigned char*>(p_buf)[0];
	//unsigned char p_type = reinterpret_cast<unsigned char*>(p_buf)[1];
	//switch (p_type)
	//{
	//case SC_LOGIN_OK:
	//{
	//	sc_packet_login_ok* packet = reinterpret_cast<sc_packet_login_ok*>(p_buf);

	//	//cout << "LoginOk - x : " << packet->x << " y : " << packet->y << "\n";
	//	m_ClientId = packet->id;
	//	m_Player = &m_Objects[m_ClientId];
	//	m_Player->SetHP(packet->HP);
	//	m_Player->SetLevel(packet->LEVEL);
	//	m_Player->SetExp(packet->EXP);
	//	m_Player->SetName(m_PlayerName.c_str());
	//	m_Player->SetGold(packet->GOLD);
	//	m_Player->MoveTo({ packet->x, packet->y });
	//	m_Player->SetType(ObjectType::Player);

	//	for (int i = 0; i < packet->itemCount; ++i) {
	//		m_Player->AddItem(packet->items[i]);
	//	}
	//	//avatar.SetName(packet->name);
	//	m_LeftX = packet->x - SCREEN_WIDTH * 0.5f;
	//	m_TopY = packet->y - SCREEN_HEIGHT * 0.5f;
	//	m_Player->Show();

	//	m_ToDrawObjects.emplace_back(m_Player);
	//	break;
	//}
	//case SC_LOGIN_FAIL:
	//	MessageBox(m_hWnd, L"�̹� ��ϵ� ID�Դϴ�!", L"�α��� ����!", MB_OK);
	//	exit(1);
	//	break;
	//}
}

void CTitleScene::Communicate(SOCKET& sock)
{ 
}

void CTitleScene::ProcessMouseClick(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{ 

}

void CTitleScene::ProcessCHARInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{ 

}

void CTitleScene::SendLoginPacket()
{
	cs_packet_login packet;
	packet.size = sizeof(packet);
	packet.type = CS_LOGIN;
	strcpy_s(packet.player_id, m_PlayerName.c_str());

	SendPacket(&packet);
}