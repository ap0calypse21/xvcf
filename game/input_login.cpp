#include "stdafx.h"
#include "constants.h"
#include "config.h"
#include "utils.h"
#include "input.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "cmd.h"
#include "buffer_manager.h"
#include "protocol.h"
#include "pvp.h"
#include "start_position.h"
#include "messenger_manager.h"
#include "guild_manager.h"
#include "party.h"
#include "dungeon.h"
#include "war_map.h"
#include "questmanager.h"
#include "building.h"
#include "wedding.h"
#include "affect.h"
#include "arena.h"
#include "OXEvent.h"
#include "priv_manager.h"
#include "block_country.h"
#include "dev_log.h"
#include "log.h"
#include "horsename_manager.h"
#include "MarkManager.h"
#include "EmailAuth.h"

#ifdef ENABLE_SWITCHBOT
#include "switchbot.h"
#endif
#ifdef ENABLE_BIOLOG_SYSTEM	
	#include "biolog.h"
#endif
#if defined(__MINI_GAME_CATCH_KING__)
#include "minigame.h"
#endif
#ifdef ENABLE_12ZI
#	include "zodiac_temple.h"
#endif
#ifdef __ENABLE_NEW_OFFLINESHOP__
#include "new_offlineshop.h"
#include "new_offlineshop_manager.h"
#endif
#ifdef ENABLE_ANTI_MULTIPLE_FARM
#include "HAntiMultipleFarm.h"
#endif
#if defined(__EVENT_MANAGER__)
#include "event_manager.h"
#endif
#if defined(ENABLE_PVP_TOURNAMENT)
#include "pvp_duel.h"
#endif

static void _send_bonus_info(LPCHARACTER ch)
{
	int item_drop_bonus = 0;
	int gold_drop_bonus = 0;
	int gold10_drop_bonus = 0;
	int exp_bonus = 0;

	item_drop_bonus = CPrivManager::instance().GetPriv(ch, PRIV_ITEM_DROP);
	gold_drop_bonus = CPrivManager::instance().GetPriv(ch, PRIV_GOLD_DROP);
	gold10_drop_bonus = CPrivManager::instance().GetPriv(ch, PRIV_GOLD10_DROP);
	exp_bonus = CPrivManager::instance().GetPriv(ch, PRIV_EXP_PCT);

	if (item_drop_bonus)
	{
		ch->ChatPacket(CHAT_TYPE_NOTICE,
			LC_TEXT("아이템 드롭률  %d%% 추가 이벤트 중입니다."), item_drop_bonus);
	}
	if (gold_drop_bonus)
	{
		ch->ChatPacket(CHAT_TYPE_NOTICE,
			LC_TEXT("골드 드롭률 %d%% 추가 이벤트 중입니다."), gold_drop_bonus);
	}
	if (gold10_drop_bonus)
	{
		ch->ChatPacket(CHAT_TYPE_NOTICE,
			LC_TEXT("대박골드 드롭률 %d%% 추가 이벤트 중입니다."), gold10_drop_bonus);
	}
	if (exp_bonus)
	{
		ch->ChatPacket(CHAT_TYPE_NOTICE,
			LC_TEXT("경험치 %d%% 추가 획득 이벤트 중입니다."), exp_bonus);
	}
}

static bool FN_is_battle_zone(LPCHARACTER ch)
{
	switch (ch->GetMapIndex())
	{
	case 1: // 신수 1차 마을
	case 2: // 신수 2차 마을
	case 21: // 천조 1차 마을
	case 23: // 천조 2차 마을
	case 41: // 진노 1차 마을
	case 43: // 진노 2차 마을
	case 113: // OX 맵
		return false;
	}

	return true;
}

void CInputLogin::Login(LPDESC d, const char* data)
{
	TPacketCGLogin* pinfo = (TPacketCGLogin*)data;

	char login[LOGIN_MAX_LEN + 1];
	trim_and_lower(pinfo->login, login, sizeof(login));

	sys_log(0, "InputLogin::Login : %s", login);

	TPacketGCLoginFailure failurePacket;

	if (g_iUseLocale && !test_server)
	{
		failurePacket.header = HEADER_GC_LOGIN_FAILURE;
		strlcpy(failurePacket.szStatus, "VERSION", sizeof(failurePacket.szStatus));
		d->Packet(&failurePacket, sizeof(TPacketGCLoginFailure));
		return;
	}

	if (g_bNoMoreClient)
	{
		failurePacket.header = HEADER_GC_LOGIN_FAILURE;
		strlcpy(failurePacket.szStatus, "SHUTDOWN", sizeof(failurePacket.szStatus));
		d->Packet(&failurePacket, sizeof(TPacketGCLoginFailure));
		return;
	}

	if (g_iUserLimit > 0)
	{
		int iTotal;
		int* paiEmpireUserCount;
		int iLocal;

		DESC_MANAGER::instance().GetUserCount(iTotal, &paiEmpireUserCount, iLocal);

		if (g_iUserLimit <= iTotal)
		{
			failurePacket.header = HEADER_GC_LOGIN_FAILURE;
			strlcpy(failurePacket.szStatus, "FULL", sizeof(failurePacket.szStatus));
			d->Packet(&failurePacket, sizeof(TPacketGCLoginFailure));
			return;
		}
	}

	TLoginPacket login_packet;

	strlcpy(login_packet.login, login, sizeof(login_packet.login));
	strlcpy(login_packet.passwd, pinfo->passwd, sizeof(login_packet.passwd));

	db_clientdesc->DBPacket(HEADER_GD_LOGIN, d->GetHandle(), &login_packet, sizeof(TLoginPacket));
}

