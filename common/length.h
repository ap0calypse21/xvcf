#ifndef __INC_COMMON_LENGTH_H__
#define __INC_COMMON_LENGTH_H__

#include "../CommonDefines.h"

#include "service.h"

#define WORD_MAX 0xffff

#if defined(ENABLE_ANTI_FLOOD)
enum EFloodTypes {
	FLOOD_BUY_SHOP,
	FLOOD_SELL_SHOP,
	FLOOD_USE_ITEM,
	FLOOD_PACKETS,
	FLOOD_DROP_ITEM,
	FLOOD_CMD_INTERPRET,
#if defined(ENABLE_MULTI_ITEM_USE)
	FLOOD_MULTI_USE_ITEM,
#endif
	FLOOD_CHAT,
	FLOOD_WHISPER,
	MAX_FLOOD_TYPE
};
#endif

#if defined(ENABLE_DS_CHANGE_ATTR)
enum EDSRefineType {
	DS_REFINE_TYPE_NORMAL,
	DS_REFINE_TYPE_CHANGEATTR,
	DS_REFINE_TYPE_MAX
};
#endif

enum EMisc
{
	MAX_HOST_LENGTH = 15,
	IP_ADDRESS_LENGTH = 15,
	LOGIN_MAX_LEN = 30,
	PASSWD_MAX_LEN = 16,
#if defined(__PLAYER_PER_ACCOUNT5__)
	PLAYER_PER_ACCOUNT = 5,
#else
	PLAYER_PER_ACCOUNT = 4,
#endif
	ACCOUNT_STATUS_MAX_LEN = 128,
	CHARACTER_NAME_MAX_LEN = 24,
#ifdef __TITLE_SYSTEM_YUMA__
	TITLE_MAX_LEN			= 25,
#endif
	SHOP_SIGN_MAX_LEN = 32,
	INVENTORY_PAGE_SIZE = 45,
#if defined(__INVENTORY_4PAGES__)
	INVENTORY_PAGE_COUNT = 4, // 180
#else
	INVENTORY_PAGE_COUNT = 2, // 90
#endif
	INVENTORY_MAX_NUM = INVENTORY_PAGE_SIZE * INVENTORY_PAGE_COUNT,
	ABILITY_MAX_NUM = 50,
	EMPIRE_MAX_NUM = 4,
	BANWORD_MAX_LEN = 24,
	SOCIAL_ID_MAX_LEN = 18,
	GUILD_NAME_MAX_LEN = 12,
	QUEST_NAME_MAX_NUM		= 64,
	SHOP_HOST_ITEM_MAX_NUM = 40, /* 호스트의 최대 아이템 개수 */
	SHOP_GUEST_ITEM_MAX_NUM = 18, /* 게스트의 최대 아이템 개수 */
	
#if defined ENABLE_OFFLINE_SHOP_SYSTEM
	OFFLINE_SHOP_HOST_ITEM_MAX_NUM = 40,
#endif

	SHOP_PRICELIST_MAX_NUM = 40, ///< 개인상점 가격정보 리스트에서 유지할 가격정보의 최대 갯수

#if defined(__MYSHOP_DECO__)
	MYSHOP_MAX_TABS = 1,
#endif

	CHAT_MAX_LEN = 512,

	QUICKSLOT_MAX_NUM = 36,

	JOURNAL_MAX_NUM = 2,

	QUERY_MAX_LEN = 8192,

	FILE_MAX_LEN = 128,

	PLAYER_EXP_TABLE_MAX = 120,
	PLAYER_MAX_LEVEL_CONST = 120,
#if defined(__CONQUEROR_LEVEL__)
	PLAYER_CONQUEROR_EXP_TABLE_MAX = 30,
	PLAYER_MAX_CONQUEROR_LEVEL_CONST = 8,
#endif
	GUILD_MAX_LEVEL = 20,
	MOB_MAX_LEVEL = 100,

	ATTRIBUTE_MAX_VALUE = 20,
	CHARACTER_PATH_MAX_NUM = 64,
	SKILL_MAX_NUM = 255,
	SKILLBOOK_DELAY_MIN = 64800,
	SKILLBOOK_DELAY_MAX = 108000,
	SKILL_MAX_LEVEL = 40,

	APPLY_NAME_MAX_LEN = 32,
	EVENT_FLAG_NAME_MAX_LEN = 32,

	MOB_SKILL_MAX_NUM = 5,

	POINT_MAX_NUM = 255,
	DRAGON_SOUL_BOX_SIZE = 32,
	DRAGON_SOUL_BOX_COLUMN_NUM = 8,
	DRAGON_SOUL_BOX_ROW_NUM = DRAGON_SOUL_BOX_SIZE / DRAGON_SOUL_BOX_COLUMN_NUM,
	DRAGON_SOUL_REFINE_GRID_SIZE = 15,
	MAX_AMOUNT_OF_MALL_BONUS = 20,

	WEAR_MAX_NUM = 36,
#ifdef __ENABLE_NEW_OFFLINESHOP__
	OFFLINE_SHOP_NAME_MAX_LEN			= 40 + CHARACTER_NAME_MAX_LEN +1,
	OFFLINESHOP_DURATION_MAX_MINUTES	= 8* 24 * 60,
	OFFLINESHOP_MAX_SEARCH_RESULT		= 250,
	OFFLINESHOP_SECONDS_PER_SEARCH		= 15,
	//*new check about auction offers
	OFFLINESHOP_SECONDS_PER_OFFER		= 7,
#if defined(ENABLE_CHEQUE_SYSTEM)
	YANG_PER_CHEQUE = 100000000,
#endif
#endif

	GOLD_MAX = 200000000, // 1999999999
#if defined(ENABLE_CHEQUE_SYSTEM)
	CHEQUE_MAX = 1000, // 999
#endif
#if defined(__GEM_SYSTEM__)
	GEM_MAX = 1000000, // 999999
#endif

	MAX_MAP_ALLOW = 33,

	PIN_CODE_LENGTH = 4,
	HWID_MAX_NUM = 128,

	SHOP_TAB_NAME_MAX = 32,
#if defined(__SHOPEX_TAB4__)
	SHOP_TAB_COUNT_MAX = 4,
#else
	SHOP_TAB_COUNT_MAX = 3,
#endif

	BELT_INVENTORY_SLOT_WIDTH = 4,
	BELT_INVENTORY_SLOT_HEIGHT = 4,
	BELT_INVENTORY_SLOT_COUNT = BELT_INVENTORY_SLOT_WIDTH * BELT_INVENTORY_SLOT_HEIGHT,

	#if defined(__SPECIAL_INVENTORY_SYSTEM__)
		SPECIAL_INVENTORY_PAGE_SIZE = 45,
		SPECIAL_INVENTORY_PAGE_COUNT = 5, // 225
		SPECIAL_INVENTORY_MAX_NUM = SPECIAL_INVENTORY_PAGE_SIZE * SPECIAL_INVENTORY_PAGE_COUNT,
		SPECIAL_INVENTORY_MAX_PAGE_NUM = SPECIAL_INVENTORY_MAX_NUM * SPECIAL_INVENTORY_PAGE_COUNT,

