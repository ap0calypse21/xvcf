#include "stdafx.h"
#include "constants.h"
#include "config.h"
#include "log.h"

#include "char.h"
#include "desc.h"
#include "item.h"
#include "locale_service.h"
#include "p2p.h"
#define LOG_ACTION_MAX_LEN 50  // Adjust the value as needed

#if defined(ENABLE_TRADE_LOG)
#include "db.h"
#endif

static char __escape_hint[1024];

LogManager::LogManager() : m_bIsConnect(false)
{
}

LogManager::~LogManager()
{
}

bool LogManager::Connect(const char* host, const int port, const char* user, const char* pwd, const char* db)
{
	if (m_sql.Setup(host, user, pwd, db, g_stLocale.c_str(), false, port))
		m_bIsConnect = true;

	return m_bIsConnect;
}

void LogManager::Query(const char* c_pszFormat, ...)
{
	char szQuery[4096];
	va_list args;

	va_start(args, c_pszFormat);
	vsnprintf(szQuery, sizeof(szQuery), c_pszFormat, args);
	va_end(args);

	if (test_server)
		sys_log(0, "LOG: %s", szQuery);

#if defined(ENABLE_COMMON_CHANGES)
	std::string sQuery(szQuery);
	m_sql.AsyncQuery(sQuery.substr(0, sQuery.find_first_of(";") == std::string::npos ? sQuery.length(): sQuery.find_first_of(";")).c_str());
#else
	m_sql.AsyncQuery(szQuery);
#endif
}

bool LogManager::IsConnected()
{
	return m_bIsConnect;
}

#if defined(__MAILBOX__)
void LogManager::MailLog(const char* const szName, const char* const szWho, const char* const szTitle, const char* const szMessage, const bool bIsGM, const DWORD dwItemVnum, const DWORD dwItemCount, const int iYang, const int iWon)
{
	Query("INSERT DELAYED INTO mailbox_log%s (name, who, title, message, gm, gold, won, ivnum, icount, date) "
		"VALUES('%s', '%s', '%s', '%s', %d, %d, %d, %lu, %lu, NOW()) ",
		get_table_postfix(), szName, szWho, szTitle, szMessage, bIsGM, iYang, iWon, dwItemVnum, dwItemCount);
}
#endif 

void LogManager::ItemLog(DWORD dwPID, DWORD x, DWORD y, DWORD dwItemID, const char* c_pszText, const char* c_pszHint, const char* c_pszIP, DWORD dwVnum)
{
    m_sql.EscapeString(__escape_hint, sizeof(__escape_hint), c_pszHint, strlen(c_pszHint));

    Query("INSERT DELAYED INTO log_item%s (type, time, who, x, y, item_id, item_text, hint, ip, vnum) VALUES('ITEM', DATE_FORMAT(NOW(), '%%Y-%%m-%%d %%H:%%i:%%s'), %u, %u, %u, %u, '%s', '%s', '%s', %u)",
        get_table_postfix(), dwPID, x, y, dwItemID, c_pszText, __escape_hint, c_pszIP, dwVnum);
}

void LogManager::ItemDestroyLog(DWORD dwPID, DWORD dwItemID, const char* c_pszItemName, const char* c_pszAction, const char* c_pszIP, DWORD dwVnum)
{
    char escape_item_name[ITEM_NAME_MAX_LEN];  // Define ITEM_NAME_MAX_LEN as the maximum length of item names.
    m_sql.EscapeString(escape_item_name, sizeof(escape_item_name), c_pszItemName, strlen(c_pszItemName));

    char escape_action[LOG_ACTION_MAX_LEN];  // Define LOG_ACTION_MAX_LEN as the maximum length of action descriptions.
    m_sql.EscapeString(escape_action, sizeof(escape_action), c_pszAction, strlen(c_pszAction));

    // Construct and execute an SQL query to insert the item destroy log data into the table
    Query("INSERT DELAYED INTO log_item%s (type, time, who, item_id, item_name, action, ip, vnum) VALUES('DESTROY', DATE_FORMAT(NOW(), '%%Y-%%m-%%d %%H:%%i:%%s'), %u, %u, '%s', '%s', '%s', %u)",
        get_table_postfix(), dwPID, dwItemID, escape_item_name, escape_action, c_pszIP, dwVnum);
}

