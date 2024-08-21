
// #define LMAO_BAN
#ifndef __INC_COMMON_SERVICE_H__
#define __INC_COMMON_SERVICE_H__

/**
* @version 5000 Owsap Productions
**/

#define __SERVICE_OWSAP__

#define __GROWTH_PET_SYSTEM__
////////////////////////////////////////////////////////////////////////////////
// Mini Games Related
#define __MINI_GAME_OKEY__ /// < Mini game rumi (Okey)
#define __MINI_GAME_CATCH_KING__ /// < Mini game catch king

////////////////////////////////////////////////////////////////////////////////
// Dragon Soul Related
#define __DS_GRADE_MYTH__ /// < Dragon soul mythical grade
#define __DS_SET__ /// < Dragon soul table bonus handling
#define __EXTENDED_DSS_RECHARGE__ /// < Dragon soul extended recharge
#define __CONQUEROR_LEVEL__ // Conqueror Level
////////////////////////////////////////////////////////////////////////////////
// Costume Related
#define __COSTUME_SYSTEM__ /// < Costume System
#define __MOUNT_COSTUME_SYSTEM__ /// < Mount costume
//#define __MOUNT_ENTITY_REFRESH__
#define __ACCE_COSTUME_SYSTEM__ /// < Acce costume
#define __WEAPON_COSTUME_SYSTEM__ /// < Weapon costume
#define __COSTUME_ATTR_SYSTEM__ /// < Costume attributes
#define __EXTENDED_COSTUME_RECHARGE__ /// < Costume extended recharge
#define __HIDE_COSTUME_SYSTEM__ /// < Hide costume parts

////////////////////////////////////////////////////////////////////////////////
// Inventory & Equipment Related
#define __INVENTORY_4PAGES__ /// < Extended inventory pages (4)
#define __QUIVER_SYSTEM__ /// < Quiver equipement
#define __SWAP_ITEM_SYSTEM__ /// < Swap items between inventory slots
#define __SPECIAL_INVENTORY_SYSTEM__ /// < Special inventory
#define __SORT_INVENTORY_ITEMS__ /// < Sorts all inventory items

////////////////////////////////////////////////////////////////////////////////
// Player, Guild, Skills Related
#define __WJ_NEW_USER_CARE__ /// < User care
#define __PLAYER_PER_ACCOUNT5__ /// < 5 players per account
#define __WOLFMAN_CHARACTER__ /// < Wolfman character
#if defined(__WOLFMAN_CHARACTER__)
//#	define __DISABLE_WOLFMAN_CREATION__ /// < Disable Wolfman creation
#endif
#define __VIEW_TARGET_DECIMAL_HP__ /// < View target decimal hp
#if defined(__VIEW_TARGET_DECIMAL_HP__)
#	define __VIEW_TARGET_PLAYER_HP__ /// < View target player's hp
#endif
#define __GENDER_ALIGNMENT__ /// < Gender alignment (M, F)
#define __GUILD_LEADER_GRADE_NAME__ /// < Shows guild leader grade name on text tail
#define __7AND8TH_SKILLS__ /// < Passive 7 & 8th skills
//#define __IGNORE_LOW_POWER_BUFF__ /// < Ignore low power buff
#define __EXPRESSING_EMOTIONS__ /// < Special actions
//#define __SKILL_COLOR_SYSTEM__ /// < Skill color
#define __IMPROVED_LOGOUT_POINTS__ /// < Improved update packet on logout
#define __SKILL_COOLTIME_UPDATE__ /// < Refresh skill cooldown after death
#define __QUEST_RENEWAL__ /// < Quest page renewal
#define __PLAYER_PIN_SYSTEM__ /// < Player PIN Code
#if defined(__PLAYER_PIN_SYSTEM__)
//#	define __DISABLE_PIN_SYSTEM__
#endif
#define __RANDOM_STATUS_PER_LEVEL__ /// < Random status per level (classic)