void CInputLogin::LoginByKey(LPDESC d, const char* data)
{
	TPacketCGLogin2* pinfo = (TPacketCGLogin2*)data;

	char login[LOGIN_MAX_LEN + 1];
	trim_and_lower(pinfo->login, login, sizeof(login));

	// is blocked ip?
	{
		dev_log(LOG_DEB0, "check_blocked_country_start");

		if (!is_block_exception(login) && is_blocked_country_ip(d->GetHostName()))
		{
			sys_log(0, "BLOCK_COUNTRY_IP (%s)", d->GetHostName());
			d->SetPhase(PHASE_CLOSE);
			return;
		}

		dev_log(LOG_DEB0, "check_blocked_country_end");
	}

	if (g_bNoMoreClient)
	{
		TPacketGCLoginFailure failurePacket;

		failurePacket.header = HEADER_GC_LOGIN_FAILURE;
		strlcpy(failurePacket.szStatus, "SHUTDOWN", sizeof(failurePacket.szStatus));
		d->Packet(&failurePacket, sizeof(TPacketGCLoginFailure));
		return;
	}

	if (g_iUserLimit > 0)
	{
		int iTotal;
		int* paiEmpireUserCount;
		int iLocal;

		DESC_MANAGER::instance().GetUserCount(iTotal, &paiEmpireUserCount, iLocal);

		if (g_iUserLimit <= iTotal)
		{
			TPacketGCLoginFailure failurePacket;

			failurePacket.header = HEADER_GC_LOGIN_FAILURE;
			strlcpy(failurePacket.szStatus, "FULL", sizeof(failurePacket.szStatus));

			d->Packet(&failurePacket, sizeof(TPacketGCLoginFailure));
			return;
		}
	}

	sys_log(0, "LOGIN_BY_KEY: %s key %u", login, pinfo->dwLoginKey);

	d->SetLoginKey(pinfo->dwLoginKey);
#ifdef ENABLE_ANTI_MULTIPLE_FARM
	d->SetLoginMacAdress(pinfo->cMAIf); // mac address sniffing 
#endif
#if !defined(__IMPROVED_PACKET_ENCRYPTION__)
	d->SetSecurityKey(pinfo->adwClientKey);
#endif

	TPacketGDLoginByKey ptod;

	strlcpy(ptod.szLogin, login, sizeof(ptod.szLogin));
	ptod.dwLoginKey = pinfo->dwLoginKey;
	thecore_memcpy(ptod.adwClientKey, pinfo->adwClientKey, sizeof(DWORD) * 4);
	strlcpy(ptod.szIP, d->GetHostName(), sizeof(ptod.szIP));

	db_clientdesc->DBPacket(HEADER_GD_LOGIN_BY_KEY, d->GetHandle(), &ptod, sizeof(TPacketGDLoginByKey));
}

void CInputLogin::ChangeName(LPDESC d, const char* data)
{
	TPacketCGChangeName* p = (TPacketCGChangeName*)data;
	const TAccountTable& c_r = d->GetAccountTable();

	if (!c_r.id)
	{
		sys_err("no account table");
		return;
	}

	if (p->index >= PLAYER_PER_ACCOUNT || p->index < 0)
	{
		sys_err("index overflow %d, login: %s", p->index, c_r.login);
		return;
	}

	if (!c_r.players[p->index].dwID)
	{
		sys_err("no player id, login %s", c_r.login);
		return;
	}

	if (!c_r.players[p->index].bChangeName)
		return;

	if (!check_name(p->name))
	{
		TPacketGCCreateFailure pack;
		pack.header = HEADER_GC_CHARACTER_CREATE_FAILURE;
		pack.bType = 0;
		d->Packet(&pack, sizeof(pack));
		return;
	}

	TPacketGDChangeName pdb;

	pdb.pid = c_r.players[p->index].dwID;
	strlcpy(pdb.name, p->name, sizeof(pdb.name));
	db_clientdesc->DBPacket(HEADER_GD_CHANGE_NAME, d->GetHandle(), &pdb, sizeof(TPacketGDChangeName));
}

void CInputLogin::CharacterSelect(LPDESC d, const char* data)
{
	struct command_player_select* pinfo = (struct command_player_select*)data;
	const TAccountTable& c_r = d->GetAccountTable();

	sys_log(0, "player_select: login: %s index: %d", c_r.login, pinfo->index);

	if (!c_r.id)
	{
		sys_err("no account table");
		return;
	}

	if (pinfo->index >= PLAYER_PER_ACCOUNT || pinfo->index < 0)
	{
		sys_err("index overflow %d, login: %s", pinfo->index, c_r.login);
		return;
	}

	if (!c_r.players[pinfo->index].dwID)
	{
		sys_err("no player id, login %s", c_r.login);
		return;
	}

	if (c_r.players[pinfo->index].bChangeName)
	{
		sys_err("name must be changed idx %d, login %s, name %s",
			pinfo->index, c_r.login, c_r.players[pinfo->index].szName);
		return;
	}

	TPlayerLoadPacket player_load_packet;

	player_load_packet.account_id = c_r.id;
	player_load_packet.player_id = c_r.players[pinfo->index].dwID;
	player_load_packet.account_index = pinfo->index;

	db_clientdesc->DBPacket(HEADER_GD_PLAYER_LOAD, d->GetHandle(), &player_load_packet, sizeof(TPlayerLoadPacket));
}

bool NewPlayerTable(TPlayerTable* table,
	const char* name,
	BYTE job,
	BYTE shape,
	BYTE bEmpire,
	BYTE bCon,
	BYTE bInt,
	BYTE bStr,
	BYTE bDex)
{
	if (job >= JOB_MAX_NUM)
		return false;

	memset(table, 0, sizeof(TPlayerTable));

	strlcpy(table->name, name, sizeof(table->name));

	table->level = 1;
	table->job = job;
	table->voice = 0;
	table->part_base = shape;
#ifdef ENABLE_AURA_SYSTEM
	table->parts[PART_AURA] = 0;
#endif
	table->st = JobInitialPoints[job].st;
	table->dx = JobInitialPoints[job].dx;
	table->ht = JobInitialPoints[job].ht;
	table->iq = JobInitialPoints[job].iq;

	table->hp = JobInitialPoints[job].max_hp + table->ht * JobInitialPoints[job].hp_per_ht;
	table->sp = JobInitialPoints[job].max_sp + table->iq * JobInitialPoints[job].sp_per_iq;
	table->stamina = JobInitialPoints[job].max_stamina;

	table->x = CREATE_START_X(bEmpire, job) + number(-300, 300);
	table->y = CREATE_START_Y(bEmpire, job) + number(-300, 300);

	table->z = 0;
	table->dir = 0;
	table->playtime = 0;
	table->gold = 0;

	table->skill_group = 0;

	if (china_event_server)
	{
		table->level = 35;

		for (int i = 1; i < 35; ++i)
		{
			int iHP = number(JobInitialPoints[job].hp_per_lv_begin, JobInitialPoints[job].hp_per_lv_end);
			int iSP = number(JobInitialPoints[job].sp_per_lv_begin, JobInitialPoints[job].sp_per_lv_end);
			table->sRandomHP += iHP;
			table->sRandomSP += iSP;
			table->stat_point += 3;
		}

		table->hp += table->sRandomHP;
		table->sp += table->sRandomSP;

		table->gold = 1000000;
	}

	return true;
}

bool RaceToJob(unsigned race, unsigned* ret_job)
{
	*ret_job = 0;

	if (race >= MAIN_RACE_MAX_NUM)
		return false;

	switch (race)
	{
	case MAIN_RACE_WARRIOR_M:
		*ret_job = JOB_WARRIOR;
		break;

	case MAIN_RACE_WARRIOR_W:
		*ret_job = JOB_WARRIOR;
		break;

	case MAIN_RACE_ASSASSIN_M:
		*ret_job = JOB_ASSASSIN;
		break;

	case MAIN_RACE_ASSASSIN_W:
		*ret_job = JOB_ASSASSIN;
		break;

	case MAIN_RACE_SURA_M:
		*ret_job = JOB_SURA;
		break;

	case MAIN_RACE_SURA_W:
		*ret_job = JOB_SURA;
		break;

	case MAIN_RACE_SHAMAN_M:
		*ret_job = JOB_SHAMAN;
		break;

	case MAIN_RACE_SHAMAN_W:
		*ret_job = JOB_SHAMAN;
		break;

	case MAIN_RACE_WOLFMAN_M:
		*ret_job = JOB_WOLFMAN;
		break;

	default:
		return false;
		break;
	}
	return true;
}

