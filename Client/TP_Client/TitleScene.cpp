#include "stdafx.h"
#include "TitleScene.h"
#include "GameScene.h"
 
CTitleScene::CTitleScene() : CScene()
{ 
	m_Type = SceneType::TitleScene;

	m_TitleImage.Load(L"Resources/Title.png");
	m_LoginUI.Load(L"Resources/UiBox.png");
	m_PlayerName = "Test";

	m_UiPos[0].x = m_UiPos[1].x = (m_rtClient.right / 5);
	m_UiPos[0].y = 283; 
	m_UiPos[1].y = 563;

	m_UiSize.x = (m_rtClient.right / 3) * 2;
	m_UiSize.y = 50;
	m_UiAlpha = 255 * 0.60f;
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

	DrawTransparent(hdc, m_UiPos[0].x, m_UiPos[0].y, m_UiSize.x, m_UiSize.y, m_UiAlpha, m_LoginUI);
	DrawTransparent(hdc, m_UiPos[1].x, m_UiPos[1].y, m_UiSize.x, m_UiSize.y, m_UiAlpha, m_LoginUI);

	//Rectangle(hdc, 0, 280, m_rtClient.right, 330);
	//Rectangle(hdc, 0, 560, m_rtClient.right, 610);

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

	HIMC m_hIMC = NULL;   // IME 핸들

	TCHAR Text[255];     // 텍스트를 저장하기위한 변수
	TCHAR  Cstr[10];      // 조합중인 문자!! 
	int len;
	switch (message)
	{
	case WM_CREATE:
		//CreateCaret(hWnd, NULL, 5, 15);
		//ShowCaret(hWnd);
		//SetCaretPos(360 + 7.8 * m_ServerIp.length(), m_CaretYPos);
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
	char buf[10000];
	///unsigned char p_size = reinterpret_cast<unsigned char*>(p_buf)[0];
	unsigned char p_type = reinterpret_cast<unsigned char*>(p_buf)[1];
	switch (p_type)
	{
	case SC_LOGIN_OK:
	{
		sc_packet_login_ok* packet = reinterpret_cast<sc_packet_login_ok*>(p_buf);

		//cout << "LoginOk - x : " << packet->x << " y : " << packet->y << "\n";
		DATA_TITLE_FROM_TO_GAMESCENE data;
		data.packet = *packet;
		data.player_name = m_PlayerName;
		CFramework::GetInstance().ChangeScene<CGameScene>((void*)&data);
		 
		break;
	}
	case SC_LOGIN_FAIL:
		//MessageBox(m_hWnd, L"이미 등록된 ID입니다!", L"로그인 실패!", MB_OK);
		exit(1);
		break;
	}
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

	CFramework::GetInstance().SendPacket(&packet);
}