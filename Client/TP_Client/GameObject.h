#pragma once

// 구현의 편의성을 위해 뭉쳐서 구현해버림
 
enum class STATE {
	WALK = 0,
	ATTACK = 1,
};
class CGameObject
{
public:
	CGameObject();
	CGameObject(LPCTSTR imageAddr,const Vector2i& tileIndex);
	virtual ~CGameObject();

public:
	void Update(float elapsedTime);
	void Draw(HDC hdc, int view_x, int view_y);

	void DrawItems(HDC hdc);

	void MoveTo(Vector2i tileIndex);

public:
	Vector2i GetTileIndex()const { return m_TileIndex; }

public:
	void SetPosition(const Vector2f& position) { m_Position = position; }
	void SetIsClientPlayer(bool answer) { m_isClientPlayer = answer; }

	void SetName(const char str[]) { m_Name = str; m_WName.assign(m_Name.begin(), m_Name.end()); }
	string GetName() const { return m_Name; }

	void Show() { m_IsOnDraw = true; }
	void Hide() { m_IsOnDraw = false; }

public:
	// getter and setters
	int GetLevel() const { return m_Level; }
	int GetHP() const { return m_HP; }
	int GetExp() const { return m_Exp; }
	int GetGold() const { return m_Gold; }
	int GetItemCount() const { return m_ItemCount; }
	STATE GetState() const { return m_State; }

	ObjectType GetType() const { return m_Type; }
	DIRECTION GetDirection() const { return m_Direction; }

	void SetLevel(int level) { m_Level = level; }
	void SetHP(int hp) { m_HP = hp; }
	void SetExp(int Exp) { m_Exp = Exp; }
	void SetGold(int Gold) { m_Gold = Gold; }
	void SetType(ObjectType type);
	void SetState(STATE state);
	void SetDirection(DIRECTION direction) { m_Direction = direction; }

	ITEM_TYPE UseItem(int idx);
	void AddItem(ITEM_TYPE itemType) { m_ItemCount++; m_Items.push_back(itemType); }
private:
	// 타입별 그리기 구분

	void DrawPlayer(HDC hdc, int view_x, int view_y); 
	void DrawMonPeaceRoaming(HDC hdc, int view_x, int view_y); 
	void DrawMonPeaceFixed(HDC hdc, int view_x, int view_y);
	void DrawMonAgroRoaming(HDC hdc, int view_x, int view_y);
	void DrawMonAgroFixed(HDC hdc, int view_x, int view_y); 
	void DrawSeller(HDC hdc, int view_x, int view_y);

	void DrawExtraInfos(HDC hdc, int view_x, int view_y, bool nameDraw, bool hpDraw);
	
	void ChangeAnimationUpdateTime();

private:
	static CImage m_Image;
	static CImage m_PlayerImage;
	static CImage m_MonAgroRoamingImage;
	static CImage m_MonAgroFixedImage;
	static CImage m_MonPeaceFixedImage;
	static CImage m_MonPeaceRoamingImage;
	static CImage m_Seller;
	static CImage m_ItemImage;

	static bool m_ImageLoad;

	// 이미지 출력 위치를 구분하기 위하여 사용
	// [0] : 공격 / [1] : 끝
	int m_ImagePosition[2];
	STATE m_State = STATE::WALK;
	float m_ImageUpdateTime = 0.0f;

	Vector2f m_Position;
	Vector2i m_TileIndex;

	bool m_isClientPlayer = false;

	bool m_IsOnDraw = false;

	string m_Name;
	wstring m_WName;

	int m_Level;
	int m_HP;
	int m_Exp;
	int m_Gold;
	int m_ItemCount = 0;
	vector<ITEM_TYPE> m_Items;

	ObjectType m_Type;

	Vector2f m_AnimationIndex;
	DIRECTION m_Direction;
};