		SKILL_BOOK_INVENTORY_MAX_NUM = SPECIAL_INVENTORY_MAX_NUM,
		UPGRADE_ITEMS_INVENTORY_MAX_NUM = SPECIAL_INVENTORY_MAX_NUM,
		STONE_INVENTORY_MAX_NUM = SPECIAL_INVENTORY_MAX_NUM,
		GIFT_BOX_INVENTORY_MAX_NUM = SPECIAL_INVENTORY_MAX_NUM,
	#endif

	/**
		 **** 현재까지 할당 된 아이템 영역 정리 (DB상 Item Position) ****
		+------------------------------------------------------+ 0
		| 캐릭터 기본 인벤토리 (45칸 * 2페이지) 90칸           |
		+------------------------------------------------------+ 90 = INVENTORY_MAX_NUM(90)
		| 캐릭터 장비 창 (착용중인 아이템) 32칸                |
		+------------------------------------------------------+ 122 = INVENTORY_MAX_NUM(90) + WEAR_MAX_NUM(32)
		| 용혼석 장비 창 (착용중인 용혼석) 12칸                |
		+------------------------------------------------------+ 134 = 122 + DS_SLOT_MAX(6) * DRAGON_SOUL_DECK_MAX_NUM(2)
		| 용혼석 장비 창 예약 (아직 미사용) 18칸               |
		+------------------------------------------------------+ 152 = 134 + DS_SLOT_MAX(6) * DRAGON_SOUL_DECK_RESERVED_MAX_NUM(3)
		| 벨트 인벤토리 (벨트 착용시에만 벨트 레벨에 따라 활성)|
		+------------------------------------------------------+ 168 = 152 + BELT_INVENTORY_SLOT_COUNT(16) = INVENTORY_AND_EQUIP_CELL_MAX
		| 미사용                                               |
		+------------------------------------------------------+ ??
	*/
};

enum EPrivateShop
{
	INVENTORY_PAGE_MAX_NUM			= 4,
	INVENTORY_PAGE_WIDTH			= 5,
	INVENTORY_PAGE_HEIGHT			= 9,
	INVENTORY_PAGE_ITEM_MAX_NUM		= INVENTORY_PAGE_WIDTH * INVENTORY_PAGE_HEIGHT,

	PRIVATE_SHOP_PAGE_MAX_NUM		= 4,
	PRIVATE_SHOP_WIDTH				= 5,
	PRIVATE_SHOP_HEIGHT				= 8,
	PRIVATE_SHOP_PAGE_ITEM_MAX_NUM	= PRIVATE_SHOP_WIDTH * PRIVATE_SHOP_HEIGHT,
	PRIVATE_SHOP_HOST_ITEM_MAX_NUM = (PRIVATE_SHOP_WIDTH * PRIVATE_SHOP_HEIGHT) * PRIVATE_SHOP_PAGE_MAX_NUM,
	PRIVATE_SHOP_DESPAWN_TIME		= 60 * 3,

	PRIVATE_SHOP_TITLE_MAX_LEN		= 32,
	PRIVATE_SHOP_TITLE_MIN_LEN		= 3,
};

enum EShopSearchMode
{
	MODE_NONE,
	MODE_LOOKING,
	MODE_TRADING,
};

enum EPrivateShopState
{
	STATE_UNAVAILABLE,
	STATE_CLOSED,
	STATE_OPEN,
	STATE_MODIFY
};

enum EGrowthPet
{
	PET_MAX_LEVEL = 105,

	PET_ATTR_CHANGE_ITEMVNUM = 55033,
	PET_NAME_MAX_SIZE = 20,
	PET_NAME_MIN_SIZE = 4,
	PET_HATCHING_MONEY = 100000,
	PET_SKILL_COUNT_MAX = 3,
	PET_GROWTH_SKILL_LEVEL_MAX = 20,
	PET_SKILL_UPGRADE_PRICE = 2000000,
	PET_GROWTH_SKILL_OPEN_EVOL_LEVEL = 4,
	PET_FEED_SLOT_MAX = 9,
	PET_REVIVE_MATERIAL_SLOT_MAX = 10,

	EGG_TO_UPBRINGING_DELTA = 300,
	MELEY_PET_EGG_VNUM = 55406,

	EXP_TYPE_MOB = 1,
	EXP_TYPE_ITEM = 2,

	PET_EVOL_MAX_ITEM_COUNT = 7,
	PET_LAST_EVOL_MIN_DAY_COUNT = 7,

	PET_MONKEY = 55701,
	PET_SPIDER = 55702,
	PET_RAZADOR = 55703,
	PET_NEMERE = 55704,
	PET_BLUE_DRAGON = 55705,
	PET_RED_DRAGON = 55706,
	PET_BASHIIDO = 55707,
	PET_MINI_EXECUTOR = 55708,
	PET_NESSIE = 55709,
	PET_AZRAEL = 55710,
	PET_EXEDYAR = 55711,
	PET_MAX_NUM = 11,

	PET_MAX_BONUS_NUM = 8,
};


#ifdef ENABLE_SWITCHBOT

enum SwitchbotValues
{
	SWITCHBOT_SLOT_COUNT = 5,
	SWITCHBOT_ALTERNATIVE_COUNT = 2,
	//1 = Switching Item
	//2 = Yang
	SWITCHBOT_PRICE_TYPE = 1,
	//Amount Swtichting Items the Yang-Price
	SWITCHBOT_PRICE_AMOUNT = 1,
};

const DWORD c_arSwitchingItems[3] =
{
	39028,
	71084,
	71151,
};
#endif

#if defined(__REFINE_FAIL_TYPE__)
enum ERefineFailType
{
	REFINE_FAIL_GRADE_DOWN,
	REFINE_FAIL_DEL_ITEM,
	REFINE_FAIL_KEEP_GRADE,
	REFINE_FAIL_MAX,
};
#endif

#if defined(__CHANGE_LOOK_SYSTEM__)
enum EChangeLookInfo
{
	CHANGE_LOOK_WINDOW_MAX_MATERIALS = 3,
	CHANGE_LOOK_TRANSMUTATION_PRICE = 50000000,
	CHANGE_LOOK_SCROLL_VNUM = 72326,
	CHANGE_LOOK_SCROLL_VNUM2 = 72341,
	CHANGE_LOOK_CAN_EQUIP_MULTI_GENDER = FALSE,
};
#endif

#if defined(__SKILLBOOK_COMB_SYSTEM__)
enum ESkillBookCombination
{
	SKILLBOOK_COMB_SLOT_MAX = 10,
	SKILLBOOK_COMB_COST = 1000000,
};
#endif

enum EWearPositions
{
	WEAR_BODY,				// 0
	WEAR_HEAD,				// 1
	WEAR_FOOTS,				// 2
	WEAR_WRIST,				// 3
	WEAR_WEAPON,			// 4
	WEAR_NECK,				// 5
	WEAR_EAR,				// 6
	WEAR_UNIQUE1,			// 7
	WEAR_UNIQUE2,			// 8
	WEAR_ARROW,				// 9
	WEAR_SHIELD,			// 10
	WEAR_ABILITY1,			// 11
	WEAR_ABILITY2,			// 12
	WEAR_ABILITY3,			// 13
	WEAR_ABILITY4,			// 14
	WEAR_ABILITY5,			// 15
	WEAR_ABILITY6,			// 16
	WEAR_ABILITY7,			// 17
	WEAR_ABILITY8,			// 18
	WEAR_COSTUME_BODY,		// 19
	WEAR_COSTUME_HAIR,		// 20
#if defined(__MOUNT_COSTUME_SYSTEM__)
	WEAR_COSTUME_MOUNT,		// 21
#endif
#if defined(__ACCE_COSTUME_SYSTEM__)
	WEAR_COSTUME_ACCE,		// 22
#endif
#if defined(__WEAPON_COSTUME_SYSTEM__)
	WEAR_COSTUME_WEAPON,	// 23
#endif
	WEAR_COSTUME_AURA,			// 24	ENABLE_AURA_SYSTEM

