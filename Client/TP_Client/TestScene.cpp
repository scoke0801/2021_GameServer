#include "stdafx.h"
#include "TestScene.h"

CTestScene::CTestScene() : CScene()
{
	m_Type = SceneType::TitleScene;
	m_TileImage.Load(L"Resources/GameTileX.png");

	m_Player = new CGameObject();	
	m_Player->SetType(ObjectType::Player);
	//m_Player->SetType(ObjectType::Mon_Peace_Roaming);
	//m_Player->SetType(ObjectType::Mon_Agro_Roaming);
	m_Player->Show();
	m_Player->MoveTo({ 10,10 });
}

CTestScene::~CTestScene()
{
}

void CTestScene::Update(float timeElapsed)
{
	m_Player->Update(timeElapsed);
}

void CTestScene::Draw(HDC hdc)
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
	m_Player->Draw(hdc,
		m_LeftX * TILE_WIDTH,
		m_TopY * TILE_WIDTH);
}

void CTestScene::ProcessPacket(unsigned char* p_buf)
{
}

void CTestScene::Communicate(SOCKET& sock)
{
}

LRESULT CTestScene::ProcessWindowInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_KEYDOWN: 
	{
		Vector2i tileIndex = m_Player->GetTileIndex();

		switch (wParam)
		{
		case VK_LEFT:
			m_Player->MoveTo({tileIndex.x - 1, tileIndex.y }); 
			m_Player->SetDirection(DIRECTION::D_W);
			break;
		case VK_RIGHT:
			m_Player->MoveTo({ tileIndex.x + 1, tileIndex.y });
			m_Player->SetDirection(DIRECTION::D_E);
			break;
		case VK_UP:
			m_Player->MoveTo({ tileIndex.x, tileIndex.y-1 });
			m_Player->SetDirection(DIRECTION::D_N);
			break;
		case VK_DOWN:
			m_Player->MoveTo({ tileIndex.x, tileIndex.y+1 });
			m_Player->SetDirection(DIRECTION::D_S);
			break;
		case VK_a:
		case VK_A:
			m_Player->SetState(STATE::ATTACK);
			break;
		}

		tileIndex = m_Player->GetTileIndex();
		m_LeftX = tileIndex.x - SCREEN_WIDTH * 0.5f;
		m_TopY = tileIndex.y - SCREEN_HEIGHT * 0.5f;
	}
			return 0;
		case WM_LBUTTONDOWN:
		{
			int mx = LOWORD(lParam);
			int my = HIWORD(lParam);
		return 0;
		}
	}
	return 0;
}

void CTestScene::ProcessMouseClick(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

}

void CTestScene::ProcessCHARInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

}

void CTestScene::ReadMapData()
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