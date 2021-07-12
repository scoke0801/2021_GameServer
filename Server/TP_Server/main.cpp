#include"stdafx.h"
#include "handle_db.h"
extern priority_queue<TIMER_EVENT> timer_queue;
extern mutex timer_lock;

extern HANDLE h_iocp;
extern MAP_TILE_DATA g_TileDatas[WORLD_HEIGHT][WORLD_WIDTH];

// 섹터 크기 정보 등은 클라에서 알 필요 없음...
extern unordered_set<int> g_sector[SECTOR_HEIGHT + 1][SECTOR_WIDTH + 1];
extern mutex	g_sector_locker;

extern array <ServerObject*, MAX_USER + 1> objects;

extern string itemNames[5]; 
int itemGolds[5] = { 0, 5, 30, 15, 100 };

bool IsInSight(int id_a, int id_b);

int FindTargetInAgroRegion(int monId);
bool IsInAgroRegion(int monId, int tartgetId);
bool IsInRoamingRegion(int monId, int x,int y);

void Disconnect(int p_id);
void DisplayError(const char* msg, int err_no);

void SendPacket(int p_id, void* p);

void DoRecv(int key);
int GetNewPlayerId(SOCKET p_socket);
void SendLoginOkPacket(int p_id);
void SendLoginFailePacket(int p_id);

void SendMovePacket(int c_id, int p_id);
void SendAddObject(int c_id, int p_id);
void SendRemoveObject(int c_id, int p_id); 
void SendStatChangePacket(int id);
void SendAddItem(int id, ITEM_TYPE type);

void attacked(int attackerId, int victimId, int power);

void DoMove(int p_id, DIRECTION dir); 
void ReservePlayerEvent(int pl, OP_TYPE eventInfo, int delay);
void DoAttack(int p_id, int power);
void ProcessPacket(int p_id, unsigned char* p_buf);

void MainWorkerFunc(HANDLE h_iocp, SOCKET l_socket);
 
bool IsNpc(int id); 
void do_npc_path_move(ServerObject* npc);
void do_npc_random_move(ServerObject* npc);
void do_npc_script_move(ServerObject* npc, int prev_x, int prev_y); // 스크립트에서 처리한 결과를 플레이어에게 전송

void AddEvent(int obj, OP_TYPE e_type, int delay_ms);

void DoTimerWork();
void WakeUpNPC(int pl_id, int npc_id);

int API_get_x(lua_State* L);
int API_get_y(lua_State* L);
int API_set_x(lua_State* L);
int API_set_y(lua_State* L); 

int API_set_move_type(lua_State* L);

int API_send_mess(lua_State* L);
int API_add_event(lua_State* L);
void DisplayLuaError(lua_State* L, const char* fmt, ...);

//int API_set_move_count(lua_State* L);
void SendChatPacket(int c_id, int p_id, const char* mess);

// 인자로 넘어오는 패킷을 대상을 보고있는 모든 플레이어에게 전송 ,섹터 갱신은 여기서 안함
void SendRemovePacketToViewObjects(int target); 
void SendStatPacketToViewObjects(int target, void* packet);

void ReadMapData();
void InitObjects(); 
void DoPlayerRespone(int id);
void DoMonsterRespone(int id);
// return -1 : 실패, 성공시 목표 대상의 id 반환
int CheckCanAttack(int atteckerId);

void ExpUp(int playerId, int monId);
void GoldUp(int playerId, int monId);
void GetItem(int playerId, int monId, ITEM_TYPE itemType);
void UseItem(int playerId, int itexIdx);

int main()
{
	setlocale(LC_ALL, "korean");
	std::wcout.imbue(std::locale("korean"));

	srand((unsigned int)time(NULL));
	ReadMapData();
	InitObjects(); 

	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);
	 
	h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	SOCKET listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(listenSocket), h_iocp, SERVER_ID, 0);
	SOCKADDR_IN serverAddr;
	memset(&serverAddr, 0, sizeof(SOCKADDR_IN));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	::bind(listenSocket, (struct sockaddr*)&serverAddr, sizeof(SOCKADDR_IN));
	listen(listenSocket, SOMAXCONN);

	EX_OVER accept_over;
	accept_over.m_op = OP_ACCEPT;
	memset(&accept_over.m_over, 0, sizeof(accept_over.m_over));
	SOCKET c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	accept_over.m_csocket = c_socket;
	BOOL ret = AcceptEx(listenSocket, c_socket,
		accept_over.m_packetbuf, 0, 32, 32, NULL, &accept_over.m_over);
	if (FALSE == ret) {
		int err_num = WSAGetLastError();
		if (err_num != WSA_IO_PENDING) {
			DisplayError("AcceptEx Error", err_num);
		}
	}

	cout << "서버열림\n";
	vector <thread> MainWorkerFunc_threads;
	for (int i = 0; i < 4; ++i)
		MainWorkerFunc_threads.emplace_back(MainWorkerFunc, h_iocp, listenSocket);

	thread timer_thread(DoTimerWork);
	timer_thread.join(); 
	for (auto& th : MainWorkerFunc_threads)
		th.join();

	//thread queue_thread(queue_MainWorkerFunc);
	//queue_thread.join();

	closesocket(listenSocket);
	WSACleanup();
}

void DisplayError(const char* msg, int err_no)
{
	WCHAR* lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	cout << msg;
	wcout << lpMsgBuf << endl;
	LocalFree(lpMsgBuf);
}
void SendPacket(int p_id, void *p)
{
	if (objects[p_id]->m_state == PL_STATE::PLST_FREE) {
		return;
	}
	int p_size = reinterpret_cast<unsigned char*>(p)[0];
	int p_type = reinterpret_cast<unsigned char*>(p)[1];
	EX_OVER* s_over = new EX_OVER;
	s_over->m_op = OP_SEND;
	memset(&s_over->m_over, 0, sizeof(s_over->m_over));
	memcpy(s_over->m_packetbuf, p, p_size);
	s_over->m_wsabuf[0].buf = reinterpret_cast<CHAR *>(s_over->m_packetbuf);
	s_over->m_wsabuf[0].len = p_size;
	
	int ret = WSASend(reinterpret_cast<SESSION*>(objects[p_id])->m_socket, s_over->m_wsabuf, 1,
		NULL, 0, &s_over->m_over, 0);
	if (0 != ret) {
		int err_no = WSAGetLastError();
		if (WSA_IO_PENDING != err_no) {
			DisplayError("WSASend : ", WSAGetLastError());
			Disconnect(p_id);
		}
	}
}

void DoRecv(int key)
{
	auto obj = reinterpret_cast<SESSION*>(objects[key]);
	obj->m_recv_over.m_wsabuf[0].buf =
		reinterpret_cast<char *>(obj->m_recv_over.m_packetbuf)
		+ obj->m_prev_size;
	obj->m_recv_over.m_wsabuf[0].len = MAX_BUFFER - obj->m_prev_size;
	memset(&obj->m_recv_over.m_over, 0, sizeof(obj->m_recv_over.m_over));
	DWORD r_flag = 0;
	int ret = WSARecv(obj->m_socket, obj->m_recv_over.m_wsabuf, 1,
		NULL, &r_flag, &obj->m_recv_over.m_over, NULL);
	if (0 != ret) {
		int err_no = WSAGetLastError();
		if (WSA_IO_PENDING != err_no)
			DisplayError("WSARecv : ", WSAGetLastError());
	}
}

int GetNewPlayerId(SOCKET p_socket)
{
	for (int i = SERVER_ID + 1; i <= MAX_USER; ++i) {
		if (PLST_FREE == objects[i]->m_state) {
			objects[i]->m_state = PLST_CONNECTED;
			reinterpret_cast<SESSION*>(objects[i])->m_socket = p_socket;
			objects[i]->m_name[0] = 0;
			return i;
		}
	}
	return -1;
}

void SendLoginOkPacket(int p_id)
{ 
	sc_packet_login_ok p; 
	p.size = sizeof(p);
	p.type = SC_LOGIN_OK;
	p.id = p_id;
	p.x = objects[p_id]->x;
	p.y = objects[p_id]->y;
	p.EXP = objects[p_id]->EXP;
	p.HP = objects[p_id]->HP;
	p.LEVEL = objects[p_id]->LEVEL;
	p.GOLD = objects[p_id]->GOLD;
	p.itemCount = objects[p_id]->itemCount;
	
	for (int i = 0; i < p.itemCount; ++i) {
		p.items[i] = objects[p_id]->Items[i];
	}
	SendPacket(p_id, &p);
}

void SendLoginFailePacket(int p_id)
{
	sc_packet_login_fail packet;
	packet.size = sizeof(packet);
	packet.type = SC_LOGIN_FAIL;
	SendPacket(p_id, &packet);
}

void SendMovePacket(int c_id, int p_id)
{
	sc_packet_position p;
	p.id = p_id;
	p.size = sizeof(p);
	p.type = SC_POSITION;
	p.x = objects[p_id]->x;
	p.y = objects[p_id]->y;
	p.direction = objects[p_id]->direction;
	p.move_time = objects[p_id]->move_time;
	SendPacket(c_id, &p);
}

void SendAddObject(int c_id, int p_id)
{
	sc_packet_add_object p;
	p.id = p_id;
	p.size = sizeof(p);
	p.type = SC_ADD_OBJECT;
	p.x = objects[p_id]->x;
	p.y = objects[p_id]->y;
	p.EXP = objects[p_id]->EXP;
	p.LEVEL = objects[p_id]->LEVEL;
	p.HP = objects[p_id]->HP;
	p.obj_class = (int)objects[p_id]->type;
	strcpy_s(p.name, objects[p_id]->m_name);
	SendPacket(c_id, &p);
}

void SendRemoveObject(int c_id, int p_id)
{
	sc_packet_remove_object p;
	p.id = p_id;
	p.size = sizeof(p);
	p.type = SC_REMOVE_OBJECT;
	SendPacket(c_id, &p);
}

void SendStatChangePacket(int id)
{
	sc_packet_stat_change packet;
	packet.EXP = objects[id]->EXP;
	packet.HP = objects[id]->HP;
	packet.id = id;
	packet.GOLD = objects[id]->GOLD;
	packet.LEVEL = objects[id]->LEVEL;
	packet.size = sizeof(packet);
	packet.type = SC_STAT_CHANGE;
	SendPacket(id, &packet);
}