// 신규 캐릭터 지원
bool NewPlayerTable2(TPlayerTable* table, const char* name, BYTE race, BYTE shape, BYTE bEmpire, const char* pin)
{
	if (race >= MAIN_RACE_MAX_NUM)
	{
		sys_err("NewPlayerTable2.OUT_OF_RACE_RANGE(%d >= max(%d))\n", race, MAIN_RACE_MAX_NUM);
		return false;
	}

	unsigned job;

	if (!RaceToJob(race, &job))
	{
		sys_err("NewPlayerTable2.RACE_TO_JOB_ERROR(%d)\n", race);
		return false;
	}

	sys_log(0, "NewPlayerTable2(name=%s, race=%d, job=%d)", name, race, job);

	memset(table, 0, sizeof(TPlayerTable));

	strlcpy(table->name, name, sizeof(table->name));

	table->level = 1;
	table->job = race; // 직업대신 종족을 넣는다
	table->voice = 0;
	table->part_base = shape;
#ifdef ENABLE_AURA_SYSTEM
	table->parts[PART_AURA] = 0;
#endif
	table->st = JobInitialPoints[job].st;
	table->dx = JobInitialPoints[job].dx;
	table->ht = JobInitialPoints[job].ht;
	table->iq = JobInitialPoints[job].iq;

	table->hp = JobInitialPoints[job].max_hp + table->ht * JobInitialPoints[job].hp_per_ht;
	table->sp = JobInitialPoints[job].max_sp + table->iq * JobInitialPoints[job].sp_per_iq;
	table->stamina = JobInitialPoints[job].max_stamina;

	table->x = CREATE_START_X(bEmpire, job) + number(-300, 300);
	table->y = CREATE_START_Y(bEmpire, job) + number(-300, 300);

	table->z = 0;
	table->dir = 0;
	table->playtime = 0;
	table->gold = 0;

	table->skill_group = 0;

	strlcpy(table->pin, pin, sizeof(table->pin));

	return true;
}

void CInputLogin::CharacterCreate(LPDESC d, const char* data)
{
	struct command_player_create* pinfo = (struct command_player_create*)data;
	TPlayerCreatePacket player_create_packet;

	sys_log(0, "PlayerCreate: name %s pos %d job %d shape %d",
		pinfo->name,
		pinfo->index,
		pinfo->job,
		pinfo->shape
	);

	TPacketGCLoginFailure packFailure;
	memset(&packFailure, 0, sizeof(packFailure));
	packFailure.header = HEADER_GC_CHARACTER_CREATE_FAILURE;

	if (true == g_BlockCharCreation)
	{
		d->Packet(&packFailure, sizeof(packFailure));
		return;
	}

	if (!g_bWolfmanCreation)
	{
		if (pinfo->job == MAIN_RACE_WOLFMAN_M)
		{
			d->Packet(&packFailure, sizeof(packFailure));
			return;
		}
	}

	// 사용할 수 없는 이름이거나, 잘못된 평상복이면 생설 실패
	if (!check_name(pinfo->name) || pinfo->shape > 1)
	{
		if (LC_IsCanada() == true)
		{
			TPacketGCCreateFailure pack;
			pack.header = HEADER_GC_CHARACTER_CREATE_FAILURE;
			pack.bType = 1;

			d->Packet(&pack, sizeof(pack));
			return;
		}

		d->Packet(&packFailure, sizeof(packFailure));
		return;
	}

	if (LC_IsEurope() == true)
	{
		const TAccountTable& c_rAccountTable = d->GetAccountTable();

		if (0 == strcmp(c_rAccountTable.login, pinfo->name))
		{
			TPacketGCCreateFailure pack;
			pack.header = HEADER_GC_CHARACTER_CREATE_FAILURE;
			pack.bType = 1;

			d->Packet(&pack, sizeof(pack));
			return;
		}
	}

	memset(&player_create_packet, 0, sizeof(TPlayerCreatePacket));

	if (!NewPlayerTable2(&player_create_packet.player_table, pinfo->name, pinfo->job, pinfo->shape, d->GetEmpire(), pinfo->pin))
	{
		sys_err("player_prototype error: job %d face %d ", pinfo->job);
		d->Packet(&packFailure, sizeof(packFailure));
		return;
	}

	const TAccountTable& c_rAccountTable = d->GetAccountTable();

	trim_and_lower(c_rAccountTable.login, player_create_packet.login, sizeof(player_create_packet.login));
	strlcpy(player_create_packet.passwd, c_rAccountTable.passwd, sizeof(player_create_packet.passwd));

	player_create_packet.account_id = c_rAccountTable.id;
	player_create_packet.account_index = pinfo->index;

	sys_log(0, "PlayerCreate: name %s account_id %d, TPlayerCreatePacketSize(%d), Packet->Gold %d",
		pinfo->name,
		pinfo->index,
		sizeof(TPlayerCreatePacket),
		player_create_packet.player_table.gold);

	db_clientdesc->DBPacket(HEADER_GD_PLAYER_CREATE, d->GetHandle(), &player_create_packet, sizeof(TPlayerCreatePacket));
}

void CInputLogin::CharacterDelete(LPDESC d, const char* data)
{
	struct command_player_delete* pinfo = (struct command_player_delete*)data;
	const TAccountTable& c_rAccountTable = d->GetAccountTable();

	if (!c_rAccountTable.id)
	{
		sys_err("PlayerDelete: no login data");
		return;
	}

	sys_log(0, "PlayerDelete: login: %s index: %d, social_id %s", c_rAccountTable.login, pinfo->index, pinfo->private_code);

	if (pinfo->index >= PLAYER_PER_ACCOUNT || pinfo->index < 0)
	{
		sys_err("PlayerDelete: index overflow %d, login: %s", pinfo->index, c_rAccountTable.login);
		return;
	}

	if (!c_rAccountTable.players[pinfo->index].dwID)
	{
		sys_err("PlayerDelete: Wrong Social ID index %d, login: %s", pinfo->index, c_rAccountTable.login);
		d->Packet(encode_byte(HEADER_GC_CHARACTER_DELETE_WRONG_SOCIAL_ID), 1);
		return;
	}

	TPlayerDeletePacket player_delete_packet;

	trim_and_lower(c_rAccountTable.login, player_delete_packet.login, sizeof(player_delete_packet.login));
	player_delete_packet.player_id = c_rAccountTable.players[pinfo->index].dwID;
	player_delete_packet.account_index = pinfo->index;
	strlcpy(player_delete_packet.private_code, pinfo->private_code, sizeof(player_delete_packet.private_code));

	db_clientdesc->DBPacket(HEADER_GD_PLAYER_DELETE, d->GetHandle(), &player_delete_packet, sizeof(TPlayerDeletePacket));
}

