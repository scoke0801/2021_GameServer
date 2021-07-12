#include "stdafx.h"
#include "GameObject.h"

CImage CGameObject::m_Image;
bool CGameObject::m_ImageLoad = false;
CGameObject::CGameObject()
{
	if (false == m_ImageLoad) { 
		m_Image.Load(L"Resources/Units.png");
		m_ImageLoad = true;
	}
	m_TileIndex = {0,0};
	m_Position = { MAP_TILE_SIZE.x * m_TileIndex.x,  MAP_TILE_SIZE.y * m_TileIndex.y }; 
}

CGameObject::CGameObject(LPCTSTR imageAddr, const Vector2i& tileIndex)
{
	m_Image.Load(imageAddr);
	m_TileIndex = tileIndex;
	m_Position = { MAP_TILE_SIZE.x * tileIndex.x,  MAP_TILE_SIZE.y * tileIndex.y };
}

CGameObject::~CGameObject()
{
}

void CGameObject::Update(float elapsedTime)
{
}

void CGameObject::Draw(HDC hdc, int view_x, int view_y)
{
	if (false == m_IsOnDraw) {
		return;
	}
	int yPositon = (m_isClientPlayer) ? m_Image.GetHeight() / 2 : 0;
	m_Image.TransparentBlt(hdc, 
		m_Position.x - view_x, m_Position.y - view_y,
		MAP_TILE_SIZE.x, MAP_TILE_SIZE.y,
		m_Image.GetWidth() / 6, yPositon,
		m_Image.GetWidth() / 6,
		m_Image.GetHeight() / 2,
		RGB(255, 0, 255));
	SetBkMode(hdc, TRANSPARENT);
	TextOut(hdc, m_Position.x - view_x, m_Position.y - view_y - 10, m_WName.c_str(), lstrlen(m_WName.c_str()));
}

void CGameObject::MoveTo(Vector2i toTile)
{
	if (toTile.x < 0) toTile.x = 0;
	if (toTile.x >= WORLD_WIDTH) toTile.x = WORLD_WIDTH - 1;
	if (toTile.y < 0) toTile.y = 0;
	if (toTile.y >= WORLD_HEIGHT) toTile.y = WORLD_HEIGHT - 1;
	m_TileIndex = toTile;
	m_Position = { MAP_TILE_SIZE.x * toTile.x, MAP_TILE_SIZE.y * toTile.y }; 
} 