	WEAR_RING1,				// 24 : 신규 반지슬롯1 (왼쪽)
	WEAR_RING2,				// 25 : 신규 반지슬롯2 (오른쪽)

	WEAR_BELT,				// 26 : 신규 벨트슬롯

#if defined(__PENDANT_SYSTEM__)
	WEAR_PENDANT,			// 27 : pendant
#endif

#if defined(__GLOVE_SYSTEM__)
	WEAR_GLOVE,				// 27 : glove
#endif

#ifdef __ITEM_SHINING__
	WEAR_SHINING_WEAPON_1,//28
	WEAR_SHINING_WEAPON_2,//29
	WEAR_SHINING_WEAPON_3,//30
	WEAR_SHINING_ARMOR_1,//31
	WEAR_SHINING_ARMOR_2,//32
	WEAR_SHINING_SPECIAL,//33
#endif


	WEAR_MAX = WEAR_MAX_NUM
};

enum EDragonSoulDeckType
{
	DRAGON_SOUL_DECK_0,
	DRAGON_SOUL_DECK_1,
	DRAGON_SOUL_DECK_MAX_NUM = 2,

	DRAGON_SOUL_DECK_RESERVED_MAX_NUM = 3, // NOTE: 중요! 아직 사용중이진 않지만, 3페이지 분량을 예약 해 둠. DS DECK을 늘릴 경우 반드시 그 수만큼 RESERVED에서 차감해야 함!
};

enum ESex
{
	SEX_MALE,
	SEX_FEMALE
};

enum EDirection
{
	DIR_NORTH,
	DIR_NORTHEAST,
	DIR_EAST,
	DIR_SOUTHEAST,
	DIR_SOUTH,
	DIR_SOUTHWEST,
	DIR_WEST,
	DIR_NORTHWEST,
	DIR_MAX_NUM
};

#define ABILITY_MAX_LEVEL 10 /* 기술 최대 레벨 */

enum EAbilityDifficulty
{
	DIFFICULTY_EASY,
	DIFFICULTY_NORMAL,
	DIFFICULTY_HARD,
	DIFFICULTY_VERY_HARD,
	DIFFICULTY_NUM_TYPES
};

enum EAbilityCategory
{
	CATEGORY_PHYSICAL, /* 신체적 어빌리티 */
	CATEGORY_MENTAL, /* 정신적 어빌리티 */
	CATEGORY_ATTRIBUTE, /* 능력 어빌리티 */
	CATEGORY_NUM_TYPES
};

enum EJobs
{
	JOB_WARRIOR,
	JOB_ASSASSIN,
	JOB_SURA,
	JOB_SHAMAN,
	JOB_WOLFMAN,
	JOB_MAX_NUM
};

enum ESkillGroups
{
	SKILL_GROUP_MAX_NUM = 2,
};

enum ERaceFlags
{
	RACE_FLAG_ANIMAL = (1 << 0),
	RACE_FLAG_UNDEAD = (1 << 1),
	RACE_FLAG_DEVIL = (1 << 2),
	RACE_FLAG_HUMAN = (1 << 3),
	RACE_FLAG_ORC = (1 << 4),
	RACE_FLAG_MILGYO = (1 << 5),
	RACE_FLAG_INSECT = (1 << 6),
	RACE_FLAG_FIRE = (1 << 7),
	RACE_FLAG_ICE = (1 << 8),
	RACE_FLAG_DESERT = (1 << 9),
	RACE_FLAG_TREE = (1 << 10),
	RACE_FLAG_ATT_ELEC = (1 << 11),
	RACE_FLAG_ATT_FIRE = (1 << 12),
	RACE_FLAG_ATT_ICE = (1 << 13),
	RACE_FLAG_ATT_WIND = (1 << 14),
	RACE_FLAG_ATT_EARTH = (1 << 15),
	RACE_FLAG_ATT_DARK = (1 << 16),
#if defined(__ELEMENT_SYSTEM__)
	RACE_FLAG_CZ = (1 << 17),
#endif
};

enum ELoads
{
	LOAD_NONE,
	LOAD_LIGHT,
	LOAD_NORMAL,
	LOAD_HEAVY,
	LOAD_MASSIVE
};

enum
{
	QUICKSLOT_TYPE_NONE,
	QUICKSLOT_TYPE_ITEM,
	QUICKSLOT_TYPE_SKILL,
	QUICKSLOT_TYPE_COMMAND,
/*
#if defined(__SPECIAL_INVENTORY_SYSTEM__)
	QUICKSLOT_SKILL_BOOK_INVENTORY,
	QUICKSLOT_UPGRADE_ITEMS_INVENTORY,
	QUICKSLOT_STONE_INVENTORY,
	QUICKSLOT_GIFT_BOX_INVENTORY,
#endif
*/
	QUICKSLOT_TYPE_MAX_NUM,
};

enum EParts
{
	PART_MAIN,
	PART_WEAPON,
	PART_HEAD,
	PART_HAIR,
#if defined(__ACCE_COSTUME_SYSTEM__)
	PART_ACCE,
#endif
#ifdef ENABLE_AURA_SYSTEM
	PART_AURA,
#endif
	PART_MAX_NUM,
	PART_WEAPON_SUB,
};

enum EChatType
{
	CHAT_TYPE_TALKING, /* 그냥 채팅 */
	CHAT_TYPE_INFO, /* 정보 (아이템을 집었다, 경험치를 얻었다. 등) */
	CHAT_TYPE_NOTICE, /* 공지사항 */
	CHAT_TYPE_PARTY, /* 파티말 */
	CHAT_TYPE_GUILD, /* 길드말 */
	CHAT_TYPE_COMMAND, /* 일반 명령 */
	CHAT_TYPE_SHOUT, /* 외치기 */
	CHAT_TYPE_WHISPER,
	CHAT_TYPE_BIG_NOTICE,
	CHAT_TYPE_MONARCH_NOTICE,
	CHAT_TYPE_BIG_CONTROL_NOTICE,
#if defined(__DICE_SYSTEM__)
	CHAT_TYPE_DICE_INFO,
#endif
#if defined(__12ZI_NOTICE__)
	CHAT_TYPE_MISSION,
	CHAT_TYPE_SUB_MISSION,
#endif
#ifdef ENABLE_12ZI
	CHAT_TYPE_CLEAR_MISSION,
#endif
	CHAT_TYPE_MAX_NUM
};

enum EWhisperType
{
	WHISPER_TYPE_NORMAL = 0,
	WHISPER_TYPE_NOT_EXIST = 1,
	WHISPER_TYPE_TARGET_BLOCKED = 2,
	WHISPER_TYPE_SENDER_BLOCKED = 3,
	WHISPER_TYPE_ERROR = 4,
	WHISPER_TYPE_GM = 5,
	WHISPER_TYPE_SYSTEM = 0xFF
};