void CInputLogin::CharacterPinCode(LPDESC d, const char* c_pData)
{
    const TPacketCGCharacterPinCode& CharacterPinCodeCGPacket = *((TPacketCGCharacterPinCode*)c_pData);
    const TAccountTable& c_rAccountTable = d->GetAccountTable();

    if (!c_rAccountTable.id)
    {
        sys_err("PlayerPinCode: no login data");
        return;
    }

    sys_log(0, "PlayerPinCode: login: %s index: %d, pin %s", c_rAccountTable.login, CharacterPinCodeCGPacket.bIndex, CharacterPinCodeCGPacket.szPinCode);

    if (CharacterPinCodeCGPacket.bIndex >= PLAYER_PER_ACCOUNT || CharacterPinCodeCGPacket.bIndex < 0)
    {
        sys_err("PlayerPinCode: index overflow %d, login: %s", CharacterPinCodeCGPacket.bIndex, c_rAccountTable.login);
        return;
    }

    if (!c_rAccountTable.players[CharacterPinCodeCGPacket.bIndex].dwID)
    {
        sys_err("PlayerPinCode: no player id, login %s", c_rAccountTable.login);
        return;
    }

    TPacketGCCharacterPinCode CharacterPinCodeGCPacket;
    CharacterPinCodeGCPacket.bHeader = HEADER_GC_PLAYER_PIN_CODE;

    // Retrieve the actual PIN code from the database
    char szPinCode[PIN_CODE_LENGTH + 1] = {};
    {
        char szQuery[128];
        snprintf(szQuery, sizeof(szQuery), "SELECT `pin` FROM `player`.`player` WHERE `id` = %u AND `account_id` = %u",
                 c_rAccountTable.players[CharacterPinCodeCGPacket.bIndex].dwID, c_rAccountTable.id);
        std::unique_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery(szQuery));
        if (pMsg->Get()->uiNumRows > 0)
        {
            MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
            strlcpy(szPinCode, row[0], sizeof(szPinCode));
        }
    }

    // Retrieve the email verification code from the database
    char szEmailVerificationCode[16] = {};
    {
        char szQuery[128];
        snprintf(szQuery, sizeof(szQuery), "SELECT `email_verification_code` FROM `account`.`account` WHERE `id` = %u", c_rAccountTable.id);
        std::unique_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery(szQuery));
        if (pMsg->Get()->uiNumRows > 0)
        {
            MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
            strlcpy(szEmailVerificationCode, row[0], sizeof(szEmailVerificationCode));
        }
    }

    // Check if the PIN code matches or if the email verification code matches
    bool isPlayerPinValid = strcmp(CharacterPinCodeCGPacket.szPinCode, szPinCode) == 0;
    bool isEmailPinValid = strcmp(CharacterPinCodeCGPacket.szPinCode, szEmailVerificationCode) == 0;

    if (isPlayerPinValid || isEmailPinValid)
    {
        CharacterPinCodeGCPacket.bValid = true;

        // Update HWID validation only if the player used their actual PIN from the player table
        if (isPlayerPinValid)
        {
            char szQuery[128];
            snprintf(szQuery, sizeof(szQuery), "UPDATE `account`.`hwid` SET `valid%d` = 1 WHERE `account_id` = %u",
                     CharacterPinCodeCGPacket.bIndex + 1, c_rAccountTable.id);
            std::unique_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery(szQuery));
        }
    }
    else
    {
        sys_log(0, "PlayerPinCode: PIN or Email Code mismatch. Entered: %s", CharacterPinCodeCGPacket.szPinCode);
        CharacterPinCodeGCPacket.bValid = false;
        d->Packet(&CharacterPinCodeGCPacket, sizeof(TPacketGCCharacterPinCode));
        d->DelayedDisconnect(3);
        return;
    }

    d->Packet(&CharacterPinCodeGCPacket, sizeof(TPacketGCCharacterPinCode));
}

#pragma pack(1)
typedef struct SPacketGTLogin
{
	BYTE header;
	WORD empty;
	DWORD id;
} TPacketGTLogin;
#pragma pack()

void CInputLogin::Entergame(LPDESC d, const char* data)
{
	LPCHARACTER ch;

	if (!(ch = d->GetCharacter()))
	{
		d->SetPhase(PHASE_CLOSE);
		return;
	}

#ifdef ENABLE_CSHIELD
	if (strncmp(d->GetAccountTable().status, "OK", 2) != 0)
	{
		d->SetPhase(PHASE_CLOSE);
		return;
	}
#endif
	PIXEL_POSITION pos = ch->GetXYZ();

	if (!SECTREE_MANAGER::instance().GetMovablePosition(ch->GetMapIndex(), pos.x, pos.y, pos))
	{
		PIXEL_POSITION pos2;
		SECTREE_MANAGER::instance().GetRecallPositionByEmpire(ch->GetMapIndex(), ch->GetEmpire(), pos2);

		sys_err("!GetMovablePosition (name %s %dx%d map %d changed to %dx%d)",
			ch->GetName(),
			pos.x, pos.y,
			ch->GetMapIndex(),
			pos2.x, pos2.y);
		pos = pos2;
	}

	CGuildManager::instance().LoginMember(ch);
	// 캐릭터를 맵에 추가 
	ch->Show(ch->GetMapIndex(), pos.x, pos.y, pos.z);

#if !defined(__BINARY_ATLAS_MARK_INFO__)
	SECTREE_MANAGER::instance().SendNPCPosition(ch);
#endif

	d->SetPhase(PHASE_GAME);

#if defined(__HIDE_COSTUME_SYSTEM__)
	if (ch->GetQuestFlag("costume_option.hide_body") != 0)
		ch->SetBodyCostumeHidden(true);
	else
		ch->SetBodyCostumeHidden(false);

	if (ch->GetQuestFlag("costume_option.hide_hair") != 0)
		ch->SetHairCostumeHidden(true);
	else
		ch->SetHairCostumeHidden(false);

#if defined(__ACCE_COSTUME_SYSTEM__)
	if (ch->GetQuestFlag("costume_option.hide_acce") != 0)
		ch->SetAcceCostumeHidden(true);
	else
		ch->SetAcceCostumeHidden(false);
#endif

#if defined(__WEAPON_COSTUME_SYSTEM__)
	if (ch->GetQuestFlag("costume_option.hide_weapon") != 0)
		ch->SetWeaponCostumeHidden(true);
	else
		ch->SetWeaponCostumeHidden(false);
#endif
# ifdef ENABLE_AURA_SYSTEM
	if (ch->GetQuestFlag("costume_option.hide_aura") != 0)
		ch->SetAuraCostumeHidden(true);
	else
		ch->SetAuraCostumeHidden(false);
# endif
#endif

#if defined(__EXPRESSING_EMOTIONS__)
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "ClearSpecialEmotions");
		for (int iActionID = 60; iActionID < 78; ++iActionID)
		{
			char szQuestFlag[20] = "0";
			snprintf(szQuestFlag, sizeof(szQuestFlag), "emotions.expire_%d", iActionID);

			DWORD dwExpireTime = ch->GetQuestFlag(szQuestFlag);
			if (dwExpireTime != 0 && dwExpireTime > (uint32_t)(get_global_time()))
				ch->ChatPacket(CHAT_TYPE_COMMAND, "RegisterSpecialEmotion %d %d 0", iActionID, dwExpireTime);
		}
	}
