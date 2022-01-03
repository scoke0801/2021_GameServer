#include "stdafx.h"
#include "TestScene.h"

CTestScene::CTestScene()
{
	m_Type = SceneType::TitleScene;
}

CTestScene::~CTestScene()
{
}

void CTestScene::Update(float timeElapsed)
{
}

void CTestScene::Draw(HDC hdc)
{
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