enum ECharacterPosition
{
	POSITION_GENERAL,
	POSITION_BATTLE,
	POSITION_DYING,
	POSITION_SITTING_CHAIR,
	POSITION_SITTING_GROUND,
	POSITION_INTRO,
	POSITION_MAX_NUM
};

enum EGMLevels
{
	GM_PLAYER,
	GM_LOW_WIZARD,
	GM_WIZARD,
	GM_HIGH_WIZARD,
	GM_GOD,
	GM_IMPLEMENTOR
};

enum EMobRank
{
	MOB_RANK_PAWN,
	MOB_RANK_S_PAWN,
	MOB_RANK_KNIGHT,
	MOB_RANK_S_KNIGHT,
	MOB_RANK_BOSS,
	MOB_RANK_KING,
	MOB_RANK_MAX_NUM
};

enum ECharType
{
	CHAR_TYPE_MONSTER,
	CHAR_TYPE_NPC,
	CHAR_TYPE_STONE,
	CHAR_TYPE_WARP,
	CHAR_TYPE_DOOR,
	CHAR_TYPE_BUILDING,
	CHAR_TYPE_PC,
	CHAR_TYPE_POLYMORPH_PC,
	CHAR_TYPE_HORSE,
	CHAR_TYPE_GOTO,
	CHAR_TYPE_PET_PAY,
	CHAR_TYPE_PET,
};

enum EBattleType
{
	BATTLE_TYPE_MELEE,
	BATTLE_TYPE_RANGE,
	BATTLE_TYPE_MAGIC,
	BATTLE_TYPE_SPECIAL,
	BATTLE_TYPE_POWER,
	BATTLE_TYPE_TANKER,
	BATTLE_TYPE_SUPER_POWER,
	BATTLE_TYPE_SUPER_TANKER,
	BATTLE_TYPE_MAX_NUM
};

enum EApplyTypes
{
	APPLY_NONE,							// 0
	APPLY_MAX_HP,						// 1
	APPLY_MAX_SP,						// 2
	APPLY_CON,							// 3
	APPLY_INT,							// 4
	APPLY_STR,							// 5
	APPLY_DEX,							// 6
	APPLY_ATT_SPEED,					// 7
	APPLY_MOV_SPEED,					// 8
	APPLY_CAST_SPEED,					// 9
	APPLY_HP_REGEN,						// 10
	APPLY_SP_REGEN,						// 11
	APPLY_POISON_PCT,					// 12
	APPLY_STUN_PCT,						// 13
	APPLY_SLOW_PCT,						// 14
	APPLY_CRITICAL_PCT,					// 15
	APPLY_PENETRATE_PCT,				// 16
	APPLY_ATTBONUS_HUMAN,				// 17
	APPLY_ATTBONUS_ANIMAL,				// 18
	APPLY_ATTBONUS_ORC,					// 19
	APPLY_ATTBONUS_MILGYO,				// 20
	APPLY_ATTBONUS_UNDEAD,				// 21
	APPLY_ATTBONUS_DEVIL,				// 22
	APPLY_STEAL_HP,						// 23
	APPLY_STEAL_SP,						// 24
	APPLY_MANA_BURN_PCT,				// 25
	APPLY_DAMAGE_SP_RECOVER,			// 26
	APPLY_BLOCK,						// 27
	APPLY_DODGE,						// 28
	APPLY_RESIST_SWORD,					// 29
	APPLY_RESIST_TWOHAND,				// 30
	APPLY_RESIST_DAGGER,				// 31
	APPLY_RESIST_BELL,					// 32
	APPLY_RESIST_FAN,					// 33
	APPLY_RESIST_BOW,					// 34
	APPLY_RESIST_FIRE,					// 35
	APPLY_RESIST_ELEC,					// 36
	APPLY_RESIST_MAGIC,					// 37
	APPLY_RESIST_WIND,					// 38
	APPLY_REFLECT_MELEE,				// 39
	APPLY_REFLECT_CURSE,				// 40
	APPLY_POISON_REDUCE,				// 41
	APPLY_KILL_SP_RECOVER,				// 42
	APPLY_EXP_DOUBLE_BONUS,				// 43
	APPLY_GOLD_DOUBLE_BONUS,			// 44
	APPLY_ITEM_DROP_BONUS,				// 45
	APPLY_POTION_BONUS,					// 46
	APPLY_KILL_HP_RECOVER,				// 47
	APPLY_IMMUNE_STUN,					// 48
	APPLY_IMMUNE_SLOW,					// 49
	APPLY_IMMUNE_FALL,					// 50
	APPLY_SKILL,						// 51
	APPLY_BOW_DISTANCE,					// 52
	APPLY_ATT_GRADE_BONUS,				// 53
	APPLY_DEF_GRADE_BONUS,				// 54
	APPLY_MAGIC_ATT_GRADE,				// 55
	APPLY_MAGIC_DEF_GRADE,				// 56
	APPLY_CURSE_PCT,					// 57
	APPLY_MAX_STAMINA,					// 58
	APPLY_ATT_BONUS_TO_WARRIOR,			// 59
	APPLY_ATT_BONUS_TO_ASSASSIN,		// 60
	APPLY_ATT_BONUS_TO_SURA,			// 61
	APPLY_ATT_BONUS_TO_SHAMAN,			// 62
	APPLY_ATT_BONUS_TO_MONSTER,			// 63
	APPLY_MALL_ATTBONUS,				// 64 공격력 +x%
	APPLY_MALL_DEFBONUS,				// 65 방어력 +x%
	APPLY_MALL_EXPBONUS,				// 66 경험치 +x%
	APPLY_MALL_ITEMBONUS,				// 67 아이템 드롭율 x/10배
	APPLY_MALL_GOLDBONUS,				// 68 돈 드롭율 x/10배
	APPLY_MAX_HP_PCT,					// 69 최대 생명력 +x%
	APPLY_MAX_SP_PCT,					// 70 최대 정신력 +x%
	APPLY_SKILL_DAMAGE_BONUS,			// 71 스킬 데미지 * (100+x)%
	APPLY_NORMAL_HIT_DAMAGE_BONUS,		// 72 평타 데미지 * (100+x)%
	APPLY_SKILL_DEFEND_BONUS,			// 73 스킬 데미지 방어 * (100-x)%
	APPLY_NORMAL_HIT_DEFEND_BONUS,		// 74 평타 데미지 방어 * (100-x)%
	APPLY_PC_BANG_EXP_BONUS,			// 75 PC방 아이템 EXP 보너스
	APPLY_PC_BANG_DROP_BONUS,			// 76 PC방 아이템 드롭율 보너스

	APPLY_EXTRACT_HP_PCT,				// 77 사용시 HP 소모

	APPLY_RESIST_WARRIOR,				// 78 무사에게 저항
	APPLY_RESIST_ASSASSIN,				// 79 자객에게 저항
	APPLY_RESIST_SURA,					// 80 수라에게 저항
	APPLY_RESIST_SHAMAN,				// 81 무당에게 저항
	APPLY_ENERGY,						// 82 기력
	APPLY_DEF_GRADE,					// 83 방어력. DEF_GRADE_BONUS는 클라에서 두배로 보여지는 의도된 버그(...)가 있다.
	APPLY_COSTUME_ATTR_BONUS,			// 84 코스튬 아이템에 붙은 속성치 보너스
	APPLY_MAGIC_ATTBONUS_PER,			// 85 마법 공격력 +x%
	APPLY_MELEE_MAGIC_ATTBONUS_PER,		// 86 마법 + 밀리 공격력 +x%

