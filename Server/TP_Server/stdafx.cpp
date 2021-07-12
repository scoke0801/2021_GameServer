#include "stdafx.h"

priority_queue<TIMER_EVENT> timer_queue;
mutex timer_lock;

queue<DB_EVENT> db_timer_queue;
mutex db_timer_lock;

HANDLE h_iocp;
MAP_TILE_DATA g_TileDatas[WORLD_HEIGHT][WORLD_WIDTH];

unordered_set<int> g_sector[SECTOR_HEIGHT + 1][SECTOR_WIDTH + 1];
mutex	g_sector_locker;

array <ServerObject*, MAX_USER + 1> objects; 
 
string itemNames[5] = { "", "포션","기력회복", "폭탄", "레벨업",  };