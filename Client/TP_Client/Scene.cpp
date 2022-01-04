#include "stdafx.h"
#include "Scene.h"

#include "TitleScene.h"
#include "TestScene.h"
#include "GameScene.h"


CScene::CScene()
{ 
	m_isServerConnected = CFramework::GetInstance().IsServerConnected();
	m_ClientId = CFramework::GetInstance().GetClientId();
	m_ServerIp = CFramework::GetInstance().GetServerIP();
	m_rtClient = CFramework::GetInstance().GetClientSize();
}

CScene::~CScene()
{
}

bool CScene::ProcessInput(UCHAR* pKeysBuffer)
{
	return false;
}
 
void CScene::DrawTransparent(HDC hdc, int startX, int startY, int sizeX, int sizeY, BYTE alphaValue, const CImage& targetImage)
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

CNullScene::CNullScene()
{
	//m_Image.Load(_T("assets/NullSceneInfoImage.png"));
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