	APPLY_RESIST_ICE,					// 87 냉기 저항
	APPLY_RESIST_EARTH,					// 88 대지 저항
	APPLY_RESIST_DARK,					// 89 어둠 저항

	APPLY_ANTI_CRITICAL_PCT,			// 90 크리티컬 저항
	APPLY_ANTI_PENETRATE_PCT,			// 91 관통타격 저항

	APPLY_BLEEDING_REDUCE = 92,			// 92
	APPLY_BLEEDING_PCT = 93,			// 93
	APPLY_ATT_BONUS_TO_WOLFMAN = 94,	// 94
	APPLY_RESIST_WOLFMAN = 95,			// 95
	APPLY_RESIST_CLAW = 96,				// 96

#if defined(__MOUNT_COSTUME_SYSTEM__)
	APPLY_MOUNT,						// 97
#endif

#if defined(__ACCE_COSTUME_SYSTEM__)
	APPLY_ACCEDRAIN_RATE,				// 98
#endif

#if defined(__MAGIC_REDUCTION__)
	APPLY_RESIST_MAGIC_REDUCTION,		// 99
#endif

#if defined(__ELEMENT_SYSTEM__)
	APPLY_ENCHANT_ELECT,				// 100
	APPLY_ENCHANT_FIRE,					// 101
	APPLY_ENCHANT_ICE,					// 102
	APPLY_ENCHANT_WIND,					// 103
	APPLY_ENCHANT_EARTH,				// 104
	APPLY_ENCHANT_DARK,					// 105
	APPLY_ATTBONUS_CZ,					// 106
	APPLY_ATTBONUS_INSECT,				// 107
	APPLY_ATTBONUS_DESERT,				// 108

	APPLY_ATTBONUS_SWORD,				// 109
	APPLY_ATTBONUS_TWOHAND,				// 110
	APPLY_ATTBONUS_DAGGER,				// 111
	APPLY_ATTBONUS_BELL,				// 112
	APPLY_ATTBONUS_FAN,					// 113
	APPLY_ATTBONUS_BOW,					// 114
	// APPLY_ATTBONUS_CLAW,				// 115

	APPLY_RESIST_HUMAN,					// 115
#endif
#if defined(__9TH_SKILL__)
	APPLY_INVINCIBLE,					// 116
#endif
#if defined(ENABLE_AUTO_PICKUP)
	APPLY_AUTOPICKUP,					// 117
#endif
#if defined(ENABLE_BONUS_METINSTONE)
	APPLY_ATTBONUS_METINSTONE,			// 118
#endif
#if defined(ENABLE_BONUS_BOSS)
	APPLY_ATTBONUS_BOSS,				// 119
#endif

#if defined(__CONQUEROR_LEVEL__)
	APPLY_SUNGMA_STR,					// 118
	APPLY_SUNGMA_HP,					// 119
	APPLY_SUNGMA_MOVE,					// 120
	APPLY_SUNGMA_IMMUNE,				// 121
#endif
	MAX_APPLY_NUM,						// 255
};

enum EOnClickEvents
{
	ON_CLICK_NONE,
	ON_CLICK_SHOP,
	ON_CLICK_TALK,
	ON_CLICK_MAX_NUM
};

enum EOnIdleEvents
{
	ON_IDLE_NONE,
	ON_IDLE_GENERAL,
	ON_IDLE_MAX_NUM
};

enum EWindows
{
	RESERVED_WINDOW,
	INVENTORY,
	EQUIPMENT,
	SAFEBOX,
	MALL,
	DRAGON_SOUL_INVENTORY,
	BELT_INVENTORY,
#if defined(__SPECIAL_INVENTORY_SYSTEM__)
	SKILL_BOOK_INVENTORY,
	UPGRADE_ITEMS_INVENTORY,
	STONE_INVENTORY,
	GIFT_BOX_INVENTORY,
#endif
#ifdef ENABLE_AURA_SYSTEM
	AURA_REFINE,
#endif
#ifdef ENABLE_SWITCHBOT
	SWITCHBOT,
#endif

	GROUND,

#ifdef __ENABLE_NEW_OFFLINESHOP__
	NEW_OFFSHOP,
	SHOP_SAFEBOX,
#endif

};

enum EMobSizes
{
	MOBSIZE_RESERVED,
	MOBSIZE_SMALL,
	MOBSIZE_MEDIUM,
	MOBSIZE_BIG
};

enum EAIFlags
{
	AIFLAG_AGGRESSIVE = (1 << 0),
	AIFLAG_NOMOVE = (1 << 1),
	AIFLAG_COWARD = (1 << 2),
	AIFLAG_NOATTACKSHINSU = (1 << 3),
	AIFLAG_NOATTACKCHUNJO = (1 << 4),
	AIFLAG_NOATTACKJINNO = (1 << 5),
	AIFLAG_ATTACKMOB = (1 << 6),
	AIFLAG_BERSERK = (1 << 7),
	AIFLAG_STONESKIN = (1 << 8),
	AIFLAG_GODSPEED = (1 << 9),
	AIFLAG_DEATHBLOW = (1 << 10),
	AIFLAG_REVIVE = (1 << 11),
	AIFLAG_HEALER = (1 << 12),
	AIFLAG_COUNT = (1 << 13),
	AIFLAG_NORECOVERY = (1 << 14),
	AIFLAG_REFLECT = (1 << 15),
	AIFLAG_FALL = (1 << 16),
	AIFLAG_VIT = (1 << 17),
	AIFLAG_RATTSPEED = (1 << 18),
	AIFLAG_RCASTSPEED = (1 << 19),
	AIFLAG_RHP_REGEN = (1 << 20),
	AIFLAG_TIMEVIT = (1 << 21),
};

enum EMobStatType
{
	MOB_STATTYPE_POWER,
	MOB_STATTYPE_TANKER,
	MOB_STATTYPE_SUPER_POWER,
	MOB_STATTYPE_SUPER_TANKER,
	MOB_STATTYPE_RANGE,
	MOB_STATTYPE_MAGIC,
	MOB_STATTYPE_MAX_NUM
};

enum EImmuneFlags
{
	IMMUNE_STUN = (1 << 0),
	IMMUNE_SLOW = (1 << 1),
	IMMUNE_FALL = (1 << 2),
	IMMUNE_CURSE = (1 << 3),
	IMMUNE_POISON = (1 << 4),
	IMMUNE_TERROR = (1 << 5),
	IMMUNE_REFLECT = (1 << 6),
};

enum EMobEnchants
{
	MOB_ENCHANT_CURSE,
	MOB_ENCHANT_SLOW,
	MOB_ENCHANT_POISON,
	MOB_ENCHANT_STUN,
	MOB_ENCHANT_CRITICAL,
	MOB_ENCHANT_PENETRATE,
	MOB_ENCHANTS_MAX_NUM
};

