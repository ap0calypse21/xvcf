#include "stdafx.h" 
#include "config.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "p2p.h"
#include "guild.h"
#include "guild_manager.h"
#include "party.h"
#include "messenger_manager.h"
#include "empire_text_convert.h"
#include "unique_item.h"
#include "xmas_event.h"
#include "affect.h"
#include "castle.h"
#include "dev_log.h"
#include "locale_service.h"
#include "questmanager.h"
#include "pcbang.h"
#include "skill.h"
#include "threeway_war.h"
#ifdef __EVENT_MANAGER__
#include "event_manager.h"
#endif
#ifdef ENABLE_SONITEX_OFFLIENSHOP
#include "char_manager.h"
#include "private_shop_manager.h"
#endif
#if defined(ENABLE_PVP_TOURNAMENT_RANKING)
#include "pvp_duel.h"
#endif

////////////////////////////////////////////////////////////////////////////////
// Input Processor
CInputP2P::CInputP2P()
{
	BindPacketInfo(&m_packetInfoGG);
}

void CInputP2P::Login(LPDESC d, const char* c_pData)
{
	P2P_MANAGER::instance().Login(d, (TPacketGGLogin*)c_pData);
}

void CInputP2P::Logout(LPDESC d, const char* c_pData)
{
	TPacketGGLogout* p = (TPacketGGLogout*)c_pData;
#ifdef ENABLE_ANTI_MULTIPLE_FARM
	P2P_MANAGER::instance().Logout(p->szName, p->bAFisWarping);
#else
	P2P_MANAGER::instance().Logout(p->szName);
#endif
}

int CInputP2P::Relay(LPDESC d, const char* c_pData, size_t uiBytes)
{
	TPacketGGRelay* p = (TPacketGGRelay*)c_pData;

	if (uiBytes < sizeof(TPacketGGRelay) + p->lSize)
		return -1;

	if (p->lSize < 0)
	{
		sys_err("invalid packet length %d", p->lSize);
		d->SetPhase(PHASE_CLOSE);
		return -1;
	}

	sys_log(0, "InputP2P::Relay : %s size %d", p->szName, p->lSize);

	LPCHARACTER pkChr = CHARACTER_MANAGER::instance().FindPC(p->szName);

	const BYTE* c_pbData = (const BYTE*)(c_pData + sizeof(TPacketGGRelay));

	if (!pkChr)
	{
		sys_log(0, "Relay could not find %s", p->szName);
		return p->lSize;
	}

	if (*c_pbData == HEADER_GC_WHISPER)
	{
		char buf[1024];
		memcpy(buf, c_pbData, MIN(p->lSize, sizeof(buf)));
		TPacketGCWhisper* p2 = (TPacketGCWhisper*)buf;

		if (pkChr->IsBlockMode(BLOCK_WHISPER) && !IS_SET(p2->bType, WHISPER_TYPE_GM)) {
			return p->lSize;
		}

		// bType 상위 4비트: Empire 번호
		// bType 하위 4비트: EWhisperType
		BYTE bToEmpire = (p2->bType >> 4);
		p2->bType = p2->bType & 0x0F;
		if (p2->bType == 0x0F)
		{
			// 시스템 메세지 귓속말은 bType의 상위비트까지 모두 사용함.
			p2->bType = WHISPER_TYPE_SYSTEM;
		}
		else
		{
			if (!pkChr->IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_LANGUAGE))
			{
				if (bToEmpire >= 1 && bToEmpire <= 3 && pkChr->GetEmpire() != bToEmpire)
				{
					ConvertEmpireText(bToEmpire,
						buf + sizeof(TPacketGCWhisper),
						p2->wSize - sizeof(TPacketGCWhisper),
						10 + 2 * pkChr->GetSkillPower(SKILL_LANGUAGE1 + bToEmpire - 1));
				}
			}
		}

		pkChr->GetDesc()->Packet(buf, p->lSize);
	}
	else
		pkChr->GetDesc()->Packet(c_pbData, p->lSize);

	return p->lSize;
}

