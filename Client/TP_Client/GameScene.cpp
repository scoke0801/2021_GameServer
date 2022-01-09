#include "stdafx.h"
#include "GameScene.h"


CGameScene::CGameScene() : CScene()
{	
	m_Type = SceneType::GameScene;

	m_Background.Load(L"Resources/Background.png");
	m_TileImage.Load(L"Resources/GameTileX.png");
	m_ChatUi.Load(L"Resources/ChatUi.png");
	m_PlayerUiBoard.Load(L"Resources/PlayerUiBoard.png");
	m_ItemUIImage.Load(L"Resources/item_ui.png");
	m_Items.Load(L"Resources/Items.png");
	m_SellerUI.Load(L"Resources/SellerUI.png");
	 
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

	m_CaretYPos = m_rtClient.bottom - 20;

	ReadMapData();
}

CGameScene::~CGameScene()
{
}

void CGameScene::SendDataToNextScene(void* pContext)
{
	DATA_TITLE_FROM_TO_GAMESCENE data = *(DATA_TITLE_FROM_TO_GAMESCENE*)pContext;
	sc_packet_login_ok packet = data.packet; 

	m_ClientId = packet.id;
	m_Player = &m_Objects[m_ClientId];
	m_Player->SetHP(packet.HP);
	m_Player->SetLevel(packet.LEVEL);
	m_Player->SetExp(packet.EXP);
	m_Player->SetName(data.player_name.c_str());
	m_Player->SetGold(packet.GOLD);
	m_Player->MoveTo({ packet.x, packet.y });
	m_Player->SetType(ObjectType::Player);

	for (int i = 0; i < packet.itemCount; ++i) {
		m_Player->AddItem(packet.items[i]);
	}
	//avatar.SetName(packet->name);
	m_LeftX = packet.x - SCREEN_WIDTH * 0.5f;
	m_TopY = packet.y - SCREEN_HEIGHT * 0.5f;
	m_Player->Show();

	m_ToDrawObjects.emplace_back(m_Player);
}

void CGameScene::Update(float timeElapsed)
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

void CGameScene::Draw(HDC hdc)
{
	DrawMap(hdc);
	for (auto& pObjects : m_ToDrawObjects)
	{
		pObjects->Draw(hdc,
			m_LeftX * TILE_WIDTH,
			m_TopY * TILE_WIDTH);
	}
	for (auto& pObjects : m_Npcs)
	{
		pObjects.Draw(hdc,
			m_LeftX * TILE_WIDTH,
			m_TopY * TILE_WIDTH);
	}
	DrawPlayerInfo(hdc);
	// 가려지도록
	DrawChatUi(hdc);

	if (m_IsSellerClicked) {
		m_SellerUI.TransparentBlt(hdc, m_rtClient.right * 0.1f, m_rtClient.bottom * 0.15f,
			m_rtClient.right * 0.8f, m_rtClient.bottom * 0.55f,
			0, 0, m_SellerUI.GetWidth(), m_SellerUI.GetHeight(), RGB(255, 0, 255));
	}
}

void CGameScene::Communicate(SOCKET& sock)
{
}

void CGameScene::ProcessMouseClick(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

}

void CGameScene::ProcessCHARInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

}
void CGameScene::ReadMapData()
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


void CGameScene::DrawMap(HDC hdc)
{
	for (int i = 0; i < 20; ++i) {
		if (m_TopY + i >= WORLD_HEIGHT || m_TopY + i < 0) {
			continue;
		}
		for (int j = 0; j < 20; ++j) {
			if (m_LeftX + j >= WORLD_WIDTH || m_LeftX + j < 0) {
				continue;
			}
			m_TileImage.StretchBlt(hdc,
				j * TILE_WIDTH, i * TILE_WIDTH, TILE_WIDTH, TILE_WIDTH,
				(int)m_TileDatas[m_TopY + i][m_LeftX + j] * TILE_WIDTH, 0, TILE_WIDTH, TILE_WIDTH);
		}
	}
}

void CGameScene::DrawChatUi(HDC hdc)
{
	DrawTransparent(hdc, m_rtClient.right * 0.1f, m_rtClient.bottom * 0.75f,
		m_rtClient.right * 0.8f, m_rtClient.bottom * 0.25f, 55, m_ChatUi);

	wstring wide_string = wstring(m_ChatData.begin(), m_ChatData.end());

	m_ChatDataT = StringToTCHAR(m_ChatData);
	TextOut(hdc, m_rtClient.right * 0.1f + 10, m_CaretYPos,
		m_ChatDataT, _tcslen(m_ChatDataT));

	for (int i = 0; i < m_ChatDatasT.size(); ++i) {
		TextOut(hdc, m_rtClient.right * 0.1f + 10, m_CaretYPos - 18 - 15 * (i + 1),
			m_ChatDatasT[i], _tcslen(m_ChatDatasT[i]));
	}
}