enum EMobResists
{
	MOB_RESIST_SWORD,
	MOB_RESIST_TWOHAND,
	MOB_RESIST_DAGGER,
	MOB_RESIST_BELL,
	MOB_RESIST_FAN,
	MOB_RESIST_BOW,
	MOB_RESIST_CLAW,
	MOB_RESIST_FIRE,
	MOB_RESIST_ELECT,
	MOB_RESIST_MAGIC,
	MOB_RESIST_WIND,
	MOB_RESIST_POISON,
	MOB_RESIST_BLEEDING,
	MOB_RESISTS_MAX_NUM
};

enum
{
	SKILL_ATTR_TYPE_NORMAL = 1,
	SKILL_ATTR_TYPE_MELEE,
	SKILL_ATTR_TYPE_RANGE,
	SKILL_ATTR_TYPE_MAGIC
	/*
	SKILL_ATTR_TYPE_FIRE,
	SKILL_ATTR_TYPE_ICE,
	SKILL_ATTR_TYPE_ELEC,
	SKILL_ATTR_TYPE_DARK,
	*/
};

enum
{
	SKILL_NORMAL,
	SKILL_MASTER,
	SKILL_GRAND_MASTER,
	SKILL_PERFECT_MASTER,
};

enum EGuildWarType
{
	GUILD_WAR_TYPE_FIELD,
	GUILD_WAR_TYPE_BATTLE,
	GUILD_WAR_TYPE_FLAG,
	GUILD_WAR_TYPE_MAX_NUM
};

enum EEmpire
{
	ALL_EMPIRES,
	SHINSOO_EMPIRE,
	CHUNJO_EMPIRE,
	JINNO_EMPIRE
};

enum EGuildWarState
{
	GUILD_WAR_NONE,
	GUILD_WAR_SEND_DECLARE,
	GUILD_WAR_REFUSE,
	GUILD_WAR_RECV_DECLARE,
	GUILD_WAR_WAIT_START,
	GUILD_WAR_CANCEL,
	GUILD_WAR_ON_WAR,
	GUILD_WAR_END,
	GUILD_WAR_OVER,
	GUILD_WAR_RESERVE,

	GUILD_WAR_DURATION = 30 * 60, // 1시간
	GUILD_WAR_WIN_POINT = 1000,
	GUILD_WAR_LADDER_HALF_PENALTY_TIME = 12 * 60 * 60,
};

enum EAttributeSet
{
	ATTRIBUTE_SET_WEAPON,
	ATTRIBUTE_SET_BODY,
	ATTRIBUTE_SET_WRIST,
	ATTRIBUTE_SET_FOOTS,
	ATTRIBUTE_SET_NECK,
	ATTRIBUTE_SET_HEAD,
	ATTRIBUTE_SET_SHIELD,
	ATTRIBUTE_SET_EAR,
#if defined(__COSTUME_SYSTEM__)
	ATTRIBUTE_SET_COSTUME_BODY,
	ATTRIBUTE_SET_COSTUME_HAIR,
#if defined(__WEAPON_COSTUME_SYSTEM__)
	ATTRIBUTE_SET_COSTUME_WEAPON,
#endif
#endif
#if defined(__PENDANT_SYSTEM__)
	ATTRIBUTE_SET_PENDANT,
#endif
#if defined(__GLOVE_SYSTEM__)
	ATTRIBUTE_SET_GLOVE,
#endif
	ATTRIBUTE_SET_MAX_NUM
};

enum EPrivType
{
	PRIV_NONE,
	PRIV_ITEM_DROP,
	PRIV_GOLD_DROP,
	PRIV_GOLD10_DROP,
	PRIV_EXP_PCT,
	MAX_PRIV_NUM,
};

enum EMoneyLogType
{
	MONEY_LOG_RESERVED,
	MONEY_LOG_MONSTER,
	MONEY_LOG_SHOP,
	MONEY_LOG_REFINE,
	MONEY_LOG_QUEST,
	MONEY_LOG_GUILD,
	MONEY_LOG_MISC,
	MONEY_LOG_MONSTER_KILL,
	MONEY_LOG_DROP,
	MONEY_LOG_TYPE_MAX_NUM,
};

enum EPremiumTypes
{
	PREMIUM_EXP, // 경험치가 1.2배
	PREMIUM_ITEM, // 아이템 드롭율이 2배
	PREMIUM_SAFEBOX, // 창고가 1칸에서 3칸
	PREMIUM_AUTOLOOT, // 돈 자동 줍기
	PREMIUM_FISH_MIND, // 고급 물고기 낚일 확률 상승
	PREMIUM_MARRIAGE_FAST, // 금실 증가 양을 빠르게합니다.
	PREMIUM_GOLD, // 돈 드롭율이 1.5배
	PREMIUM_MAX_NUM = 9
};

enum SPECIAL_EFFECT
{
	SE_NONE,

	SE_HPUP_RED,
	SE_SPUP_BLUE,
	SE_SPEEDUP_GREEN,
	SE_DXUP_PURPLE,
	SE_CRITICAL,
	SE_PENETRATE,
	SE_BLOCK,
	SE_DODGE,
	SE_CHINA_FIREWORK,
	SE_SPIN_TOP,
	SE_SUCCESS,
	SE_FAIL,
	SE_FR_SUCCESS,
	SE_LEVELUP_ON_14_FOR_GERMANY,
	SE_LEVELUP_UNDER_15_FOR_GERMANY,
	SE_PERCENT_DAMAGE1,
	SE_PERCENT_DAMAGE2,
	SE_PERCENT_DAMAGE3,

	SE_AUTO_HPUP,
	SE_AUTO_SPUP,

	SE_EQUIP_RAMADAN_RING, // 라마단 초승달의 반지(71135) 착용할 때 이펙트 (발동이펙트임, 지속이펙트 아님)
	SE_EQUIP_HALLOWEEN_CANDY, // 할로윈 사탕을 착용(-_-;)한 순간에 발동하는 이펙트
	SE_EQUIP_HAPPINESS_RING, // 크리스마스 행복의 반지(71143) 착용할 때 이펙트 (발동이펙트임, 지속이펙트 아님)
	SE_EQUIP_LOVE_PENDANT, // 발렌타인 사랑의 팬던트(71145) 착용할 때 이펙트 (발동이펙트임, 지속이펙트 아님)
	SE_CAPE_OF_COURAGE_EFFECT,	//@fixme510

	SE_EQUIP_MAGIC_RING,
#if defined(__ACCE_COSTUME_SYSTEM__)
	SE_ACCE_SUCESS_ABSORB,
	SE_EQUIP_ACCE,
#endif
	SE_EQUIP_EASTER_CANDY,
	SE_CAPE_OF_COURAGE,
	SE_EQUIP_CHOCOLATE_PENDANT,
	SE_PEPSI_EVENT,
	SE_BATTLE_POTION,
	SE_EQUIP_NAZAR_PENDANT,
	SE_EQUIP_GUARDIAN_PENDANT,
	SE_FLOWER_EVENT,
	SE_EQUIP_GEM_PENDANT,

	SE_THUNDER_AREA,
	SE_THUNDER,
	SE_HEAL,
	
#ifdef __GROWTH_PET_SYSTEM__
	SE_GYEONGGONG_BOOM,
#endif
#ifdef ENABLE_12ZI
	SE_SKILL_DAMAGE_ZONE,
	SE_SKILL_SAFE_ZONE,