////////////////////////////////////////////////////////////////////////////////
// Item Related
#define __ITEM_DROP_RENEWAL__ /// < Renewal of item drops with color effect
#define __WJ_PICKUP_ITEM_EFFECT__ /// < Pickup item effect
#define __NEW_DROP_DIALOG__ /// < New drop dialog with remove item option
#define __SOUL_BIND_SYSTEM__ /// < Soul bind items
#define __GACHA_SYSTEM__ /// < __BOSS_BOX__ Gacha boxes (x use time)
#define __MAGIC_REDUCTION__ /// < Magic reduction item
#define __STONE_OF_BLESS__ /// < Stone of bless (refinement item)
#define __SOUL_SYSTEM__ /// < Soul items
#define __BLEND_AFFECT__ /// < New blend affects with icon
#define __EXTENDED_BLEND_AFFECT__ /// < Extended blend item affect
#define __ANTI_EXP_RING__ /// < Anti experience ring

////////////////////////////////////////////////////////////////////////////////
// Game Related
#define __MAILBOX__ /*
* Description: Mail box system
* Author: Blackdragonx61 / Mali
* GitHub: https://github.com/blackdragonx61/Metin2-Official-MailBox
*/
#define __MESSENGER_GM__ /// < Messenger gm list
#define __MESSENGER_BLOCK_SYSTEM__ /// < Messenger block
#define __WJ_SHOW_MOB_INFO__ /// < Show monsters level & aggressive flag

#define __TEMPLE_OCHAO__ /// < Temple of the Ochao dungeon
#define __EREBUS_DUNGEON__ /// < Erebus dungeon
#define __SKILLBOOK_COMB_SYSTEM__ /// < Skill book combination
#define __PET_SYSTEM__ /// < Pet system
//#define __PRIVATE_SHOP_SEARCH_SYSTEM__ /// < Private shop search
#if defined(__PRIVATE_SHOP_SEARCH_SYSTEM__)
#	define __PRIVATE_SHOP_SEARCH_NEED_ITEM__
#endif
#define __CHANGE_LOOK_SYSTEM__ /// < Change look of item
#if defined(__CHANGE_LOOK_SYSTEM__)
#	define __MOUNT_CHANGE_LOOK__ /// < Change look of mount
#endif
#define __DICE_SYSTEM__ /// < New dice system
#define __SEND_TARGET_INFO__ /// < Monster target information
#define __SEND_TARGET_ELEMENT__ /// < Target element
#define __ELEMENT_SYSTEM__ /// < Elements of monsters and pendants
#define __PENDANT_SYSTEM__ /// < Talismans of elements
#define __EXTENDED_ITEM_AWARD__ /// < Extended item award
#define __MULTI_LANGUAGE_SYSTEM__ /// < Multi language
#define __EXTENDED_WHISPER_DETAILS__ /// < Extended whisper details
#define __REFINE_FAIL_TYPE__ /// < Extended refine fail message
#define __CHANNEL_STATUS_UPDATE__ /// < Channel player count
#define __9TH_SKILL__ /// < Conqueror of Yohara
#define __WJ_SHOW_PARTY_ON_MINIMAP__ /// < Show party member on atlas
#define __BINARY_ATLAS_MARK_INFO__ /// < Enable atlas mark info from client
#define __PARTY_PROFICY__ /// < Party proficy passive skill
#define __PVP_COUNTDOWN__ /// < PvP duel countdown
#define __EVENT_BANNER_FLAG__ /// < Game event banner flags
#define __ENVIRONMENT__ /// < System environment
#define __MYSHOP_DECO__ /// < Private shop decoration
#define __GLOVE_SYSTEM__ /// < Glove equipement
#define __EXTENDED_RELOAD__ /// < Extended GM reload commands (for drops)
#define __ITEM_APPLY4__

//////////////////////////////////////////////////////////////////////////
// Network Related
//#define __IMPROVED_PACKET_ENCRYPTION__ // 패킷 암호화 개선
#define __UDP_BLOCK__ /// < UDP Block
//#define __SEND_SEQUENCE__ /// Code for finding sequence bugs
//#define __ALLOW_EXTERNAL_PEER__ /// < Allow external peer API
#define __HWID_DETAILS__ /// < HWID