int CInputP2P::Notice(LPDESC d, const char* c_pData, size_t uiBytes)
{
	TPacketGGNotice* p = (TPacketGGNotice*)c_pData;

	if (uiBytes < sizeof(TPacketGGNotice) + p->lSize)
		return -1;

	if (p->lSize < 0)
	{
		sys_err("invalid packet length %d", p->lSize);
		d->SetPhase(PHASE_CLOSE);
		return -1;
	}

	char szBuf[256 + 1];
	strlcpy(szBuf, c_pData + sizeof(TPacketGGNotice), MIN(p->lSize + 1, sizeof(szBuf)));
	if (strlen(p->szArg) > 0)
	{
		SendRestrictedNotice(szBuf, p->szArg);
		return (p->lSize);
	}
	else
	{
		SendNotice(szBuf);
		return (p->lSize);
	}
}

int CInputP2P::BigNotice(LPDESC d, const char* c_pData, size_t uiBytes)
{
	TPacketGGBigNotice* p = (TPacketGGBigNotice*)c_pData;

	if (uiBytes < sizeof(TPacketGGBigNotice) + p->lSize)
		return -1;

	if (p->lSize < 0)
	{
		sys_err("invalid packet length %d", p->lSize);
		d->SetPhase(PHASE_CLOSE);
		return -1;
	}

	char szBuf[256 + 1];
	strlcpy(szBuf, c_pData + sizeof(TPacketGGBigNotice), MIN(p->lSize + 1, sizeof(szBuf)));
	SendBigNotice(szBuf);
	return (p->lSize);
}

int CInputP2P::MonarchNotice(LPDESC d, const char* c_pData, size_t uiBytes)
{
	TPacketGGMonarchNotice* p = (TPacketGGMonarchNotice*)c_pData;

	if (uiBytes < p->lSize + sizeof(TPacketGGMonarchNotice))
		return -1;

	if (p->lSize < 0)
	{
		sys_err("invalid packet length %d", p->lSize);
		d->SetPhase(PHASE_CLOSE);
		return -1;
	}

	char szBuf[256 + 1];
	strlcpy(szBuf, c_pData + sizeof(TPacketGGMonarchNotice), MIN(p->lSize + 1, sizeof(szBuf)));
	SendMonarchNotice(p->bEmpire, szBuf);
	return (p->lSize);
}

int CInputP2P::MonarchTransfer(LPDESC d, const char* c_pData)
{
	TPacketMonarchGGTransfer* p = (TPacketMonarchGGTransfer*)c_pData;
	LPCHARACTER pTargetChar = CHARACTER_MANAGER::instance().FindByPID(p->dwTargetPID);

	if (pTargetChar != NULL)
	{
		unsigned int qIndex = quest::CQuestManager::instance().GetQuestIndexByName("monarch_transfer");

		if (qIndex != 0)
		{
			pTargetChar->SetQuestFlag("monarch_transfer.x", p->x);
			pTargetChar->SetQuestFlag("monarch_transfer.y", p->y);
			quest::CQuestManager::instance().Letter(pTargetChar->GetPlayerID(), qIndex, 0);
		}
	}

	return 0;
}

int CInputP2P::Guild(LPDESC d, const char* c_pData, size_t uiBytes)
{
	TPacketGGGuild* p = (TPacketGGGuild*)c_pData;
	uiBytes -= sizeof(TPacketGGGuild);
	c_pData += sizeof(TPacketGGGuild);

	CGuild* g = CGuildManager::instance().FindGuild(p->dwGuild);

	switch (p->bSubHeader)
	{
	case GUILD_SUBHEADER_GG_CHAT:
	{
		if (uiBytes < sizeof(TPacketGGGuildChat))
			return -1;

		TPacketGGGuildChat* p = (TPacketGGGuildChat*)c_pData;

		if (g)
			g->P2PChat(p->szText);

		return sizeof(TPacketGGGuildChat);
	}

	case GUILD_SUBHEADER_GG_SET_MEMBER_COUNT_BONUS:
	{
		if (uiBytes < sizeof(int))
			return -1;

		int iBonus = *((int*)c_pData);
		CGuild* pGuild = CGuildManager::instance().FindGuild(p->dwGuild);
		if (pGuild)
		{
			pGuild->SetMemberCountBonus(iBonus);
		}
		return sizeof(int);
	}
	default:
		sys_err("UNKNOWN GUILD SUB PACKET");
		break;
	}
	return 0;
}

