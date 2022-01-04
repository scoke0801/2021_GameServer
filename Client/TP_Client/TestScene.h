#pragma once
#include "Scene.h"
class CTestScene : public CScene
{
public:
	CTestScene();
	~CTestScene();

	virtual void SendDataToNextScene(void* pContext) {}

public:
	virtual void Update(float timeElapsed);
	virtual void Draw(HDC hdc);

	// �����κ��� ���� �����͸� ��Ŷ������ ó��
	void ProcessPacket(unsigned char* p_buf) override;

	virtual void Communicate(SOCKET& sock);

	virtual LRESULT ProcessWindowInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void ProcessMouseClick(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void ProcessMouseInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {};
	virtual void ProcessKeyboardUpInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {}
	virtual void ProcessKeyboardDownInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {}
	virtual void ProcessCHARInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	void ReadMapData();

private:

	CGameObject* m_Player = nullptr;
	MAP_TILE_DATA m_TileDatas[WORLD_HEIGHT][WORLD_WIDTH];

	// �� Ÿ���� �׸��� ���� ��ġ
	short m_LeftX = 0;
	short m_TopY = 0;

	CImage m_TileImage;
};