#if defined(__MINI_GAME_OKEY__)
void LogManager::OkeyEventLog(int dwPID, const char* c_pszText, int points)
{
	Query("INSERT INTO okey_event%s (pid, name, points) VALUES(%d, '%s', %d)", get_table_postfix(), dwPID, c_pszText, points);
}
#endif

void LogManager::ItemLog(LPCHARACTER ch, LPITEM item, const char* c_pszText, const char* c_pszHint)
{
	if (NULL == ch || NULL == item)
	{
		sys_err("character or item nil (ch %p item %p text %s)", get_pointer(ch), get_pointer(item), c_pszText);
		return;
	}

	ItemLog(ch->GetPlayerID(), ch->GetX(), ch->GetY(), item->GetID(),
		NULL == c_pszText ? "" : c_pszText,
		c_pszHint, ch->GetDesc() ? ch->GetDesc()->GetHostName() : "",
		item->GetOriginalVnum());
}

void LogManager::ItemLog(LPCHARACTER ch, int itemID, int itemVnum, const char* c_pszText, const char* c_pszHint)
{
	ItemLog(ch->GetPlayerID(), ch->GetX(), ch->GetY(), itemID, c_pszText, c_pszHint, ch->GetDesc() ? ch->GetDesc()->GetHostName() : "", itemVnum);
}

void LogManager::CharLog(DWORD dwPID, DWORD x, DWORD y, DWORD dwValue, const char* c_pszText, const char* c_pszHint, const char* c_pszIP)
{
	m_sql.EscapeString(__escape_hint, sizeof(__escape_hint), c_pszHint, strlen(c_pszHint));

    Query("INSERT DELAYED INTO log_item%s (type, time, who, x, y, item_id, item_text, hint, ip, vnum) VALUES('ITEM', DATE_FORMAT(NOW(), '%%Y-%%m-%%d %%H:%%i:%%s'), %u, %u, %u, %u, '%s', '%s', '%s', %u)",
		get_table_postfix(), dwPID, x, y, dwValue, c_pszText, __escape_hint, c_pszIP);
}

void LogManager::CharLog(LPCHARACTER ch, DWORD dw, const char* c_pszText, const char* c_pszHint)
{
	if (ch)
		CharLog(ch->GetPlayerID(), ch->GetX(), ch->GetY(), dw, c_pszText, c_pszHint, ch->GetDesc() ? ch->GetDesc()->GetHostName() : "");
	else
		CharLog(0, 0, 0, dw, c_pszText, c_pszHint, "");
}

void LogManager::LoginLog(bool isLogin, DWORD dwAccountID, DWORD dwPID, BYTE bLevel, BYTE bJob, DWORD dwPlayTime)
{
	Query("INSERT DELAYED INTO loginlog%s (type, time, channel, account_id, pid, level, job, playtime) VALUES (%s, NOW(), %d, %u, %u, %d, %d, %u)",
		get_table_postfix(), isLogin ? "'LOGIN'" : "'LOGOUT'", g_bChannel, dwAccountID, dwPID, bLevel, bJob, dwPlayTime);
}

void LogManager::MoneyLog(BYTE type, DWORD vnum, int gold)
{
	if (type == MONEY_LOG_RESERVED || type >= MONEY_LOG_TYPE_MAX_NUM)
	{
		sys_err("TYPE ERROR: type %d vnum %u gold %d", type, vnum, gold);
		return;
	}

	Query("INSERT DELAYED INTO money_log%s VALUES (NOW(), %d, %d, %d)", get_table_postfix(), type, vnum, gold);
}

void LogManager::HackLog(const char* c_pszHackName, const char* c_pszLogin, const char* c_pszName, const char* c_pszIP)
{
	m_sql.EscapeString(__escape_hint, sizeof(__escape_hint), c_pszHackName, strlen(c_pszHackName));

	Query("INSERT INTO hack_log (time, login, name, ip, server, why) VALUES(NOW(), '%s', '%s', '%s', '%s', '%s')", c_pszLogin, c_pszName, c_pszIP, g_stHostname.c_str(), __escape_hint);
}

