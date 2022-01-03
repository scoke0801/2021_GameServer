#pragma once

#include "Framework.h"

enum class SceneType
{
	TitleScene = 0, 
	GameScene, 
	NullScene
};

class CScene
{
private:
	CFramework* m_pFramework;

protected:
	RECT m_rtClient;
	SceneType m_Type;
	void* m_Context;

protected:
	template<class SceneName>
	void ChangeScene(void* pContext = nullptr) { 
		//m_pFramework->ChangeScene<SceneName>(pContext); 
	}

public:
	CScene();
	virtual ~CScene();

	void Init(RECT rt, CFramework* framework) { m_rtClient = rt, m_pFramework = framework; }

	virtual void SendDataToNextScene(void* pContext) {}

public:
	virtual void Update(float timeElapsed) = 0;
	virtual void Draw(HDC hdc) = 0;

	virtual void Communicate(SOCKET& sock) = 0;

	virtual bool ProcessInput(UCHAR* pKeysBuffer);

	virtual LRESULT ProcessWindowInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) { return 0; }
	virtual void ProcessMouseClick(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {}
	virtual void ProcessMouseInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {}
	virtual void ProcessKeyboardUpInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {}
	virtual void ProcessKeyboardDownInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {}
	virtual void ProcessCHARInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {}
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