#endif

	if (ch->affectInfo)
	{
		ch->LoadAffect(ch->affectInfo->count, (TPacketAffectElement*)ch->affectInfo->data);
		M2_DELETE_ARRAY(ch->affectInfo->data);
		M2_DELETE(ch->affectInfo);

		ch->affectInfo = NULL;
	}

	if (ch->IsGM() && ch->IsAffectFlag(AFF_INVISIBILITY) && !test_server)
	{
		ch->SetObserverMode(true);
	}

	ch->Show(ch->GetMapIndex(), pos.x, pos.y, pos.z);

	if (ch->GetItemAward_cmd()[0] != '\0') // 게임페이즈 들어가면
		quest::CQuestManager::instance().ItemInformer(ch->GetPlayerID(), ch->GetItemAward_vnum()); // questmanager 호출

	sys_log(0, "ENTERGAME: %s %dx%dx%d %s map_index %d",
		ch->GetName(), ch->GetX(), ch->GetY(), ch->GetZ(), d->GetHostName(), ch->GetMapIndex());

	if (ch->GetHorseLevel() > 0)
	{
		ch->EnterHorse();
	}

// #if defined(__MOUNT_COSTUME_SYSTEM__)
	// LPITEM pCostumeMount = ch->GetWear(WEAR_COSTUME_MOUNT);
	// if (pCostumeMount)
	// {
		// DWORD dwMountVnum = pCostumeMount->GetMountVnum();
		// if (ch->GetMapIndex() == 1 || ch->GetMapIndex() == 21 || ch->GetMapIndex() == 41)
		// {
			// ch->UnMount(true);
		// }
		// if (dwMountVnum > 0)
			// ch->MountVnum(dwMountVnum);
	// }
// #endif

#if defined(__MOUNT_COSTUME_SYSTEM__)
	LPITEM pCostumeMount = ch->GetWear(WEAR_COSTUME_MOUNT);
	if (pCostumeMount)
	{
		DWORD dwMountVnum = pCostumeMount->GetMountVnum();
		if (dwMountVnum > 0)
			ch->MountVnum(dwMountVnum);
	}
#endif

	// 플레이시간 레코딩 시작
	ch->ResetPlayTime();

	// 자동 저장 이벤트 추가
	ch->StartSaveEvent();
	ch->StartRecoveryEvent();
#ifdef ENABLE_CSHIELD
#ifdef ENABLE_CAPTCHA_AND_DEV_WHITELIST
	if (!IsAccountWhitelisted(d->GetAccountTable().login))
	{
			ch->StartCShieldEvent();
			ch->StartCShieldDataEvent();
	}
#else
	ch->StartCShieldEvent();
	ch->StartCShieldDataEvent();
#endif
#endif


	ch->StartCheckSpeedHackEvent();
#ifdef ENABLE_YMIR_AFFECT_FIX
    ch->SetQuestFlag("item.last_time", get_global_time());
#endif

	CPVPManager::instance().Connect(ch);
	CPVPManager::instance().SendList(d);

	MessengerManager::instance().Login(ch->GetName());

	CPartyManager::instance().SetParty(ch);
	CGuildManager::instance().SendGuildWar(ch);

	building::CManager::instance().SendLandList(d, ch->GetMapIndex());

	marriage::CManager::instance().Login(ch);
#if defined(ENABLE_PVP_TOURNAMENT)
	CPvPDuel::Instance().Login(ch);
#endif



#ifdef __EVENT_MANAGER__
	if (ch->GetMapIndex() == KINGDOM_WAR_MAP_INDEX && CEventManager::Instance().GetEventState(EVENT_TYPE_KINGDOM_WAR)) {
		CEventManager::Instance().SendKingdomWarScorePacket(ch);
	}
#endif

#ifdef ENABLE_ANTI_MULTIPLE_FARM //@fix core cross-fire
	std::string sMAIf = d->GetLoginMacAdress();
	int8_t i8GetOldState = static_cast<int8_t>(CAntiMultipleFarm::instance().GetPlayerDropState(sMAIf, ch->GetPlayerID()));
	CAntiMultipleFarm::instance().Login(sMAIf, ch->GetPlayerID(), i8GetOldState);
#endif
#ifdef __GROWTH_PET_SYSTEM__
	ch->SendDeadPetMessage();
#endif
	TPacketGCTime p;
	p.bHeader = HEADER_GC_TIME;
	p.time = get_global_time();
	d->Packet(&p, sizeof(p));

	TPacketGCChannel p2;
	p2.header = HEADER_GC_CHANNEL;
	p2.channel = g_bChannel;
	d->Packet(&p2, sizeof(p2));

	ch->SendGreetMessage();
#if defined(__MAILBOX__)
	CMailBox::UnreadData(ch);
#endif
#ifdef u1x
	ch->SendRanks();
#endif
#ifdef ENABLE_BIOLOG_SYSTEM	
	int pGetCompleted[] =
	{
		(ch->GetBiologState("biolog_lv92.completed")), (ch->GetBiologState("biolog_lv94.completed")), (ch->GetBiologState("biolog_lv92.used_window")), (ch->GetBiologState("biolog_lv94.used_window"))
	};
		
	if (pGetCompleted[1] != 1)
	{
		BiologManager::instance().SendUpdate_Binary(ch); }
	
	if (pGetCompleted[0] == 1 && pGetCompleted[2] < 1)
	{
		BiologManager::instance().SendWindow_SelectType(ch, 1); }
		
	if (pGetCompleted[1] == 1 && pGetCompleted[3] < 1)
	{
		BiologManager::instance().SendWindow_SelectType(ch, 2); }
#endif

	ch->ReviveInvisible(5);
#if defined(__CONQUEROR_LEVEL__)
	ch->SetSungMaWill();