void LogManager::HackLog(const char* c_pszHackName, LPCHARACTER ch)
{
	if (ch->GetDesc())
	{
		HackLog(c_pszHackName,
			ch->GetDesc()->GetAccountTable().login,
			ch->GetName(),
			ch->GetDesc()->GetHostName());
	}
}


#ifdef ENABLE_CSHIELD
void LogManager::CShieldLog(LPCHARACTER ch, const DWORD errorcode, const bool kickPlayer)
{
	if (!ch)
		return;

	LPDESC d = ch->GetDesc();

	if (!d)
		return;

	if (ch->GetCShield()->isErrorcodeInList(errorcode))
		return;

	Query("INSERT INTO cshield_log%s (time, accountid, name, level, mapIndex, x, y, ip, errorcode, why) VALUES(NOW(), %u, '%s', %d, %ld, %ld, %ld, '%s', %u, '%s')", get_table_postfix(), d->GetAccountTable().id, ch->GetName(), ch->GetLevel(), ch->GetMapIndex(), ch->GetX(), ch->GetY(), d->GetHostName(), errorcode, MapErrorCode(errorcode).c_str());
	ch->GetCShield()->addErrorcodeToList(errorcode);


	if (errorcode == 550) {
		TPacketGGGMNotification p;
		p.bHeader = HEADER_GG_GM_NOTIFICATION;
		p.dwErrorCode = errorcode;
		snprintf(p.szMessage, sizeof(p.szMessage), "CShield Hack  detected for : %s", ch->GetName());
		
		P2P_MANAGER::instance().Send(&p, sizeof(p));
	}
	
	if (errorcode == 10007) {
		TPacketGGGMNotification p;
		p.bHeader = HEADER_GG_GM_NOTIFICATION;
		p.dwErrorCode = errorcode;
		snprintf(p.szMessage, sizeof(p.szMessage), "CShield captcha not solve  for : %s", ch->GetName());
		
		P2P_MANAGER::instance().Send(&p, sizeof(p));
	}	


	if (kickPlayer)
		d->SetPhase(PHASE_CLOSE);
}
#endif


void LogManager::HackCRCLog(const char* c_pszHackName, const char* c_pszLogin, const char* c_pszName, const char* c_pszIP, DWORD dwCRC)
{
	Query("INSERT INTO hack_crc_log (time, login, name, ip, server, why, crc) VALUES(NOW(), '%s', '%s', '%s', '%s', '%s', %u)", c_pszLogin, c_pszName, c_pszIP, g_stHostname.c_str(), c_pszHackName, dwCRC);
}

void LogManager::PCBangLoginLog(DWORD dwPCBangID, const char* c_szPCBangIP, DWORD dwPlayerID, DWORD dwPlayTime)
{
	Query("INSERT INTO pcbang_loginlog (time, pcbang_id, ip, pid, play_time) VALUES (NOW(), %u, '%s', %u, %u)",
		dwPCBangID, c_szPCBangIP, dwPlayerID, dwPlayTime);
}

void LogManager::GoldBarLog(DWORD dwPID, DWORD dwItemID, GOLDBAR_HOW eHow, const char* c_pszHint)
{
	char szHow[32 + 1];

	switch (eHow)
	{
	case PERSONAL_SHOP_BUY:
		snprintf(szHow, sizeof(szHow), "'BUY'");
		break;

	case PERSONAL_SHOP_SELL:
		snprintf(szHow, sizeof(szHow), "'SELL'");
		break;

	case SHOP_BUY:
		snprintf(szHow, sizeof(szHow), "'SHOP_BUY'");
		break;

	case SHOP_SELL:
		snprintf(szHow, sizeof(szHow), "'SHOP_SELL'");
		break;

	case EXCHANGE_TAKE:
		snprintf(szHow, sizeof(szHow), "'EXCHANGE_TAKE'");
		break;

	case EXCHANGE_GIVE:
		snprintf(szHow, sizeof(szHow), "'EXCHANGE_GIVE'");
		break;

	case QUEST:
		snprintf(szHow, sizeof(szHow), "'QUEST'");
		break;

	default:
		snprintf(szHow, sizeof(szHow), "''");
		break;
	}

	Query("INSERT DELAYED INTO goldlog%s (date, time, pid, what, how, hint) VALUES(CURDATE(), CURTIME(), %u, %u, %s, '%s')",
		get_table_postfix(), dwPID, dwItemID, szHow, c_pszHint);
}