void CGameScene::DrawPlayerInfo(HDC hdc)
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

LRESULT CGameScene::ProcessWindowInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	DWORD bytes_sent, bytes_recv, recv_flag = 0;
	WSABUF s_wsabuf[1], r_wsabuf[1];

	HIMC m_hIMC = NULL;   // IME 핸들

	TCHAR Text[255];     // 텍스트를 저장하기위한 변수
	TCHAR  Cstr[10];      // 조합중인 문자!! 
	int len;
	switch (message)
	{
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
					sprintf_s(szTemp, "완성된 글자 : %s\r\n", Text);
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
				sprintf_s(szTemp, "조합중인 글자 : %s\r\n", Cstr);
				//OutputDebugString(_T(szTemp));
			}
		}

		ImmReleaseContext(hWnd, m_hIMC);	// IME 핸들 반환!!
		return 0;
	case WM_CHAR:
	{
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


			break;
		case VK_BACK:
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

			return 0;
		case VK_RETURN:
			if (false == m_IsOnChatting) {
				return 0;
			}
			SendChatPacket();
			m_ChatData.clear();
			//SetCaretPos(m_rtClient.right * 0.1f + 10 + 7.8 * m_ChatData.length(), m_CaretYPos);

			return 0;

		default:
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
		return 0;
	case WM_LBUTTONDOWN:
	{
		int mx = LOWORD(lParam);
		int my = HIWORD(lParam);


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
				else if (my > m_rtClient.bottom * 0.26f && my < m_rtClient.bottom * 0.35f) {
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
						CFramework::GetInstance().SendPacket(&packet);
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

	return 0;
	}

	return 0;
}

void CGameScene::ProcessPacket(unsigned char* p_buf)
{
	char buf[10000];
	///unsigned char p_size = reinterpret_cast<unsigned char*>(p_buf)[0];
	unsigned char p_type = reinterpret_cast<unsigned char*>(p_buf)[1];
	switch (p_type)
	{ 
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
		 
		if (packet->isAttackState) {
			STATE state = m_Objects[packet->attacker].GetState();
			if (state != STATE::ATTACK) {
				m_Objects[packet->attacker].SetState(STATE::ATTACK);
			}
			if (packet->attacker == packet->id) {
				break;
			}

			DIRECTION dir;
			Vector2i my_pos = m_Objects[packet->id].GetTileIndex();
			Vector2i attacker_pos = m_Objects[packet->attacker].GetTileIndex();
			if (my_pos.x < attacker_pos.x) {
				m_Objects[packet->attacker].SetDirection(DIRECTION::D_W);
			}
			else if (my_pos.x > attacker_pos.x) {
				m_Objects[packet->attacker].SetDirection(DIRECTION::D_E);
			}
			else if (my_pos.y < attacker_pos.y) {
				m_Objects[packet->attacker].SetDirection(DIRECTION::D_N);
			}
			else if (my_pos.y > attacker_pos.y) {
				m_Objects[packet->attacker].SetDirection(DIRECTION::D_S);
			}
		}
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

void CGameScene::SendMovePacket(DIRECTION dir)
{
	cs_packet_move packet;
	packet.size = sizeof(packet);
	packet.type = CS_MOVE;
	packet.direction = dir;
	CFramework::GetInstance().SendPacket(&packet);
}
void CGameScene::SendChatPacket()
{
	if (m_ChatData.size() == 0) {
		// 채팅 데이터가 없을 때는 반환
		return;
	}
	cs_packet_chat packet;
	packet.size = sizeof(packet);
	packet.type = CS_CHAT;
	strcpy_s(packet.message, m_ChatData.c_str());
	CFramework::GetInstance().SendPacket(&packet);
}
void CGameScene::SendAttackPacket()
{
	cs_packet_attack packet;
	packet.size = sizeof(packet);
	packet.type = CS_ATTACK;

	CFramework::GetInstance().SendPacket(&packet);
}
void CGameScene::SendUseItem(int idx)
{
	cout << m_Player->GetItemCount() << ", " << idx << " 아이템 사용하라고 \n";
	if (m_Player->GetItemCount() <= idx) {
		return;
	}
	m_Player->UseItem(idx);
	cs_packet_use_item packet;
	packet.size = sizeof(packet);
	packet.type = CS_USE_ITEM;
	packet.itemIndex = idx;
	CFramework::GetInstance().SendPacket(&packet);
}