#endif
	_send_bonus_info(ch);

	char szQuery2[128];
	snprintf(szQuery2, sizeof(szQuery2), "UPDATE player%s SET `last_play` = NOW() WHERE `name` = '%s'", get_table_postfix(), ch->GetName());
	std::unique_ptr<SQLMsg> msg(DBManager::instance().DirectQuery(szQuery2));
	
	// std::unique_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("UPDATE player%s SET `last_play` = NOW() WHERE `name` = '%s'",
		// get_table_postfix(), ch->GetName())
	// );

	for (int i = 0; i <= PREMIUM_MAX_NUM; ++i)
	{
		int remain = ch->GetPremiumRemainSeconds(i);

		if (remain <= 0)
			continue;

		ch->AddAffect(AFFECT_PREMIUM_START + i, POINT_NONE, 0, 0, remain, 0, true);
		sys_log(0, "PREMIUM: %s type %d %dmin", ch->GetName(), i, remain);
	}

	if (g_bCheckClientVersion)
	{
		sys_log(0, "VERSION CHECK %s %s", g_stClientVersion.c_str(), d->GetClientVersion());

		if (!d->GetClientVersion())
		{
			d->DelayedDisconnect(10);
		}
		else
		{
			if (0 != g_stClientVersion.compare(d->GetClientVersion()))
			{
				ch->ChatPacket(CHAT_TYPE_NOTICE, LC_TEXT("클라이언트 버전이 틀려 로그아웃 됩니다. 정상적으로 패치 후 접속하세요."));
				d->DelayedDisconnect(0);
				LogManager::instance().HackLog("VERSION_CONFLICT", ch);

				sys_log(0, "VERSION : WRONG VERSION USER : account:%s name:%s hostName:%s server_version:%s client_version:%s",
					d->GetAccountTable().login,
					ch->GetName(),
					d->GetHostName(),
					g_stClientVersion.c_str(),
					d->GetClientVersion());
			}
		}
	}
	else
	{
		sys_log(0, "VERSION : NO CHECK");
	}

	if (ch->IsGM() == true)
		ch->ChatPacket(CHAT_TYPE_COMMAND, "ConsoleEnable");

	if (ch->GetMapIndex() >= 10000)
	{
		if (CWarMapManager::instance().IsWarMap(ch->GetMapIndex()))
			ch->SetWarMap(CWarMapManager::instance().Find(ch->GetMapIndex()));
		else if (marriage::WeddingManager::instance().IsWeddingMap(ch->GetMapIndex()))
			ch->SetWeddingMap(marriage::WeddingManager::instance().Find(ch->GetMapIndex()));
#ifdef ENABLE_12ZI
		else if (ch->GetMapIndex() >= 3550000 && ch->GetMapIndex() < 3560000)
			ch->SetZodiac(CZodiacManager::instance().FindByMapIndex(ch->GetMapIndex()));
#endif
		else
		{
			ch->SetDungeon(CDungeonManager::instance().FindByMapIndex(ch->GetMapIndex()));
		}
	}
	else if (CArenaManager::instance().IsArenaMap(ch->GetMapIndex()) == true)
	{
		int memberFlag = CArenaManager::instance().IsMember(ch->GetMapIndex(), ch->GetPlayerID());
		if (memberFlag == MEMBER_OBSERVER)
		{
			ch->SetObserverMode(true);
			ch->SetArenaObserverMode(true);
			if (CArenaManager::instance().RegisterObserverPtr(ch, ch->GetMapIndex(), ch->GetX() / 100, ch->GetY() / 100))
			{
				sys_log(0, "ARENA : Observer add failed");
			}

			if (ch->IsHorseRiding() == true)
			{
				ch->StopRiding();
				ch->HorseSummon(false);
			}
		}
		else if (memberFlag == MEMBER_DUELIST)
		{
			TPacketGCDuelStart duelStart;
			duelStart.header = HEADER_GC_DUEL_START;
			duelStart.wSize = sizeof(TPacketGCDuelStart);

			ch->GetDesc()->Packet(&duelStart, sizeof(TPacketGCDuelStart));

			if (ch->IsHorseRiding() == true)
			{
				ch->StopRiding();
				ch->HorseSummon(false);
			}

			LPPARTY pParty = ch->GetParty();
			if (pParty != NULL)
			{
				if (pParty->GetMemberCount() == 2)
				{
					CPartyManager::instance().DeleteParty(pParty);
				}
				else
				{
					pParty->Quit(ch->GetPlayerID());
				}
			}
		}
		/*
		else if (memberFlag == MEMBER_NO)
		{
			if (ch->GetGMLevel() == GM_PLAYER)
				ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));
		}
		else
		{
			// wtf
		}
		*/
	}
	else if (ch->GetMapIndex() == 113)
	{
#ifdef ENABLE_FEATURES_OXEVENT
		COXEventManager::instance().InitializePacket(ch);
#endif
		// ox 이벤트 맵
		if (COXEventManager::instance().Enter(ch) == false)
		{
			// ox 맵 진입 허가가 나지 않음. 플레이어면 마을로 보내자
			if (ch->GetGMLevel() == GM_PLAYER)
				ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));
		}
	}
	else
	{
		if (CWarMapManager::instance().IsWarMap(ch->GetMapIndex()) ||
			marriage::WeddingManager::instance().IsWeddingMap(ch->GetMapIndex()))
		{
			if (!test_server)
				ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));
		}
	}

	if (ch->GetHorseLevel() > 0)
	{
		DWORD pid = ch->GetPlayerID();

		if (pid > 0)
		{
			const char* horseName = CHorseNameManager::instance().GetHorseName(pid);
			if (!horseName || !*horseName)
				db_clientdesc->DBPacket(HEADER_GD_REQ_HORSE_NAME, 0, &pid, sizeof(DWORD));
		}
	}
#ifdef ENABLE_12ZI
	ch->BeadTime();
	ch->MarkTime();
	if (ch && ch->GetMapIndex() == 355)
	{
		if (DISTANCE_APPROX(ch->GetX() - 333300, ch->GetY() - 1434400) > 7000)
			ch->WarpSet(333200, 1431000, 355);
	}

	if (ch->GetMapIndex() >= 3550000 && ch->GetMapIndex() < 3560000)
	{
		if (ch->GetQuestFlag("12zi_temple.IsDead") == 1)
			ch->Dead();
		LPZODIAC pZodiac = CZodiacManager::instance().FindByMapIndex(ch->GetMapIndex());
		if (pZodiac)
		{
			BYTE Floor = pZodiac->GetFloor();
			if (Floor != 0)
			{
				DWORD time = 0;
				if (Floor == 0)
				{
					ch->SetQuestFlag("12zi_temple.PrismNeed", 0);
					ch->SetQuestFlag("12zi_temple.IsDead", 0);
				}
				if (Floor == 7 || Floor == 14 || Floor == 21 || Floor == 28 || Floor == 35 || Floor == 36 || Floor == 37 || Floor == 38 || Floor == 39 || Floor == 40) //Bonus floor
					time = 5*60;
				else //Normal floor
					time = 10*60;

				ch->ZodiacFloorMessage(Floor);

				DWORD Time_1 = (get_global_time()+1) - pZodiac->GetTime();

				if (Floor == 40)
					ch->ChatPacket(CHAT_TYPE_COMMAND, "ZodiacTime %d %d %d %d", Floor, 0, time, Time_1);
				else
					ch->ChatPacket(CHAT_TYPE_COMMAND, "ZodiacTime %d %d %d %d", Floor, 1, time, Time_1);
			}
			else
			{
				ch->ChatPacket(CHAT_TYPE_COMMAND, "ZodiacTime %d %d %d %d", 0, 1, 10, 0);
			}
		}
		else
		{
			ch->WarpSet(333200, 1431000, 355);
		}
	}
