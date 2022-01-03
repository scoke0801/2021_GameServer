#pragma once
#include "Scene.h"

class CGameScene :  public CScene 
{
public:
	CGameScene();
	~CGameScene();

	virtual void SendDataToNextScene(void* pContext) {}

public:
	virtual void Update(float timeElapsed);
	virtual void Draw(HDC hdc);

	virtual void Communicate(SOCKET& sock);

	virtual LRESULT ProcessWindowInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	virtual void ProcessMouseClick(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void ProcessMouseInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {};
	virtual void ProcessKeyboardUpInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {}
	virtual void ProcessKeyboardDownInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {}
	virtual void ProcessCHARInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	void ReadMapData();

	void DrawMap(HDC hdc);
	void DrawChatUi(HDC hdc);

	void DrawPlayerInfo(HDC hdc);

	// 서버로부터 받은 데이터를 패킷단위로 처리
	void ProcessPacket(unsigned char* p_buf) override;

	void SendMovePacket(DIRECTION dir);
	void SendChatPacket();
	void SendAttackPacket();
	void SendUseItem(int idx);
	 
private:
	CImage m_Background;
	CImage m_TileImage;
	CImage m_ChatUi;
	CImage m_PlayerUiBoard;
	CImage m_ItemUIImage;
	CImage m_Items;
	CImage m_SellerUI;

	array<CGameObject, MAX_USER> m_Objects;
	array<CGameObject, 400 > m_Npcs;
	vector<CGameObject*> m_ToDrawObjects;

	CGameObject* m_Player = nullptr;

	string  m_ChatData;
	TCHAR* m_ChatDataT = NULL;

	vector<TCHAR*> m_ChatDatasT;
	vector<string> m_ChatDatas;

	bool m_IsOnChatting = false;
	bool m_IsSellerClicked = false;
	ITEM_TYPE m_SelectedItem = ITEM_TYPE::I_NOT;

	MAP_TILE_DATA m_TileDatas[WORLD_HEIGHT][WORLD_WIDTH];

	// 맵 타일을 그리기 위한 위치
	short m_LeftX = 0;
	short m_TopY = 0;
};