void SendAddItem(int id, ITEM_TYPE type)
{
	sc_packet_add_item packet;
	packet.size = sizeof(packet);
	packet.type = SC_ADD_ITEM;
	packet.itemType = type;
	SendPacket(id, &packet);
}

void attacked(int attackerId, int victimId, int power)
{  
	if (IsNpc(victimId) && objects[victimId]->HP == 100) {
		objects[victimId]->targetId = attackerId;
	}
	objects[victimId]->HP -= power; 
	{
		sc_packet_stat_change packet;
		packet.id = victimId;
		packet.EXP = objects[victimId]->EXP;
		packet.LEVEL = objects[victimId]->LEVEL;
		packet.HP = objects[victimId]->HP;
		packet.size = sizeof(packet);
		packet.GOLD = objects[victimId]->GOLD;
		packet.type = SC_STAT_CHANGE;
		// 바뀐 정보를
		// 쳐다보고 있는 모든 플레이어들에게 전송해야함  
		
		if (false == IsNpc(attackerId)) {
			string mess = string(objects[attackerId]->m_name) + "가 " + string(objects[victimId]->m_name) + "에게 " + to_string(power)
				+ "의 피해를 입혔습니다"; 
			SendChatPacket(attackerId, 0, mess.c_str());
		}
		if (false == IsNpc(victimId)) {
			string mess =  string(objects[attackerId]->m_name) + "의 공격으로 " + to_string(power) +
				+ " 의 피해를 입었습니다."; 
			SendChatPacket(victimId, 0, mess.c_str());
		}
		if (false == IsNpc(victimId)) { 
			AddEvent(victimId, OP_TYPE::OP_HEAL, 5000);
		}
		SendStatPacketToViewObjects(attackerId, &packet);
	}
}