struct FuncShout
{
	const char* m_szText;
	BYTE m_bEmpire;
#if defined(__MESSENGER_BLOCK_SYSTEM__)
	const char* m_szName;

	FuncShout(const char* c_szText, const char* c_szName, BYTE bEmpire) : m_szText(c_szText), m_szName(c_szName), m_bEmpire(bEmpire)
	{
	}
#else
	FuncShout(const char* c_szText, BYTE bEmpire) : m_szText(c_szText), m_bEmpire(bEmpire)
	{
	}
#endif

	void operator () (LPDESC d)
	{
		if (g_bGlobalShout)
		{
			if (!d->GetCharacter())
				return;
		}
		else
		{
			if (!d->GetCharacter() || (d->GetCharacter()->GetGMLevel() == GM_PLAYER && d->GetEmpire() != m_bEmpire))
				return;
		}

#if defined(__MESSENGER_BLOCK_SYSTEM__)
		if (MessengerManager::instance().IsBlocked(d->GetCharacter()->GetName(), m_szName))
			return;
#endif

		d->GetCharacter()->ChatPacket(CHAT_TYPE_SHOUT, "%s", m_szText);
	}
};

#if defined(__MESSENGER_BLOCK_SYSTEM__)
void SendShout(const char* c_szText, const char* c_szName, BYTE bEmpire)
#else
void SendShout(const char* szText, BYTE bEmpire)
#endif
{
	const DESC_MANAGER::DESC_SET& c_ref_set = DESC_MANAGER::instance().GetClientSet();
#if defined(__MESSENGER_BLOCK_SYSTEM__)
	std::for_each(c_ref_set.begin(), c_ref_set.end(), FuncShout(c_szText, c_szName, bEmpire));
#else
	std::for_each(c_ref_set.begin(), c_ref_set.end(), FuncShout(c_szText, bEmpire));
#endif
}

void CInputP2P::Shout(const char* c_pData)
{
	TPacketGGShout* p = (TPacketGGShout*)c_pData;
#if defined(__MESSENGER_BLOCK_SYSTEM__)
	SendShout(p->szText, p->szName, p->bEmpire);
#else
	SendShout(p->szText, p->bEmpire);
#endif
}

void SendLCNotice(bool bBigFont, const char* szNotice, ...)
{
	const DESC_MANAGER::DESC_SET& c_ref_set = DESC_MANAGER::instance().GetClientSet();
	DESC_MANAGER::DESC_SET::const_iterator it = c_ref_set.begin();

	while (it != c_ref_set.end())
	{
		LPDESC d = *(it++);
		if (d->GetCharacter())
		{
			std::string strMsg = szNotice;
			const char* c_pszBuf;

			if (!strMsg.empty() && std::all_of(strMsg.begin(), strMsg.end(), ::isdigit))
			{
				DWORD dwKey = atoi(szNotice);
				BYTE bLanguage = (d ? d->GetLanguage() : LOCALE_YMIR);

				c_pszBuf = LC_LOCALE_QUEST_TEXT(dwKey, bLanguage);
			}
			else
			{
				c_pszBuf = szNotice;
			}

			std::string strBuffFilter = c_pszBuf;
			std::string strReplace("%d");

			size_t pos = 0;
			while ((pos = strBuffFilter.find(strReplace)) != std::string::npos)
			{
				strBuffFilter.replace(pos, strReplace.length(), "%s");
			}

			const char* c_pszConvBuf = strBuffFilter.c_str();
			char szNoticeBuf[CHAT_MAX_LEN + 1];

			va_list args;
			va_start(args, szNotice);
			va_end(args);
			int len = vsnprintf(szNoticeBuf, sizeof(szNoticeBuf), c_pszConvBuf, args);

			const char* c_pszToken;
			const char* c_pszLast = szNoticeBuf;

			std::string strBuff = szNoticeBuf;
			std::string strDelim = "[ENTER]";
			std::string strToken;

			while ((pos = strBuff.find(strDelim)) != std::string::npos)
			{
				strToken = strBuff.substr(0, pos);
				c_pszToken = strToken.c_str();
				d->GetCharacter()->ChatPacket(bBigFont ? CHAT_TYPE_BIG_NOTICE : CHAT_TYPE_NOTICE, "%s", c_pszToken);

				c_pszLast = strBuff.erase(0, pos + strDelim.length()).c_str();
			}
			d->GetCharacter()->ChatPacket(bBigFont ? CHAT_TYPE_BIG_NOTICE : CHAT_TYPE_NOTICE, "%s", c_pszLast);
		}
	}
}