	SE_METEOR,
	SE_BEAD_RAIN,
	SE_FALL_ROCK,
	SE_ARROW_RAIN,
	SE_HORSE_DROP,
	SE_EGG_DROP,
	SE_DEAPO_BOOM,

	SE_SKILL_DAMAGE_ZONE_BIG,
	SE_SKILL_DAMAGE_ZONE_MIDDLE,
	SE_SKILL_DAMAGE_ZONE_SMALL,

	SE_SKILL_SAFE_ZONE_BIG,
	SE_SKILL_SAFE_ZONE_MIDDLE,
	SE_SKILL_SAFE_ZONE_SMALL,
#endif
#if defined(ENABLE_EXTENDED_BATTLE_PASS)
	SE_EFFECT_BP_NORMAL_MISSION_COMPLETED,
	SE_EFFECT_BP_PREMIUM_MISSION_COMPLETED,
	SE_EFFECT_BP_EVENT_MISSION_COMPLETED,
	SE_EFFECT_BP_NORMAL_BATTLEPASS_COMPLETED,
	SE_EFFECT_BP_PREMIUM_BATTLEPASS_COMPLETED,
	SE_EFFECT_BP_EVENT_BATTLEPASS_COMPLETED,
#endif
};

#include "item_length.h"

// inventory의 position을 나타내는 구조체
// int와의 암시적 형변환이 있는 이유는,
// 인벤 관련된 모든 함수가 window_type은 받지 않고, cell 하나만 받았기 때문에,(기존에는 인벤이 하나 뿐이어서 inventory type이란게 필요없었기 때문에,)
// 인벤 관련 모든 함수 호출부분을 수정하는 것이 난감하기 떄문이다.

enum EDragonSoulRefineWindowSize
{
	DRAGON_SOUL_REFINE_GRID_MAX = 15,
};

enum EMisc2
{
	DRAGON_SOUL_EQUIP_SLOT_START = INVENTORY_MAX_NUM + WEAR_MAX_NUM, // 180 + 32 ( 212 )
	DRAGON_SOUL_EQUIP_SLOT_END = DRAGON_SOUL_EQUIP_SLOT_START + (DS_SLOT_MAX * DRAGON_SOUL_DECK_MAX_NUM), // 212 + 12 ( 224 )
	DRAGON_SOUL_EQUIP_RESERVED_SLOT_END = DRAGON_SOUL_EQUIP_SLOT_END + (DS_SLOT_MAX * DRAGON_SOUL_DECK_RESERVED_MAX_NUM), // 224 + 18 ( 242 )

	BELT_INVENTORY_SLOT_START = DRAGON_SOUL_EQUIP_RESERVED_SLOT_END, // 242
    BELT_INVENTORY_SLOT_END = BELT_INVENTORY_SLOT_START + BELT_INVENTORY_SLOT_COUNT, // 246 + 16 - 1 = 261

#if defined(__SPECIAL_INVENTORY_SYSTEM__)
    SKILL_BOOK_INVENTORY_SLOT_START = BELT_INVENTORY_SLOT_END , // 262
    SKILL_BOOK_INVENTORY_SLOT_END = SKILL_BOOK_INVENTORY_SLOT_START + SKILL_BOOK_INVENTORY_MAX_NUM, // 262 + 224 - 1 = 485

    UPGRADE_ITEMS_INVENTORY_SLOT_START = SKILL_BOOK_INVENTORY_SLOT_END, // 486
    UPGRADE_ITEMS_INVENTORY_SLOT_END = UPGRADE_ITEMS_INVENTORY_SLOT_START + UPGRADE_ITEMS_INVENTORY_MAX_NUM, // 486 + 224 - 1 = 709

    STONE_INVENTORY_SLOT_START = UPGRADE_ITEMS_INVENTORY_SLOT_END, // 710
    STONE_INVENTORY_SLOT_END = STONE_INVENTORY_SLOT_START + STONE_INVENTORY_MAX_NUM, // 710 + 224 - 1 = 933

    GIFT_BOX_INVENTORY_SLOT_START = STONE_INVENTORY_SLOT_END, // 934
    GIFT_BOX_INVENTORY_SLOT_END = GIFT_BOX_INVENTORY_SLOT_START + GIFT_BOX_INVENTORY_MAX_NUM, // 934 + 224 - 1 = 1157
#endif

#if defined(__SPECIAL_INVENTORY_SYSTEM__)
    INVENTORY_AND_EQUIP_SLOT_MAX = GIFT_BOX_INVENTORY_SLOT_END + 1, // 1158
#else
    INVENTORY_AND_EQUIP_SLOT_MAX = BELT_INVENTORY_SLOT_END + 1, // 262
#endif
};

#pragma pack(push, 1)

typedef struct SItemPos
{
	BYTE window_type;
	WORD cell;
	SItemPos()
	{
		window_type = INVENTORY;
		cell = WORD_MAX;
	}

	SItemPos(BYTE _window_type, WORD _cell)
	{
		window_type = _window_type;
		cell = _cell;
	}

	bool IsValidItemPosition() const
	{
		switch (window_type)
		{
		case RESERVED_WINDOW:
			return false;

		case INVENTORY:
		case EQUIPMENT:
		case BELT_INVENTORY:
#if defined(__SPECIAL_INVENTORY_SYSTEM__)
		case SKILL_BOOK_INVENTORY:
		case UPGRADE_ITEMS_INVENTORY:
		case STONE_INVENTORY:
		case GIFT_BOX_INVENTORY:
#endif
			return cell < INVENTORY_AND_EQUIP_SLOT_MAX;

		case DRAGON_SOUL_INVENTORY:
			return cell < (DRAGON_SOUL_INVENTORY_MAX_NUM);
			// 동적으로 크기가 정해지는 window는 valid 체크를 할 수가 없다.
		case SAFEBOX:
		case MALL:
			return false;
#ifdef ENABLE_SWITCHBOT
		case SWITCHBOT:
			return cell < SWITCHBOT_SLOT_COUNT;
#endif
		default:
			return false;
		}

		return false;
	}

	bool IsSamePosition(const SItemPos & other) const
	{
		return *this==other
			|| ((INVENTORY == window_type || EQUIPMENT == window_type)
				&& (INVENTORY == other.window_type || EQUIPMENT == other.window_type)
				&& cell == other.cell);
	}
	
	bool IsEquipPosition() const
	{
		return (((INVENTORY == window_type || EQUIPMENT == window_type) && (cell >= INVENTORY_MAX_NUM && cell < INVENTORY_MAX_NUM + WEAR_MAX_NUM)) || IsDragonSoulEquipPosition());
	}

	bool IsDragonSoulEquipPosition() const
	{
		return (INVENTORY == window_type && (DRAGON_SOUL_EQUIP_SLOT_START <= cell && DRAGON_SOUL_EQUIP_SLOT_END > cell));
	}

	bool IsBeltInventoryPosition() const
	{
		return (BELT_INVENTORY_SLOT_START <= cell && BELT_INVENTORY_SLOT_END > cell);
	}

	bool IsDefaultInventoryPosition() const
	{
		return (INVENTORY == window_type && cell < INVENTORY_MAX_NUM);
	}

#if defined(__SPECIAL_INVENTORY_SYSTEM__)
	bool IsSkillBookInventoryPosition() const
	{
		return (INVENTORY == window_type && (cell >= SKILL_BOOK_INVENTORY_SLOT_START && cell < SKILL_BOOK_INVENTORY_SLOT_END));
	}

	bool IsUpgradeItemsInventoryPosition() const
	{
		return (INVENTORY == window_type && (cell >= UPGRADE_ITEMS_INVENTORY_SLOT_START && cell < UPGRADE_ITEMS_INVENTORY_SLOT_END));
	}

	bool IsStoneInventoryPosition() const
	{
		return (INVENTORY == window_type && (cell >= STONE_INVENTORY_SLOT_START && cell < STONE_INVENTORY_SLOT_END));
	}

	bool IsGiftBoxInventoryPosition() const
	{
		return (INVENTORY == window_type && (cell >= GIFT_BOX_INVENTORY_SLOT_START && cell < GIFT_BOX_INVENTORY_SLOT_END));
	}
#endif

#ifdef ENABLE_SWITCHBOT
	bool IsSwitchbotPosition() const
	{
		return SWITCHBOT == window_type && cell < SWITCHBOT_SLOT_COUNT;
	}
#endif

	bool operator==(const struct SItemPos& rhs) const
	{
		return (window_type == rhs.window_type) && (cell == rhs.cell);
	}

	bool operator!=(const struct SItemPos& rhs) const
	{
		return (window_type != rhs.window_type) || (cell != rhs.cell);
	}

	bool operator<(const struct SItemPos& rhs) const
	{
		return (window_type < rhs.window_type) || ((window_type == rhs.window_type) && (cell < rhs.cell));
	}
} TItemPos;

