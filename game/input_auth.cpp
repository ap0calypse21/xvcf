#include "stdafx.h" 
#include "constants.h"
#include "config.h"
#include "input.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "protocol.h"
#include "locale_service.h"
#include "db.h"
#include "utils.h"
#include "EmailAuth.h"  // Include the EmailAuth utilities

#if defined(ENABLE_CSHIELD)
#include "cshieldLib.h"
#endif
#ifndef __WIN32__
#include "limit_time.h"
#endif
extern time_t get_global_time();

bool FN_IS_VALID_LOGIN_STRING(const char* str)
{
	const char* tmp;

	if (!str || !*str)
		return false;

	if (strlen(str) < 2)
		return false;

	for (tmp = str; *tmp; ++tmp)
	{
		// 알파벳과 수자만 허용
		if (isdigit(*tmp) || isalpha(*tmp))
			continue;

		// 캐나다는 몇몇 특수문자 허용
		if (LC_IsCanada())
		{
			switch (*tmp)
			{
			case ' ':
			case '_':
			case '-':
			case '.':
			case '!':
			case '@':
			case '#':
			case '$':
			case '%':
			case '^':
			case '&':
			case '*':
			case '(':
			case ')':
				continue;
			}
		}

		if (LC_IsYMIR() == true || LC_IsKorea() == true)
		{
			switch (*tmp)
			{
			case '-':
			case '_':
				continue;
			}
		}

		if (LC_IsBrazil() == true)
		{
			switch (*tmp)
			{
			case '_':
			case '-':
			case '=':
				continue;
			}
		}

		if (LC_IsJapan() == true)
		{
			switch (*tmp)
			{
			case '-':
			case '_':
			case '@':
			case '#':
				continue;
			}
		}

		return false;
	}

	return true;
}

bool Login_IsInChannelService(const char* c_login)
{
	if (c_login[0] == '[')
		return true;
	return false;
}

CInputAuth::CInputAuth()
{
}

