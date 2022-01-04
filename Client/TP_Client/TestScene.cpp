#include "stdafx.h"
#include "TestScene.h"

CTestScene::CTestScene() : CScene()
{
	m_Type = SceneType::TitleScene;
	m_TileImage.Load(L"Resources/GameTileX.png");
}

CTestScene::~CTestScene()
{
}

void CTestScene::Update(float timeElapsed)
{
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
}

void CTestScene::ProcessPacket(unsigned char* p_buf)
{
}

void CTestScene::Communicate(SOCKET& sock)
{
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