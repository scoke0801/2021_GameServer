#include "stdafx.h"
#include "astar.h"

extern priority_queue<TIMER_EVENT> timer_queue;
extern mutex timer_lock;

extern HANDLE h_iocp;
extern MAP_TILE_DATA g_TileDatas[WORLD_HEIGHT][WORLD_WIDTH];

// ���� ũ�� ���� ���� Ŭ�󿡼� �� �ʿ� ����...
extern unordered_set<int> g_sector[SECTOR_HEIGHT + 1][SECTOR_WIDTH + 1];
extern mutex	g_sector_locker;

extern array <ServerObject*, MAX_USER + 1> objects;
 
using NodeH = pair<H, Node>;

bool operator<(const Point2i& lhs, const Point2i& rhs)
{
	if (lhs.x == rhs.x) {
		return lhs.y < rhs.y;
	}
	return lhs.x < rhs.x;
}

bool operator<(const Node& lhs, const Node& rhs)
{
	return false;
}

bool operator<(const H& lhs, const H& rhs)
{
	return (lhs.movedDist + lhs.hValue) <
		(rhs.movedDist + rhs.hValue);
} 

constexpr int MAX_FIND_COUNT = 75;
bool FindPath(int sx, int sy, int tx, int ty, int monId)
{ 
	objects[monId]->path.clear(); 

	if (sx == tx && sy == ty) {
		return true;
	}
	priority_queue<NodeH, vector<NodeH>, greater<NodeH>> openQ;
	map<Point2i, Point2i> closedQ;

	// ���� ���� ���� ��ġ�� ��������
	NodeH startNode;
	startNode.second.curPos = { sx, sy };
	startNode.second.prevPos = { -1, -1 };
	startNode.first.movedDist = 0;
	startNode.first.hValue = -1;

	openQ.push(startNode);

	int count = 0;

	while (false == openQ.empty()) {
		++count;
		if (count > MAX_FIND_COUNT) {
			// ����Ƚ���̻� ��ã������ ��ȯ!
			return false; 
		}

		NodeH temp = openQ.top();
		openQ.pop();

		Node curNode = temp.second;
		H curHValue = temp.first;

		if (closedQ.find(curNode.curPos) != closedQ.end()) {
			continue;
		}

		// Ž�� ���� ���� ���� ��忡 �߰�
		closedQ.insert({ curNode.curPos, curNode.prevPos });

		int x = curNode.curPos.x;
		int y = curNode.curPos.y;

		if (x == tx && y == ty) {
			// ã������ ��!
			break;
		}

		for (int i = 0; i < 4; ++i) {
			int nextX = x, nextY = y;
			switch ((DIRECTION)i) {
			case D_N: if (y > 0) nextY--; break;
			case D_S: if (y < (WORLD_HEIGHT - 1)) nextY++; break;
			case D_W: if (x > 0) nextX--; break;
			case D_E: if (x < (WORLD_WIDTH - 1)) nextX++; break;
			}
			if (nextX >= 0 && nextX < WORLD_WIDTH && nextY >= 0 && nextY < WORLD_HEIGHT)
			{
				
				// ���� 7 �̻��̶�� ��ֹ�! - �����Ѵ�!
				// �̹� �湮�ߴ� ���̶��? - �����Ѵ�!
				if ((int)g_TileDatas[nextY][nextX] >= (int)MAP_TILE_DATA::TILE_GRASS_TREE ||
					closedQ.find({ nextX,nextY }) != closedQ.end())
					continue;

				// �޸���ƽ���� �� ������ �Ÿ���
				int deltaX = nextX - tx;
				int deltaY = nextY - ty;
				float directDist = sqrt(deltaX * deltaX + deltaY * deltaY);

				H hVal;
				hVal.movedDist = curHValue.movedDist + 1;
				hVal.hValue = directDist;

				Node nextNode;
				nextNode.curPos.x = nextX;
				nextNode.curPos.y = nextY;
				nextNode.prevPos.x = x;
				nextNode.prevPos.y = y;

				openQ.push({ hVal, nextNode });
			}
		}
	}
	// ���������� ���� ��ΰ� ������ �� ������Ϳ� �ִ´�.
	if (closedQ.find({ tx, ty }) != closedQ.end())
	{
		Point2i location = { tx, ty };
		while (location.x != -1)
		{
			objects[monId]->path.push_back(location);
			location = closedQ[location];
		} 
		if (false == objects[monId]->path.empty()) {
			// ���� ��ġ�� �������� �ʵ��� ����
			objects[monId]->path.erase(objects[monId]->path.begin());
			objects[monId]->path.pop_back();
		}
		return true;
	}
	else {
		return false;
	}
}