const TItemPos NPOS(RESERVED_WINDOW, WORD_MAX);

typedef enum
{
	SHOP_COIN_TYPE_GOLD, // DEFAULT VALUE
	SHOP_COIN_TYPE_SECONDARY_COIN,
} EShopCoinType;

#if defined(__SKILL_COLOR_SYSTEM__)
namespace ESkillColorLength
{
	enum ESkillColorLength
	{
#if defined(__9TH_SKILL__)
		MAX_SKILL_COUNT = 10,
#else
		MAX_SKILL_COUNT = 6,
#endif
		MAX_EFFECT_COUNT = 5,
		MAX_BUFF_COUNT = 6,
		BUFF_BEGIN = MAX_SKILL_COUNT,
	};
}
#endif

enum EMultiLocale
{
	MAX_QUEST_NOTICE_ARGS = 5
};

enum ELocale
{
	LOCALE_YMIR, // Korea
	LOCALE_EN, // United Kingdom
	LOCALE_PT, // Portugal
	LOCALE_ES, // Spain
	LOCALE_FR, // France
	LOCALE_DE, // Germany
	LOCALE_RO, // Romania
	LOCALE_PL, // Poland
	LOCALE_IT, // Italy
	LOCALE_CZ, // Czech
	LOCALE_HU, // Hungary
	LOCALE_TR, // Turkey
	LOCALE_AE, // arabic
	LOCALE_MAX_NUM,
	LOCALE_DEFAULT = LOCALE_EN,
};
#ifdef __GROWTH_PET_SYSTEM__
enum EPetState
{
	STATE_NONE,
	STATE_UPBRINGING,
	STATE_BAG,
	STATE_SAFEBOX,
};
#endif
// added by dream 
#ifdef ENABLE_ANTI_MULTIPLE_FARM
enum EAntiMultipleFarmConfigs
{
	MA_LENGTH = 255,
	MULTIPLE_FARM_MAX_ACCOUNT = 2,// number of account related to the system always should be < 1 
};
#endif

#if defined(ENABLE_EXTENDED_BATTLE_PASS)
enum EExtBattlePassType {
	BATTLEPASS_NORMAL,
	BATTLEPASS_PREMIUM,
	BATTLEPASS_EVENT,
	BATTLEPASS_TYPE_MAX,
};

enum EExtBattleMissionPassType {
	MISSION_TYPE_NONE,

	KILL_MONSTER,
	KILL_PLAYER,

	DAMAGE_MONSTER,
	DAMAGE_PLAYER,

	BP_ITEM_USE,
	BP_ITEM_SELL,
	BP_ITEM_CRAFT,
	BP_ITEM_REFINE,
	BP_ITEM_DESTROY,
	BP_ITEM_COLLECT,

	FISH_FISHING,
	FISH_GRILL,
	FISH_CATCH,

	EXP_COLLECT,
	YANG_COLLECT,

	GUILD_PLAY_GUILDWAR,
	GUILD_SPENT_EXP,

	GAYA_CRAFT_GAYA,
	GAYA_BUY_ITEM_GAYA_COST,

	//PET_ENCHANT,

	COMPLETE_DUNGEON,
	COMPLETE_MINIGAME,

	MISSION_TYPE_MAX,
};
#endif

#ifdef ENABLE_AURA_SYSTEM
typedef struct SAuraRefineInfo
{
	BYTE bAuraRefineInfoLevel;
	BYTE bAuraRefineInfoExpPercent;
} TAuraRefineInfo;

enum EAuraMisc
{
	AURA_MAX_LEVEL = 250,
	AURA_REFINE_MAX_DISTANCE = 1000,
};

enum EAuraWindowType
{
	AURA_WINDOW_TYPE_ABSORB,
	AURA_WINDOW_TYPE_GROWTH,
	AURA_WINDOW_TYPE_EVOLVE,
	AURA_WINDOW_TYPE_MAX,
};

enum EAuraSlotType
{
	AURA_SLOT_MAIN,
	AURA_SLOT_SUB,
	AURA_SLOT_RESULT,
	AURA_SLOT_MAX
};

enum EAuraGradeType
{
	AURA_GRADE_NONE,
	AURA_GRADE_ORDINARY,
	AURA_GRADE_SIMPLE,
	AURA_GRADE_NOBLE,
	AURA_GRADE_SPARKLING,
	AURA_GRADE_MAGNIFICENT,
	AURA_GRADE_RADIANT,
	AURA_GRADE_MAX_NUM,
};
enum EAuraRefineInfoType
{
	AURA_REFINE_INFO_STEP,
	AURA_REFINE_INFO_LEVEL_MIN,
	AURA_REFINE_INFO_LEVEL_MAX,
	AURA_REFINE_INFO_NEED_EXP,
	AURA_REFINE_INFO_MATERIAL_VNUM,
	AURA_REFINE_INFO_MATERIAL_COUNT,
	AURA_REFINE_INFO_NEED_GOLD,
	AURA_REFINE_INFO_EVOLVE_PCT,
	AURA_REFINE_INFO_MAX
};

enum ERefineInfoType
{
	AURA_REFINE_INFO_SLOT_CURRENT,
	AURA_REFINE_INFO_SLOT_NEXT,
	AURA_REFINE_INFO_SLOT_EVOLVED,
	AURA_REFINE_INFO_SLOT_MAX
};
#endif

#ifdef __BACK_DUNGEON__
enum
{
	BACK_DUNGEON_SAVE,
	BACK_DUNGEON_CHECK,
	BACK_DUNGEON_WARP,
	BACK_DUNGEON_REMOVE,
};
#endif
#pragma pack(pop)

#endif // __INC_COMMON_LENGTH_H__
