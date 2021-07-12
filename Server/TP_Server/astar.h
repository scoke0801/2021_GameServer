#pragma once
#include <vector>
struct Point2i {
	int x, y;

	Point2i() { x = y = 0; }
	Point2i(int x, int y) :x(x), y(y) {}
};

bool operator<(const Point2i& lhs, const Point2i& rhs);

struct Node {
	Point2i	prevPos;
	Point2i curPos;
};
bool operator < (const Node& lhs, const Node& rhs);

struct H {
	int movedDist; //이동한 거리
	float hValue; // 가중치
};
bool operator<(const H& lhs, const H& rhs);

// return : 찾기 성공 or 실패
// int sx, sy : 시작 지점
// int tx, ty : 목표 지점
// vector<Point2i> : 성공 시 목표 지점까지의 경로
bool FindPath(int sx, int sy, int tx, int ty, int monId);