void DoMove(int p_id, DIRECTION dir)
{
	auto prev_sector_x = objects[p_id]->x / SECTOR_SIZE;
	auto prev_sector_y = objects[p_id]->y / SECTOR_SIZE;

	auto& x = objects[p_id]->x;
	auto& y = objects[p_id]->y; 
	 
	switch (dir)
	{
	case D_E:
		if (x < WORLD_WIDTH) {
			if ((int)g_TileDatas[y][x + 1] <= 7) {
				x++; 
			}
		}
		break;
	case D_W:
		if (x > 0) {
			if ((int)g_TileDatas[y][x - 1] <= 7) {
				x--; 
			}
		}
		break;
	case D_S:
		if (y < WORLD_HEIGHT) {
			if ((int)g_TileDatas[y + 1][x] <= 7) {
				y++; 
			}
		}
		break;
	case D_N:
		if (y > 0) {
			if ((int)g_TileDatas[y - 1][x] <= 7) {
				y--; 
			}
		}
		break;
	}
	 
	int cur_sector_x = x / SECTOR_SIZE;
	int cur_sector_y = y / SECTOR_SIZE;
	auto obj = reinterpret_cast<SESSION*>(objects[p_id]);
	unordered_set <int> old_vl;
	obj->m_vl.lock();
	old_vl = obj->m_view_list;
	obj->m_vl.unlock();

	unordered_set<int> sector_copies[5];

	g_sector_locker.lock();
	sector_copies[0] = g_sector[cur_sector_y][cur_sector_x];
	unordered_set<int> prev_pos_sector_copy = g_sector[prev_sector_x][prev_sector_y];

	if (((x + VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_x) {
		if (cur_sector_x + 1 < SECTOR_WIDTH + 1) {
			sector_copies[1] = g_sector[cur_sector_y][cur_sector_x + 1];
		}
	}
	if (((x - VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_x) {
		if (cur_sector_x - 1 > 0) {
			sector_copies[2] = g_sector[cur_sector_y][cur_sector_x - 1];
		}
	} 
	if (((y + VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_y) {
		if (cur_sector_y + 1 < SECTOR_HEIGHT + 1) {
			sector_copies[3] = g_sector[cur_sector_y + 1][cur_sector_x];
		}
	}
	if (((y - VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_y) { 
		if (cur_sector_y - 1 > 0) {
			sector_copies[4] = g_sector[cur_sector_y - 1][cur_sector_x];
		}
	}
	g_sector_locker.unlock();

	unordered_set<int> new_vl;
	for (int i = 0; i < 5; ++i) {
		if (sector_copies[i].size() == 0) {
			continue;
		}
		for (auto id : sector_copies[i]) {
			if (id == p_id) {
				continue;
			} 
			if (objects[id]->HP <= 0) {
				continue;
			}
			if (IsInSight(p_id, id)) {
				if (new_vl.count(id) == 0) {
					new_vl.insert(id); 
				}
			}
		}
	} 
 
	//자기 자신에게는 항상 결과를 
	SendMovePacket(p_id, p_id);

	for (auto pl : new_vl) {
		obj->m_vl.lock();
		if (0 == obj->m_view_list.count(pl)) {		// 1. 새로 시야에 들어오는 객체
			obj->m_view_list.insert(pl);
			obj->m_vl.unlock();
			SendAddObject(p_id, pl);

			if (false == IsNpc(pl)) { 
				auto obj_pl = reinterpret_cast<SESSION*>(objects[pl]);
				obj_pl->m_vl.lock();
				if (0 == obj_pl->m_view_list.count(p_id)) {
					obj_pl->m_view_list.insert(p_id);
					obj_pl->m_vl.unlock();
					SendAddObject(pl, p_id);
				}
				else {
					obj_pl->m_vl.unlock();
					SendMovePacket(pl, p_id);
				}
			}
			else {
				if (objects[pl]->HP > 0) {
					// 리스폰 중이 아니라면 깨운다
					WakeUpNPC(p_id, pl);
				}
			}
		}
		else {		// 2. 기존 시야에도 있고 새 시야에도 있는 경우
			obj->m_vl.unlock();
			if (false == IsNpc(pl)) {
				auto obj_pl = reinterpret_cast<SESSION*>(objects[pl]);
				obj_pl->m_vl.lock();
				if (0 == obj_pl->m_view_list.count(p_id)) {
					obj_pl->m_view_list.insert(p_id);
					obj_pl->m_vl.unlock();
					SendAddObject(pl, p_id);
				}
				else {
					obj_pl->m_vl.unlock();
					SendMovePacket(pl, p_id);
				}
			} 
		}
	}

	// 3 기존 시야에 있고, 새 시야에 없는경우
	for (auto pl : old_vl) {
		if (0 == new_vl.count(pl)) {
			// 3. 시야에서 사라진 경우 
			obj->m_vl.lock();
			obj->m_view_list.erase(pl);
			obj->m_vl.unlock();
			SendRemoveObject(p_id, pl);

			if (false == IsNpc(pl)) { 
				auto obj_pl = reinterpret_cast<SESSION*>(objects[pl]);
				obj_pl->m_vl.lock();
				if (0 != obj_pl->m_view_list.count(p_id)) {
					obj_pl->m_view_list.erase(p_id);
					obj_pl->m_vl.unlock();
					SendRemoveObject(pl, p_id);
				}
				else {
					obj_pl->m_vl.unlock();
				}
			}
		} 
	} 

	if ((prev_sector_x != cur_sector_x) ||
		(prev_sector_y != cur_sector_y)) {
		g_sector_locker.lock();
		if (g_sector[prev_sector_y][prev_sector_x].count(p_id) != 0) {
			g_sector[prev_sector_y][prev_sector_x].erase(p_id);
		}
		g_sector[cur_sector_y][cur_sector_x].insert(p_id);
		g_sector_locker.unlock();
	}

	objects[p_id]->is_active = false;
}

void ReservePlayerEvent(int pl, OP_TYPE eventInfo, int delay)
{
	bool old_state = false;
 
	if (true == atomic_compare_exchange_strong(&objects[pl]->is_active, &old_state, true)) { 
		AddEvent(pl, eventInfo, delay);
	}
}

void DoAttack(int p_id, int power)
{
	unordered_set<int> obj_vl;
	if (IsNpc(p_id)) {

		int cur_sector_x = objects[p_id]->x / SECTOR_SIZE;
		int cur_sector_y = objects[p_id]->y / SECTOR_SIZE;

		unordered_set<int> sector_copies[5];

		g_sector_locker.lock();
		sector_copies[0] = g_sector[cur_sector_y][cur_sector_x];


		if (((cur_sector_x + VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_x) {
			if (cur_sector_x + 1 < SECTOR_WIDTH + 1) {
				sector_copies[1] = g_sector[cur_sector_y][cur_sector_x + 1];
			}
		}
		if (((cur_sector_x - VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_x) {
			if (cur_sector_x - 1 > 0) {
				sector_copies[2] = g_sector[cur_sector_y][cur_sector_x - 1];
			}
		}
		if (((cur_sector_y + VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_y) {
			if (cur_sector_y + 1 < SECTOR_HEIGHT + 1) {
				sector_copies[3] = g_sector[cur_sector_y + 1][cur_sector_x];
			}
		}
		if (((cur_sector_y - VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_y) {
			if (cur_sector_y - 1 > 0) {
				sector_copies[4] = g_sector[cur_sector_y - 1][cur_sector_x];
			}
		}
		g_sector_locker.unlock();

		for (int i = 0; i < 5; ++i) {
			if (sector_copies[i].size() == 0) {
				continue;
			}
			for (auto id : sector_copies[i]) {
				if (id == p_id) {
					continue;
				}
				if (objects[id]->HP <= 0) {
					continue;
				}
				if (PL_STATE::PLST_INGAME == objects[id]->m_state) {
					if (obj_vl.count(id) == 0) {
						// 몬스터가 플레이어를 공격하는 경우!
						obj_vl.insert(id);
					}
				}
			}
		}
	}
	else { 
		auto obj = reinterpret_cast<SESSION*>(objects[p_id]);
		obj->m_vl.lock();
		obj_vl = obj->m_view_list;
		obj->m_vl.unlock();
	}

	int px = objects[p_id]->x;
	int py = objects[p_id]->y; 
	for (auto pl : obj_vl) {
		int x = objects[pl]->x;
		int y = objects[pl]->y;

		bool isAttacked = false;
		if (px == x - 1 && py == y) {// 왼쪽 
			attacked(p_id, pl, power); 
			isAttacked = true;
		}
		else if (px == x + 1 && py == y) { // 오른쪽 
			attacked(p_id, pl, power);
			isAttacked = true;
		}
		else if (px == x && py == y + 1) { // 아래쪽 
			attacked(p_id, pl, power);
			isAttacked = true;
		}
		else if (px == x && py == y - 1) { // 위쪽 
			attacked(p_id, pl, power);
			isAttacked = true;
		} 
		if (false == isAttacked) {
			continue;
		}
		// 이런 체력이라면 돔황챠
		if (objects[pl]->HP > 0 && objects[pl]->HP <= 25) {
			if (false == objects[pl]->isOnRunaway) { 
				if (objects[pl]->type == ObjectType::Mon_Peace_Roaming ||
					objects[pl]->type == ObjectType::Mon_Agro_Roaming) {

					string mess = string(objects[pl]->m_name) + "가 도망칩니다.";
					SendChatPacket(p_id, 0, mess.c_str());
					objects[pl]->targetId = p_id;
					EX_OVER* ex_over = new EX_OVER;
					ex_over->m_op = OP_RUN_AWAY;
					*reinterpret_cast<int*>(ex_over->m_packetbuf) = p_id;
					PostQueuedCompletionStatus(h_iocp,
						1, pl, &ex_over->m_over);
				}
			}
		}
		// 죽었다 - 처리하자
		else if (objects[pl]->HP <= 0) {
			// 쳐다보고 있는 모든 플레이어들에게 remove시키고
			// 몬스터라면 리스폰하도록..

			SendRemovePacketToViewObjects(pl);
			if (IsNpc(pl)) {
				// 리스폰~~~
				//cout << pl << " - 몬스터 리스폰!!!!!!!!!!!!!!!!!!\n";
				ExpUp(p_id, pl);
				GoldUp(p_id, pl);
				GetItem(p_id, pl, ITEM_TYPE::I_NOT);
				bool old_state = false;
				if (true == atomic_compare_exchange_strong(&objects[pl]->is_active, &old_state, true)) { 
					AddEvent(pl, OP_RESPONE, 30000);
				}

			}
			else { 
				auto obj = reinterpret_cast<SESSION*>(objects[pl]);
				// 플레이어를 바라보는 대상에게는 삭제 메시지를 보냈으니,
				// 플레이어가 지금까지 보고 있던 대상들을 모두 삭제!
				obj->m_vl.lock();
				 
				while (false == obj->m_view_list.empty()) { 
					auto target = obj->m_view_list.begin();
					SendRemoveObject(pl, *target);
					if (IsNpc(*target))
					{
						if (objects[(*target)]->targetId == pl) {
							objects[(*target)]->targetId = 0;
						}
					}
					obj->m_view_list.erase(target);
				}
			
				obj->m_vl.unlock();

				AddEvent(pl, OP_RESPONE, 0);
			}
		}

	}
	//objects[p_id]->is_active = false; 
	if (IsNpc(p_id) && objects[p_id]->HP > 0) {
		AddEvent(p_id, OP_MONSTER_MOVE, 0);
	}
}

void ProcessPacket(int p_id, unsigned char* p_buf)
{
	char buf[10000];
	int copyPos = 0;
	switch (p_buf[1]) {
	case CS_LOGIN: {
		cs_packet_login* packet = reinterpret_cast<cs_packet_login*>(p_buf);
		
		for (int i = 1; i < MAX_USER; ++i) {
			if (0 == strcmp(packet->player_id, objects[i]->m_name)) {
				SendLoginFailePacket(p_id);
				break;
			}
		}
		if (false == IsNpc(p_id)) {
			strcpy_s(objects[p_id]->m_name, packet->player_id);
		}
		if (SearchUserID(packet->player_id, p_id)) { 

			SendLoginOkPacket(p_id);
			objects[p_id]->m_state = PLST_INGAME;

			int sector_x = objects[p_id]->x / SECTOR_SIZE;
			int sector_y = objects[p_id]->y / SECTOR_SIZE;
			g_sector[sector_y][sector_x].insert(p_id);

			auto obj = reinterpret_cast<SESSION*>(objects[p_id]);
			for (auto& pl : objects) {
				if (p_id != pl->id) {
					lock_guard <mutex> gl{ pl->m_slock };
					if (PLST_INGAME == pl->m_state) {
						// 모든 플레이어의 정보를 보내던 구조에서
						// 주위에 있는 플레이어의 정보만 전송하도록 변경

						if (IsInSight(p_id, pl->id)) {
							obj->m_vl.lock();
							obj->m_view_list.insert(pl->id); // 나에게 상대방을
							obj->m_vl.unlock();
							SendAddObject(p_id, pl->id);

							if (false == IsNpc(pl->id)) {
								auto obj_pl = reinterpret_cast<SESSION*>(objects[pl->id]);
								obj_pl->m_vl.lock();
								obj_pl->m_view_list.insert(p_id); // 상대방에게 나를
								obj_pl->m_vl.unlock();
								SendAddObject(pl->id, p_id);
							}
							else {
								if (objects[pl->id]->HP > 0) {
									// 리스폰 중이 아니라면 깨운다
									WakeUpNPC(p_id, pl->id);
								}
							}
						}
					}
				}
			}
		}
		else {
			cout << "dddd\n";
			ZeroMemory(objects[p_id]->m_name, sizeof(objects[p_id]->m_name));
			SendLoginFailePacket(p_id);
		}
	}
		break;

	case CS_TELEPORT:
	{
		cs_packet_teleport* packet = reinterpret_cast<cs_packet_teleport*>(p_buf);
		/// 프로토콜 정보가 없음,,,
	}
		break;

	case CS_MOVE: { 
		cs_packet_move* packet = reinterpret_cast<cs_packet_move*>(p_buf);
		objects[p_id]->move_time = packet->move_time;
		//ReservePlayerEvent(p_id, OP_TYPE::OP_PLAYER_MOVE, 100);
		objects[p_id]->direction = (DIRECTION)packet->direction;
		DoMove(p_id, (DIRECTION)packet->direction);
		//cout << "CS_MOVE\n";
	}
		break;
	case CS_CHAT:{
		cs_packet_chat* packet = reinterpret_cast<cs_packet_chat*>(p_buf);
		auto obj = reinterpret_cast<SESSION*>(objects[p_id]);
		obj->m_vl.lock();
		unordered_set<int> obj_vl = obj->m_view_list;
		obj->m_vl.unlock();
		for (auto id : obj_vl) {
			if (IsNpc(id)) {
				continue;
			} 
			SendChatPacket(id, p_id, packet->message); 
		}
		// 자기 자신한테도!
		SendChatPacket(p_id, p_id, packet->message);
		cout << "CS_CHAT\n";
		break;
		}
	case CS_ATTACK:
	{
		cs_packet_attack* packet = reinterpret_cast<cs_packet_attack*>(p_buf);
		
		//ReservePlayerEvent(p_id, OP_TYPE::OP_ATTACK, 1000);
		DoAttack(p_id, objects[p_id]->power);
		cout << "CS_ATTACK\n";
		break;
	}
	case CS_USE_ITEM:
	{
		cs_packet_use_item* packet = reinterpret_cast<cs_packet_use_item*>(p_buf);

		UseItem(p_id, packet->itemIndex);
		break;
	}
	case CS_BUY_ITEM:
	{
		cs_packet_buy_item* packet = reinterpret_cast<cs_packet_buy_item*>(p_buf);

		if (objects[p_id]->itemCount >= 4) {
			SendChatPacket(p_id, 0, "아이템을 보관할 공간이 부족합니다.");
			break;
		}
		if (objects[p_id]->GOLD < itemGolds[packet->itemType]) { 
			SendChatPacket(p_id, 0, "아이템을 구매할 골드가 부족합니다.");
			break;
		}
		objects[p_id]->GOLD -= itemGolds[packet->itemType];
		GetItem(p_id, 0, packet->itemType);
		SendStatChangePacket(p_id);
		break;
	}
	default:
		cout << "Unknown Packet Type from Client[" << p_id;
		cout << "] Packet Type [" << p_buf[1] << "]";
		while (true);
	}
}

bool IsInSight(int id_a, int id_b)
{
	// 3d 게임은 시야가 원, sqrt(xGap^2 + yGap^2)
	// 2d 게임은 시야가 사각형 - 모니터가 사각형...
	return VIEW_RADIUS >= abs(objects[id_a]->x - objects[id_b]->x) &&
		VIEW_RADIUS >= abs(objects[id_a]->y - objects[id_b]->y);
}

int FindTargetInAgroRegion(int monId)
{
	int cur_sector_x = objects[monId]->x / SECTOR_SIZE;
	int cur_sector_y = objects[monId]->y / SECTOR_SIZE;

	unordered_set<int> sector_copies[5];

	int x = objects[monId]->x;
	int y = objects[monId]->y;
	g_sector_locker.lock();
	sector_copies[0] = g_sector[cur_sector_y][cur_sector_x];
	 
	if (((x + VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_x) {
		if (cur_sector_x + 1 < SECTOR_WIDTH + 1) {
			sector_copies[1] = g_sector[cur_sector_y][cur_sector_x + 1];
		}
	}
	if (((x - VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_x) {
		if (cur_sector_x - 1 > 0) {
			sector_copies[2] = g_sector[cur_sector_y][cur_sector_x - 1];
		}
	}
	if (((y + VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_y) {
		if (cur_sector_y + 1 < SECTOR_HEIGHT + 1) {
			sector_copies[3] = g_sector[cur_sector_y + 1][cur_sector_x];
		}
	}
	if (((y - VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_y) {
		if (cur_sector_y - 1 > 0) {
			sector_copies[4] = g_sector[cur_sector_y - 1][cur_sector_x];
		}
	}
	g_sector_locker.unlock(); 

	unordered_set<int> obj_vl;
	for (int i = 0; i < 5; ++i) {
		if (sector_copies[i].size() == 0) {
			continue;
		}
		for (auto id : sector_copies[i]) {
			if (id == monId) {
				continue;
			}
			if (objects[id]->HP <= 0) {
				continue;
			}
			if (PL_STATE::PLST_INGAME == objects[id]->m_state) {
				if (obj_vl.count(id) == 0) {
					if (false == IsNpc(id)) {
						obj_vl.insert(id);
					} 
				}
			}
		}
	}

	for (auto& id : obj_vl) {
		if (IsInAgroRegion(monId, id)) {
			return id;
		}
	}
	return 0;
}

bool IsInAgroRegion(int monId, int tartgetId)
{
	return AGRO_RADIUS >= abs(objects[monId]->start_x - objects[tartgetId]->x) &&
		AGRO_RADIUS >= abs(objects[monId]->start_y - objects[tartgetId]->y);
}

bool IsInRoamingRegion(int monId, int x, int y)
{
	return SCREEN_WIDTH >= abs(objects[monId]->start_x - x) &&
		SCREEN_HEIGHT >= abs(objects[monId]->start_y - y);
}

void Disconnect(int p_id)
{
	{
		if (PLST_FREE == objects[p_id]->m_state) {
			return;
		}

		SaveUserData(p_id);

		closesocket(reinterpret_cast<SESSION*>(objects[p_id])->m_socket);
		objects[p_id]->m_state = PLST_FREE;

		int sector_x = objects[p_id]->x / SECTOR_SIZE;
		int sector_y = objects[p_id]->y / SECTOR_SIZE;

		g_sector_locker.lock();
		if (g_sector[sector_y][sector_x].count(p_id) != 0) {
			g_sector[sector_y][sector_x].erase(p_id);
		} 
		g_sector_locker.unlock(); 
		cout << "플레이어 로그아웃\n";
	}
	for (auto& pl : objects) {
		if (IsNpc(pl->id)) {
			if (pl->targetId == p_id) {
				pl->targetId = 0;
			} 
			continue;
		}

		auto obj_pl = reinterpret_cast<SESSION*>(pl);
		if (0 == obj_pl->m_view_list.count(p_id)) {
			continue;
		}
		obj_pl->m_vl.lock();
		obj_pl->m_view_list.erase(p_id);
		obj_pl->m_vl.unlock();

		if (PLST_INGAME == obj_pl->m_state)
			SendRemoveObject(obj_pl->id, p_id);
	}
}

void MainWorkerFunc(HANDLE h_iocp, SOCKET l_socket)
{
	while (true) {
		DWORD num_bytes;
		ULONG_PTR ikey;
		WSAOVERLAPPED* over;

		BOOL ret = GetQueuedCompletionStatus(h_iocp, &num_bytes,
			&ikey, &over, INFINITE);

		int key = static_cast<int>(ikey);
		if (FALSE == ret) {
			if (SERVER_ID == key) {
				DisplayError("GQCS : ", WSAGetLastError());
				exit(-1);
			}
			else {
				DisplayError("GQCS : ", WSAGetLastError());
				Disconnect(key);
			}
		}
		if ((key != SERVER_ID) && (0 == num_bytes)) {
			Disconnect(key);
			continue;
		}
		EX_OVER* ex_over = reinterpret_cast<EX_OVER*>(over);

		switch (ex_over->m_op) {
		case OP_RECV: {
			auto obj = reinterpret_cast<SESSION*>(objects[key]);
			unsigned char* packet_ptr = ex_over->m_packetbuf;
			int num_data = num_bytes + obj->m_prev_size;
			int packet_size = packet_ptr[0];

			while (num_data >= packet_size) {
				ProcessPacket(key, packet_ptr);
				num_data -= packet_size;
				packet_ptr += packet_size;
				if (0 >= num_data) break;
				packet_size = packet_ptr[0];
			}
			obj->m_prev_size = num_data;
			if (0 != num_data)
				memcpy(ex_over->m_packetbuf, packet_ptr, num_data);
			DoRecv(key);
		} 
					break;
		case OP_SEND:
			delete ex_over;
			break;
		case OP_ACCEPT: {
			int c_id = GetNewPlayerId(ex_over->m_csocket);
			if (-1 != c_id) {
				auto obj = reinterpret_cast<SESSION*>(objects[c_id]);
				obj->m_recv_over.m_op = OP_RECV;
				obj->m_prev_size = 0;
				CreateIoCompletionPort(
					reinterpret_cast<HANDLE>(obj->m_socket), h_iocp, c_id, 0);
				DoRecv(c_id);
			}
			else {
				closesocket(reinterpret_cast<SESSION*>(objects[c_id])->m_socket);
			}

			memset(&ex_over->m_over, 0, sizeof(ex_over->m_over));
			SOCKET c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			ex_over->m_csocket = c_socket;
			AcceptEx(l_socket, c_socket,
				ex_over->m_packetbuf, 0, 32, 32, NULL, &ex_over->m_over);
		}
					  break;
		case OP_PLAYER_MOVE:
		{
			DoMove(key, objects[key]->direction); 
			delete ex_over;
		}
			break;
		case OP_MONSTER_MOVE:	
		{ 
			if (objects[key]->HP <= 0) {
				break;
			}
			if (objects[key]->isOnRunaway) { 
				break;
			}
			int targetId = objects[key]->targetId;
			int x = objects[targetId]->x;
			int y = objects[targetId]->y;
			if (ObjectType::Mon_Peace_Roaming == objects[key]->type) {  
				if (targetId == 0) {
					// 누군가가 공격하기 전에는 가만히 있는다! 
					AddEvent(key, OP_MONSTER_MOVE, 1000);
					break;
				}  
				objects[key]->pathLock.lock();
				if (true == FindPath(objects[key]->x, objects[key]->y, x, y, key)) {
					// 경로를 탐색한 경우  
					//cout << "몬스터 경로 이동\n";
					do_npc_path_move(objects[key]);
					objects[key]->pathLock.unlock();
				}
				else {
					objects[key]->pathLock.unlock();
					do_npc_random_move(objects[key]);
				}
			}

			else if (ObjectType::Mon_Agro_Roaming == objects[key]->type) {
				if (targetId == 0) {
					// 누군가가 공격하기 전에는 자유로이 이동한다! 
					do_npc_random_move(objects[key]); 
					// 이동 후, 영역에 접근한 대상이 있다면 쫒아 가도록
					auto temp = FindTargetInAgroRegion(key);
					if (temp != 0) {
						objects[key]->targetId = temp;
					}
					break;
				}
				else {
					// 공격할 대상이 있으면 탐색하여 찾아간다!  
					objects[key]->pathLock.lock();
					if (true == FindPath(objects[key]->x, objects[key]->y, x, y, key)) {
						// 경로를 탐색한 경우 

						//cout << "몬스터 경로 이동\n";
						do_npc_path_move(objects[key]);
						objects[key]->pathLock.unlock();
					}
					else {
						objects[key]->pathLock.unlock();
						do_npc_random_move(objects[key]);
					}
				}
			}
			else if (ObjectType::Mon_Agro_Fixed == objects[key]->type) { 
				auto targetId = CheckCanAttack(key);
				if (targetId == -1) {
					// 공격할 대상을 찾지 못한 경우이니 계속 이동을 합시다! 
					AddEvent(key, OP_MONSTER_MOVE, 1000);
				}
				else {
					// 공격할 대상을 찾았으니 공격상태로 갑시다! 
					AddEvent(key, OP_ATTACK, 1000);
				}
			}
			else if (ObjectType::Mon_Peace_Fixed == objects[key]->type) {
				if (targetId == 0) {
					// 누군가가 공격하기 전에는 공격하지않는다!
					AddEvent(key, OP_MONSTER_MOVE, 1000);
				}
				else { 
					// 공격할 대상을 찾았으니 공격상태로 갑시다! 
					AddEvent(key, OP_ATTACK, 1000);
				}
			}
		}
			delete ex_over;
			break;
	
		case OP_ATTACK:
			//cout << "공격이벤트 발생 - id : " << key << "\n";
			if (false == objects[key]->isOnRunaway) {
				DoAttack(key, objects[key]->power);
			}
			delete ex_over;
			break;
		case OP_PLAYER_APPROCH:
		{
			objects[key]->m_sl.lock();
			int move_player = *reinterpret_cast<int*>(ex_over->m_packetbuf);
			lua_State* L = objects[key]->L;

			lua_getglobal(L, "player_is_near");
			lua_pushnumber(L, move_player);
			lua_pcall(L, 1, 0, 0);

			objects[key]->m_sl.unlock();
			delete ex_over;
			break;
		}
		case OP_RUN_AWAY:
		{
			if (objects[key] <= 0) {
				delete ex_over;
				break;
			}
			objects[key]->m_sl.lock();

			lua_State* L = objects[key]->L;
			lua_getglobal(L, "run_away_from_player");
			lua_pushnumber(L, objects[key]->targetId);
			//cout << "listen : " << objects[key]->targetId << "\n";
			int res = lua_pcall(L, 1, 0, 0);
			if (0 != res) {
				DisplayLuaError(L, "error running function ‘XXX’: %s\n",
					lua_tostring(L, -1));
			}
			objects[key]->m_sl.unlock(); 
			 
			delete ex_over; 
			if (objects[key]->isOnRunaway) {
				AddEvent(key, OP_RUN_AWAY, 1000);
			} 
			break;
		}
		case OP_RESPONE:
		{
			if (IsNpc(key)) {
				DoMonsterRespone(key);
			}
			else {
				DoPlayerRespone(key);
			}
			delete ex_over;
			break;
		}
		case OP_HEAL:
		{
			int prevHp = objects[key]->HP;
			if (prevHp < 100) {
				objects[key]->HP = min(prevHp + 10, 100);
				string mess = string(objects[key]->m_name) + " 가 " + to_string(objects[key]->HP - prevHp)
					+ "의 체력을 회복하였습니다.";
				SendChatPacket(key, 0, mess.c_str());  
				if (objects[key]->HP < 100) {
					AddEvent(key, OP_TYPE::OP_HEAL, 5000);
				}
				SendStatChangePacket(key);
			}
			delete ex_over;
			 
			break;
		}
		case OP_POTION_HEAL:
		{
			int prevHp = objects[key]->HP; 
			objects[key]->HP = min(prevHp + POTION_HP_HEAL_AMOUNT, 100);
			string mess = "포션을 사용하여 " + to_string(objects[key]->HP - prevHp) + "의 체력을 회복하였습니다.";
			SendChatPacket(key, 0, mess.c_str());
			SendStatChangePacket(key);
		
			delete ex_over;

			break;
		}
		case OP_FULL_HEAL:
		{
			int prevHp = objects[key]->HP; 
			objects[key]->HP = min(prevHp + POTION_HP_HEAL_AMOUNT, 100);
			string mess = "아이템을 사용하여 " + to_string(objects[key]->HP - prevHp) + "의 체력을 회복하였습니다.";
			SendChatPacket(key, 0, mess.c_str());
			SendStatChangePacket(key);

			delete ex_over;

			break;
		}
		
		}
	}
} 

bool IsNpc(int id)
{
	return id >= NPC_ID_START;
}
void do_npc_path_move(ServerObject* npc)
{ 
	if (npc->path.empty()) {
		auto targetId = CheckCanAttack(npc->id);
		if (targetId == -1) {
			// 공격할 대상을 찾지 못한 경우이니 계속 이동을 합시다! 
			AddEvent(npc->id, OP_MONSTER_MOVE, 1000);
		}
		else {
			// 공격할 대상을 찾았으니 공격상태로 갑시다!
			//cout << "몬스터가 공격할 대상을 찾았습니다! : " << npc->id << " - " << targetId << "\n";
			 
			AddEvent(npc->id, OP_ATTACK, 1000);
		} 
		return;
	}
	//cout << "경로 찾아가는중!\n";
	bool haveto_sleep = true;
	auto prev_sector_x = npc->x / SECTOR_SIZE;
	auto prev_sector_y = npc->y / SECTOR_SIZE;

	auto x = npc->x;
	auto y = npc->y;
	
	auto tartgetPos = npc->path.back();
	
	if( false == IsInRoamingRegion(npc->id, tartgetPos.x, tartgetPos.y)) {
		// 공격할 대상을 찾았으나 범위 밖입니다. 타겟을 새로이 지정합시다.
		npc->targetId = 0;
		AddEvent(npc->id, OP_MONSTER_MOVE, 1000);
		return;
	}

	if (tartgetPos.x > x) npc->direction = DIRECTION::D_E;
	else if (tartgetPos.x < x) npc->direction = DIRECTION::D_W;
	else if (tartgetPos.y > y) npc->direction = DIRECTION::D_S;
	else if (tartgetPos.y < y) npc->direction = DIRECTION::D_N;

	npc->x = tartgetPos.x;
	npc->y = tartgetPos.y;
	npc->path.pop_back(); 

	int cur_sector_x = x / SECTOR_SIZE;
	int cur_sector_y = y / SECTOR_SIZE;

	unordered_set<int> sector_copies[5];

	g_sector_locker.lock();
	sector_copies[0] = g_sector[cur_sector_y][cur_sector_x]; 
	 
	if (((x + VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_x) {
		if (cur_sector_x + 1 < SECTOR_WIDTH + 1) {
			sector_copies[1] = g_sector[cur_sector_y][cur_sector_x + 1];
		}
	}
	if (((x - VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_x) {
		if (cur_sector_x - 1 > 0) {
			sector_copies[2] = g_sector[cur_sector_y][cur_sector_x - 1];
		}
	}
	if (((y + VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_y) {
		if (cur_sector_y + 1 < SECTOR_HEIGHT + 1) {
			sector_copies[3] = g_sector[cur_sector_y + 1][cur_sector_x];
		}
	}
	if (((y - VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_y) {
		if (cur_sector_y - 1 > 0) {
			sector_copies[4] = g_sector[cur_sector_y - 1][cur_sector_x];
		}
	}
	g_sector_locker.unlock();

	unordered_set<int> obj_vl;
	for (int i = 0; i < 5; ++i) {
		if (sector_copies[i].size() == 0) {
			continue;
		}
		for (auto id : sector_copies[i]) {
			if (PLST_INGAME != objects[id]->m_state) {
				continue;
			}
			if (true == IsNpc(objects[id]->id)) {
				continue;
			}
			if (obj_vl.count(id) == 0) {
				obj_vl.insert(id);
			}
		}
	}

	// npc 행동 후 시야 범위 안에 플레이어가 존재하는 경우.
	for (auto pl : obj_vl) {
		auto obj = reinterpret_cast<SESSION*>(objects[pl]);
		// 플레이어에게 npc가 보인다면
		// 추가 or 이동
		if (IsInSight(pl, npc->id)) {
			obj->m_slock.lock();
			if (obj->m_view_list.count(npc->id) == 0) {
				obj->m_view_list.insert(npc->id);
				obj->m_slock.unlock();

				haveto_sleep = false;
				SendAddObject(pl, npc->id);
			} 
			else {
				obj->m_slock.unlock();
				haveto_sleep = false;
				SendMovePacket(pl, npc->id);
			}
		}
		// 보이지 않게 되었고 만약 npc를 보고있었다면
		// 삭제
		else {
			obj->m_slock.lock();
			if (0 != obj->m_view_list.count(npc->id)) {
				obj->m_view_list.erase(pl);
				obj->m_slock.unlock();
				//cout << "remove " << "\n";
				SendRemoveObject(pl, npc->id);
			}
			else {
				//cout << "단순히 안보임 처리\n";
				obj->m_slock.unlock();
			}
		}
	} 
	if (haveto_sleep) {
		npc->is_active = false;
		DoMonsterRespone(npc->id); 
	}
	else { 
		auto targetId = CheckCanAttack(npc->id);
		if (targetId == -1) {
			// 공격할 대상을 찾지 못한 경우이니 계속 이동을 합시다! 
			AddEvent(npc->id, OP_MONSTER_MOVE, 1000); 
		}
		else {
			// 공격할 대상을 찾았으니 공격상태로 갑시다!
			//cout << "몬스터가 공격할 대상을 찾았습니다! : " << npc->id << " - " << targetId << "\n";
			npc->path.clear();
			AddEvent(npc->id, OP_ATTACK, 1000);
		}
	}

	if ((prev_sector_x != cur_sector_x) ||
		(prev_sector_y != cur_sector_y)) {
		g_sector_locker.lock();
		if (g_sector[prev_sector_y][prev_sector_x].count(npc->id) != 0) {
			g_sector[prev_sector_y][prev_sector_x].erase(npc->id);
		}
		g_sector[cur_sector_y][cur_sector_x].insert(npc->id);
		g_sector_locker.unlock();
	}
}

void do_npc_random_move(ServerObject* npc)
{
	// 이동 반영을 하여 클라이언트에게 정보를 보내주기 위하여는
	// 시야 처리가 필요함..
	bool haveto_sleep = true;
	auto prev_sector_x = npc->x / SECTOR_SIZE;
	auto prev_sector_y = npc->y / SECTOR_SIZE;

	auto& x = npc->x;
	auto& y = npc->y;
	
	int sx = npc->start_x;
	int sy = npc->start_y;
	switch (rand() % 4)
	{
	case 0:
		if (x < WORLD_WIDTH && x < sx + SCREEN_WIDTH) {
			if ((int)g_TileDatas[y][x + 1] <= 7) {
				x++;
				npc->direction = DIRECTION::D_E;
			}
		}
		break;
	case 1: 
		if (x > 0 && x > sx - SCREEN_WIDTH) {
			if ((int)g_TileDatas[y][x - 1] <= 7) {
				x--;				
				npc->direction = DIRECTION::D_W;
			}
		} 
		break;
	case 2: 
		if (y < WORLD_HEIGHT && y < sy + SCREEN_HEIGHT) {
			if ((int)g_TileDatas[y + 1][x] <= 7) {
				y++;
				npc->direction = DIRECTION::D_S;
			}
		}
		break;
	case 3:
		if (y > 0 && y > sy - SCREEN_HEIGHT) {
			if ((int)g_TileDatas[y + 1][x] <= 7) {
				y--;
				npc->direction = DIRECTION::D_N;
			}
		}
		break;
	}

	int cur_sector_x = x / SECTOR_SIZE;
	int cur_sector_y = y / SECTOR_SIZE;
	 
	unordered_set<int> sector_copies[5];

	g_sector_locker.lock();
	sector_copies[0] = g_sector[cur_sector_y][cur_sector_x];
	unordered_set<int> prev_pos_sector_copy = g_sector[prev_sector_x][prev_sector_y];
	 
	if (((x + VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_x) {
		if (cur_sector_x + 1 < SECTOR_WIDTH + 1) {
			sector_copies[1] = g_sector[cur_sector_y][cur_sector_x + 1];
		}
	}
	if (((x - VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_x) {
		if (cur_sector_x - 1 > 0) {
			sector_copies[2] = g_sector[cur_sector_y][cur_sector_x - 1];
		}
	}
	if (((y + VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_y) {
		if (cur_sector_y + 1 < SECTOR_HEIGHT + 1) {
			sector_copies[3] = g_sector[cur_sector_y + 1][cur_sector_x];
		}
	}
	if (((y - VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_y) {
		if (cur_sector_y - 1 > 0) {
			sector_copies[4] = g_sector[cur_sector_y - 1][cur_sector_x];
		}
	}
	g_sector_locker.unlock();

	unordered_set<int> obj_vl;  
	for (int i = 0; i < 5; ++i) {
		if (sector_copies[i].size() == 0) {
			continue;
		}
		for (auto id : sector_copies[i]) {
			if (PLST_INGAME != objects[id]->m_state) {
				continue;
			}
			if (true == IsNpc(objects[id]->id)) {
				continue;
			}	
			if (obj_vl.count(id) == 0) {
				obj_vl.insert(id);
			}
		}
	}
	 
	// npc 행동 후 시야 범위 안에 플레이어가 존재하는 경우.
	for (auto pl : obj_vl) {
		auto obj = reinterpret_cast<SESSION*>(objects[pl]);
		// 플레이어에게 npc가 보인다면
		// 추가 or 이동
		if (IsInSight(pl, npc->id)) {
			obj->m_slock.lock();
			if (obj->m_view_list.count(npc->id) == 0) {
				obj->m_view_list.insert(npc->id);
				obj->m_slock.unlock();

				haveto_sleep = false;
				SendAddObject(pl, npc->id);
			}
			else {
				obj->m_slock.unlock();
				haveto_sleep = false;
				SendMovePacket(pl, npc->id);
			}
		}
		// 보이지 않게 되었고 만약 npc를 보고있었다면
		// 삭제
		else {
			obj->m_slock.lock();
			if (0 != obj->m_view_list.count(npc->id)) {
				obj->m_view_list.erase(pl);
				obj->m_slock.unlock();			
				SendRemoveObject(pl, npc->id);
			}
			else {
				obj->m_slock.unlock();
			}
		}  
	}
	 
	if (haveto_sleep) {
		npc->is_active = false;
		DoMonsterRespone(npc->id);
	}
	else {
		AddEvent(npc->id, OP_MONSTER_MOVE, 1000);
	}
	 
	if ((prev_sector_x != cur_sector_x) ||
		(prev_sector_y != cur_sector_y)) {
		g_sector_locker.lock();
		if (g_sector[prev_sector_y][prev_sector_x].count(npc->id) != 0) {
			g_sector[prev_sector_y][prev_sector_x].erase(npc->id);
		}
		g_sector[cur_sector_y][cur_sector_x].insert(npc->id);
		g_sector_locker.unlock();
	} 
}

void do_npc_script_move(ServerObject* npc, int prev_x, int prev_y)
{
	auto prev_sector_x = prev_x / SECTOR_SIZE;
	auto prev_sector_y = prev_y / SECTOR_SIZE;

	auto& x = npc->x;
	auto& y = npc->y;
	  
	int cur_sector_x = x / SECTOR_SIZE;
	int cur_sector_y = y / SECTOR_SIZE;

	unordered_set<int> sector_copies[5];

	g_sector_locker.lock();
	sector_copies[0] = g_sector[cur_sector_y][cur_sector_x];
	unordered_set<int> prev_pos_sector_copy = g_sector[prev_sector_x][prev_sector_y];

	if (((x + VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_x) {
		if (cur_sector_x + 1 < SECTOR_WIDTH + 1) {
			sector_copies[1] = g_sector[cur_sector_y][cur_sector_x + 1];
		}
	}
	if (((x - VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_x) {
		if (cur_sector_x - 1 > 0) {
			sector_copies[2] = g_sector[cur_sector_y][cur_sector_x - 1];
		}
	}
	if (((y + VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_y) {
		if (cur_sector_y + 1 < SECTOR_HEIGHT + 1) {
			sector_copies[3] = g_sector[cur_sector_y + 1][cur_sector_x];
		}
	}
	if (((y - VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_y) {
		if (cur_sector_y - 1 > 0) {
			sector_copies[4] = g_sector[cur_sector_y - 1][cur_sector_x];
		}
	}
	g_sector_locker.unlock();

	unordered_set<int> obj_vl;
	for (int i = 0; i < 5; ++i) {
		if (sector_copies[i].size() == 0) {
			continue;
		}
		for (auto id : sector_copies[i]) {
			if (PLST_INGAME != objects[id]->m_state) {
				continue;
			}
			if (true == IsNpc(objects[id]->id)) {
				continue;
			}
			if (obj_vl.count(id) == 0) {
				obj_vl.insert(id);
			}
		}
	}

	// 1. 이동 전에도 보임
	// 2. 이동 후에 보인다
	// 3. 이동 후에 안보인다

	// npc 행동 후 시야 범위 안에 플레이어가 존재하는 경우.
	for (auto pl : obj_vl) {
		auto obj = reinterpret_cast<SESSION*>(objects[pl]);
		// 플레이어에게 npc가 보인다면
		// 추가 or 이동
		if (IsInSight(pl, npc->id)) {
			obj->m_slock.lock();
			if (obj->m_view_list.count(npc->id) == 0) {
				obj->m_view_list.insert(npc->id);
				obj->m_slock.unlock();
				 
				SendAddObject(pl, npc->id);
			}
			else { 
				obj->m_slock.unlock();
				SendMovePacket(pl, npc->id);
			}
		}
		// 보이지 않게 되었고 만약 npc를 보고있었다면
		// 삭제
		else {
			obj->m_slock.lock();
			if (0 != obj->m_view_list.count(npc->id)) {
				obj->m_view_list.erase(pl);
				obj->m_slock.unlock(); 
				SendRemoveObject(pl, npc->id);
			}
			else {
				obj->m_slock.unlock();
			}
		} 
	}


	if ((prev_sector_x != cur_sector_x) ||
		(prev_sector_y != cur_sector_y)) {
		g_sector_locker.lock();
		if (g_sector[prev_sector_y][prev_sector_x].count(npc->id) != 0) {
			g_sector[prev_sector_y][prev_sector_x].erase(npc->id);
		}
		g_sector[cur_sector_y][cur_sector_x].insert(npc->id);
		g_sector_locker.unlock();
	}
}

void AddEvent(int obj, OP_TYPE e_type, int delay_ms)
{
	using namespace chrono;
	TIMER_EVENT ev;
	ev.e_type = e_type;
	ev.objcet = obj;
	ev.start_t = system_clock::now() + milliseconds(delay_ms);

	timer_lock.lock();
	timer_queue.push(ev);
	timer_lock.unlock();
}

void DoTimerWork()
{
	using namespace chrono;

	while (true) {
		timer_lock.lock();
		if (true == timer_queue.empty()) {
			timer_lock.unlock();
			this_thread::sleep_for(10ms);
		}
		else {
			if (timer_queue.top().start_t <= system_clock::now()) {
				// 이 경우에 실행하도록
				TIMER_EVENT ev = timer_queue.top();
				timer_queue.pop();
				timer_lock.unlock();

				EX_OVER* ex_over = new EX_OVER;
				ZeroMemory(ex_over, sizeof(EX_OVER));	 
				ex_over->m_op = ev.e_type; 
				PostQueuedCompletionStatus(h_iocp, 1, ev.objcet, &ex_over->m_over); 
			}
			else {
				timer_lock.unlock();
				this_thread::sleep_for(10ms);
			}
		}
	}
}

void WakeUpNPC(int pl_id, int npc_id)
{
	if (ObjectType::Mon_Agro_Fixed == objects[npc_id]->type ||
		ObjectType::Mon_Peace_Fixed == objects[npc_id]->type) {
		// 고정형 몬스터인 경우
		if (false == objects[npc_id]->is_active) {
			bool old_state = false;
			if (true == atomic_compare_exchange_strong(&objects[npc_id]->is_active, &old_state, true)) {
				AddEvent(npc_id, OP_MONSTER_MOVE, 1000);
			}
		}
	}
	else if (ObjectType::Mon_Peace_Roaming == objects[npc_id]->type||
		ObjectType::Mon_Agro_Roaming == objects[npc_id]->type) {
		// 로밍형 몬스터인 경우
		 
		if (false == objects[npc_id]->is_active) {
			objects[npc_id]->path.clear();
			bool old_state = false;
			if (true == atomic_compare_exchange_strong(&objects[npc_id]->is_active, &old_state, true)) {
				AddEvent(npc_id, OP_MONSTER_MOVE, 1000);
			}
		}
	}
}

int API_get_x(lua_State* L)
{
	int obj_id = lua_tonumber(L, -1);
	lua_pop(L, 2);
	int x = objects[obj_id]->x;
	lua_pushnumber(L, x);
	return 1;
}

int API_get_y(lua_State* L)
{
	int obj_id = lua_tonumber(L, -1);
	lua_pop(L, 2);
	int y = objects[obj_id]->y;
	lua_pushnumber(L, y);
	return 1;
}

int API_set_x(lua_State* L)
{
	int obj_id = (int)lua_tonumber(L, -2);
	int x = (int)lua_tonumber(L, -1);
	int prev_x = objects[obj_id]->x;
	lua_pop(L, 2); 


	if ((int)g_TileDatas[objects[obj_id]->y][x] <= 7) {
		if (objects[obj_id]->HP > 0 && objects[obj_id]->HP <= 25) {
			objects[obj_id]->x = x;
			do_npc_script_move(objects[obj_id], prev_x, objects[obj_id]->y);
		}
	}
	return 1;
}

int API_set_y(lua_State* L)
{
	int obj_id = (int)lua_tonumber(L, -2);
	int y = (int)lua_tonumber(L, -1);
	int prev_y = objects[obj_id]->x; 
	lua_pop(L, 2);
	
	if ((int)g_TileDatas[y][objects[obj_id]->x] <= 7) {
		if (objects[obj_id]->HP > 0 && objects[obj_id]->HP <= 25) {
			objects[obj_id]->y = y;
			do_npc_script_move(objects[obj_id], objects[obj_id]->x, prev_y);
		}
	}
	return 1;
}

int API_set_move_type(lua_State* L)
{
	int npc_id = (int)lua_tonumber(L, -2);
	int move_type = (int)lua_tonumber(L, -1); 
	lua_pop(L, 2);

	//bool old_state = true;
	//atomic_compare_exchange_strong(&objects[npc_id]->is_active, &old_state, false);
	objects[npc_id]->isOnRunaway = move_type;
	if (move_type == 0) {
		objects[npc_id]->targetId = 0;
	}
	//objects[npc_id]->npc_move_type = (NPC_MOVE_TYPE)move_type;
	return 1;
} 

void SendChatPacket(int c_id, int p_id, const char* mess)
{
	sc_packet_chat p;
	p.id = p_id;
	p.size = sizeof(p);
	p.type = SC_CHAT;
	strcpy_s(p.message, mess);
	SendPacket(c_id, &p);
}

void SendRemovePacketToViewObjects(int target)
{ 
	auto& x = objects[target]->x;
	auto& y = objects[target]->y;
	  
	int cur_sector_x = x / SECTOR_SIZE;
	int cur_sector_y = y / SECTOR_SIZE;

	unordered_set<int> sector_copies[5];

	g_sector_locker.lock();
	sector_copies[0] = g_sector[cur_sector_y][cur_sector_x]; 

	if (((x + VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_x) {
		if (cur_sector_x + 1 < SECTOR_WIDTH + 1) {
			sector_copies[1] = g_sector[cur_sector_y][cur_sector_x + 1];
		}
	}
	if (((x - VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_x) {
		if (cur_sector_x - 1 > 0) {
			sector_copies[2] = g_sector[cur_sector_y][cur_sector_x - 1];
		}
	}
	if (((y + VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_y) {
		if (cur_sector_y + 1 < SECTOR_HEIGHT + 1) {
			sector_copies[3] = g_sector[cur_sector_y + 1][cur_sector_x];
		}
	}
	if (((y - VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_y) {
		if (cur_sector_y - 1 > 0) {
			sector_copies[4] = g_sector[cur_sector_y - 1][cur_sector_x];
		}
	}
	g_sector_locker.unlock();

	unordered_set<int> obj_vl;
	for (int i = 0; i < 5; ++i) {
		if (sector_copies[i].size() == 0) {
			continue;
		}
		for (auto id : sector_copies[i]) {
			if (PLST_INGAME != objects[id]->m_state) {
				continue;
			}
			if (obj_vl.count(id) == 0) {
				obj_vl.insert(id);
			}
		}
	}

	// 시야 범위 안에 플레이어가 존재하는 경우. 
	for (auto pl : obj_vl) {
		if (true == IsNpc(pl)) {
			if (target == objects[pl]->targetId) {
				objects[pl]->targetId = 0;
			}
			continue;
		}
		auto obj = reinterpret_cast<SESSION*>(objects[pl]); 

		obj->m_vl.lock();
		// 시야 범위안에 있었다면 제거
		if (obj->m_view_list.count(target) != 0) { 
			//cout << "죽은 몬스터 시야범위에서 제거했다!\n";
			obj->m_view_list.erase(target);
			SendRemoveObject(pl, target);
		}  
		obj->m_vl.unlock(); 
	}  
}

void SendStatPacketToViewObjects(int target, void* packet)
{
	auto& x = objects[target]->x;
	auto& y = objects[target]->y;

	int cur_sector_x = x / SECTOR_SIZE;
	int cur_sector_y = y / SECTOR_SIZE;

	unordered_set<int> sector_copies[5];

	g_sector_locker.lock();
	sector_copies[0] = g_sector[cur_sector_y][cur_sector_x]; 

	if (((x + VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_x) {
		if (cur_sector_x + 1 < SECTOR_WIDTH + 1) {
			sector_copies[1] = g_sector[cur_sector_y][cur_sector_x + 1];
		}
	}
	if (((x - VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_x) {
		if (cur_sector_x - 1 > 0) {
			sector_copies[2] = g_sector[cur_sector_y][cur_sector_x - 1];
		}
	}
	if (((y + VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_y) {
		if (cur_sector_y + 1 < SECTOR_HEIGHT + 1) {
			sector_copies[3] = g_sector[cur_sector_y + 1][cur_sector_x];
		}
	}
	if (((y - VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_y) {
		if (cur_sector_y - 1 > 0) {
			sector_copies[4] = g_sector[cur_sector_y - 1][cur_sector_x];
		}
	}
	g_sector_locker.unlock();

	unordered_set<int> obj_vl;
	for (int i = 0; i < 5; ++i) {
		if (sector_copies[i].size() == 0) {
			continue;
		}
		for (auto id : sector_copies[i]) {
			if (PLST_INGAME != objects[id]->m_state) {
				continue;
			}
			if (true == IsNpc(objects[id]->id)) {
				continue;
			}
			if (obj_vl.count(id) == 0) {
				obj_vl.insert(id);
			}
		}
	}

	// 자기 자신에게도 알림을 보낸다
	if (false == IsNpc(target)) {
		SendPacket(target, packet);
	}

	// 시야 범위 안에 플레이어가 존재하는 경우.
	// npc인 경우는 위에서 걸러짐
	for (auto pl : obj_vl) {
		auto obj = reinterpret_cast<SESSION*>(objects[pl]);
		 
		// 시야 범위안에 있었다면 제거
		obj->m_vl.lock();
		if (obj->m_view_list.count(target) != 0) { 
			SendPacket(pl, packet);		
		}
		obj->m_vl.unlock();
	}
}

int API_send_mess(lua_State* L)
{
	int player_id = lua_tonumber(L, -3);
	int npc_id = lua_tonumber(L, -2);
	const char* mess = lua_tostring(L, -1);
	lua_pop(L, 4);
	SendChatPacket(player_id, npc_id, mess); 
	return 1;
}

int API_add_event(lua_State* L)
{
	int npc_id = (int)lua_tonumber(L, -3);
	int delay = (int)lua_tonumber(L, -2); 
	int op_type = (int)lua_tonumber(L, -1);
	lua_pop(L, 4);

	AddEvent(npc_id, (OP_TYPE)op_type, delay);

	return 1;
}

void DisplayLuaError(lua_State* L, const char* fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);
	vfprintf(stderr, fmt, argp);
	va_end(argp);
	lua_close(L);
	exit(EXIT_FAILURE);
}

void ReadMapData()
{
	ifstream fileIn("Resources/MapInfos.txt");
	for (int i = 0; i < WORLD_HEIGHT; ++i)
	{
		for (int j = 0; j < WORLD_WIDTH; ++j)
		{
			string text; fileIn >> text;
			g_TileDatas[i][j] = (MAP_TILE_DATA)stoi(text);
		}
	}

	cout << "맵 데이터 읽기 완료!\n";
}

void InitObjects()
{
	for (int i = SERVER_ID; i <= MAX_USER; ++i) {
		if (true == IsNpc(i)) {
			objects[i] = new ServerObject();
		}
		else {
			objects[i] = new SESSION();
		}
	}
	for (int i = SERVER_ID; i <= MAX_USER; ++i) {
		auto& pl = objects[i];
		pl->id = i;
		pl->startHP = pl->HP = 100;
		pl->EXP = 0;
		pl->LEVEL = 1;
		pl->power = 25;  
		pl->m_state = PLST_FREE; 
		pl->type = (ObjectType::Player);
		if (true == IsNpc(i)) {
			sprintf_s(pl->m_name, "N%d", i);
			pl->m_state = PLST_INGAME;

			while (1) {
				int x = rand() % WORLD_WIDTH;
				int y = rand() % WORLD_HEIGHT;
				if ((int)g_TileDatas[y][x] <= 7) {
					pl->start_x = pl->x = x;
					pl->start_y = pl->y = y;
					break;
				}
			} 
			pl->GOLD = rand() % 10;
			pl->type = (ObjectType)(rand() % 4 + 2);
			pl->path.reserve(300);
			pl->isOnRunaway = false;
			int cur_sector_x = pl->x / SECTOR_SIZE;
			int cur_sector_y = pl->y / SECTOR_SIZE;
			g_sector[cur_sector_y][cur_sector_x].insert(pl->id);

			pl->L = luaL_newstate();
			luaL_openlibs(pl->L);
			luaL_loadfile(pl->L, "npc_act.lua");

			int res = lua_pcall(pl->L, 0, 0, 0);
			if (0 != res) {
				DisplayLuaError(pl->L, "error running function ‘XXX’: %s\n",
					lua_tostring(pl->L, -1));
			}
			lua_getglobal(pl->L, "set_uid");
			lua_pushnumber(pl->L, i);
			lua_pcall(pl->L, 1, 0, 0);

			lua_register(pl->L, "API_get_x", API_get_x);
			lua_register(pl->L, "API_get_y", API_get_y);

			lua_register(pl->L, "API_set_x", API_set_x);
			lua_register(pl->L, "API_set_y", API_set_y);

			lua_register(pl->L, "API_set_move_type", API_set_move_type);

			//lua_register(pl->L, "API_set_move_count", API_set_move_count);
			lua_register(pl->L, "API_send_mess", API_send_mess);
			lua_register(pl->L, "API_add_event", API_add_event);
		}
	}

	cout << "객체 준비 완료!\n";
}

void DoPlayerRespone(int id)
{
	objects[id]->HP = 100;
	objects[id]->EXP *= 0.5f;
	//objects[id]->x = PLAYER_START_POS_X;
	//objects[id]->y = PLAYER_START_POS_Y;
	
	int prev_sector_x = objects[id]->x / SECTOR_SIZE;
	int prev_sector_y = objects[id]->y / SECTOR_SIZE;
	
	objects[id]->x = rand() % 100 - 50 + PLAYER_START_POS_X;
	objects[id]->y = rand() % 100 - 50 + PLAYER_START_POS_Y;
	//cout << "Player Respone!\n";
	SendStatChangePacket(id);
	SendMovePacket(id, id);
	 
	int sector_x = objects[id]->x / SECTOR_SIZE;
	int sector_y = objects[id]->y / SECTOR_SIZE;
	g_sector_locker.lock();
	if (g_sector[prev_sector_y][prev_sector_x].count(id) != 0) {
		g_sector[prev_sector_y][prev_sector_x].erase(id);
	}
	g_sector[sector_y][sector_x].insert(id);
	g_sector_locker.unlock();
	
	auto obj = reinterpret_cast<SESSION*>(objects[id]);
	for (auto& pl : objects) {
		if (id != pl->id) {
			lock_guard <mutex> gl{ pl->m_slock };
			if (PLST_INGAME == pl->m_state) {
				// 모든 플레이어의 정보를 보내던 구조에서
				// 주위에 있는 플레이어의 정보만 전송하도록 변경

				if (IsInSight(id, pl->id)) {
					obj->m_vl.lock();
					obj->m_view_list.insert(pl->id); // 나에게 상대방을
					obj->m_vl.unlock();
					SendAddObject(id, pl->id);

					if (false == IsNpc(pl->id)) {
						auto obj_pl = reinterpret_cast<SESSION*>(objects[pl->id]);
						obj_pl->m_vl.lock();
						obj_pl->m_view_list.insert(id); // 상대방에게 나를
						obj_pl->m_vl.unlock();
						SendAddObject(pl->id, id);
					}
					else {
						if (objects[pl->id]->HP > 0) {
							// 리스폰 중이 아니라면 깨운다
							WakeUpNPC(id, pl->id);
						}
					}
				}
			}
		}
	} 
}

void DoMonsterRespone(int id)
{
	//cout << "몬스터 정보 초기화!\n";
	objects[id]->x = objects[id]->start_x;
	objects[id]->y = objects[id]->start_y;
	objects[id]->is_active = false;
	objects[id]->path.clear();
	objects[id]->targetId = 0;
	objects[id]->HP = objects[id]->startHP;
}


int CheckCanAttack(int atteckerId)
{
	if( objects[atteckerId]->targetId != 0 ){
		int x = objects[atteckerId]->x;
		int y = objects[atteckerId]->y;
		int tx = objects[objects[atteckerId]->targetId]->x;
		int ty = objects[objects[atteckerId]->targetId]->y;

		if (tx == x - 1 && ty == y) {// 왼쪽 
			return objects[atteckerId]->targetId;
		}
		else if (tx == x + 1 && ty == y) { // 오른쪽 
			return objects[atteckerId]->targetId;
		}
		else if (tx == x && ty == y + 1) { // 아래쪽 
			return objects[atteckerId]->targetId;
		}
		else if (tx == x && ty == y - 1) { // 위쪽 
			return objects[atteckerId]->targetId;
		}

		return -1;
	}
	int cur_sector_x = objects[atteckerId]->x / SECTOR_SIZE;
	int cur_sector_y = objects[atteckerId]->y / SECTOR_SIZE;

	int x = objects[atteckerId]->x;
	int y = objects[atteckerId]->y;

	unordered_set<int> sector_copies[5];

	g_sector_locker.lock();
	sector_copies[0] = g_sector[cur_sector_y][cur_sector_x];
	 
	if (((x + VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_x) {
		if (cur_sector_x + 1 < SECTOR_WIDTH + 1) {
			sector_copies[1] = g_sector[cur_sector_y][cur_sector_x + 1];
		}
	}
	if (((x - VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_x) {
		if (cur_sector_x - 1 > 0) {
			sector_copies[2] = g_sector[cur_sector_y][cur_sector_x - 1];
		}
	}
	if (((y + VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_y) {
		if (cur_sector_y + 1 < SECTOR_HEIGHT + 1) {
			sector_copies[3] = g_sector[cur_sector_y + 1][cur_sector_x];
		}
	}
	if (((y - VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_y) {
		if (cur_sector_y - 1 > 0) {
			sector_copies[4] = g_sector[cur_sector_y - 1][cur_sector_x];
		}
	}
	g_sector_locker.unlock();
	 

	unordered_set<int> obj_vl;
	for (int i = 0; i < 5; ++i) {
		if (sector_copies[i].size() == 0) {
			continue;
		}
		for (auto id : sector_copies[i]) {
			if (id == atteckerId) {
				continue;
			}
			if (PL_STATE::PLST_INGAME == objects[id]->m_state) {
				if (obj_vl.count(id) == 0) {

					// 몬스터가 플레이어를 공격하는 경우!
					if (false == IsNpc(id)) {
						obj_vl.insert(id);
					}
				}
			}
		}
	}

	int px = objects[atteckerId]->x;
	int py = objects[atteckerId]->y;
	for (auto pl : obj_vl) {
		int x = objects[pl]->x;
		int y = objects[pl]->y;

		if (px == x - 1 && py == y) {// 왼쪽 
			return pl;
		}
		else if (px == x + 1 && py == y) { // 오른쪽 
			return pl;
		}
		else if (px == x && py == y + 1) { // 아래쪽 
			return pl;
		}
		else if (px == x && py == y - 1) { // 위쪽 
			return pl;
		}
	} 
	return -1;
}

void ExpUp(int playerId, int monId)
{
	int expBase = objects[playerId]->LEVEL;
	expBase = expBase * expBase * 2;
	int expIncreased = 0;
	switch (objects[monId]->type)
	{ 
	case ObjectType::Mon_Peace_Roaming:
		expIncreased = expBase * 2;
		break;
	case ObjectType::Mon_Peace_Fixed:
		expIncreased = expBase;
		break;
	case ObjectType::Mon_Agro_Roaming:
		expIncreased = expBase * 4;
		break;
	case ObjectType::Mon_Agro_Fixed:
		expIncreased = expBase;
		break;
	default:
		cout << "ExpUp() - 불가능한 상황\n";
		break;
	}
	int playerExp = objects[playerId]->EXP;
	int playerLevel = objects[playerId]->LEVEL;
	playerExp += expIncreased;
	string mess = string(objects[monId]->m_name) + "를 무찔러서 " + to_string(expIncreased) +
		"의 경험치를 얻었습니다.";
	SendChatPacket(playerId, 0, mess.c_str());
	while (1) {
		auto requireExp = pow(2, playerLevel - 1) * 100; 
		
		if (requireExp > playerExp) {
			break;
		}		
		else {
			SendChatPacket(playerId, 0, "------------------레벨 업!!!!!------------------");
			playerExp -= requireExp;
			playerLevel += 1;
		}
	}

	objects[playerId]->LEVEL = playerLevel;
	objects[playerId]->EXP = playerExp;
	SendStatChangePacket(playerId);

}

void GoldUp(int playerId, int monId)
{
	int gold = objects[monId]->GOLD;
	 
	if (gold == 0) {
		return;
	}
	string mess = string(objects[monId]->m_name) + "를 무찔러서 " + to_string(gold) +
		"의 골드를 얻었습니다.";
	SendChatPacket(playerId, 0, mess.c_str()); 
	objects[playerId]->GOLD += gold; 
	SendStatChangePacket(playerId);
}

void GetItem(int playerId, int monId, ITEM_TYPE itemType)
{
	if (objects[playerId]->itemCount >= MAX_ITEM_COUNT) {
		return;
	}
	// 대략 30%확률로 아이템을 획득하도록...
	if (rand() % 3 == 2) {
		return;
	}

	ITEM_TYPE newItem = ITEM_TYPE(rand() % 4 + 1);
	string mess;
	if (monId != 0) {
		mess = string(objects[monId]->m_name) + "를 무찔러서 " + itemNames[(int)newItem] +
			" 을 획득 하였습니다.";
	}
	else {
		newItem = itemType;
		mess = itemNames[itemType] +
			" 을 구매 하였습니다.";
	}
	objects[playerId]->Items.push_back(newItem);
	objects[playerId]->Items[objects[playerId]->itemCount] = newItem;
	objects[playerId]->itemCount++;

	SendChatPacket(playerId, 0, mess.c_str()); 
	SendAddItem(playerId, newItem);
}

void UseItem(int playerId, int itemIdx)
{ 
	if (objects[playerId]->itemCount <= itemIdx) {
		return;
	}
	auto itemType = objects[playerId]->Items[itemIdx];
	objects[playerId]->Items.erase(objects[playerId]->Items.begin() + itemIdx);  
	objects[playerId]->itemCount--;

	cout << itemNames[itemType] << "을 사용하였습니다.\n";

	string mess = string(objects[playerId]->m_name)+ "가 " +
		itemNames[itemType] + "을 사용하였습니다.";
	SendChatPacket(playerId, 0, mess.c_str());


	// 아이템 효과 코드가 적용되어야 할 영역...
	switch (itemType)
	{
	case I_NOT:
		break;
	case I_POTION:
		AddEvent(playerId, OP_POTION_HEAL, 0);
		break;
	case I_LEVELUP:
		SendChatPacket(playerId, 0, "------------------레벨 업!!!!!------------------");
		objects[playerId]->EXP = 0;
		objects[playerId]->LEVEL += 1;

		SendStatChangePacket(playerId);
		break;
	case I_BOMB:
	{ 
		auto obj = reinterpret_cast<SESSION*>(objects[playerId]);
		unordered_set <int> old_vl;
		obj->m_vl.lock();
		old_vl = obj->m_view_list;
		obj->m_vl.unlock(); 

		unordered_set<int> sector_copies[5];

		int cur_sector_x = obj->x / SECTOR_SIZE;
		int cur_sector_y = obj->y / SECTOR_SIZE;
		g_sector_locker.lock();
		sector_copies[0] = g_sector[cur_sector_y][cur_sector_x]; 

		if (((obj->x + VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_x) {
			if (cur_sector_x + 1 < SECTOR_WIDTH + 1) {
				sector_copies[1] = g_sector[cur_sector_y][cur_sector_x + 1];
			}
		}
		if (((obj->x - VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_x) {
			if (cur_sector_x - 1 > 0) {
				sector_copies[2] = g_sector[cur_sector_y][cur_sector_x - 1];
			}
		}
		if (((obj->y + VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_y) {
			if (cur_sector_y + 1 < SECTOR_HEIGHT + 1) {
				sector_copies[3] = g_sector[cur_sector_y + 1][cur_sector_x];
			}
		}
		if (((obj->y - VIEW_RADIUS) / SECTOR_SIZE) != cur_sector_y) {
			if (cur_sector_y - 1 > 0) {
				sector_copies[4] = g_sector[cur_sector_y - 1][cur_sector_x];
			}
		}
		g_sector_locker.unlock();

		unordered_set<int> obj_vl;
		for (int i = 0; i < 5; ++i) {
			if (sector_copies[i].size() == 0) {
				continue;
			}
			for (auto id : sector_copies[i]) {
				if (id == playerId) {
					continue;
				}
				if (false == IsNpc(id)) {
					continue;
				}
				if (objects[id]->HP <= 0) {
					continue;
				}

				if (obj_vl.count(id) == 0) {
					obj_vl.insert(id);
				}
			}
		}

		for (auto pl : obj_vl) { 

			attacked(playerId, pl, 100);
			// 죽었다 - 처리하자
			if (objects[pl]->HP <= 0) {
				// 쳐다보고 있는 모든 플레이어들에게 remove시키고
				// 몬스터라면 리스폰하도록..

				SendRemovePacketToViewObjects(pl);
				if (IsNpc(pl)) {
					// 리스폰~~~
					//cout << pl << " - 몬스터 리스폰!!!!!!!!!!!!!!!!!!\n";
					// 아이템으로 죽였으니 부가 효과는 없어요~
					//ExpUp(p_id, pl);
					//GoldUp(p_id, pl);
					//GetItem(p_id, pl);

					bool old_state = false;
					if (true == atomic_compare_exchange_strong(&objects[pl]->is_active, &old_state, true)) {
						AddEvent(pl, OP_RESPONE, 30000);
					}
				}
			}
		}
	}
	break;
	case I_POWERUP: 
		AddEvent(playerId, OP_FULL_HEAL, 0);
		break;
	default:
		break;
	}
} 