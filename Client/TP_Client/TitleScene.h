#pragma once
#include "Scene.h"

class CTitleScene : public CScene
{
public:
	CTitleScene();
	~CTitleScene();

	virtual void SendDataToNextScene(void* pContext) {}

public:
	virtual void Update(float timeElapsed);
	virtual void Draw(HDC hdc);


	// 서버로부터 받은 데이터를 패킷단위로 처리
	void ProcessPacket(unsigned char* p_buf) override; 

	virtual void Communicate(SOCKET& sock);

	virtual LRESULT ProcessWindowInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void ProcessMouseClick(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void ProcessMouseInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {};
	virtual void ProcessKeyboardUpInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {}
	virtual void ProcessKeyboardDownInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {}
	virtual void ProcessCHARInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	void SendLoginPacket();

private: 
	CImage m_TitleImage;

	string	m_PlayerName;
	bool m_IsOnTypingID = false;
};