void LogManager::CubeLog(DWORD dwPID, DWORD x, DWORD y, DWORD item_vnum, DWORD item_uid, int item_count, bool success)
{
	Query("INSERT DELAYED INTO cube%s (pid, time, x, y, item_vnum, item_uid, item_count, success) "
		"VALUES(%u, NOW(), %u, %u, %u, %u, %d, %d)",
		get_table_postfix(), dwPID, x, y, item_vnum, item_uid, item_count, success ? 1 : 0);
}

#if defined(__ACCE_COSTUME_SYSTEM__)
void LogManager::AcceLog(DWORD dwPID, DWORD x, DWORD y, DWORD item_vnum, DWORD item_uid, int item_count, int abs_chance, bool success)
{
	Query("INSERT DELAYED INTO acce%s (pid, time, x, y, item_vnum, item_uid, item_count, item_abs_chance, success) VALUES(%u, NOW(), %u, %u, %u, %u, %d, %d, %d)", get_table_postfix(), dwPID, x, y, item_vnum, item_uid, item_count, abs_chance, success ? 1 : 0);
}
#endif

void LogManager::WhisperLog(DWORD fromPID, DWORD toPID, const char* message)
{
	m_sql.EscapeString(__escape_hint, sizeof(__escape_hint), message, strlen(message));

	if (strcmp(__escape_hint, message) == 0) {
		std::string log;

		try {
			log = fmt::sprintf("INSERT DELAYED INTO `whisper_log%s` "
							   "(`time`, `from`, `to`, `message`)"
							   "VALUES(NOW(), %u, %u, '%s')", get_table_postfix(),
															  fromPID,
															  toPID,
															  __escape_hint
			);
		} catch (const std::exception &e) {
			sys_err("%s", e.what());
		}

		Query(log.c_str());
	}
}

void LogManager::SpeedHackLog(DWORD pid, DWORD x, DWORD y, int hack_count)
{
	Query("INSERT INTO speed_hack%s (pid, time, x, y, hack_count) "
		"VALUES(%u, NOW(), %u, %u, %d)",
		get_table_postfix(), pid, x, y, hack_count);
}

void LogManager::ChangeNameLog(DWORD pid, const char* old_name, const char* new_name, const char* ip)
{
	Query("INSERT DELAYED INTO change_name%s (pid, old_name, new_name, time, ip) "
		"VALUES(%u, '%s', '%s', NOW(), '%s') ",
		get_table_postfix(), pid, old_name, new_name, ip);
}

void LogManager::GMCommandLog(DWORD dwPID, const char* szName, const char* szIP, BYTE byChannel, const char* szCommand)
{
	m_sql.EscapeString(__escape_hint, sizeof(__escape_hint), szCommand, strlen(szCommand));

	Query("INSERT DELAYED INTO command_log%s (userid, server, ip, port, username, command, date ) "
		"VALUES(%u, 999, '%s', %u, '%s', '%s', NOW()) ",
		get_table_postfix(), dwPID, szIP, byChannel, szName, __escape_hint);
}

void LogManager::RefineLog(DWORD pid, const char* item_name, DWORD item_id, int item_refine_level, int is_success, const char* how)
{
	m_sql.EscapeString(__escape_hint, sizeof(__escape_hint), item_name, strlen(item_name));

	Query("INSERT INTO refinelog%s (pid, item_name, item_id, step, time, is_success, setType) VALUES(%u, '%s', %u, %d, NOW(), %d, '%s')",
		get_table_postfix(), pid, __escape_hint, item_id, item_refine_level, is_success, how);
}

void LogManager::ShoutLog(BYTE bChannel, BYTE bEmpire, const char* pszText) {
	m_sql.EscapeString(__escape_hint, sizeof(__escape_hint), pszText, strlen(pszText));

	if (strcmp(__escape_hint, pszText) == 0) {
		std::string log;

		try {
			log = fmt::sprintf("INSERT DELAYED INTO `shout_log%s` "
							   "VALUES(NOW(), %d, %d, '%s')", get_table_postfix(),
															  bChannel,
															  bEmpire,
															  __escape_hint
			);
		} catch (const std::exception &e) {
			sys_err("%s", e.what());
		}

		Query(log.c_str());
	}
}

