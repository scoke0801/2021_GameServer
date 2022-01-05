#pragma once

#include "Framework.h"

enum class SceneType
{
	TitleScene = 0, 
	GameScene, 
	NullScene
};

struct DATA_TITLE_FROM_TO_GAMESCENE {
	sc_packet_login_ok packet;
	string player_name;
};

class CScene
{ 
public:
	CScene();
	virtual ~CScene();

	void Init(RECT rt, CFramework* framework) { m_rtClient = rt, m_pFramework = framework; }

	virtual void SendDataToNextScene(void* pContext) {}

protected:
	// 투명하게 그릴 때, 이미지를 넘겨주고 직접 그리게 
	void DrawTransparent(HDC hdc, int startX, int startY, int sizeX, int sizeY, BYTE alphaValue, const CImage& targetImage);
	 
public:
	virtual void Update(float timeElapsed) = 0;
	virtual void Draw(HDC hdc) = 0;

	// 서버로부터 받은 데이터를 패킷단위로 처리
	virtual void ProcessPacket(unsigned char* p_buf) {}

	virtual void Communicate(SOCKET& sock) = 0;

	virtual bool ProcessInput(UCHAR* pKeysBuffer);

	virtual LRESULT ProcessWindowInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) { return 0; }

	virtual void ProcessMouseClick(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {}
	virtual void ProcessMouseInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {}
	virtual void ProcessKeyboardUpInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {}
	virtual void ProcessKeyboardDownInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {}
	virtual void ProcessCHARInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {}

protected:
	template<class SceneName>
	void ChangeScene(void* pContext = nullptr) {
		m_pFramework->ChangeScene<SceneName>(pContext); 
	}

private:
	CFramework* m_pFramework;

protected:
	RECT	m_rtClient;
	SceneType m_Type;
	void*	m_Context;

	bool	m_isServerConnected = false;
	short	m_ClientId = -1;
	string	m_ServerIp = "127.0.0.1";

	short	m_CaretYPos = 300;
};


class CNullScene : public CScene
{
private:
	CImage m_Image;

	int m_ID = 0;
	int m_Idx = 0;

public:
	 
	CNullScene();

	virtual void Update(float timeElapsed) {}
	virtual void Draw(HDC hdc);

	virtual void Communicate(SOCKET& sock);

	virtual bool ProcessInput(UCHAR* pKeysBuffer) { return false; }

	UINT GetIndex() const { return m_Idx; }

	virtual LRESULT ProcessWindowInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) { return 0; }
	virtual void ProcessMouseInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {}
	virtual void ProcessKeyboardUpInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {}
	virtual void ProcessKeyboardDownInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void ProcessCHARInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {}
};