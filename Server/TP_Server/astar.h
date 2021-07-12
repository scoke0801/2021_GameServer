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
	int movedDist; //�̵��� �Ÿ�
	float hValue; // ����ġ
};
bool operator<(const H& lhs, const H& rhs);

// return : ã�� ���� or ����
// int sx, sy : ���� ����
// int tx, ty : ��ǥ ����
// vector<Point2i> : ���� �� ��ǥ ���������� ���
bool FindPath(int sx, int sy, int tx, int ty, int monId);