void LogManager::LevelLog(LPCHARACTER pChar, unsigned int level, unsigned int playhour)
{
	if (true == LC_IsEurope())
	{
		DWORD aid = 0;

		if (NULL != pChar->GetDesc())
		{
			aid = pChar->GetDesc()->GetAccountTable().id;
		}

		Query("REPLACE INTO levellog%s (name, level, time, account_id, pid, playtime) VALUES('%s', %u, NOW(), %u, %u, %d)",
			get_table_postfix(), pChar->GetName(), level, aid, pChar->GetPlayerID(), playhour);
	}
	else
	{
		Query("REPLACE INTO levellog%s (name, level, time, playtime) VALUES('%s', %u, NOW(), %d)",
			get_table_postfix(), pChar->GetName(), level, playhour);
	}
}

void LogManager::BootLog(const char* c_pszHostName, BYTE bChannel)
{
	Query("INSERT INTO bootlog (time, hostname, channel) VALUES(NOW(), '%s', %d)",
		c_pszHostName, bChannel);
}

void LogManager::FishLog(DWORD dwPID, int prob_idx, int fish_id, int fish_level, DWORD dwMiliseconds, DWORD dwVnum, DWORD dwValue)
{
	Query("INSERT INTO fish_log%s VALUES(NOW(), %u, %d, %u, %d, %u, %u, %u)",
		get_table_postfix(),
		dwPID,
		prob_idx,
		fish_id,
		fish_level,
		dwMiliseconds,
		dwVnum,
		dwValue);
}

void LogManager::QuestRewardLog(const char* c_pszQuestName, DWORD dwPID, DWORD dwLevel, int iValue1, int iValue2)
{
	Query("INSERT INTO quest_reward_log%s VALUES('%s',%u,%u,2,%u,%u,NOW())",
		get_table_postfix(),
		c_pszQuestName,
		dwPID,
		dwLevel,
		iValue1,
		iValue2);
}

void LogManager::DetailLoginLog(bool isLogin, LPCHARACTER ch)
{
	if (NULL == ch->GetDesc())
		return;

	if (true == isLogin)
	{
		Query("INSERT INTO loginlog2(type, is_gm, login_time, channel, account_id, pid, ip, client_version) "
			"VALUES('INVALID', %s, NOW(), %d, %u, %u, inet_aton('%s'), '%s')",
			ch->IsGM() == true ? "'Y'" : "'N'",
			g_bChannel,
			ch->GetDesc()->GetAccountTable().id,
			ch->GetPlayerID(),
			ch->GetDesc()->GetHostName(),
			ch->GetDesc()->GetClientVersion());
	}
	else
	{
		Query("SET @i = (SELECT MAX(id) FROM loginlog2 WHERE account_id=%u AND pid=%u)",
			ch->GetDesc()->GetAccountTable().id,
			ch->GetPlayerID());

		Query("UPDATE loginlog2 SET type='VALID', logout_time=NOW(), playtime=TIMEDIFF(logout_time,login_time) WHERE id=@i");
	}
}

void LogManager::DragonSlayLog(DWORD dwGuildID, DWORD dwDragonVnum, DWORD dwStartTime, DWORD dwEndTime)
{
	Query("INSERT INTO dragon_slay_log%s VALUES( %d, %d, FROM_UNIXTIME(%d), FROM_UNIXTIME(%d) )",
		get_table_postfix(),
		dwGuildID, dwDragonVnum, dwStartTime, dwEndTime);
}