void CInputP2P::LocaleNotice(const char* c_pData)
{
	TPacketGGLocaleNotice* p = (TPacketGGLocaleNotice*)c_pData;
	SendLCNotice(p->bBigFont, p->szNotice[0], p->szNotice[1], p->szNotice[2], p->szNotice[3], p->szNotice[4], p->szNotice[5]);
}

#ifdef ENABLE_SONITEX_OFFLIENSHOP
void CInputP2P::PrivateShopItemTransaction(const char* c_pData)
{
	const TPacketGGPrivateShopItemTransaction* p = (TPacketGGPrivateShopItemTransaction*)c_pData;

	LPCHARACTER pCustomer = CHARACTER_MANAGER::Instance().FindByPID(p->dwCustomerPID);
	if (!pCustomer)
	{
		// Transaction was unsuccessful, mark the item as available
		CPrivateShopManager::Instance().SendItemTransactionResult(p->dwShopID, p->shopItem.wPos, p->shopItem.dwID, false);
		return;
	}

	CPrivateShopManager::Instance().ItemTransaction(pCustomer, p->dwShopID, p->dwShopPort, &p->shopItem);
}

void CInputP2P::PrivateShopItemRemove(const char* c_pData)
{
	const TPacketGGPrivateShopItemRemove* p = (TPacketGGPrivateShopItemRemove*)c_pData;

	LPPRIVATE_SHOP pPrivateShop = CPrivateShopManager::Instance().GetPrivateShop(p->dwShopID);
	if (!pPrivateShop)
	{
		sys_err("Could not find private shop id %d", p->dwShopID);
		return;
	}

	pPrivateShop->RemoveItem(p->wPos);
}

void CInputP2P::PrivateShopItemSearch(const char* c_pData)
{
	TPacketGGPrivateShopItemSearch* p = (TPacketGGPrivateShopItemSearch*)c_pData;

	LPDESC pPeer = P2P_MANAGER::Instance().GetPeer(p->dwCustomerPort);
	if (!pPeer)
		return;

	TEMP_BUFFER buf;
	CPrivateShopManager::Instance().SearchItem(nullptr, p->dwVnum, p->bIgnoreRefineGrade, p->iJob, p->bItemType, p->bItemSubType, p->aAttr, p->llMaxGold, p->dwMaxCheque, &buf);

	if (buf.size())
	{
		TPacketGGPrivateShopItemSearchResult mainPacket;
		mainPacket.bHeader = HEADER_GG_PRIVATE_SHOP_ITEM_SEARCH_RESULT;
		mainPacket.wSize = buf.size();
		mainPacket.dwCustomerID = p->dwCustomerID;

		pPeer->BufferedPacket(&mainPacket, sizeof(mainPacket));
		pPeer->LargePacket(buf.read_peek(), buf.size());
	}
}

int CInputP2P::PrivateShopItemSearchResult(const char* c_pData, size_t uiBytes)
{
	TPacketGGPrivateShopItemSearchResult* p = (TPacketGGPrivateShopItemSearchResult*)c_pData;

	if (uiBytes < sizeof(TPacketGGPrivateShopItemSearchResult) + p->wSize)
		return -1;

	c_pData += sizeof(TPacketGGPrivateShopItemSearchResult);

	LPCHARACTER pCustomer = CHARACTER_MANAGER::Instance().FindByPID(p->dwCustomerID);
	if (!pCustomer)
		return p->wSize;

	TPacketGCPrivateShop mainPacket;
	mainPacket.bHeader = HEADER_GC_PRIVATE_SHOP;
	mainPacket.wSize = sizeof(TPacketGCPrivateShop) + p->wSize;
	mainPacket.bSubHeader = SUBHEADER_GC_SHOP_SEARCH_RESULT;

	pCustomer->GetDesc()->BufferedPacket(&mainPacket, sizeof(TPacketGCPrivateShop));
	pCustomer->GetDesc()->LargePacket(c_pData, p->wSize);

	return p->wSize;
}
#endif

