#pragma once

// ������ ���� :: �ƹ��͵��ƴ�, ����, ��üȸ��, ��ź, ������
enum ITEM_TYPE { I_NOT, I_POTION, I_POWERUP, I_BOMB, I_LEVELUP };
constexpr int POTION_HP_HEAL_AMOUNT = 25;
 
enum DIRECTION { D_N, D_S, D_W, D_E, D_NO };
constexpr int MAX_USER = 21000;                // �������� �ִ� ��ü ����,  ��ü ID�� �ִ� ��
constexpr int MAX_STR_LEN = 50;
constexpr int MAX_ID_LEN = 20;

// NPC�� ID�� �����ϴ� ����, ���� �÷��̾�� 0���� NPC_ID_START������ ID�� ����
// NPC�� ������ MAX_USER - NPC_ID_START = 20000,  20�� ������ NPC�� ����
constexpr int NPC_ID_START = 10000;		

// ������ �� �ִ� �������� �ִ� ����
constexpr int MAX_ITEM_COUNT = 4;

// ��ü�� Ÿ��
enum class ObjectType : BYTE {
	None = 0,			// �ƹ��͵� �ƴ�
	Player = 1,			// �÷��̾� 
	Mon_Peace_Roaming,	// �� ����X + ���� �̵�
	Mon_Peace_Fixed,	// �� ����X + ������ ��ġ
	Mon_Agro_Roaming,	// �� ����O + ���� �̵�
	Mon_Agro_Fixed,		// �� ����O + ������ ��ġ
	Seller				// ����
};

enum class MAP_TILE_DATA {
	TILE_GRASS_1 = 0,
	TILE_GRASS_2,
	TILE_SAND_1,
	TILE_SAND_2,
	TILE_SAND_3,
	TILE_SAND_4,
	TILE_ICE_1,
	TILE_ICE_2,
	TILE_GRASS_TREE,
	TILE_SAND_TREE1,
	TILE_SAND_TREE2,
	TILE_ICE_TREE,
	TILE_GRASS_MOUTAIN,
	TILE_SAND_MOUTAIN1,
	TILE_SAND_MOUTAIN2,
	TILE_ICE_MOUTAIN,
	COUNT
};

// Ŭ���̾�Ʈ �� ȭ�鿡 �׷��� ���� ũ��
constexpr auto SCREEN_WIDTH = 20;
constexpr auto SCREEN_HEIGHT = 20;

// ���� ��ü ũ��
#define WORLD_WIDTH			2000
#define WORLD_HEIGHT		2000

#define VIEW_RADIUS			7 // �÷��̾��� �þ� ������
#define AGRO_RADIUS         5 // �� ������ ������ �ν� ���� ������	

#define SERVER_PORT			3500

#define CS_LOGIN			1		// Ŭ���̾�Ʈ�� ������ ���� ��û
#define CS_MOVE				2		// Ŭ���̾�Ʈ�� �ƹ�Ÿ�� �̵��� ������ ��û
#define CS_ATTACK			3		// �ƹ�Ÿ�� ������ ���͸� ����
#define CS_CHAT				4		// �ƹ�Ÿ�� ä��
#define CS_LOGOUT			5		// Ŭ���̾�Ʈ ����
#define CS_TELEPORT			6		// ���� �ڷ���Ʈ ��û, �����׽�Ʈ�� �ƹ�Ÿ�� �ʿ� ���� ��ġ��Ű�� ����, �̰��� ������ ���� ��ġ�� HOTSPOT�� ��
#define CS_USE_ITEM		    7       // �������� ����� ���
#define CS_BUY_ITEM			8		// �������� ������ ���

#define SC_LOGIN_OK			1		// CS_LOGIN�� ���� ��Ŷ, �������� Ŭ���̾�Ʈ�� ������ ����
#define SC_LOGIN_FAIL		2		// CS_LOGIN�� ���� ��Ŷ, �������� Ŭ���̾�Ʈ�� ������ ����
#define SC_POSITION			3		// OBJECT�� ��ġ ������ Ŭ���̾�Ʈ�� �뺸
#define SC_CHAT				4		// OBJECT�� ä���� �뺸
#define SC_STAT_CHANGE		5		// OBJECT�� ������ ����Ǿ����� �뺸
#define SC_REMOVE_OBJECT	6		// OBJECT�� �þ߿��� ��� ����
#define SC_ADD_OBJECT		7		// ���ο� OBJECT�� �þ߿� ��� ����
#define SC_ADD_ITEM			8	    // �������� ���� ���.

#pragma pack(push ,1)

struct sc_packet_login_ok {
	unsigned char size;
	char type;
	int id;
	short	x, y;
	int	HP, LEVEL, EXP, GOLD;
	int itemCount;
	ITEM_TYPE items[4];
};

struct sc_packet_login_fail {
	unsigned char size;
	char type;
};

struct sc_packet_position {
	unsigned char size;
	char type;
	int id;
	short x, y;
	int move_time;			// Stress Test ���α׷����� delay�� ������ �� ���, 
							// ������ �ش� id�� ������ Ŭ���̾�Ʈ���� ������ �ֽ� ���� return �ؾ� �Ѵ�.
	DIRECTION	direction;
};

struct sc_packet_chat {
	unsigned char size;
	char	type;
	int	id;
	char	message[MAX_STR_LEN];
};

struct sc_packet_stat_change {
	unsigned char size;
	char	type;
	int	id;
	int	HP, LEVEL, EXP, GOLD;  
};


struct sc_packet_remove_object {
	unsigned char size;
	char type;
	int id;
};

struct sc_packet_add_object {
	unsigned char	size;
	char	type;
	int	id;
	int	obj_class;		// 1: PLAYER,    2:ORC,  3:Dragon, ��..  ���־��� �����ϴ� ��, �����ϱ� ����
	short	x, y;
	int	HP, LEVEL, EXP;
	char	name[MAX_ID_LEN];

};

struct sc_packet_add_item {
	unsigned char	size;
	char	type;
	ITEM_TYPE itemType;
};

struct cs_packet_login {
	unsigned char	size;
	char	type;
	char    player_id[MAX_ID_LEN];
};

struct cs_packet_move {
	unsigned char	size;
	char	type;
	char	direction;		// 0:Up, 1:Down, 2:Left, 3:Right
	int move_time;			// Stress Test ���α׷����� delay�� ������ �� ���, 
						// ������ �ش� id�� ������ Ŭ���̾�Ʈ���� ������ �ֽ� ���� return �ؾ� �Ѵ�.
};

struct cs_packet_attack {
	unsigned char	size;
	char	type;
};

struct cs_packet_chat {
	unsigned char	size;
	char	type;
	char 	message[MAX_STR_LEN];
};

struct cs_packet_logout {
	unsigned char	size;
	char	type;
};

struct cs_packet_teleport {
	unsigned char	size;
	char	type;
};

struct cs_packet_use_item {
	unsigned char	size;
	char	type;
	int		itemIndex;
};

struct cs_packet_buy_item {
	unsigned char	size;
	char	type;
	ITEM_TYPE itemType;
};

#pragma pack (pop)