#if defined(ENABLE_TRADE_LOG)
void LogManager::ExchangeItemLog(uint32_t id, const LPITEM item, uint32_t fromID, std::string fromName, uint32_t toID, std::string toName) {
	if (id == 0) {
		sys_err("Lost trade due to id error that is 0.");
		return;
	} else if (!item) {
		sys_err("Lost trade due to null item pointer.");
		return;
	}

	std::string log;

	try {
		log = fmt::sprintf("INSERT INTO `log`.`exchange_items` (`id`, `fromid`, `fromname`, `toid`, `toname`, "
						   "`item_id`, `item_vnum`, `item_count`, "
						   "`socket0`, `socket1`, `socket2`, "
						   "`attrtype0`, `attrvalue0`, "
						   "`attrtype1`, `attrvalue1`, "
						   "`attrtype2`, `attrvalue2`, "
						   "`attrtype3`, `attrvalue3`, "
						   "`attrtype4`, `attrvalue4`, "
						   "`attrtype5`, `attrvalue5`, "
						   "`attrtype6`, `attrvalue6`, "
						   "`date`) "
						   "VALUES (%u, %u, '%s', %u, '%s', "
						   "%u, %u, %d, "
						   "%u, %u, %u, "
						   "%d, %d, "
						   "%d, %d, "
						   "%d, %d, "
						   "%d, %d, "
						   "%d, %d, "
						   "%d, %d, "
						   "%d, %d, "
						   "NOW())",
						   id, fromID, fromName.c_str(), toID, toName.c_str(),
						   item->GetID(), item->GetVnum(), item->GetCount(),
						   item->GetSocket(0), item->GetSocket(1), item->GetSocket(2),
						   item->GetAttributeType(0), item->GetAttributeValue(0),
						   item->GetAttributeType(1), item->GetAttributeValue(1),
						   item->GetAttributeType(2), item->GetAttributeValue(2),
						   item->GetAttributeType(3), item->GetAttributeValue(3),
						   item->GetAttributeType(4), item->GetAttributeValue(4),
						   item->GetAttributeType(5), item->GetAttributeValue(5),
						   item->GetAttributeType(6), item->GetAttributeValue(6)
						);
	} catch (const std::exception &e) {
		sys_err("%s", e.what());
	}

	if (!log.empty()) {
		Query(log.c_str());
	}
}
#ifdef __ENABLE_NEW_OFFLINESHOP__

void LogManager::LogSafeboxItem(const char* action, DWORD ownerID, DWORD itemID, int itemCount) {
    // Construct and execute the SQL query
    Query("INSERT INTO log_safebox (action, owner_id, item_id, item_count) VALUES('%s', %u, %u, %d)", 
          action, ownerID, itemID, itemCount);
}
void LogManager::LogOfflineShopTransaction(const char* action, DWORD itemID, DWORD ownerID) {
    // Construct and execute the SQL query
    Query("INSERT INTO log_offlineshop (action, item_id, owner_id) VALUES('%s', %u, %u)", 
          action, itemID, ownerID);
}
#endif

uint32_t LogManager::ExchangeLog(uint32_t pid1, std::string name1, uint32_t pid2, std::string name2, uint32_t x, uint32_t y, int64_t gold1, int64_t gold2
#if defined(ENABLE_CHEQUE_SYSTEM)
, int64_t cheque1, int64_t cheque2
#endif
) {
	std::string log;

	try {
		log = fmt::sprintf("INSERT INTO `log`.`exchanges` (`playerA`, `playerNameA`, `playerB`, `playerNameB`, "
						   "`goldA`, `goldB`, "
#if defined(ENABLE_CHEQUE_SYSTEM)
						   "`chequeA`, `chequeB`, "
#endif
						   "`x`, `y`, "
						   "`date`) "
						   "VALUES (%u, '%s', %u, '%s', "
						   "%lld, %lld, "
#if defined(ENABLE_CHEQUE_SYSTEM)
						   "%lld, %lld, "
#endif
						   "%u, %u, "
						   "NOW())",
						   pid1, name1.c_str(), pid2, name2.c_str(),
						   gold1, gold2,
#if defined(ENABLE_CHEQUE_SYSTEM)
						   cheque1, cheque2,
#endif
						   x, y
						);
	} catch (const std::exception &e) {
		sys_err("%s", e.what());
	}

	if (log.empty()) {
		return 0;
	}

	std::unique_ptr<SQLMsg> msg(DBManager::instance().DirectQuery(log.c_str()));

	if (!msg || msg->Get()->uiAffectedRows == 0) {
		sys_err("Issue logging trade. Query: %s", log.c_str());
		return 0;
	}

	return (uint32_t)msg->Get()->uiInsertID;
}
#endif