void CInputP2P::Disconnect(const char* c_pData)
{
	TPacketGGDisconnect* p = (TPacketGGDisconnect*)c_pData;

	LPDESC d = DESC_MANAGER::instance().FindByLoginName(p->szLogin);

	if (!d)
		return;

	if (!d->GetCharacter())
	{
		d->SetPhase(PHASE_CLOSE);
	}
	else
		d->DisconnectOfSameLogin();
}

void CInputP2P::Setup(LPDESC d, const char* c_pData)
{
	TPacketGGSetup* p = (TPacketGGSetup*)c_pData;
	sys_log(0, "P2P: Setup %s:%d", d->GetHostName(), p->wPort);
	d->SetP2P(d->GetHostName(), p->wPort, p->bChannel);
}

void CInputP2P::MessengerAdd(const char* c_pData)
{
	TPacketGGMessenger* p = (TPacketGGMessenger*)c_pData;
	sys_log(0, "P2P: Messenger Add %s %s", p->szAccount, p->szCompanion);
	MessengerManager::instance().__AddToList(p->szAccount, p->szCompanion);
}

void CInputP2P::MessengerRemove(const char* c_pData)
{
	TPacketGGMessenger* p = (TPacketGGMessenger*)c_pData;
	sys_log(0, "P2P: Messenger Remove %s %s", p->szAccount, p->szCompanion);
	MessengerManager::instance().__RemoveFromList(p->szAccount, p->szCompanion);
}

#if defined(__MESSENGER_BLOCK_SYSTEM__)
void CInputP2P::MessengerBlockAdd(const char* c_pData)
{
	TPacketGGMessenger* p = (TPacketGGMessenger*)c_pData;
	MessengerManager::instance().__AddToBlockList(p->szAccount, p->szCompanion);
}

void CInputP2P::MessengerBlockRemove(const char* c_pData)
{
	TPacketGGMessenger* p = (TPacketGGMessenger*)c_pData;
	MessengerManager::instance().__RemoveFromBlockList(p->szAccount, p->szCompanion);
}
#endif

void CInputP2P::FindPosition(LPDESC d, const char* c_pData)
{
	TPacketGGFindPosition* p = (TPacketGGFindPosition*)c_pData;
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(p->dwTargetPID);
#ifdef ENABLE_CMD_WARP_IN_DUNGEON
	if (ch)
#else
	if (ch && ch->GetMapIndex() < 10000)
#endif
	{
		TPacketGGWarpCharacter pw;
		pw.header = HEADER_GG_WARP_CHARACTER;
		pw.pid = p->dwFromPID;
		pw.x = ch->GetX();
		pw.y = ch->GetY();
#ifdef ENABLE_CMD_WARP_IN_DUNGEON
		pw.mapIndex = (ch->GetMapIndex() < 10000) ? 0 : ch->GetMapIndex();
#endif
		d->Packet(&pw, sizeof(pw));
	}
}

void CInputP2P::WarpCharacter(const char* c_pData)
{
	TPacketGGWarpCharacter* p = (TPacketGGWarpCharacter*)c_pData;
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(p->pid);
#ifdef ENABLE_CMD_WARP_IN_DUNGEON
	if (ch)
		ch->WarpSet(p->x, p->y, p->mapIndex);
#else
	if (ch)
		ch->WarpSet(p->x, p->y);
#endif
}

void CInputP2P::GuildWarZoneMapIndex(const char* c_pData)
{
	TPacketGGGuildWarMapIndex* p = (TPacketGGGuildWarMapIndex*)c_pData;
	CGuildManager& gm = CGuildManager::instance();

	sys_log(0, "P2P: GuildWarZoneMapIndex g1(%u) vs g2(%u), mapIndex(%d)", p->dwGuildID1, p->dwGuildID2, p->lMapIndex);

	CGuild* g1 = gm.FindGuild(p->dwGuildID1);
	CGuild* g2 = gm.FindGuild(p->dwGuildID2);

	if (g1 && g2)
	{
		g1->SetGuildWarMapIndex(p->dwGuildID2, p->lMapIndex);
		g2->SetGuildWarMapIndex(p->dwGuildID1, p->lMapIndex);
	}
}

