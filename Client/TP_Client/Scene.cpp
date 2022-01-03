#include "stdafx.h"
#include "Scene.h"

#include "TitleScene.h"
#include "TestScene.h"
#include "GameScene.h"


CScene::CScene()
{
}

CScene::~CScene()
{
}

bool CScene::ProcessInput(UCHAR* pKeysBuffer)
{
	return false;
}

CNullScene::CNullScene()
{
	m_Image.Load(_T("assets/NullSceneInfoImage.png"));
	m_Type = SceneType::NullScene;
}

void CNullScene::Draw(HDC hdc)
{
	RECT rt{ 170, 60, m_Image.GetWidth() + 170, m_Image.GetHeight() + 60 };
	m_Image.Draw(hdc, rt);
}

void CNullScene::Communicate(SOCKET& sock)
{
	 
}

void CNullScene::ProcessKeyboardDownInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case VK_F1:	// 0
		ChangeScene<CTitleScene>();
		break;

	case VK_F2:	// 0
		ChangeScene<CGameScene>();
		break;

	case VK_F3:	// 0
		ChangeScene<CTestScene>();
		break;
	}
}