#endif
	// 중립맵에 들어갔을때 안내하기
	if (g_noticeBattleZone)
	{
		if (FN_is_battle_zone(ch))
		{
			ch->ChatPacket(CHAT_TYPE_NOTICE, LC_TEXT("이 맵에선 강제적인 대전이 있을수 도 있습니다."));
			ch->ChatPacket(CHAT_TYPE_NOTICE, LC_TEXT("이 조항에 동의하지 않을시"));
			ch->ChatPacket(CHAT_TYPE_NOTICE, LC_TEXT("본인의 주성 및 부성으로 돌아가시기 바랍니다."));
		}
	}
	
#ifdef ENABLE_SWITCHBOT
	CSwitchbotManager::Instance().EnterGame(ch);
#endif
#if defined(__MINI_GAME_CATCH_KING__)
	CMiniGameManager::instance().MiniGameCatchKingEventInfo(ch);
#endif

#if defined(__MOUNT_COSTUME_SYSTEM__)
	if (IS_BLOCKED_MOUNT_SUMMON_MAP(ch->GetMapIndex()) || IS_BLOCKED_MOUNT_DUNGEON_MAP(ch->GetMapIndex()))
		ch->UnMount(true);
#endif

#if defined(ENABLE_CH_CHANGE_INGAME)
	ch->ChatPacket(CHAT_TYPE_COMMAND, "ChannelInfo %d %d", g_bChannel, ch->GetMapIndex());
#endif
#ifdef __ENABLE_NEW_OFFLINESHOP__
	if (ch->IsPC())
	{
		offlineshop::CShop* pkShop= offlineshop::GetManager().GetShopByOwnerID(ch->GetPlayerID());
		if(pkShop)
			ch->SetOfflineShop(pkShop);

		offlineshop::CAuction* auction = offlineshop::GetManager().GetAuctionByOwnerID(ch->GetPlayerID());
		if(auction)
			ch->SetAuction(auction);
	}
#endif
}

void CInputLogin::Empire(LPDESC d, const char* c_pData)
{
	const TPacketCGEmpire* p = reinterpret_cast<const TPacketCGEmpire*>(c_pData);

	if (EMPIRE_MAX_NUM <= p->bEmpire)
	{
		d->SetPhase(PHASE_CLOSE);
		return;
	}

	const TAccountTable& r = d->GetAccountTable();

	if (r.bEmpire != 0)
	{
		for (int i = 0; i < PLAYER_PER_ACCOUNT; ++i)
		{
			if (0 != r.players[i].dwID)
			{
				sys_err("EmpireSelectFailed %d", r.players[i].dwID);
				return;
			}
		}
	}

	TEmpireSelectPacket pd;

	pd.dwAccountID = r.id;
	pd.bEmpire = p->bEmpire;

	db_clientdesc->DBPacket(HEADER_GD_EMPIRE_SELECT, d->GetHandle(), &pd, sizeof(pd));
}

int CInputLogin::GuildSymbolUpload(LPDESC d, const char* c_pData, size_t uiBytes)
{
	if (uiBytes < sizeof(TPacketCGGuildSymbolUpload))
		return -1;

	sys_log(0, "GuildSymbolUpload uiBytes %u", uiBytes);

	TPacketCGGuildSymbolUpload* p = (TPacketCGGuildSymbolUpload*)c_pData;

	if (uiBytes < p->size)
		return -1;

	int iSymbolSize = p->size - sizeof(TPacketCGGuildSymbolUpload);

	if (iSymbolSize <= 0 || iSymbolSize > 64 * 1024)
	{
		// 64k 보다 큰 길드 심볼은 올릴수없다
		// 접속을 끊고 무시
		d->SetPhase(PHASE_CLOSE);
		return 0;
	}

	// 땅을 소유하지 않은 길드인 경우.
	if (!test_server)
		if (!building::CManager::instance().FindLandByGuild(p->guild_id))
		{
			d->SetPhase(PHASE_CLOSE);
			return 0;
		}

	sys_log(0, "GuildSymbolUpload Do Upload %02X%02X%02X%02X %d", c_pData[7], c_pData[8], c_pData[9], c_pData[10], sizeof(*p));

	CGuildMarkManager::instance().UploadSymbol(p->guild_id, iSymbolSize, (const BYTE*)(c_pData + sizeof(*p)));
	CGuildMarkManager::instance().SaveSymbol(GUILD_SYMBOL_FILENAME);
	return iSymbolSize;
}

void CInputLogin::GuildSymbolCRC(LPDESC d, const char* c_pData)
{
	const TPacketCGSymbolCRC& CGPacket = *((TPacketCGSymbolCRC*)c_pData);

	sys_log(0, "GuildSymbolCRC %u %u %u", CGPacket.guild_id, CGPacket.crc, CGPacket.size);

	const CGuildMarkManager::TGuildSymbol* pkGS = CGuildMarkManager::instance().GetGuildSymbol(CGPacket.guild_id);

	if (!pkGS) {
#if defined(ENABLE_COMMON_CHANGES)
		if (CGPacket.lastRequest) {
			TPacketGCGuildSymbolData GCPacket;
			GCPacket.header = HEADER_GC_SYMBOL_DATA;
			GCPacket.size = 0;
			GCPacket.guild_id = CGPacket.guild_id;
			d->Packet(&GCPacket, sizeof(GCPacket));
		}
#endif
		return;
	}

	sys_log(0, "  Server %u %u", pkGS->crc, pkGS->raw.size());

	if (pkGS->raw.size() != CGPacket.size || pkGS->crc != CGPacket.crc)
	{
		TPacketGCGuildSymbolData GCPacket;

		GCPacket.header = HEADER_GC_SYMBOL_DATA;
		GCPacket.size = sizeof(GCPacket) + pkGS->raw.size();
		GCPacket.guild_id = CGPacket.guild_id;

		d->BufferedPacket(&GCPacket, sizeof(GCPacket));
		d->Packet(&pkGS->raw[0], pkGS->raw.size());

		sys_log(0, "SendGuildSymbolHead %02X%02X%02X%02X Size %d",
			pkGS->raw[0], pkGS->raw[1], pkGS->raw[2], pkGS->raw[3], pkGS->raw.size());
	}
}

void CInputLogin::GuildMarkUpload(LPDESC d, const char* c_pData)
{
	TPacketCGMarkUpload* p = (TPacketCGMarkUpload*)c_pData;
	CGuildManager& rkGuildMgr = CGuildManager::instance();
	CGuild* pkGuild;

	if (!(pkGuild = rkGuildMgr.FindGuild(p->gid)))
	{
		sys_err("MARK_SERVER: GuildMarkUpload: no guild. gid %u", p->gid);
		return;
	}

	if (pkGuild->GetLevel() < guild_mark_min_level)
	{
		sys_log(0, "MARK_SERVER: GuildMarkUpload: level < %u (%u)", guild_mark_min_level, pkGuild->GetLevel());
		return;
	}

	CGuildMarkManager& rkMarkMgr = CGuildMarkManager::instance();

	sys_log(0, "MARK_SERVER: GuildMarkUpload: gid %u", p->gid);

	bool isEmpty = true;

	for (DWORD iPixel = 0; iPixel < SGuildMark::SIZE; ++iPixel)
		if (*((DWORD*)p->image + iPixel) != 0x00000000)
			isEmpty = false;

	if (isEmpty)
		rkMarkMgr.DeleteMark(p->gid);
	else
		rkMarkMgr.SaveMark(p->gid, p->image);
}