void CInputP2P::Transfer(const char* c_pData)
{
	TPacketGGTransfer* p = (TPacketGGTransfer*)c_pData;

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(p->szName);

	if (ch)
		ch->WarpSet(p->lX, p->lY);
}

void CInputP2P::XmasWarpSanta(const char* c_pData)
{
	TPacketGGXmasWarpSanta* p = (TPacketGGXmasWarpSanta*)c_pData;

	if (p->bChannel == g_bChannel && map_allow_find(p->lMapIndex))
	{
		int iNextSpawnDelay = 60;

		if (LC_IsYMIR())
			iNextSpawnDelay = 20 * 60;
		else
			iNextSpawnDelay = 50 * 60;

		xmas::SpawnSanta(p->lMapIndex, iNextSpawnDelay); // 50분있다가 새로운 산타가 나타남 (한국은 20분)

		TPacketGGXmasWarpSantaReply pack_reply;
		pack_reply.bHeader = HEADER_GG_XMAS_WARP_SANTA_REPLY;
		pack_reply.bChannel = g_bChannel;
		P2P_MANAGER::instance().Send(&pack_reply, sizeof(pack_reply));
	}
}

void CInputP2P::XmasWarpSantaReply(const char* c_pData)
{
	TPacketGGXmasWarpSantaReply* p = (TPacketGGXmasWarpSantaReply*)c_pData;

	if (p->bChannel == g_bChannel)
	{
		CharacterVectorInteractor i;

		if (CHARACTER_MANAGER::instance().GetCharactersByRaceNum(xmas::MOB_SANTA_VNUM, i))
		{
			CharacterVectorInteractor::iterator it = i.begin();

			while (it != i.end())
			{
				M2_DESTROY_CHARACTER(*it++);
			}
		}
	}
}

void CInputP2P::LoginPing(LPDESC d, const char* c_pData)
{
	TPacketGGLoginPing* p = (TPacketGGLoginPing*)c_pData;

	if (!g_pkAuthMasterDesc) // If I am master, I have to broadcast
		P2P_MANAGER::instance().Send(p, sizeof(TPacketGGLoginPing), d);
}

// BLOCK_CHAT
void CInputP2P::BlockChat(const char* c_pData)
{
	TPacketGGBlockChat* p = (TPacketGGBlockChat*)c_pData;

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(p->szName);

	if (ch)
	{
		sys_log(0, "BLOCK CHAT apply name %s dur %d", p->szName, p->lBlockDuration);
		ch->AddAffect(AFFECT_BLOCK_CHAT, POINT_NONE, 0, AFF_NONE, p->lBlockDuration, 0, true);
	}
	else
	{
		sys_log(0, "BLOCK CHAT fail name %s dur %d", p->szName, p->lBlockDuration);
	}
}
// END_OF_BLOCK_CHAT
//

void CInputP2P::PCBangUpdate(const char* c_pData)
{
	TPacketPCBangUpdate* p = (TPacketPCBangUpdate*)c_pData;

	CPCBangManager::instance().RequestUpdateIPList(p->ulPCBangID);
}

#if defined(ENABLE_PVP_TOURNAMENT_RANKING)
void CInputP2P::PvPDuelRank(const char* c_pData)
{
	CPvPDuel::Instance().InitializeRanking();
	sys_log(0, "P2P PvPDuelRank was updated!");
}
#endif

void CInputP2P::IamAwake(LPDESC d, const char* c_pData)
{
	std::string hostNames;
	P2P_MANAGER::instance().GetP2PHostNames(hostNames);
	sys_log(0, "P2P Awakeness check from %s. My P2P connection number is %d. and details...\n%s", d->GetHostName(), P2P_MANAGER::instance().GetDescCount(), hostNames.c_str());
}

#ifdef MAINTENANCE_INFO
void CInputP2P::MaintenanceInfo(const char* c_pData)
{
	TPacketGGMaintenanceInfo* p = (TPacketGGMaintenanceInfo*) c_pData;
	DESC_MANAGER::Instance().SetMaintenanceInfo(p->info);
}
#endif

