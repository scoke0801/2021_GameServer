#include "stdafx.h"
#include "GameObject.h"

CImage CGameObject::m_Image; 
CImage CGameObject::m_PlayerImage; 
CImage CGameObject::m_MonAgroRoamingImage;
CImage CGameObject::m_MonAgroFixedImage;
CImage CGameObject::m_MonPeaceFixedImage;
CImage CGameObject::m_MonPeaceRoamingImage; 
CImage CGameObject::m_ItemImage;	
CImage CGameObject::m_Seller;
bool CGameObject::m_ImageLoad = false;

CGameObject::CGameObject()
{
	if (false == m_ImageLoad) { 
		m_Image.Load(L"Resources/Units.png");
		 
		m_PlayerImage.Load(L"Resources/zelda.png"); 

		m_MonAgroRoamingImage.Load(L"Resources/skeleton.png"); 
		m_MonPeaceRoamingImage.Load(L"Resources/thief.png");


		m_MonAgroFixedImage.Load(L"Resources/MonAgroFixed.png"); 
		m_MonPeaceFixedImage.Load(L"Resources/MonPeaceFixed.png");


		m_ItemImage.Load(L"Resources/Items.png"); 
		m_Seller.Load(L"Resources/seller.png");
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
	switch (m_Type)
	{
	case ObjectType::None:
		break;
	case ObjectType::Player:
	case ObjectType::Mon_Peace_Roaming:
	case ObjectType::Mon_Agro_Roaming:
		m_AnimationIndex.x += elapsedTime * m_ImageUpdateTime;
		
		if (m_AnimationIndex.x >= m_ImagePosition[(int)m_State]) {
			if (STATE::WALK == m_State) {
				m_AnimationIndex.x = 0;
			}
			else {
				m_State = STATE::WALK;
				m_AnimationIndex.x = 0;
				//m_AnimationIndex.x = m_ImagePosition[0];
			}
		} 
		break;
	case ObjectType::Mon_Peace_Fixed:  
		break;
	case ObjectType::Mon_Agro_Fixed: 
		break;
	case ObjectType::Seller:
		m_AnimationIndex.x += elapsedTime *0.0625f ;

		if (m_AnimationIndex.x >= 16) {
			m_AnimationIndex.x = 0;
		}
		break;
	default:
		cout << "Update - 이런 몬스터 없어용\n";
		break;
	}
}

void CGameObject::Draw(HDC hdc, int view_x, int view_y)
{
	if (false == m_IsOnDraw) {
		return;
	}

	switch (m_Type)
	{
	case ObjectType::None:
		break;
	case ObjectType::Player:
		DrawPlayer(hdc, view_x, view_y);
		DrawExtraInfos(hdc, view_x, view_y, true, false); 
		break;
	case ObjectType::Mon_Peace_Roaming:
		DrawMonPeaceRoaming(hdc, view_x, view_y);
		DrawExtraInfos(hdc, view_x, view_y, true, true);
		break;
	case ObjectType::Mon_Peace_Fixed:
		DrawMonPeaceFixed(hdc, view_x, view_y); 
		DrawExtraInfos(hdc, view_x, view_y, true, true);
		break;
	case ObjectType::Mon_Agro_Roaming:
		DrawMonAgroRoaming(hdc, view_x, view_y); 
		DrawExtraInfos(hdc, view_x, view_y, true, true);
		break;
	case ObjectType::Mon_Agro_Fixed:
		DrawMonAgroFixed(hdc, view_x, view_y); 
		DrawExtraInfos(hdc, view_x, view_y, true, true);
		break;
	case ObjectType::Seller:
		DrawSeller(hdc, view_x, view_y);
		DrawExtraInfos(hdc, view_x, view_y, true, false);
		break;
	default:
		cout << "Update - 이런 몬스터 없어용\n";
		{ 
			int yPositon = (m_isClientPlayer) ? m_Image.GetHeight() / 2 : 0;
			m_Image.TransparentBlt(hdc,
				m_Position.x - view_x, m_Position.y - view_y,
				MAP_TILE_SIZE.x, MAP_TILE_SIZE.y,
				m_Image.GetWidth() / 6, yPositon,
				m_Image.GetWidth() / 6,
				m_Image.GetHeight() / 2,
				RGB(255, 0, 255));
		}
		break;
	}
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

void CGameObject::SetType(ObjectType type)
{
	m_Type = type;
	switch (m_Type)
	{
	case ObjectType::None:
		break;
	case ObjectType::Player: 
		m_ImagePosition[0] = 6;
		m_ImagePosition[1] = 9;
		break;
	case ObjectType::Mon_Peace_Roaming:	// thief
		m_ImagePosition[0] = 4;
		m_ImagePosition[1] = 7;
		break;
	case ObjectType::Mon_Peace_Fixed:  
		break;
	case ObjectType::Mon_Agro_Roaming: // skeleton
		m_ImagePosition[0] = 3;
		m_ImagePosition[1] = 8;
		break;
	case ObjectType::Mon_Agro_Fixed: 
		break;
	case ObjectType::Seller: 
		break;
	default: 
		break;
	}
	ChangeAnimationUpdateTime();
}

void CGameObject::SetState(STATE state)
{ 
	m_State = state;  
	ChangeAnimationUpdateTime();
}
 
void CGameObject::ChangeAnimationUpdateTime()
{
	if (STATE::WALK == m_State) {
		m_ImageUpdateTime = (0.6f) / (m_ImagePosition[0]);
	}
	else {
		m_ImageUpdateTime = (0.7f) / (m_ImagePosition[1] - m_ImagePosition[0]);
	}
}
ITEM_TYPE CGameObject::UseItem(int idx)
{
	auto itemType = m_Items[idx];
	m_Items.erase(m_Items.begin() + idx);
	--m_ItemCount;
	return itemType;
}

void CGameObject::DrawPlayer(HDC hdc, int view_x, int view_y)
{  
	m_PlayerImage.TransparentBlt(hdc,
		m_Position.x - view_x, m_Position.y - view_y,
		MAP_TILE_SIZE.x, MAP_TILE_SIZE.y,
		(int)m_AnimationIndex.x * m_PlayerImage.GetWidth() / 9.0f, 
		(int)m_Direction * m_PlayerImage.GetHeight() / 4.0f,
		m_PlayerImage.GetWidth() / 9.0f,
		m_PlayerImage.GetHeight() / 4.0f,
		RGB(255, 0, 255)); 
}
	
void CGameObject::DrawMonPeaceRoaming(HDC hdc, int view_x, int view_y)
{
	m_MonPeaceRoamingImage.TransparentBlt(hdc,
		m_Position.x - view_x, m_Position.y - view_y,
		MAP_TILE_SIZE.x, MAP_TILE_SIZE.y,
		(int)m_AnimationIndex.x * m_MonPeaceRoamingImage.GetWidth() / 7.0f,
		(int)m_Direction * m_MonPeaceRoamingImage.GetHeight() / 4.0f,
		m_MonPeaceRoamingImage.GetWidth() / 7.0f,
		m_MonPeaceRoamingImage.GetHeight() / 4.0f,
		RGB(255, 0, 255));
}

void CGameObject::DrawMonPeaceFixed(HDC hdc, int view_x, int view_y)
{
	int yPos = max(m_Direction - 2, 0);
	m_MonPeaceFixedImage.TransparentBlt(hdc,
		m_Position.x - view_x, m_Position.y - view_y,
		MAP_TILE_SIZE.x, MAP_TILE_SIZE.y,
		0,
		yPos * m_MonPeaceFixedImage.GetHeight() / 2.0f,
		m_MonPeaceFixedImage.GetWidth(),
		m_MonPeaceFixedImage.GetHeight() / 2.0f,
		RGB(255, 0, 255));
}
void CGameObject::DrawMonAgroRoaming(HDC hdc, int view_x, int view_y)
{ 
	float x_correction = MAP_TILE_SIZE.x * 0.2;
	float y_correction = MAP_TILE_SIZE.y * 0.2;

	m_MonAgroRoamingImage.TransparentBlt(hdc,
		m_Position.x - view_x- x_correction, m_Position.y - view_y- y_correction,
		MAP_TILE_SIZE.x * 1.3, MAP_TILE_SIZE.y*1.3,
		(int)m_AnimationIndex.x * m_MonAgroRoamingImage.GetWidth() / 8.0f,
		(int)m_Direction * m_MonAgroRoamingImage.GetHeight() / 4.0f,
		m_MonAgroRoamingImage.GetWidth() / 8.0f,
		m_MonAgroRoamingImage.GetHeight() / 4.0f,
		RGB(255, 0, 255)); 
}
void CGameObject::DrawMonAgroFixed(HDC hdc, int view_x, int view_y)
{  
	int yPos = max(m_Direction - 2, 0);
	m_MonAgroFixedImage.TransparentBlt(hdc,
		m_Position.x - view_x, m_Position.y - view_y,
		MAP_TILE_SIZE.x, MAP_TILE_SIZE.y, 
		0,
		yPos * m_MonAgroFixedImage.GetHeight() / 2.0f,
		m_MonAgroFixedImage.GetWidth(),
		m_MonAgroFixedImage.GetHeight() / 2.0f,
		RGB(255, 0, 255));
}


void CGameObject::DrawSeller(HDC hdc, int view_x, int view_y)
{
	m_Seller.TransparentBlt(hdc,
		m_Position.x - view_x, m_Position.y - view_y,
		MAP_TILE_SIZE.x, MAP_TILE_SIZE.y,
		(int)m_AnimationIndex.x * m_Seller.GetWidth() / 16.0f, 0,
		m_Seller.GetWidth() / 16,
		m_Seller.GetHeight(),
		RGB(255, 0, 255));
}
void CGameObject::DrawExtraInfos(HDC hdc, int view_x, int view_y, 
	bool nameDraw, bool hpDraw)
{
	if (nameDraw) {
		TextOut(hdc, m_Position.x - view_x, m_Position.y - view_y - 10, m_WName.c_str(), lstrlen(m_WName.c_str()));
	}
	if (hpDraw) {
		wstring wHp = L"HP - " + to_wstring(m_HP);
		TextOut(hdc, m_Position.x - view_x, m_Position.y - view_y + TILE_WIDTH + 10, wHp.c_str(), lstrlen(wHp.c_str()));
	}
}


void CGameObject::DrawItems(HDC hdc)
{
	for (int i = 0; i < m_ItemCount; ++i) {
		int xPos = m_Items[i] - 1; 
		m_ItemImage.TransparentBlt(hdc, WINDOW_WIDTH - 256 + 15 + i * (60), 7, 48, 48,
			xPos * 48, 0, 48, 48, RGB(255, 0, 255));
	}
}
