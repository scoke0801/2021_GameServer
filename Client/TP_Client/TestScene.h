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

	// 서버로부터 받은 데이터를 패킷단위로 처리
	void ProcessPacket(unsigned char* p_buf) override;

	virtual void Communicate(SOCKET& sock);

	virtual LRESULT ProcessWindowInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) { return 0; }
	virtual void ProcessMouseClick(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void ProcessMouseInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {};
	virtual void ProcessKeyboardUpInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {}
	virtual void ProcessKeyboardDownInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {}
	virtual void ProcessCHARInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
};