int CInputP2P::Analyze(LPDESC d, BYTE bHeader, const char* c_pData)
{
	if (test_server)
		sys_log(0, "CInputP2P::Anlayze[Header %d]", bHeader);

	int iExtraLen = 0;

	switch (bHeader)
	{
	case HEADER_GG_SETUP:
		Setup(d, c_pData);
		break;

	case HEADER_GG_LOGIN:
		Login(d, c_pData);
		break;

	case HEADER_GG_LOGOUT:
		Logout(d, c_pData);
		break;

	case HEADER_GG_RELAY:
		if ((iExtraLen = Relay(d, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;

	case HEADER_GG_NOTICE:
		if ((iExtraLen = Notice(d, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;

	case HEADER_GG_SHUTDOWN:
		sys_err("Accept shutdown p2p command from %s.", d->GetHostName());
		Shutdown(10);
		break;

	case HEADER_GG_GUILD:
		if ((iExtraLen = Guild(d, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;

	case HEADER_GG_SHOUT:
		Shout(c_pData);
		break;

	case HEADER_GG_DISCONNECT:
		Disconnect(c_pData);
		break;

	case HEADER_GG_MESSENGER_ADD:
		MessengerAdd(c_pData);
		break;

	case HEADER_GG_MESSENGER_REMOVE:
		MessengerRemove(c_pData);
		break;

#if defined(__MESSENGER_BLOCK_SYSTEM__)
	case HEADER_GG_MESSENGER_BLOCK_ADD:
		MessengerBlockAdd(c_pData);
		break;

	case HEADER_GG_MESSENGER_BLOCK_REMOVE:
		MessengerBlockRemove(c_pData);
		break;
#endif

	case HEADER_GG_FIND_POSITION:
		FindPosition(d, c_pData);
		break;

	case HEADER_GG_WARP_CHARACTER:
		WarpCharacter(c_pData);
		break;

	case HEADER_GG_GUILD_WAR_ZONE_MAP_INDEX:
		GuildWarZoneMapIndex(c_pData);
		break;

	case HEADER_GG_TRANSFER:
		Transfer(c_pData);
		break;

	case HEADER_GG_XMAS_WARP_SANTA:
		XmasWarpSanta(c_pData);
		break;

	case HEADER_GG_XMAS_WARP_SANTA_REPLY:
		XmasWarpSantaReply(c_pData);
		break;

	case HEADER_GG_RELOAD_CRC_LIST:
		LoadValidCRCList();
		break;

	case HEADER_GG_CHECK_CLIENT_VERSION:
		CheckClientVersion();
		break;

	case HEADER_GG_LOGIN_PING:
		LoginPing(d, c_pData);
		break;

	case HEADER_GG_BLOCK_CHAT:
		BlockChat(c_pData);
		break;

	case HEADER_GG_SIEGE:
	{
		TPacketGGSiege* pSiege = (TPacketGGSiege*)c_pData;
		castle_siege(pSiege->bEmpire, pSiege->bTowerCount);
	}
	break;

	case HEADER_GG_BIG_NOTICE:
		if ((iExtraLen = BigNotice(d, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;

	case HEADER_GG_MONARCH_NOTICE:
		if ((iExtraLen = MonarchNotice(d, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;

	case HEADER_GG_MONARCH_TRANSFER:
		MonarchTransfer(d, c_pData);
		break;

	case HEADER_GG_PCBANG_UPDATE:
		PCBangUpdate(c_pData);
		break;

	case HEADER_GG_CHECK_AWAKENESS:
		IamAwake(d, c_pData);
		break;
#ifdef MAINTENANCE_INFO
		case HEADER_GG_MAINTENANCE_INFO:
			MaintenanceInfo(c_pData);
			break;
#endif
#if defined(ENABLE_PVP_TOURNAMENT_RANKING)
	case HEADER_GG_PVP_DUEL_RANK:
		PvPDuelRank(c_pData);
		break;
#endif
#ifdef __EVENT_MANAGER__
	case HEADER_GG_EVENT_RELOAD:
		BroadcastEventReload();
		break;
	case HEADER_GG_EVENT:
		Event(c_pData);
		break;
	case HEADER_GG_EVENT_HIDE_AND_SEEK:
		HideAndSeekEvent(c_pData);
		break;
#endif
#ifdef ENABLE_SWITCHBOT
	case HEADER_GG_SWITCHBOT:
		Switchbot(d, c_pData);
		break;
#endif
#ifdef ENABLE_ANTI_MULTIPLE_FARM
	case HEADER_GG_ANTI_FARM:
		RecvAntiFarmUpdateStatus(d, c_pData);
		break;
#endif
	case HEADER_GG_LOCALE_NOTICE:
		LocaleNotice(c_pData);
		break;
		
#ifdef ENABLE_SONITEX_OFFLIENSHOP
		case HEADER_GG_PRIVATE_SHOP_ITEM_TRANSACTION:
			PrivateShopItemTransaction(c_pData);
			break;
			
		case HEADER_GG_PRIVATE_SHOP_ITEM_REMOVE:
			PrivateShopItemRemove(c_pData);
			break;
			
		case HEADER_GG_PRIVATE_SHOP_ITEM_SEARCH:
			PrivateShopItemSearch(c_pData);
			break;

		case HEADER_GG_PRIVATE_SHOP_ITEM_SEARCH_RESULT:
			if ((iExtraLen = PrivateShopItemSearchResult(c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;
#endif
	case HEADER_GG_GM_NOTIFICATION:
	{
		const TPacketGGGMNotification* p = reinterpret_cast<const TPacketGGGMNotification*>(c_pData);
		
		const auto& desc_set = DESC_MANAGER::instance().GetClientSet();
		for (const auto& desc : desc_set)
		{
			LPCHARACTER gmChar = desc->GetCharacter();
			
			if (gmChar && gmChar->GetGMLevel() == GM_IMPLEMENTOR)
			{
				char message[CHAT_MAX_LEN];
				snprintf(message, sizeof(message), "%s", p->szMessage);
				
				TPacketGCWhisper whisperPacket;
				memset(&whisperPacket, 0, sizeof(TPacketGCWhisper));
				whisperPacket.bHeader = HEADER_GC_WHISPER;
				whisperPacket.bType = WHISPER_TYPE_SYSTEM;
				whisperPacket.wSize = sizeof(TPacketGCWhisper) + strlen(message) + 1;
				strncpy(whisperPacket.szNameFrom, "System", sizeof(whisperPacket.szNameFrom));
				
				gmChar->GetDesc()->BufferedPacket(&whisperPacket, sizeof(whisperPacket));
				gmChar->GetDesc()->Packet(message, strlen(message) + 1);
			}
		}
	}
	break;
	}

	return (iExtraLen);
}

#ifdef ENABLE_SWITCHBOT
#include "switchbot.h"
void CInputP2P::Switchbot(LPDESC d, const char* c_pData)
{
	const TPacketGGSwitchbot* p = reinterpret_cast<const TPacketGGSwitchbot*>(c_pData);
	if (p->wPort != mother_port)
	{
		return;
	}

	CSwitchbotManager::Instance().P2PReceiveSwitchbot(p->table);
}
#endif

#ifdef ENABLE_ANTI_MULTIPLE_FARM
#include "HAntiMultipleFarm.h"

auto CInputP2P::RecvAntiFarmUpdateStatus(LPDESC d, const char* c_pData) -> void // checking the drop status if FALSE then don't drop items for the player unless he closes one client or logout from the other account 
{
	if (!d) return;
	
	CAntiMultipleFarm::TP2PChangeDropStatus* p = (CAntiMultipleFarm::TP2PChangeDropStatus*)c_pData;// here 
	
	std::vector <DWORD> dwPIDs;
	for (uint8_t i = 0; i < MULTIPLE_FARM_MAX_ACCOUNT; ++i)
		dwPIDs.emplace_back(p->dwPIDs[i]);
	
	CAntiMultipleFarm::instance().P2PSendBlockDropStatusChange(p->cMAIf, dwPIDs);
}
#endif

#ifdef __EVENT_MANAGER__
void CInputP2P::Event(const char* c_pData)
{
	TPacketGGEvent* p = (TPacketGGEvent*)c_pData;
	CEventManager::Instance().SetEventState(&p->table, p->bState);
}

void CInputP2P::HideAndSeekEvent(const char* c_pData)
{
	TPacketGGEventHideAndSeek* p = (TPacketGGEventHideAndSeek*)c_pData;

	// Hide and Seek event is only active on one specific channel
	if (g_bChannel != HIDE_AND_SEEK_CHANNEL)
		return;

	CEventManager::Instance().HideAndSeekNPC(p->iPosition, p->iRound);
}
#endif