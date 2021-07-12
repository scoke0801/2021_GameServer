#pragma once
class CGameObject
{
private:
	static CImage m_Image;
	static bool m_ImageLoad;

	Vector2f m_Position;
	Vector2i m_TileIndex;

	bool m_isClientPlayer = false;

	bool m_IsOnDraw = false;

	string m_Name;
	wstring m_WName;
public:
	CGameObject();
	CGameObject(LPCTSTR imageAddr,const Vector2i& tileIndex);
	virtual ~CGameObject();

public:
	void Update(float elapsedTime);
	void Draw(HDC hdc, int view_x, int view_y);
	void MoveTo(Vector2i tileIndex);

public:
	Vector2i GetTileIndex()const { return m_TileIndex; }

public:
	void SetPosition(const Vector2f& position) { m_Position = position; }
	void SetIsClientPlayer(bool answer) { m_isClientPlayer = answer; }

	void SetName(const char str[]) { m_Name = str; m_WName.assign(m_Name.begin(), m_Name.end()); }

	void Show() { m_IsOnDraw = true; }
	void Hide() { m_IsOnDraw = false; }
};