void CInputLogin::GuildMarkIDXList(LPDESC d, const char* c_pData)
{
	CGuildMarkManager& rkMarkMgr = CGuildMarkManager::instance();

	DWORD bufSize = sizeof(WORD) * 2 * rkMarkMgr.GetMarkCount();
	char* buf = NULL;

	if (bufSize > 0)
	{
		buf = (char*)malloc(bufSize);
		rkMarkMgr.CopyMarkIdx(buf);
	}

	TPacketGCMarkIDXList p;
	p.header = HEADER_GC_MARK_IDXLIST;
	p.bufSize = sizeof(p) + bufSize;
	p.count = rkMarkMgr.GetMarkCount();

	if (buf)
	{
		d->BufferedPacket(&p, sizeof(p));
		d->LargePacket(buf, bufSize);
		free(buf);
	}
	else
		d->Packet(&p, sizeof(p));

	sys_log(0, "MARK_SERVER: GuildMarkIDXList %d bytes sent.", p.bufSize);
}

void CInputLogin::GuildMarkCRCList(LPDESC d, const char* c_pData)
{
	TPacketCGMarkCRCList* pCG = (TPacketCGMarkCRCList*)c_pData;

	std::map<BYTE, const SGuildMarkBlock*> mapDiffBlocks;
	CGuildMarkManager::instance().GetDiffBlocks(pCG->imgIdx, pCG->crclist, mapDiffBlocks);

	DWORD blockCount = 0;
	TEMP_BUFFER buf(1024 * 1024); // 1M 버퍼

	for (itertype(mapDiffBlocks) it = mapDiffBlocks.begin(); it != mapDiffBlocks.end(); ++it)
	{
		BYTE posBlock = it->first;
		const SGuildMarkBlock& rkBlock = *it->second;

		buf.write(&posBlock, sizeof(BYTE));
		buf.write(&rkBlock.m_sizeCompBuf, sizeof(DWORD));
		buf.write(rkBlock.m_abCompBuf, rkBlock.m_sizeCompBuf);

		++blockCount;
	}

	TPacketGCMarkBlock pGC;

	pGC.header = HEADER_GC_MARK_BLOCK;
	pGC.imgIdx = pCG->imgIdx;
	pGC.bufSize = buf.size() + sizeof(TPacketGCMarkBlock);
	pGC.count = blockCount;

	sys_log(0, "MARK_SERVER: Sending blocks. (imgIdx %u diff %u size %u)", pCG->imgIdx, mapDiffBlocks.size(), pGC.bufSize);

	if (buf.size() > 0)
	{
		d->BufferedPacket(&pGC, sizeof(TPacketGCMarkBlock));
		d->LargePacket(buf.read_peek(), buf.size());
	}
	else
		d->Packet(&pGC, sizeof(TPacketGCMarkBlock));
}

int CInputLogin::Analyze(LPDESC d, BYTE bHeader, const char* c_pData)
{
	int iExtraLen = 0;

	switch (bHeader)
	{
	case HEADER_CG_PONG:
		Pong(d);
		break;

	case HEADER_CG_TIME_SYNC:
		Handshake(d, c_pData);
		break;

	case HEADER_CG_LOGIN:
		Login(d, c_pData);
		break;

	case HEADER_CG_LOGIN2:
		LoginByKey(d, c_pData);
		break;

	case HEADER_CG_CHARACTER_SELECT:
		CharacterSelect(d, c_pData);
		break;

	case HEADER_CG_CHARACTER_CREATE:
		CharacterCreate(d, c_pData);
		break;

	case HEADER_CG_CHARACTER_DELETE:
		CharacterDelete(d, c_pData);
		break;

	case HEADER_CG_ENTERGAME:
		Entergame(d, c_pData);
		break;

	case HEADER_CG_EMPIRE:
		Empire(d, c_pData);
		break;

	case HEADER_CG_MOVE:
	case HEADER_CG_ITEM_USE:
	case HEADER_CG_TARGET:
// #if defined(__DISABLE_SEND_SEQUENCE__)
	// case HEADER_CG_MARK_LOGIN:
// #endif
		break;

		///////////////////////////////////////
		// Guild Mark
		/////////////////////////////////////
	case HEADER_CG_MARK_CRCLIST:
		GuildMarkCRCList(d, c_pData);
		break;

	case HEADER_CG_MARK_IDXLIST:
		GuildMarkIDXList(d, c_pData);
		break;

	case HEADER_CG_MARK_UPLOAD:
		GuildMarkUpload(d, c_pData);
		break;

		//////////////////////////////////////
		// Guild Symbol
		/////////////////////////////////////
	case HEADER_CG_GUILD_SYMBOL_UPLOAD:
		if ((iExtraLen = GuildSymbolUpload(d, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;

	case HEADER_CG_SYMBOL_CRC:
		GuildSymbolCRC(d, c_pData);
		break;
		/////////////////////////////////////

#ifdef FIX_HEADER_CG_MARK_LOGIN
	case HEADER_CG_MARK_LOGIN:
	break;
#endif

	case HEADER_CG_HACK:
		break;

	case HEADER_CG_CHANGE_NAME:
		ChangeName(d, c_pData);
		break;

	case HEADER_CG_CLIENT_VERSION:
		Version(d->GetCharacter(), c_pData);
		break;

	case HEADER_CG_CLIENT_VERSION2:
		Version(d->GetCharacter(), c_pData);
		break;

	case HEADER_CG_PLAYER_PIN_CODE:
		CharacterPinCode(d, c_pData);
		break;
		
#ifdef __GROWTH_PET_SYSTEM__
	case HEADER_CG_PET_WINDOW_TYPE:
		break;
#endif
#if defined(ENABLE_ANTI_FLOOD)
	case HEADER_CG_CHAT:
	case HEADER_CG_SCRIPT_BUTTON:
#if defined(__CHANGE_LOOK_SYSTEM__)
	case HEADER_CG_CHANGE_LOOK:
#endif
#ifdef ENABLE_CSHIELD
		case HEADER_CG_CSHIELD:
			break;
#endif
	{
		break;
	}
#endif
	default: {
		sys_err("login phase does not handle this packet! header %d", bHeader);
#if defined(ENABLE_ANTI_FLOOD)
		return -1;
#else
		//d->SetPhase(PHASE_CLOSE);
		return (0);
#endif
	}
	}

	return (iExtraLen);
}