void CInputAuth::Login(LPDESC d, const char* c_pData)
{
	char szQuery[512];
	    time_t banUntil = 0;  // Declare banUntil at the beginning of the function

#ifdef ENABLE_LIMIT_TIME
    extern bool Metin2Server_IsInvalid();

    if (Metin2Server_IsInvalid())
    {
        extern void ClearAdminPages();
        ClearAdminPages();
        exit(1);
        return;
    }
#endif

    TPacketCGLogin3* pinfo = (TPacketCGLogin3*)c_pData;

    if (!g_bAuthServer)
    {
        sys_err("CInputAuth class is not for game server. IP %s might be a hacker.",
            inet_ntoa(d->GetAddr().sin_addr));
        d->DelayedDisconnect(5);
        return;
    }

    char login[LOGIN_MAX_LEN + 1];
    trim_and_lower(pinfo->login, login, sizeof(login));

    char passwd[PASSWD_MAX_LEN + 1];
    strlcpy(passwd, pinfo->passwd, sizeof(passwd));

#if defined(ENABLE_BINARY_CHECK)
    char keyMD5[128 + 1];
    strlcpy(keyMD5, pinfo->key, sizeof(keyMD5));
    sys_log(0, "InputAuth::Login : %s(%d) desc %p binaryMD5 (%s)", login, strlen(login), get_pointer(d), keyMD5);
#else
    sys_log(0, "InputAuth::Login : %s(%d) desc %p", login, strlen(login), get_pointer(d));
#endif

    if (false == FN_IS_VALID_LOGIN_STRING(login))
    {
        sys_log(0, "InputAuth::Login : IS_NOT_VALID_LOGIN_STRING(%s) desc %p",
            login, get_pointer(d));
        LoginFailure(d, "NOID");
        return;
    }

    if (g_bNoMoreClient)
    {
        TPacketGCLoginFailure failurePacket;

        failurePacket.header = HEADER_GC_LOGIN_FAILURE;
        strlcpy(failurePacket.szStatus, "SHUTDOWN", sizeof(failurePacket.szStatus));

        d->Packet(&failurePacket, sizeof(failurePacket));
        return;
    }

    if (DESC_MANAGER::instance().FindByLoginName(login))
    {
        LoginFailure(d, "ALREADY");
        return;
    }

#if defined(ENABLE_BINARY_CHECK)
    char szKeyMD5[128 * 2 + 1];
    DBManager::instance().EscapeString(szKeyMD5, sizeof(szKeyMD5), keyMD5, strlen(keyMD5));
    if (m_binaryMD5.compare(szKeyMD5)) {
        LoginFailure(d, "UPDATE");
        return;
    }
#endif

    DWORD dwKey = DESC_MANAGER::instance().CreateLoginKey(d);
    DWORD dwPanamaKey = dwKey ^ pinfo->adwClientKey[0] ^ pinfo->adwClientKey[1] ^ pinfo->adwClientKey[2] ^ pinfo->adwClientKey[3];
    d->SetPanamaKey(dwPanamaKey);

    sys_log(0, "InputAuth::Login : key %u:0x%x login %s", dwKey, dwPanamaKey, login);

    TPacketCGLogin3* p = M2_NEW TPacketCGLogin3;
    thecore_memcpy(p, pinfo, sizeof(TPacketCGLogin3));

    char szPasswd[PASSWD_MAX_LEN * 2 + 1];
    DBManager::instance().EscapeString(szPasswd, sizeof(szPasswd), passwd, strlen(passwd));

    char szLogin[LOGIN_MAX_LEN * 2 + 1];
    DBManager::instance().EscapeString(szLogin, sizeof(szLogin), login, strlen(login));

    // First check if the username and password are correct
    SQLMsg* pLoginCheck = DBManager::instance().DirectQuery(
        "SELECT `id`, SHA1('%s') = `password` AS `password_match` FROM `account` WHERE `login` = '%s'", 
        szPasswd, szLogin);

    if (pLoginCheck->Get()->uiNumRows == 0)
    {
        sys_log(0, "Login: Login failed, account does not exist");
        LoginFailure(d, "NOID");
        return;
    }

    MYSQL_ROW loginRow = mysql_fetch_row(pLoginCheck->Get()->pSQLResult);
    int accountID = atoi(loginRow[0]);
    bool passwordMatch = atoi(loginRow[1]) == 1;

    if (!passwordMatch)
    {
        sys_log(0, "Login: Wrong password for login %s", login);
        LoginFailure(d, "WRONGPWD");
        return;
    }

    // Check if the account is banned
    bool isBanned = false;
    snprintf(szQuery, sizeof(szQuery), "SELECT `ban`, UNIX_TIMESTAMP(`ban_until`) FROM `account`.`hwid` WHERE `account_id` = %u", accountID);
    SQLMsg* pBanCheck = DBManager::instance().DirectQuery(szQuery);

if (pBanCheck->Get()->uiNumRows > 0)
{
    MYSQL_ROW banRow = mysql_fetch_row(pBanCheck->Get()->pSQLResult);
    int banStatus = atoi(banRow[0]);

    if (banStatus == 1)
    {
        if (banRow[1] != nullptr)
        {
            // ????? ?????? ?????? ?????
            sys_log(0, "Raw ban_until value from DB (string): %s", banRow[1]);

            banUntil = (time_t)atoll(banRow[1]);
            sys_log(0, "Converted ban_until value (time_t): %ld", banUntil);

            if (time(0) > banUntil)
            {
                // ??? ????? ?????? ?? ???????
                snprintf(szQuery, sizeof(szQuery), "UPDATE `account`.`hwid` SET `ban` = 0, `ban_until` = NULL WHERE `account_id` = %u", accountID);
                DBManager::instance().DirectQuery(szQuery);
                sys_log(0, "Login: Ban expired and removed for account ID %u", accountID);
            }
            else
            {
                isBanned = true;
            }
        }
        else
        {
            sys_err("Login: ban_until is NULL for account ID %u", accountID);
            isBanned = true; // ????? ???????? ??????? ??? ???? `ban_until` ?? NULL
        }
    }
}

if (isBanned)
{
    if (banUntil > 0)
    {
        // ????? banUntil ??? ????? ???? ??????
        char banUntilStr[64];
        strftime(banUntilStr, sizeof(banUntilStr), "%Y-%m-%d %H:%M:%S", localtime(&banUntil));

        sys_log(0, "Login: Account ID %u is currently banned until %s.", accountID, banUntilStr);

        char banMessage[128];
        snprintf(banMessage, sizeof(banMessage), "BANNED until %s", banUntilStr);
        LoginFailure(d, banMessage);
    }
    else
    {
        sys_log(0, "Error: Invalid or null ban_until time for Account ID %u.", accountID);
        LoginFailure(d, "BANNED (invalid ban_until)");
    }

    return;
}


    bool hwidValid = false;

    // Retrieve the current HWID from the `hwid` table
    snprintf(szQuery, sizeof(szQuery), "SELECT `hwid` FROM `account`.`hwid` WHERE `account_id` = %u", accountID);
    SQLMsg* pHWIDCheck = DBManager::instance().DirectQuery(szQuery);
    
    if (pHWIDCheck->Get()->uiNumRows > 0)
    {
        MYSQL_ROW hwidRow = mysql_fetch_row(pHWIDCheck->Get()->pSQLResult);
        const char* currentHWID = hwidRow[0]; // Current HWID from the database

        // Compare the current HWID with the one stored in the database
        if (strcmp(currentHWID, pinfo->hwid) != 0) // Assuming `hwid` is a part of the packet
        {
            // HWID has changed, so update it and reset all valid fields to 0
            snprintf(szQuery, sizeof(szQuery), 
                     "UPDATE `account`.`hwid` SET `hwid`='%s', `old_hwid`='%s', `valid1`=0, `valid2`=0, `valid3`=0, `valid4`=0, `valid5`=0 WHERE `account_id`=%u",
                     pinfo->hwid, currentHWID, accountID);
            DBManager::instance().DirectQuery(szQuery);
            sys_log(0, "Login: HWID has changed for account ID: %u, HWID updated", accountID);
        }

        // Loop through valid1 to valid5 and check if any of them are valid
        for (int i = 1; i <= 5; ++i)
        {
            snprintf(szQuery, sizeof(szQuery), "SELECT `valid%d` FROM `account`.`hwid` WHERE `account_id` = %u", i, accountID);
            std::unique_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery(szQuery));
            
            if (pMsg->Get()->uiNumRows > 0)
            {
                MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
                if (atoi(row[0]) == 1)
                {
                    hwidValid = true;
                    break; // Stop checking further if one valid is found
                }
            }
        }
    }
    else
    {
        sys_err("Login: Failed to retrieve HWID for account ID %u", accountID);
       // LoginFailure(d, "HWID_NOT_FOUND");
        //return; // Stop further processing due to the error
    }

    if (!hwidValid)
    {
        // Generate a verification code
        std::string verificationCode = GenerateVerificationCode();
    
        // Store the verification code in the database
        snprintf(szQuery, sizeof(szQuery), "UPDATE `account`.`account` SET `email_verification_code` = '%s', `email_verification_timestamp` = NOW() WHERE `login` = '%s'", verificationCode.c_str(), login);
        DBManager::instance().DirectQuery(szQuery);
    
        // Retrieve the email address associated with the account
        char szEmail[128] = {};
        snprintf(szQuery, sizeof(szQuery), "SELECT `email` FROM `account`.`account` WHERE `login` = '%s'", login);
        std::unique_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery(szQuery));
        
        if (pMsg->Get()->uiNumRows > 0)
        {
            MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
            strlcpy(szEmail, row[0], sizeof(szEmail));
    
            // Send the verification email
            if (SendVerificationEmail(szEmail, verificationCode, login, inet_ntoa(d->GetAddr().sin_addr)))
            {
                sys_log(0, "Login: Sent verification code to email: %s", szEmail);
                // Show a message to the user that the verification code has been sent
            }
            else
            {
                sys_err("Login: Failed to send verification email to %s", szEmail);
                LoginFailure(d, "EMAIL_SEND_ERROR");
                return; // Stop further processing due to the error
            }
        }
        else
        {
            sys_err("Login: Failed to retrieve email for login %s", login);
            LoginFailure(d, "EMAIL_NOT_FOUND");
            return; // Stop further processing due to the error
        }
    }
    else
    {
        sys_log(0, "Login: HWID already validated for login %s", szLogin);
    }

    // Proceed with the regular login process if HWID is valid
    DBManager::instance().ReturnQuery(QID_AUTH_LOGIN, dwKey, p,
        "SELECT SHA1('%s'), `password`, `social_id`, `id`, `status`,"
        "`language`,"
        "`availDt` - NOW() > 0,"
        "UNIX_TIMESTAMP(`silver_expire`),"
        "UNIX_TIMESTAMP(`gold_expire`),"
        "UNIX_TIMESTAMP(`safebox_expire`),"
        "UNIX_TIMESTAMP(`autoloot_expire`),"
        "UNIX_TIMESTAMP(`fish_mind_expire`),"
        "UNIX_TIMESTAMP(`marriage_fast_expire`),"
        "UNIX_TIMESTAMP(`money_drop_rate_expire`),"
        "UNIX_TIMESTAMP(`create_time`)"
        " FROM `account` WHERE `login` = '%s'",
        szPasswd, szLogin);
}


extern void socket_timeout(socket_t s, long sec, long usec);

int CInputAuth::Analyze(LPDESC d, BYTE bHeader, const char* c_pData)
{
	if (!g_bAuthServer)
	{
		sys_err("CInputAuth class is not for game server. IP %s might be a hacker.",
			inet_ntoa(d->GetAddr().sin_addr));
		d->DelayedDisconnect(5);
		return 0;
	}

	int iExtraLen = 0;

	if (test_server)
		sys_log(0, " InputAuth Analyze Header[%d] ", bHeader);

	switch (bHeader)
	{
	case HEADER_CG_PONG:
		Pong(d);
		break;

	case HEADER_CG_LOGIN3:
		Login(d, c_pData);
		break;

	case HEADER_CG_HANDSHAKE:
		break;

	default:
		sys_err("This phase does not handle this header %d (0x%x)(phase: AUTH)", bHeader, bHeader);
		break;
	}

	return iExtraLen;
}