/////////////////////////////////////////////////////////////////////////
// dream addons
#define ENABLE_SAFEBOX_EX_SYSTEM					// Dream
//#define ENABLE_SELL_ITEM							//Dream
#define ENABLE_SKILL_MASTER_LEVEL					//Dream
#define ENABLE_SHOW_CHEST_DROP						//Dream
#define ENABLE_SPIRIT_STONE_READING						//Dream
#define ENABLE_CHEQUE_TO_GOLD						//Dream
#define ENABLE_DRAGONCOIN_SYSTEM					//Dream
#define ENABLE_QUEST_BLOCK							//Dream
#define ENABLE_SWITCHBOT						//Dream
#define ENABLE_GIVE_BASIC_ITEMS						//Dream
#define BL_REMOTE_SHOP						//Dream
#define ENABLE_BIOLOG_SYSTEM						//Dream
#define __TITLE_SYSTEM_YUMA__						//Dream
#define ENABLE_CUBE_RENEWAL_WORLDARD						//Dream
#define ENABLE_COUNT_NPC_FUNC					//Dream
#define u1x							//ranking system
#define DEATHMATCH_MODE						//Dream
#define __EVENT_MANAGER__					//Dream
#define ENABLE_ANTI_MULTIPLE_FARM					//Dream
//#define __EXTRA_PARTY_SYSTEM__					//Dream
#define ENABLE_AFFECT_POTION					//Dream
#define ENABLE_FEATURES_OXEVENT					//Dream
#define FIX_DUPE_DROP_PENALTY					//Dream
#define ENABLE_12ZI					//Dream
#ifdef ENABLE_12ZI
#define __12ZI_NOTICE__
// #	define ENABLE_CHAT_MISSION_ALTERNATIVE //Only enable this if you have the error of unsupported operand type
//#	define ENABLE_SERVERTIME_PORTAL_SPAWN
#endif
#define ENABLE_NEW_EXCHANGE_WINDOW
//////////////////////////////// end of dream addons



#define __ITEM_SHINING__ // by dcarys

//OFFLINESHOP by lkarus 
#define __ENABLE_NEW_OFFLINESHOP_FIX_FLOOD__
#define __ENABLE_NEW_OFFLINESHOP__
#define __ENABLE_CHANGELOOK_SYSTEM__
#define ENABLE_NEW_OFFLINESHOP_LOGS
#ifdef __ENABLE_NEW_OFFLINESHOP__
	//#define ENABLE_NEW_SHOP_IN_CITIES
#endif
#define ENABLE_SOULBIND_SYSTEM /// < block offlineshop soul bind items
//#define ENABLE_OFFLINESHOP_DEBUG
#define ITEM_CHECKINOUT_UPDATE // by Mali 

#define __IMPROVED_HANDSHAKE_PROCESS__

#define ENABLE_ITEMSHOP
#define ENABLE_AURA_SYSTEM
#define ENABLE_CMD_WARP_IN_DUNGEON
#define FIX_HEADER_CG_MARK_LOGIN
#define __RENEWAL_BRAVE_CAPE__
//#define ENABLE_PULSE_MANAGER
//#define LMAO_BAN
#define __DUNGEON_INFO_SYSTEM__ 
#define ENABLE_DUNGEON_INFO_SYSTEM

#define __MOB_DISAPPEAR_FAST__
#define __MOB_DISAPPEAR_FAST__VALUE 1 //  Replaces the value 1, while the number of seconds to disappear after killing mobs their recommended is 1 second to leave because they still do the job.
//#define MAINTENANCE_INFO
#define ENABLE_TIMER_EVENT_FIX
#define ENABLE_BOSS_KICK_INTO_WALLS_FIX
#define ENABLE_DUNGEON_PARTY_FIX
#define ENABLE_EXPLOIT_QUEST_FIX
#define ENABLE_COMMAND_FLUSH_FIX
#define ENABLE_EXP_GROUP_FIX
//#define ENABLE_KICK_SYNC_FIX
#define ENABLE_PC_SELECT_QUEST_FIX
#define ENABLE_SYNC_HACK_FIX
#define ENABLE_CRASH_CORE_ARROW_FIX
#define ENABLE_LOAD_MOBS_WITH_MOUNT_FIX
#define ENABLE_DESC_FIND_PC_FIX
#define ENABLE_DBMANAGER_SQL_INJECT_FIX
//#define ENABLE_GET_DAMAGE_FIX
// #define __BACK_DUNGEON__
//#define ENABLE_DUNGEON_TURN_BACK					//Zindan Geri Donme Sistemi
#define ENABLE_GLOBAL_CHAT // AKTIVIERT DEN GLOBAL CHAT
#endif // __INC_COMMON_SERVICE_H__

