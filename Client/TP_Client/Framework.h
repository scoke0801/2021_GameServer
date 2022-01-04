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

	// 입력받은 IP주소로 서버와 연결
	bool ConnectToServer();

	// 패킷 전송
	bool SendPacket(void* p);
	bool SendPacket(SOCKET& sock, char* packet, int packetSize, int& retVal);

public:
	string GetServerIP() const { return m_ServerIp; }
	bool IsServerConnected() const { return m_isServerConnected; }
	short GetClientId() const { return m_ClientId; }
	  
private:
	// WM_SOCKET 메시지 받은 경우 이를 처리
	void ProcessCommunication(WPARAM wParam, LPARAM lParam);


	// 서버로부터 데이터를 받아옴
	void DoRecv();

	// 서버로부터 받은 데이터를 패킷단위로 처리
	void ProcessPacket(unsigned char* p_buf);

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


private:
	CScene* m_CurScene;

private:
	string	m_ServerIp = "127.0.0.1";

	bool	m_isServerConnected = false;

	SOCKET	m_SocketServer;

	short	m_ClientId = -1;

	SESSION m_Client;
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
};