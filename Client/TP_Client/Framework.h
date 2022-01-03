#pragma once
#include "GameObject.h"

class CScene;

constexpr int MAX_BUFFER = 1024;
constexpr int MAX_NAME_LEN = 200;
struct EX_OVER {
	WSAOVERLAPPED   m_over;
	WSABUF          m_wsabuf;
	unsigned char   m_packetbuf[MAX_BUFFER];
	SOCKET m_csocket; // OP_ACCEPT 에서만 사용
};

struct SESSION {
	EX_OVER		m_recv_over;
	SOCKET		m_socket;

	int			m_prev_size;

	char		m_name[MAX_NAME_LEN];
};
 
class CFramework
{
private:
	CScene* m_CurScene;
	CImage m_Background;
	CImage m_TileImage;
	CImage m_ChatUi;
	CImage m_PlayerUiBoard;
	CImage m_ItemUIImage;
	CImage m_Items;
	CImage m_SellerUI;
	CImage m_TitleImage;

	array<CGameObject, MAX_USER> m_Objects;
	array<CGameObject, 400 > m_Npcs;
	vector<CGameObject*> m_ToDrawObjects;

	CGameObject* m_Player = nullptr;

	bool m_IsOnChatting = false;
	bool m_IsOnTypingID = false;
	bool m_IsSellerClicked = false;
	ITEM_TYPE m_SelectedItem = ITEM_TYPE::I_NOT;
	short m_CaretYPos = 300;

	MAP_TILE_DATA m_TileDatas[WORLD_HEIGHT][WORLD_WIDTH];

	// 맵 타일을 그리기 위한 위치
	short m_LeftX = 0;
	short m_TopY = 0;

private:
	string	m_ServerIp = "127.0.0.1";
	string	m_PlayerName;
	string  m_ChatData;
	TCHAR* m_ChatDataT = NULL;

	vector<TCHAR*> m_ChatDatasT;
	vector<string> m_ChatDatas;

	bool	m_isServerConnected = false;

	SOCKET	m_SocketServer;

	short	m_ClientId = -1;
private:
	HWND m_hWnd;
	HINSTANCE m_hInst;

	RECT m_rtClient;

	// 더블버퍼링 처리를 위한 변수입니다.
	HDC m_hdc;
	HBITMAP m_hbmp;

	// 시간 처리를 위한 변수입니다. 
	std::chrono::system_clock::time_point m_currentTime[2];
	std::chrono::duration<double> m_timeElapsed;   // 시간이 얼마나 지났나

	std::chrono::duration<double> m_serverUpdated; // 시간이 얼마나 지났나 
	double m_dLag;
	double m_fps;

	// 타이틀바 출력 관련 변수입니다.
	TCHAR m_captionTitle[50];
	int m_titleLength;
	std::chrono::system_clock::time_point m_lastUpdateTime;
	std::chrono::duration<double> m_updateElapsed;

private:
	SESSION m_Client;

private:
	void BuildScene();
	void InitBuffers();

private:
	CFramework();
	~CFramework();

	CFramework(const CFramework& other) = delete;
	CFramework& operator=(const CFramework& other) = delete;
public:
	static CFramework& GetInstance() {
		static CFramework self;
		return self;
	}
	void init(HWND hWnd, HINSTANCE hInst);

public:
	void preUpdate();
	void update(float timeElapsed);
	void draw(HDC hdc);

	LRESULT ProcessWindowInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	// WM_SOCKET 메시지 받은 경우 이를 처리
	void ProcessCommunication(WPARAM wParam, LPARAM lParam);

	// 입력받은 IP주소로 서버와 연결
	bool ConnectToServer();

	// 서버로부터 데이터를 받아옴
	void DoRecv();

	// 서버로부터 받은 데이터를 패킷단위로 처리
	void ProcessPacket(unsigned char* p_buf);

	bool SendPacket(void* p);
	bool SendPacket(SOCKET& sock, char* packet, int packetSize, int& retVal);

	void SendLoginPacket();
	void SendMovePacket(DIRECTION dir);
	void SendChatPacket();
	void SendAttackPacket();
	void SendUseItem(int idx);
private:
	// 투명하게 그릴 때, 각각 1.함수 포인터를 이용해서 
	// 2.이미지를 넘겨주고 직접 그리게
	void DrawTransparent(HDC hdc, BYTE alphaValue, void (CFramework::* ptr)(HDC));
	void DrawTransparent(HDC hdc, int startX, int startY, int sizeX, int sizeY, BYTE alphaValue, const CImage& targetImage);
	void DrawMap(HDC hdc);
	void DrawChatUi(HDC hdc);

	void DrawPlayerInfo(HDC hdc);
	void ReadMapData();

public:
	template <typename SceneName>
	void ChangeScene(void* pContext = nullptr)
	{ 
		CScene* scene = new SceneName;
		static CScene* prevScene;
		scene->Init(m_rtClient, this);
		scene->SendDataToNextScene(pContext);

		if (m_CurScene)
		{
			prevScene = m_CurScene;
			//delete m_pCurScene;
			m_CurScene = nullptr;
		}

		m_CurScene = scene; 
	}
};