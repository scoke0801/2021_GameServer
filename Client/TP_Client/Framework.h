#pragma once
#include "GameObject.h"

class CScene;

constexpr int MAX_BUFFER = 1024;
constexpr int MAX_NAME_LEN = 200;
struct EX_OVER {
	WSAOVERLAPPED   m_over;
	WSABUF          m_wsabuf;
	unsigned char   m_packetbuf[MAX_BUFFER];
	SOCKET m_csocket; // OP_ACCEPT ������ ���
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

	// �� Ÿ���� �׸��� ���� ��ġ
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

	// ������۸� ó���� ���� �����Դϴ�.
	HDC m_hdc;
	HBITMAP m_hbmp;

	// �ð� ó���� ���� �����Դϴ�. 
	std::chrono::system_clock::time_point m_currentTime[2];
	std::chrono::duration<double> m_timeElapsed;   // �ð��� �󸶳� ������

	std::chrono::duration<double> m_serverUpdated; // �ð��� �󸶳� ������ 
	double m_dLag;
	double m_fps;

	// Ÿ��Ʋ�� ��� ���� �����Դϴ�.
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
	// WM_SOCKET �޽��� ���� ��� �̸� ó��
	void ProcessCommunication(WPARAM wParam, LPARAM lParam);

	// �Է¹��� IP�ּҷ� ������ ����
	bool ConnectToServer();

	// �����κ��� �����͸� �޾ƿ�
	void DoRecv();

	// �����κ��� ���� �����͸� ��Ŷ������ ó��
	void ProcessPacket(unsigned char* p_buf);

	bool SendPacket(void* p);
	bool SendPacket(SOCKET& sock, char* packet, int packetSize, int& retVal);

	void SendLoginPacket();
	void SendMovePacket(DIRECTION dir);
	void SendChatPacket();
	void SendAttackPacket();
	void SendUseItem(int idx);
private:
	// �����ϰ� �׸� ��, ���� 1.�Լ� �����͸� �̿��ؼ� 
	// 2.�̹����� �Ѱ��ְ� ���� �׸���
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