#include <iostream>
#include <string>
#include <unordered_map>
#include "stdafx.h"
#include "constants.h"
#include "config.h"
#include "utils.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "buffer_manager.h"
#include "packet.h"
#include "protocol.h"
#include "char.h"
#include "char_manager.h"
#include "item.h"
#include "item_manager.h"
#include "cmd.h"
#include "shop.h"
#include "shop_manager.h"
#include "safebox.h"
#include "regen.h"
#include "battle.h"
#include "exchange.h"
#include "questmanager.h"
#include "profiler.h"
#include "messenger_manager.h"
#include "party.h"
#include "p2p.h"
#include "affect.h"
#include "guild.h"
#include "guild_manager.h"
#include "log.h"
#include "banword.h"
#include "empire_text_convert.h"
#include "unique_item.h"
#include "building.h"
#include "locale_service.h"
#include "gm.h"
#include "spam.h"
#include "ani.h"
#include "motion.h"
#include "OXEvent.h"
#include "locale_service.h"
#include "DragonSoul.h"
#include "belt_inventory_helper.h"
#ifdef ENABLE_SWITCHBOT
#include "switchbot.h"
#endif
#if defined(__MINI_GAME_CATCH_KING__)
#include "minigame.h"
#endif
#ifdef __EVENT_MANAGER__
#include "event_manager.h"
#endif
#if defined(__SKILLBOOK_COMB_SYSTEM__)
#include <random>
#endif
#if defined(ENABLE_EXTENDED_BATTLE_PASS)
#include "battlepass_manager.h"
#endif
#if defined(ENABLE_PVP_TOURNAMENT)
#include "pvp_duel.h"
#endif

#if defined(__PRIVATE_SHOP_SEARCH_SYSTEM__)
#include "entity.h"
#include <unordered_map>
bool CompareItemVnumAcPriceAC(COfflineShop::OFFLINE_SHOP_ITEM i, COfflineShop::OFFLINE_SHOP_ITEM j)
{
	return (i.vnum < j.vnum) && (i.price < j.price);
}
#endif

#ifdef __GROWTH_PET_SYSTEM__
#include "growth_pet.h"
#endif
#if defined(ENABLE_INGAME_WIKI)
#include "mob_manager.h"
#endif

#if defined(__MESSENGER_BLOCK_SYSTEM__)
extern void SendShout(const char* szText, const char* c_szName, BYTE bEmpire);
#else
extern void SendShout(const char* szText, BYTE bEmpire);
#endif
extern int g_nPortalLimitTime;

static int __deposit_limit()
{
	return (1000 * 10000); // 1천만
}

void SendBlockChatInfo(LPCHARACTER ch, int sec)
{
	if (sec <= 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("채팅 금지 상태입니다."));
		return;
	}

	long hour = sec / 3600;
	sec -= hour * 3600;

	long min = (sec / 60);
	sec -= min * 60;

	char buf[128 + 1];

#if defined(ENABLE_TEXT_PACKET)
	if (hour > 0 && min > 0) {
		ch->ChatPacketNew(CHAT_TYPE_INFO, 5, "%d#%d#%d", hour, min, sec);
	} else if (hour > 0 && min == 0) {
		ch->ChatPacketNew(CHAT_TYPE_INFO, 6, "%d#%d", hour, sec);
	} else if (hour == 0 && min > 0) {
		ch->ChatPacketNew(CHAT_TYPE_INFO, 4, "%d#%d", min, sec);
	} else {
		ch->ChatPacketNew(CHAT_TYPE_INFO, 7, "%d", sec);
	}
#endif
}

EVENTINFO(spam_event_info)
{
	char host[MAX_HOST_LENGTH + 1];

	spam_event_info()
	{
		::memset(host, 0, MAX_HOST_LENGTH + 1);
	}
};

typedef std::unordered_map<std::string, std::pair<unsigned int, LPEVENT>> spam_score_of_ip_t;
spam_score_of_ip_t spam_score_of_ip;

EVENTFUNC(block_chat_by_ip_event)
{
	spam_event_info* info = dynamic_cast<spam_event_info*>(event->info);

	if (info == NULL)
	{
		sys_err("block_chat_by_ip_event> <Factor> Null pointer");
		return 0;
	}

	const char* host = info->host;

	spam_score_of_ip_t::iterator it = spam_score_of_ip.find(host);

	if (it != spam_score_of_ip.end())
	{
		it->second.first = 0;
		it->second.second = NULL;
	}

	return 0;
}

bool SpamBlockCheck(LPCHARACTER ch, const char* const buf, const size_t buflen)
{
	extern int g_iSpamBlockMaxLevel;

	if (ch->GetLevel() < g_iSpamBlockMaxLevel)
	{
		spam_score_of_ip_t::iterator it = spam_score_of_ip.find(ch->GetDesc()->GetHostName());

		if (it == spam_score_of_ip.end())
		{
			spam_score_of_ip.insert(std::make_pair(ch->GetDesc()->GetHostName(), std::make_pair(0, (LPEVENT)NULL)));
			it = spam_score_of_ip.find(ch->GetDesc()->GetHostName());
		}

		if (it->second.second)
		{
			SendBlockChatInfo(ch, event_time(it->second.second) / passes_per_sec);
			return true;
		}

		unsigned int score;
		const char* word = SpamManager::instance().GetSpamScore(buf, buflen, score);

		it->second.first += score;

		if (word)
			sys_log(0, "SPAM_SCORE: %s text: %s score: %u total: %u word: %s", ch->GetName(), buf, score, it->second.first, word);

		extern unsigned int g_uiSpamBlockScore;
		extern unsigned int g_uiSpamBlockDuration;

		if (it->second.first >= g_uiSpamBlockScore)
		{
			spam_event_info* info = AllocEventInfo<spam_event_info>();
			strlcpy(info->host, ch->GetDesc()->GetHostName(), sizeof(info->host));

			it->second.second = event_create(block_chat_by_ip_event, info, PASSES_PER_SEC(g_uiSpamBlockDuration));
			sys_log(0, "SPAM_IP: %s for %u seconds", info->host, g_uiSpamBlockDuration);

			LogManager::instance().CharLog(ch, 0, "SPAM", word);

			SendBlockChatInfo(ch, event_time(it->second.second) / passes_per_sec);

			return true;
		}
	}

	return false;
}

enum
{
	TEXT_TAG_PLAIN,
	TEXT_TAG_TAG, // ||
	TEXT_TAG_COLOR, // |cffffffff
	TEXT_TAG_HYPERLINK_START, // |H
	TEXT_TAG_HYPERLINK_END, // |h ex) |Hitem:1234:1:1:1|h
	TEXT_TAG_RESTORE_COLOR,
};

int GetTextTag(const char* src, int maxLen, int& tagLen, std::string& extraInfo)
{
	tagLen = 1;

	if (maxLen < 2 || *src != '|')
		return TEXT_TAG_PLAIN;

	const char* cur = ++src;

	if (*cur == '|') // ||는 |로 표시한다.
	{
		tagLen = 2;
		return TEXT_TAG_TAG;
	}
	else if (*cur == 'c') // color |cffffffffblahblah|r
	{
		tagLen = 2;
		return TEXT_TAG_COLOR;
	}
	else if (*cur == 'H') // hyperlink |Hitem:10000:0:0:0:0|h[이름]|h
	{
		tagLen = 2;
		return TEXT_TAG_HYPERLINK_START;
	}
	else if (*cur == 'h') // end of hyperlink
	{
		tagLen = 2;
		return TEXT_TAG_HYPERLINK_END;
	}

	return TEXT_TAG_PLAIN;
}

void GetTextTagInfo(const char* src, int src_len, int& hyperlinks, bool& colored)
{
	colored = false;
	hyperlinks = 0;

	int len;
	std::string extraInfo;

	for (int i = 0; i < src_len;)
	{
		int tag = GetTextTag(&src[i], src_len - i, len, extraInfo);

		if (tag == TEXT_TAG_HYPERLINK_START)
			++hyperlinks;

		if (tag == TEXT_TAG_COLOR)
			colored = true;

		i += len;
	}
}

int ProcessTextTag(LPCHARACTER ch, const char* c_pszText, size_t len)
{
	// 2012.05.17 김용욱
	// 0 : 정상적으로 사용
	// 1 : 금강경 부족
	// 2 : 금강경이 있으나, 개인상점에서 사용중
	// 3 : 교환중
	// 4 : 에러

	int hyperlinks;
	bool colored;

	GetTextTagInfo(c_pszText, len, hyperlinks, colored);

	if (colored == true && hyperlinks == 0)
		return 4;

	if (ch->GetExchange())
	{
		if (hyperlinks == 0)
			return 0;
		else
			return 3;
	}

	int nPrismCount = ch->CountSpecifyItem(ITEM_PRISM);

	if (nPrismCount < hyperlinks)
		return 1;

	if (!ch->GetMyShop())
	{
		ch->RemoveSpecifyItem(ITEM_PRISM, hyperlinks);
		return 0;
	}
	else
	{
		int sellingNumber = ch->GetMyShop()->GetNumberByVnum(ITEM_PRISM);
		if (nPrismCount - sellingNumber < hyperlinks)
		{
			return 2;
		}
		else
		{
			ch->RemoveSpecifyItem(ITEM_PRISM, hyperlinks);
			return 0;
		}
	}

	return 4;
}

int CInputMain::Whisper(LPCHARACTER ch, const char* data, size_t uiBytes)
{
	const TPacketCGWhisper* pinfo = reinterpret_cast<const TPacketCGWhisper*>(data);

	if (uiBytes < pinfo->wSize)
		return -1;

	int iExtraLen = pinfo->wSize - sizeof(TPacketCGWhisper);

	if (iExtraLen < 0)
	{
		sys_err("invalid packet length (len %d size %u buffer %u)", iExtraLen, pinfo->wSize, uiBytes);
		ch->GetDesc()->SetPhase(PHASE_CLOSE);
		return -1;
	}

	if (ch->FindAffect(AFFECT_BLOCK_CHAT))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("채팅 금지 상태입니다."));
		return (iExtraLen);
	}

#if defined(ENABLE_ANTI_FLOOD)
	if (thecore_pulse() > ch->GetAntiFloodPulse(FLOOD_WHISPER) + PASSES_PER_SEC(1)) {
		ch->SetAntiFloodCount(FLOOD_WHISPER, 0);
		ch->SetAntiFloodPulse(FLOOD_WHISPER, thecore_pulse());
	}

	if (ch->IncreaseAntiFloodCount(FLOOD_WHISPER) >= 10) {
		sys_log(0, "WHISPER_HACK: %s", ch->GetName());
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Please do not spam the chat."));
		const LPDESC d = ch->GetDesc();
		if (d) {
			d->DelayedDisconnect(5);
		}

		return iExtraLen;
	}
#endif

	LPCHARACTER pkChr = CHARACTER_MANAGER::instance().FindPC(pinfo->szNameTo);

	if (pkChr == ch && !test_server)
		return iExtraLen;

	LPDESC pkDesc = NULL;
	BYTE bOpponentEmpire = 0;
	DWORD dwOpponentPID = 0;

	if (test_server)
	{
		if (!pkChr)
			sys_log(0, "Whisper to %s(%s) from %s", "Null", pinfo->szNameTo, ch->GetName());
		else
			sys_log(0, "Whisper to %s(%s) from %s", pkChr->GetName(), pinfo->szNameTo, ch->GetName());
	}

	if (ch->IsBlockMode(BLOCK_WHISPER))
	{
		if (ch->GetDesc())
		{
			TPacketGCWhisper pack;
			pack.bHeader = HEADER_GC_WHISPER;
			pack.bType = WHISPER_TYPE_SENDER_BLOCKED;
			pack.wSize = sizeof(TPacketGCWhisper);
			strlcpy(pack.szNameFrom, pinfo->szNameTo, sizeof(pack.szNameFrom));
			ch->GetDesc()->Packet(&pack, sizeof(pack));
		}
		return iExtraLen;
	}

	if (!pkChr)
	{
		CCI* pkCCI = P2P_MANAGER::instance().Find(pinfo->szNameTo);

		if (pkCCI)
		{
			pkDesc = pkCCI->pkDesc;
			pkDesc->SetRelay(pinfo->szNameTo);
			bOpponentEmpire = pkCCI->bEmpire;
			dwOpponentPID = pkCCI->dwPID;
			if (test_server)
				sys_log(0, "Whisper to %s from %s (Channel %d Mapindex %d)", "Null", ch->GetName(), pkCCI->bChannel, pkCCI->lMapIndex);
		}
	}
	else
	{
		pkDesc = pkChr->GetDesc();
		bOpponentEmpire = pkChr->GetEmpire();
		dwOpponentPID = pkChr->GetPlayerID();
	}

	if (!pkDesc)
	{
		if (ch->GetDesc())
		{
			TPacketGCWhisper pack;

			pack.bHeader = HEADER_GC_WHISPER;
			pack.bType = WHISPER_TYPE_NOT_EXIST;
			pack.wSize = sizeof(TPacketGCWhisper);
			strlcpy(pack.szNameFrom, pinfo->szNameTo, sizeof(pack.szNameFrom));
#ifdef ENABLE_WHISPER_RENEWAL
			char buf[CHAT_MAX_LEN + 1];
			strlcpy(buf, data + sizeof(TPacketCGWhisper), MIN(iExtraLen + 1, sizeof(buf)));
			if (!(std::string(buf).find("|?whisper_renewal>|") != std::string::npos || std::string(buf).find("|?whisper_renewal<|") != std::string::npos)) {
				ch->GetDesc()->Packet(&pack, sizeof(TPacketGCWhisper));
				sys_log(0, "WHISPER: no player");
			}
#else
			ch->GetDesc()->Packet(&pack, sizeof(TPacketGCWhisper));
			sys_log(0, "WHISPER: no player");
#endif
		}
	}
	else
	{
		if (ch->IsBlockMode(BLOCK_WHISPER))
		{
			if (ch->GetDesc())
			{
				TPacketGCWhisper pack;
				pack.bHeader = HEADER_GC_WHISPER;
				pack.bType = WHISPER_TYPE_SENDER_BLOCKED;
				pack.wSize = sizeof(TPacketGCWhisper);
				strlcpy(pack.szNameFrom, pinfo->szNameTo, sizeof(pack.szNameFrom));
				ch->GetDesc()->Packet(&pack, sizeof(pack));
			}
		}
		else if (pkChr && pkChr->IsBlockMode(BLOCK_WHISPER))
		{
			if (ch->GetDesc())
			{
				TPacketGCWhisper pack;
				pack.bHeader = HEADER_GC_WHISPER;
				pack.bType = WHISPER_TYPE_TARGET_BLOCKED;
				pack.wSize = sizeof(TPacketGCWhisper);
				strlcpy(pack.szNameFrom, pinfo->szNameTo, sizeof(pack.szNameFrom));
				ch->GetDesc()->Packet(&pack, sizeof(pack));
			}
		}
#if defined(__MESSENGER_BLOCK_SYSTEM__)
		else if (pkDesc && MessengerManager::instance().IsBlocked(ch->GetName(), pinfo->szNameTo))
		{
			if (ch->GetDesc())
			{
				TPacketGCWhisper pack;

				char msg[CHAT_MAX_LEN + 1];
				snprintf(msg, sizeof(msg), LC_TEXT("Unblock %s to continue."), pinfo->szNameTo);
				int len = MIN(CHAT_MAX_LEN, strlen(msg) + 1);

				pack.bHeader = HEADER_GC_WHISPER;
				pack.wSize = sizeof(TPacketGCWhisper) + len;
				pack.bType = WHISPER_TYPE_SYSTEM;
				strlcpy(pack.szNameFrom, pinfo->szNameTo, sizeof(pack.szNameFrom));

				TEMP_BUFFER buf;

				buf.write(&pack, sizeof(TPacketGCWhisper));
				buf.write(msg, len);
				ch->GetDesc()->Packet(buf.read_peek(), buf.size());

				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Unblock %s to continue."), pinfo->szNameTo);
			}
		}
		else if (pkDesc && MessengerManager::instance().IsBlocked(pinfo->szNameTo, ch->GetName()))
		{
			if (ch->GetDesc())
			{
				TPacketGCWhisper pack;

				char msg[CHAT_MAX_LEN + 1];
				snprintf(msg, sizeof(msg), LC_TEXT("%s has blocked you."), pinfo->szNameTo);
				int len = MIN(CHAT_MAX_LEN, strlen(msg) + 1);

				pack.bHeader = HEADER_GC_WHISPER;
				pack.wSize = sizeof(TPacketGCWhisper) + len;
				pack.bType = WHISPER_TYPE_SYSTEM;
				strlcpy(pack.szNameFrom, pinfo->szNameTo, sizeof(pack.szNameFrom));

				TEMP_BUFFER buf;

				buf.write(&pack, sizeof(TPacketGCWhisper));
				buf.write(msg, len);
				ch->GetDesc()->Packet(buf.read_peek(), buf.size());

#if defined(ENABLE_TEXT_PACKET)
				ch->ChatPacketNew(CHAT_TYPE_INFO, 15, "%s", pinfo->szNameTo);
#endif
			}
		}
#endif
		else
		{
			BYTE bType = WHISPER_TYPE_NORMAL;

			char buf[CHAT_MAX_LEN + 1];
			strlcpy(buf, data + sizeof(TPacketCGWhisper), MIN(iExtraLen + 1, sizeof(buf)));
			const size_t buflen = strlen(buf);

			//if (true == SpamBlockCheck(ch, buf, buflen))
			//{
			//	if (!pkChr)
			//	{
			//		CCI* pkCCI = P2P_MANAGER::instance().Find(pinfo->szNameTo);
			//
			//		if (pkCCI)
			//		{
			//			pkDesc->SetRelay("");
			//		}
			//	}
			//	return iExtraLen;
			//}

			//if (LC_IsCanada() == false)
			//{
			//	CBanwordManager::instance().ConvertString(buf, buflen);
			//}

			//if (g_bEmpireWhisper)
			//	if (!ch->IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_LANGUAGE))
			//		if (!(pkChr && pkChr->IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_LANGUAGE)))
			//			if (bOpponentEmpire != ch->GetEmpire() && ch->GetEmpire() && bOpponentEmpire // 서로 제국이 다르면서
			//				&& ch->GetGMLevel() == GM_PLAYER && gm_get_level(pinfo->szNameTo) == GM_PLAYER) // 둘다 일반 플레이어이면
			//			// 이름 밖에 모르니 gm_get_level 함수를 사용
			//			{
			//				if (!pkChr)
			//				{
			//					// 다른 서버에 있으니 제국 표시만 한다. bType의 상위 4비트를 Empire번호로 사용한다.
			//					bType = ch->GetEmpire() << 4;
			//				}
			//				else
			//				{
			//					ConvertEmpireText(ch->GetEmpire(), buf, buflen, 10 + 2 * pkChr->GetSkillPower(SKILL_LANGUAGE1 + ch->GetEmpire() - 1) /* 변환확률 */);
			//				}
			//			}

			if (!g_bDisableGlassInsight)
			{
				int processReturn = ProcessTextTag(ch, buf, buflen);
				if (processReturn != 0)
				{
					if (ch->GetDesc())
					{
						TItemTable* pTable = ITEM_MANAGER::instance().GetTable(ITEM_PRISM);

						if (pTable)
						{
							std::string msg;

							try {
								if (processReturn == 3) {
									msg = LC_TEXT("다른 거래중(창고,교환,상점)에는 개인상점을 사용할 수 없습니다.");
								} else {
									msg = fmt::sprintf(LC_TEXT("%s 아이템이 필요합니다"),
														LC_LOCALE_ITEM_TEXT(ITEM_PRISM, ch->GetLanguage()));
								}
							} catch (const std::exception &e) {
								sys_err("%s", e.what());
							}

							TPacketGCWhisper pack;

							pack.bHeader = HEADER_GC_WHISPER;
							pack.bType = WHISPER_TYPE_ERROR;
							pack.wSize = sizeof(TPacketGCWhisper) + msg.length();
							strlcpy(pack.szNameFrom, pinfo->szNameTo, sizeof(pack.szNameFrom));

							ch->GetDesc()->BufferedPacket(&pack, sizeof(pack));
							ch->GetDesc()->Packet(msg.c_str(), msg.length());

							sys_log(0, "WHISPER: not enough %s: char: %s", pTable->szLocaleName, ch->GetName());
						}
					}

					// 릴래이 상태일 수 있으므로 릴래이를 풀어준다.
					pkDesc->SetRelay("");
					return (iExtraLen);
				}
			}

			if (ch->IsGM())
				bType = (bType & 0xF0) | WHISPER_TYPE_GM;

			if (buflen > 0)
			{
				TPacketGCWhisper pack;

				pack.bHeader = HEADER_GC_WHISPER;
				pack.wSize = sizeof(TPacketGCWhisper) + buflen;
				pack.bType = bType;
				strlcpy(pack.szNameFrom, ch->GetName(), sizeof(pack.szNameFrom));

				// desc->BufferedPacket을 하지 않고 버퍼에 써야하는 이유는 
				// P2P relay되어 패킷이 캡슐화 될 수 있기 때문이다.
				TEMP_BUFFER tmpbuf;

				tmpbuf.write(&pack, sizeof(pack));
				tmpbuf.write(buf, buflen);

				pkDesc->Packet(tmpbuf.read_peek(), tmpbuf.size());

				if (test_server)
					sys_log(0, "WHISPER: %s -> %s : %s", ch->GetName(), pinfo->szNameTo, buf);

				if (g_bWhisperLog)
					LogManager::instance().WhisperLog(ch->GetPlayerID(), dwOpponentPID, buf);
			}
		}
	}
	if (pkDesc)
		pkDesc->SetRelay("");

	return (iExtraLen);
}

struct RawPacketToCharacterFunc
{
	const void* m_buf;
	int m_buf_len;

	RawPacketToCharacterFunc(const void* buf, int buf_len) : m_buf(buf), m_buf_len(buf_len)
	{
	}

	void operator () (LPCHARACTER c)
	{
		if (!c->GetDesc())
			return;

		c->GetDesc()->Packet(m_buf, m_buf_len);
	}
};

struct FEmpireChatPacket
{
	packet_chat& p;
	const char* orig_msg;
	int orig_len;
	char converted_msg[CHAT_MAX_LEN + 1];

	BYTE bEmpire;
	int iMapIndex;
	int namelen;

	FEmpireChatPacket(packet_chat& p, const char* chat_msg, int len, BYTE bEmpire, int iMapIndex, int iNameLen)
		: p(p), orig_msg(chat_msg), orig_len(len), bEmpire(bEmpire), iMapIndex(iMapIndex), namelen(iNameLen)
	{
		memset(converted_msg, 0, sizeof(converted_msg));
	}

	void operator () (LPDESC d)
	{
		if (!d->GetCharacter())
			return;

		if (d->GetCharacter()->GetMapIndex() != iMapIndex)
			return;

		d->BufferedPacket(&p, sizeof(packet_chat));

		if (d->GetEmpire() == bEmpire ||
			bEmpire == 0 ||
			d->GetCharacter()->GetGMLevel() > GM_PLAYER ||
			d->GetCharacter()->IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_LANGUAGE))
		{
			d->Packet(orig_msg, orig_len);
		}
		else
		{
			// 사람마다 스킬레벨이 다르니 매번 해야합니다
			size_t len = strlcpy(converted_msg, orig_msg, sizeof(converted_msg));

			if (len >= sizeof(converted_msg))
				len = sizeof(converted_msg) - 1;

			ConvertEmpireText(bEmpire, converted_msg + namelen, len - namelen, 10 + 2 * d->GetCharacter()->GetSkillPower(SKILL_LANGUAGE1 + bEmpire - 1));
			d->Packet(converted_msg, orig_len);
		}
	}
};

struct FYmirChatPacket
{
	packet_chat& packet;
	const char* m_szChat;
	size_t m_lenChat;
	const char* m_szName;

	int m_iMapIndex;
	BYTE m_bEmpire;
	bool m_ring;

	char m_orig_msg[CHAT_MAX_LEN + 1];
	int m_len_orig_msg;
	char m_conv_msg[CHAT_MAX_LEN + 1];
	int m_len_conv_msg;

	FYmirChatPacket(packet_chat& p, const char* chat, size_t len_chat, const char* name, size_t len_name, int iMapIndex, BYTE empire, bool ring)
		: packet(p),
		m_szChat(chat), m_lenChat(len_chat),
		m_szName(name),
		m_iMapIndex(iMapIndex), m_bEmpire(empire),
		m_ring(ring)
	{
		m_len_orig_msg = snprintf(m_orig_msg, sizeof(m_orig_msg), "%s : %s", m_szName, m_szChat) + 1; // 널 문자 포함

		if (m_len_orig_msg < 0 || m_len_orig_msg >= (int)sizeof(m_orig_msg))
			m_len_orig_msg = sizeof(m_orig_msg) - 1;

		m_len_conv_msg = snprintf(m_conv_msg, sizeof(m_conv_msg), "??? : %s", m_szChat) + 1; // 널 문자 미포함

		if (m_len_conv_msg < 0 || m_len_conv_msg >= (int)sizeof(m_conv_msg))
			m_len_conv_msg = sizeof(m_conv_msg) - 1;

		ConvertEmpireText(m_bEmpire, m_conv_msg + 6, m_len_conv_msg - 6, 10); // 6은 "??? : "의 길이
	}

	void operator() (LPDESC d)
	{
		if (!d->GetCharacter())
			return;

		if (d->GetCharacter()->GetMapIndex() != m_iMapIndex)
			return;

		if (m_ring ||
			d->GetEmpire() == m_bEmpire ||
			d->GetCharacter()->GetGMLevel() > GM_PLAYER ||
			d->GetCharacter()->IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_LANGUAGE))
		{
			packet.size = m_len_orig_msg + sizeof(TPacketGCChat);

			d->BufferedPacket(&packet, sizeof(packet_chat));
			d->Packet(m_orig_msg, m_len_orig_msg);
		}
		else
		{
			packet.size = m_len_conv_msg + sizeof(TPacketGCChat);

			d->BufferedPacket(&packet, sizeof(packet_chat));
			d->Packet(m_conv_msg, m_len_conv_msg);
		}
	}
};


int CInputMain::Chat(LPCHARACTER ch, const char* data, size_t uiBytes) {
    const TPacketCGChat* pinfo = reinterpret_cast<const TPacketCGChat*>(data);

    if (uiBytes < pinfo->size)
        return -1;

    const int iExtraLen = pinfo->size - sizeof(TPacketCGChat);

    if (iExtraLen < 0) {
        sys_err("invalid packet length (len %d size %u buffer %u)", iExtraLen, pinfo->size, uiBytes);
        ch->GetDesc()->SetPhase(PHASE_CLOSE);
        return -1;
    }

    if (quest::CQuestManager::instance().GetEventFlag("ox_chat_block") == 1) {
        if (!ch->IsGM()) {
            if (ch->GetMapIndex() == 113 && (pinfo->type == CHAT_TYPE_TALKING || pinfo->type == CHAT_TYPE_PARTY || pinfo->type == CHAT_TYPE_GUILD || pinfo->type == CHAT_TYPE_SHOUT)) {
                ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ox_chat_block"));
                return iExtraLen;
            }
        }
    }

    char buf[CHAT_MAX_LEN - (CHARACTER_NAME_MAX_LEN + 3) + 1];
    strlcpy(buf, data + sizeof(TPacketCGChat), MIN(iExtraLen + 1, sizeof(buf)));
    const size_t buflen = strlen(buf);

    if (buflen > 1 && *buf == '/') {
        interpret_command(ch, buf + 1, buflen - 1);
        return iExtraLen;
    }

#if defined(ENABLE_ANTI_FLOOD)
    if (thecore_pulse() > ch->GetAntiFloodPulse(FLOOD_CHAT) + PASSES_PER_SEC(1)) {
        ch->SetAntiFloodCount(FLOOD_CHAT, 0);
        ch->SetAntiFloodPulse(FLOOD_CHAT, thecore_pulse());
    }

    if (ch->IncreaseAntiFloodCount(FLOOD_CHAT) >= 10) {
        sys_log(0, "CHAT_HACK: %s", ch->GetName());
        ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Please do not spam the chat."));
        if (!ch->FindAffect(AFFECT_BLOCK_CHAT)) {
            ch->AddAffect(AFFECT_BLOCK_CHAT, POINT_NONE, 0, AFF_NONE, 10, 0, true);
        }

        return iExtraLen;
    }
#endif

    if (g_bChatWhisper) {
        std::string findstr(buf);
        if (findstr.at(0) == '@') {
            findstr = findstr.substr(1); // or findstr.erase(0, 1);
            int pos = findstr.find(" ");
            std::string message = findstr.substr(pos + 1);
            std::string chname = findstr.substr(0, pos);

            if (!check_name(chname.c_str()) || chname.length() > GUILD_NAME_MAX_LEN) {
                ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("그런 사람이 없습니다."));
                return iExtraLen;
            }

            LPCHARACTER stch = CHARACTER_MANAGER::instance().FindPC(chname.c_str());

            if (!stch) {
                ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s is not around."), chname.c_str());
                return iExtraLen;
            }

            if (stch != ch) {
#if defined(__MESSENGER_BLOCK_SYSTEM__)
                if (stch && MessengerManager::instance().IsBlocked(ch->GetName(), stch->GetName())) {
                    ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Unblock %s to continue."), stch->GetName());
                    return iExtraLen;
                }

                if (stch && MessengerManager::instance().IsBlocked(stch->GetName(), ch->GetName())) {
#if defined(ENABLE_TEXT_PACKET)
                    ch->ChatPacketNew(CHAT_TYPE_INFO, 15, "%s", stch->GetName());
#endif
                    return iExtraLen;
                }
#endif
                ch->ChatPacket(CHAT_TYPE_WHISPER, "%s -> %s : %s", ch->GetName(), stch->GetName(), message.c_str());
                stch->ChatPacket(CHAT_TYPE_WHISPER, "%s : %s", ch->GetName(), message.c_str());
            } else
                ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot whisper to yourself."));

            return iExtraLen;
        }
    }

    const CAffect* pAffect = ch->FindAffect(AFFECT_BLOCK_CHAT);

    if (pAffect != NULL) {
        SendBlockChatInfo(ch, pAffect->lDuration);
        return iExtraLen;
    }

    if (true == SpamBlockCheck(ch, buf, buflen)) {
        return iExtraLen;
    }

    if (CHAT_TYPE_SHOUT == pinfo->type && g_bDisableShout && !ch->IsGM()) {
        ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The shout is disabled."));
        return iExtraLen;
    }

    char chatbuf[CHAT_MAX_LEN + 1];
    int len;

    if (pinfo->type == CHAT_TYPE_SHOUT) {
        std::string levelInfo;

        // Check for Conqueror Level
        if (ch->GetConquerorLevel() > 0) {
		levelInfo = "Con." + std::to_string(ch->GetConquerorLevel());
        } else {
            levelInfo = "Lv." + std::to_string(ch->GetLevel());
        }

        // Use the utility function to get the colored localized text
        std::string strInfo = GetColoredLocalizedText(ch->GetGMLevel(), ch->GetEmpire());

        len = snprintf(chatbuf, sizeof(chatbuf), "%s |Hmsg:%s|h %s %s|h|r : %s", strInfo.c_str(), ch->GetName(), levelInfo.c_str(), ch->GetName(), buf);
    } else {
        len = snprintf(chatbuf, sizeof(chatbuf), "Lv.%d %s : %s", ch->GetLevel(), ch->GetName(), buf);
    }

    // Ensure we don't exceed the buffer size
    if (len < 0 || len >= static_cast<int>(sizeof(chatbuf))) {
        len = sizeof(chatbuf) - 1;
    }

    CBanwordManager::instance().ConvertString(chatbuf, len);

    if (!g_bDisableGlassInsight) {
        int processReturn = ProcessTextTag(ch, chatbuf, len);
        if (processReturn != 0) {
            const TItemTable* pTable = ITEM_MANAGER::instance().GetTable(ITEM_PRISM);

            if (NULL != pTable) {
                if (3 == processReturn) //교환중
                    ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("다른 거래중(창고,교환,상점)에는 개인상점을 사용할 수 없습니다."));
                else {
                    ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s 아이템이 필요합니다"), LC_LOCALE_ITEM_TEXT(ITEM_PRISM, ch->GetLanguage()));
                }
            }

            return iExtraLen;
        }
    }

    if (pinfo->type == CHAT_TYPE_SHOUT) {
        const int SHOUT_LIMIT_LEVEL = g_iUseLocale ? 15 : 3;

        if (ch->GetLevel() < SHOUT_LIMIT_LEVEL) {
            ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("외치기는 레벨 %d 이상만 사용 가능 합니다."), SHOUT_LIMIT_LEVEL);
            return iExtraLen;
        }

        if (thecore_heart->pulse - static_cast<int>(ch->GetLastShoutPulse()) < passes_per_sec * 15 && !ch->IsGM()) {
            ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can only call every 15 seconds."));
            return iExtraLen;
        }

        ch->SetLastShoutPulse(thecore_heart->pulse);

        if (g_bGlobalShout) {
            TPacketGGShout p;
            p.bHeader = HEADER_GG_SHOUT;
            p.bEmpire = ch->GetEmpire();
#if defined(__MESSENGER_BLOCK_SYSTEM__)
            strlcpy(p.szName, ch->GetName(), sizeof(p.szName));
#endif
            strlcpy(p.szText, chatbuf, sizeof(p.szText));

            P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGShout));
#if defined(__MESSENGER_BLOCK_SYSTEM__)
            SendShout(chatbuf, ch->GetName(), ch->GetEmpire());
#else
            SendShout(chatbuf, ch->GetEmpire());
#endif
        } else {
            TPacketGGShout p;
            p.bHeader = HEADER_GG_SHOUT;
            p.bEmpire = ch->GetEmpire();
            strlcpy(p.szText, chatbuf, sizeof(p.szText));

            P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGShout));

#if defined(__MESSENGER_BLOCK_SYSTEM__)
            SendShout(chatbuf, ch->GetName(), ch->GetEmpire());
#else
            SendShout(chatbuf, ch->GetEmpire());
#endif
        }

        return iExtraLen;
    }

    TPacketGCChat pack_chat;
    pack_chat.header = HEADER_GC_CHAT;
    pack_chat.size = sizeof(TPacketGCChat) + len;
    pack_chat.type = pinfo->type;
    pack_chat.id = ch->GetVID();

    switch (pinfo->type) {
        case CHAT_TYPE_TALKING: {
            const DESC_MANAGER::DESC_SET& c_ref_set = DESC_MANAGER::instance().GetClientSet();

            if (false) {
                std::for_each(c_ref_set.begin(), c_ref_set.end(),
                              FYmirChatPacket(pack_chat,
                                              buf,
                                              strlen(buf),
                                              ch->GetName(),
                                              strlen(ch->GetName()),
                                              ch->GetMapIndex(),
                                              ch->GetEmpire(),
                                              ch->IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_LANGUAGE)));
            } else {
                std::for_each(c_ref_set.begin(), c_ref_set.end(),
                              FEmpireChatPacket(pack_chat,
                                                chatbuf,
                                                len,
                                                (ch->GetGMLevel() > GM_PLAYER ||
                                                 ch->IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_LANGUAGE)) ? 0 : ch->GetEmpire(),
                                                ch->GetMapIndex(), strlen(ch->GetName())));
            }
        }
            break;

        case CHAT_TYPE_PARTY: {
            if (!ch->GetParty())
                ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("파티 중이 아닙니다."));
            else {
                TEMP_BUFFER tbuf;
                tbuf.write(&pack_chat, sizeof(pack_chat));
                tbuf.write(chatbuf, len);
                RawPacketToCharacterFunc f(tbuf.read_peek(), tbuf.size());
                ch->GetParty()->ForEachOnlineMember(f);
            }
        }
            break;

        case CHAT_TYPE_GUILD: {
            if (!ch->GetGuild())
                ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("길드에 가입하지 않았습니다."));
            else
                ch->GetGuild()->Chat(chatbuf);
        }
            break;

        default:
            sys_err("Unknown chat type %d", pinfo->type);
            break;
    }

    return iExtraLen;
}

void CInputMain::ItemUse(LPCHARACTER ch, const char* data)
{
	ch->UseItem(((struct command_item_use*)data)->Cell);
}

#if defined(ENABLE_MULTI_ITEM_USE)
void CInputMain::MultiItemUse(LPCHARACTER ch, const char* data)
{
	TPacketCGMultiItemUse* p = (TPacketCGMultiItemUse*)data;
	ch->MultiUseItem(p->count, p->pos);
}
#endif

void CInputMain::ItemToItem(LPCHARACTER ch, const char* pcData)
{
	TPacketCGItemUseToItem* p = (TPacketCGItemUseToItem*)pcData;
	if (ch)
		ch->UseItem(p->Cell, p->TargetCell);
}

void CInputMain::ItemDrop(LPCHARACTER ch, const char* data)
{
	struct command_item_drop* pinfo = (struct command_item_drop*)data;

	// MONARCH_LIMIT
	//if (ch->IsMonarch())
	//	return;
	// END_MONARCH_LIMIT
	if (!ch)
		return;

	// 엘크가 0보다 크면 엘크를 버리는 것 이다.
	if (pinfo->gold > 0)
		ch->DropGold(pinfo->gold);
#if defined(ENABLE_CHEQUE_SYSTEM)
	else if (pinfo->cheque > 0)
		ch->DropCheque(pinfo->cheque);
#endif
	else
		ch->DropItem(pinfo->Cell);
}

void CInputMain::ItemDrop2(LPCHARACTER ch, const char* data)
{
	// MONARCH_LIMIT
	//if (ch->IsMonarch())
	//	return;
	// END_MONARCH_LIMIT

	TPacketCGItemDrop2* pinfo = (TPacketCGItemDrop2*)data;

	// 엘크가 0보다 크면 엘크를 버리는 것 이다.

	if (!ch)
		return;
	if (pinfo->gold > 0)
		ch->DropGold(pinfo->gold);
#if defined(ENABLE_CHEQUE_SYSTEM)
	else if (pinfo->cheque > 0)
		ch->DropCheque(pinfo->cheque);
#endif
	else
		ch->DropItem(pinfo->Cell, pinfo->count);
}

#ifdef ENABLE_SELL_ITEM
void CInputMain::ItemSell(LPCHARACTER ch, const char * data)
{
	TPacketCGItemSell * pinfo = (TPacketCGItemSell *) data;
	
	if (!ch)
		return;

	if (pinfo->gold > 0)
		ch->DropGold(pinfo->gold);
	else
		ch->SellItem(pinfo->Cell);
}
#endif

#if defined(__NEW_DROP_DIALOG__)
void CInputMain::ItemDestroy(LPCHARACTER ch, const char* data)
{
	struct command_item_destroy* pinfo = (struct command_item_destroy*)data;
	if (ch)
		ch->DestroyItem(pinfo->Cell);
}
#endif

#if defined(__PRIVATE_SHOP_SEARCH_SYSTEM__)
void CInputMain::ShopSearch(LPCHARACTER ch, const char* data, bool bName)
{
	TPacketCGShopSearch* pinfo = (TPacketCGShopSearch*)data;

	if (!ch)
		return;

	if (!data)
		return;

	if (ch->IsOpenSafebox() || ch->GetShop() || ch->IsCubeOpen() || ch->IsDead() || ch->GetExchange() || ch->GetMyShop())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("CANT_DO_THIS_BECAUSE_OTHER_WINDOW_OPEN"));
		return;
	}

#if defined(__PRIVATE_SHOP_SEARCH_NEED_ITEM__)
	if (!ch->FindSpecifyItem(60004) && !ch->FindSpecifyItem(60005))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("PRIVATE_SHOP_SEARCH_NEED_ITEM"));
		return;
	}
#endif

	int32_t Race = pinfo->Race;
	int32_t Cat = pinfo->ItemCat;
	int32_t SubCat = pinfo->SubCat;
	int32_t MinLevel = pinfo->MinLevel;
	int32_t MaxLevel = pinfo->MaxLevel;
	int32_t MinRefine = pinfo->MinRefine;
	int32_t MaxRefine = pinfo->MaxRefine;
	uint64_t MinGold = pinfo->MinGold;
	uint64_t MaxGold = pinfo->MaxGold;
#if defined(ENABLE_CHEQUE_SYSTEM)
	uint64_t MinCheque = pinfo->MinCheque;
	uint64_t MaxCheque = pinfo->MaxCheque;
#endif
	std::string itemName = "";

	//Checks
	if (Race < JOB_WARRIOR || Race > JOB_WOLFMAN)
		return;

	if (Cat < ITEM_NONE || Cat > ITEM_SOUL) // @ enum EItemTypes common/item_length.h
		return;

	if (SubCat < USE_POTION || SubCat > USE_EMOTION_PACK)
		return;

	if (MinLevel < 0 || MinLevel > 999 /*PLAYER_MAX_LEVEL_CONST*/)
		return;

	if (MaxLevel < 0 || MaxLevel > 999 /*PLAYER_MAX_LEVEL_CONST*/)
		return;

	if (MinRefine < 0 || MinRefine > 999)
		return;

	if (MaxRefine < 0 || MaxRefine > 999)
		return;

	if (MinGold < 0 || MinGold > g_MaxGold)
		return;

	if (MaxGold < 0 || MaxGold > g_MaxGold)
		return;

#if defined(ENABLE_CHEQUE_SYSTEM)
	if (MinCheque < 0 || MinCheque > CHEQUE_MAX)
		return;

	if (MaxCheque < 0 || MaxCheque > CHEQUE_MAX)
		return;
#endif

	if (MinLevel > MaxLevel)
		return;

	if (MinRefine > MaxRefine)
		return;

	if (MinGold > MaxGold)
		return;

#if defined(ENABLE_CHEQUE_SYSTEM)
	if (MinCheque > MaxCheque)
		return;
#endif

	if (bName)
	{
		itemName = pinfo->ItemName;
		std::replace(itemName.begin(), itemName.end(), '_', ' ');
	}

	quest::PC* pPC = quest::CQuestManager::instance().GetPC(ch->GetPlayerID());

	if (!pPC)
		return;

	DWORD dwShopSearchSecCycle = 2; // 1 sec
	DWORD dwNowSec = get_global_time();
	DWORD dwLastShopSearchAttrSec = pPC->GetFlag("ShopSearch.LastShopSearchSecAttr");

	if (dwLastShopSearchAttrSec + dwShopSearchSecCycle > dwNowSec)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("PRIVATE_SHOP_SEARCH_NEED_WAIT_%d_LEFT_(%d)"),
			dwShopSearchSecCycle, dwShopSearchSecCycle - (dwNowSec - dwLastShopSearchAttrSec));
		return;
	}

	pPC->SetFlag("ShopSearch.LastShopSearchSecAttr", dwNowSec);

	std::vector<COfflineShop::OFFLINE_SHOP_ITEM> sendItems;
	std::vector<COfflineShop::OFFLINE_SHOP_ITEM> shop_items;
	std::vector<COfflineShop::OFFLINE_SHOP_ITEM>::iterator item;

	char szQuery[1024];
	if (bName)
	{
		sprintf(szQuery, "select of.*,(case when of.vnum in(50300,70037) then (select concat(convert(sp.szName using latin5),' ',convert(ip.locale_name using latin5)) from skill_proto sp where sp.dwVnum = of.socket0) else convert(ip.locale_name using latin5) end) as item_name from offline_shop_item of inner join item_proto ip on ip.vnum = of.vnum where (case when of.vnum in(50300,70037) then (select concat(convert(sp.szName using latin5),' ',convert(ip.locale_name using latin5)) from skill_proto sp where sp.dwVnum = of.socket0) else convert(ip.locale_name using latin5) end) like CONCAT('%%%%', '%s', '%%%%')", itemName.c_str());
	}
	else
	{
		sprintf(szQuery, "select of.*,(case when of.vnum in(50300,70037) then (select concat(convert(sp.szName using latin5),' ',convert(ip.locale_name using latin5)) from skill_proto sp where sp.dwVnum = of.socket0) else convert(ip.locale_name using latin5) end) as item_name from offline_shop_item of inner join item_proto ip on ip.vnum = of.vnum");
	}

	std::unique_ptr<SQLMsg> pSearchQuery(DBManager::instance().DirectQuery(szQuery));
	if (pSearchQuery->uiSQLErrno != 0)
	{
		sys_err("Item Search Query Failed, Error code: %ld", pSearchQuery->uiSQLErrno);
		return;
	}

	if (!pSearchQuery->Get()->uiNumRows)
		return;

	int i = 0;
	while (MYSQL_ROW row = mysql_fetch_row(pSearchQuery->Get()->pSQLResult))
	{
		shop_items.resize(i + 1);

		const TItemTable* item_table;

		DWORD vnum = 0;
		str_to_number(vnum, row[4]);

		item_table = ITEM_MANAGER::instance().GetTable(vnum);

		if (!item_table)
		{
			sys_err("OfflineShop: no item table by item vnum #%d", vnum);
			continue;
		}

		COfflineShop::OFFLINE_SHOP_ITEM& item = shop_items[i];
		str_to_number(item.id, row[0]);									// 0
		str_to_number(item.owner_id, row[1]);							// 1
		str_to_number(item.pos, row[2]);								// 2
		str_to_number(item.count, row[3]);								// 3
		str_to_number(item.vnum, row[4]);								// 4
		str_to_number(item.alSockets[0], row[5]);						// 5
		str_to_number(item.alSockets[1], row[6]);						// 6
		str_to_number(item.alSockets[2], row[7]);						// 7
#if defined(ENABLE_ITEM_SOCKET5)
		str_to_number(item.alSockets[3], row[8]);						// 8
		str_to_number(item.alSockets[4], row[9]);						// 9
		str_to_number(item.alSockets[5], row[10]);						// 10
#endif
		str_to_number(item.aAttr[0].bType, row[11]);					// 11 ~ 8
		str_to_number(item.aAttr[0].sValue, row[12]);					// 12 ~ 9
		str_to_number(item.aAttr[1].bType, row[13]);					// 13 ~ 10
		str_to_number(item.aAttr[1].sValue, row[14]);					// 14 ~ 11
		str_to_number(item.aAttr[2].bType, row[15]);					// 15 ~ 12
		str_to_number(item.aAttr[2].sValue, row[16]);					// 16 ~ 13
		str_to_number(item.aAttr[3].bType, row[17]);					// 17 ~ 14
		str_to_number(item.aAttr[3].sValue, row[18]);					// 18 ~ 15
		str_to_number(item.aAttr[4].bType, row[19]);					// 19 ~ 16
		str_to_number(item.aAttr[4].sValue, row[20]);					// 20 ~ 17
		str_to_number(item.aAttr[5].bType, row[21]);					// 21 ~ 18
		str_to_number(item.aAttr[5].sValue, row[22]);					// 22 ~ 19
		str_to_number(item.aAttr[6].bType, row[23]);					// 23 ~ 20
		str_to_number(item.aAttr[6].sValue, row[24]);					// 24 ~ 21
		str_to_number(item.aAttr[7].bType, row[25]);					// 25 ~ 22
		str_to_number(item.aAttr[7].sValue, row[26]);					// 26 ~ 23
		str_to_number(item.aAttr[8].bType, row[27]);					// 27 ~ 24
		str_to_number(item.aAttr[8].sValue, row[28]);					// 28 ~ 25
		str_to_number(item.aAttr[9].bType, row[29]);					// 29 ~ 26
		str_to_number(item.aAttr[9].sValue, row[30]);					// 30 ~ 27
		str_to_number(item.aAttr[10].bType, row[31]);					// 31 ~ 28
		str_to_number(item.aAttr[10].sValue, row[32]);					// 32 ~ 29
		str_to_number(item.aAttr[11].bType, row[33]);					// 33 ~ 30
		str_to_number(item.aAttr[11].sValue, row[34]);					// 34 ~ 31
		str_to_number(item.aAttr[12].bType, row[35]);					// 35 ~ 32
		str_to_number(item.aAttr[12].sValue, row[36]);					// 36 ~ 33
		str_to_number(item.aAttr[13].bType, row[37]);					// 37 ~ 34
		str_to_number(item.aAttr[13].sValue, row[38]);					// 38 ~ 35
		str_to_number(item.aAttr[14].bType, row[39]);					// 39 ~ 36
		str_to_number(item.aAttr[14].sValue, row[40]);					// 40 ~ 37
		str_to_number(item.price, row[41]);								// 41 ~ 38
#if defined(ENABLE_CHEQUE_SYSTEM)
		str_to_number(item.price_cheque, row[42]);						// 42 ~ 39
#endif
		str_to_number(item.status, row[43]);							// 43 ~ 40
		strlcpy(item.szBuyerName, row[44], sizeof(item.szBuyerName));	// 44 ~ 41
#if defined(__CHANGE_LOOK_SYSTEM__)
		str_to_number(item.transmutation, row[45]);						// 45 ~ 42
#endif

		strlcpy(item.szName, row[46], sizeof(item.szName));				// 46 ~ 43
		str_to_number(item.refine_level, row[47]);						// 47 ~ 44

		i++;
	}

	for (item = shop_items.begin(); item != shop_items.end(); ++item)
	{
		bool bLimit = false;

		if (item->status != 0 || item->vnum == 0)
			continue;

		TItemTable* itemTable = ITEM_MANAGER::instance().GetTable(item->vnum);

		if (!itemTable)
			continue;

		if (!bName)
		{
			if (itemTable->bType == Cat && (itemTable->bSubType == SubCat || itemTable->bSubType == 99))
			{
				if (itemTable->bType == ITEM_WEAPON || itemTable->bType == ITEM_ARMOR)
				{
					if (!(item->refine_level >= MinRefine && item->refine_level <= MaxRefine))
						continue;
				}
			}
			else
			{
				continue;
			}
		}

		for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
		{
			long lLimit = itemTable->aLimits[i].lValue;
			switch (itemTable->aLimits[i].bType)
			{
			case LIMIT_LEVEL:
				if (!(lLimit >= MinLevel && lLimit <= MaxLevel))
					bLimit = true;
				break;
			case LIMIT_REAL_TIME:
				if (lLimit == 0)
					bLimit = true;
				break;
			}
		}

		if (bLimit)
			continue;

		if (!(item->price >= MinGold && item->price <= MaxGold))
			continue;

#if defined(ENABLE_CHEQUE_SYSTEM)
		if (!(item->price_cheque >= MinCheque && item->price_cheque <= MaxCheque))
			continue;
#endif

		bool skillBook = false;
		DWORD bkType = 0;

		if ((itemTable->bType == ITEM_SKILLBOOK) || (itemTable->bType == ITEM_SKILLFORGET))
		{
			CSkillProto* pkSk = CSkillManager::instance().Get(item->alSockets[0]);
			bkType = pkSk->dwType;
			skillBook = false;
		}

		bool cont = false;
		if (skillBook)
		{
			switch (Race)
			{
			case JOB_WARRIOR:
				if (bkType != JOB_WARRIOR + 1)
				{
					cont = true;
				}
				break;
			case JOB_ASSASSIN:
				if (bkType != JOB_ASSASSIN + 1)
				{
					cont = true;
				}
				break;
			case JOB_SURA:
				if (bkType != JOB_SURA + 1)
				{
					cont = true;
				}
				break;
			case JOB_SHAMAN:
				if (bkType != JOB_SHAMAN + 1)
				{
					cont = true;
				}
				break;
			case JOB_WOLFMAN:
				if (bkType != JOB_WOLFMAN + 1)
				{
					cont = true;
				}
				break;
			}
		}
		else
		{
			switch (Race)
			{
			case JOB_WARRIOR:
				if (IS_SET(itemTable->dwAntiFlags, ITEM_ANTIFLAG_WARRIOR))
					cont = true;
				break;

			case JOB_ASSASSIN:
				if (IS_SET(itemTable->dwAntiFlags, ITEM_ANTIFLAG_ASSASSIN))
					cont = true;
				break;

			case JOB_SURA:
				if (IS_SET(itemTable->dwAntiFlags, ITEM_ANTIFLAG_SURA))
					cont = true;
				break;

			case JOB_SHAMAN:
				if (IS_SET(itemTable->dwAntiFlags, ITEM_ANTIFLAG_SHAMAN))
					cont = true;
				break;

			case JOB_WOLFMAN:
				if (IS_SET(itemTable->dwAntiFlags, ITEM_ANTIFLAG_WOLFMAN))
					cont = true;
				break;

			}
		}

		if (cont)
			continue;

		sendItems.push_back(*item);
	}

	std::stable_sort(sendItems.begin(), sendItems.end(), CompareItemVnumAcPriceAC);

	for (item = sendItems.begin(); item != sendItems.end(); ++item)
	{
		if (item->status != 0 || item->vnum == 0)
			continue;

		TItemTable* itemTable = ITEM_MANAGER::instance().GetTable(item->vnum);

		LPCHARACTER offlineShopNpc = CHARACTER_MANAGER::instance().Find(COfflineShopManager::instance().FindMyOfflineShop(item->owner_id));
		if (!offlineShopNpc || !pPC)
			return;

		if (itemTable)
		{
			LPDESC d = ch->GetDesc();

			if (!d)
				return;

			TPacketGCShopSearchItemSet pack;
			pack.header = HEADER_GC_SHOPSEARCH_SET;

			pack.count = static_cast<BYTE>(item->count);
			pack.vnum = item->vnum;
			pack.flags = itemTable->dwFlags;
			pack.anti_flags = itemTable->dwAntiFlags;
			pack.price = item->price;
#if defined(ENABLE_CHEQUE_SYSTEM)
			pack.price_cheque = item->price_cheque;
#endif
			pack.vid = offlineShopNpc->GetVID();
#if defined(__CHANGE_LOOK_SYSTEM__)
			pack.transmutation = item->transmutation;
#endif
			pack.Cell = item->pos;

			thecore_memcpy(pack.alSockets, item->alSockets, sizeof(pack.alSockets));
			thecore_memcpy(pack.aAttr, item->aAttr, sizeof(pack.aAttr));

			d->LargePacket(&pack, sizeof(TPacketGCShopSearchItemSet));
		}
	}
}

void CInputMain::ShopSearchBuy(LPCHARACTER ch, const char* data)
{
	TPacketCGShopSearchBuy* pinfo = (TPacketCGShopSearchBuy*)data;

	int32_t shopVid = pinfo->shopVid;
	int32_t shopItemPos = pinfo->shopItemPos;

	if (!ch)
		return;

	if (0 == quest::CQuestManager::instance().GetEventFlag("enable_shop_search"))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("PRIVATE_SHOP_SEARCH_SYSTEM_DISABLED"));
		return;
	}

#if defined(__PRIVATE_SHOP_SEARCH_NEED_ITEM__)
	if (!ch->FindSpecifyItem(60005))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("PRIVATE_SHOP_SEARCH_NEED_ITEM"));
		return;
	}
#endif

	LPCHARACTER pkChrShop = CHARACTER_MANAGER::instance().Find(shopVid);

	if (!pkChrShop)
		return;

	LPOFFLINESHOP pkShop = pkChrShop->GetOfflineShop();

	if (!pkShop)
		return;

	if (DISTANCE_APPROX(ch->GetX() - pkChrShop->GetX(), ch->GetY() - pkChrShop->GetY()) > VIEW_RANGE)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상점과의 거리가 너무 멀어 물건을 살 수 없습니다."));
		return;
	}

	if (ch->GetMapIndex() != pkChrShop->GetMapIndex())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("YOU_CANNOT_BUY_THIS_BECAUSE_NOT_IN_SAME_MAP"));
		return;
	}

	ch->SetOfflineShopOwner(pkChrShop);
	pkShop->SetGuestMap(ch);
	int32_t returnHeader = pkShop->Buy(ch, shopItemPos);

	if (SHOP_SUBHEADER_GC_OK == returnHeader)
	{
		// being lazy starting in 3 2 ..
		ch->ChatPacket(CHAT_TYPE_COMMAND, "ShopSearchBuy");
		ch->SetOfflineShop(NULL);
		ch->SetOfflineShopOwner(NULL);
		pkShop->RemoveGuestMap(ch);
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "ShopSearchError %d", returnHeader);
		ch->SetOfflineShop(NULL);
		ch->SetOfflineShopOwner(NULL);
		pkShop->RemoveGuestMap(ch);
	}
}
#endif

void CInputMain::ItemMove(LPCHARACTER ch, const char* data)
{
	struct command_item_move* pinfo = (struct command_item_move*)data;

	if (ch)
		ch->MoveItem(pinfo->Cell, pinfo->CellTo, pinfo->count);
}

void CInputMain::ItemPickup(LPCHARACTER ch, const char* data)
{
	struct command_item_pickup* pinfo = (struct command_item_pickup*)data;
	if (ch)
#ifdef ENABLE_CHECK_PICKUP_HACK
		if (ch->PickupItem(pinfo->vid))
		{
			DWORD d_difference = get_dword_time() - ch->GetLastPickupTime();
			if (d_difference < ENABLE_CHECK_PICKUP_HACK && d_difference > 3)
			{
				LogManager::instance().CShieldLog(ch->GetAID(), ch->GetName(), 10006);
				LPDESC d = ch->GetDesc();
				if (d)
					d->SetPhase(PHASE_CLOSE);
			}
			ch->SetLastPickupTime();
		}
#else
		ch->PickupItem(pinfo->vid);
#endif
}

void CInputMain::QuickslotAdd(LPCHARACTER ch, const char* data)
{
	struct command_quickslot_add* pinfo = (struct command_quickslot_add*)data;
	ch->SetQuickslot(pinfo->pos, pinfo->slot);
}

void CInputMain::QuickslotDelete(LPCHARACTER ch, const char* data)
{
	struct command_quickslot_del* pinfo = (struct command_quickslot_del*)data;
	ch->DelQuickslot(pinfo->pos);
}

void CInputMain::QuickslotSwap(LPCHARACTER ch, const char* data)
{
	struct command_quickslot_swap* pinfo = (struct command_quickslot_swap*)data;
	ch->SwapQuickslot(pinfo->pos, pinfo->change_pos);
}

int CInputMain::Messenger(LPCHARACTER ch, const char* c_pData, size_t uiBytes)
{
	TPacketCGMessenger* p = (TPacketCGMessenger*)c_pData;

	if (uiBytes < sizeof(TPacketCGMessenger))
		return -1;

	c_pData += sizeof(TPacketCGMessenger);
	uiBytes -= sizeof(TPacketCGMessenger);

	switch (p->subheader)
	{
	case MESSENGER_SUBHEADER_CG_ADD_BY_VID:
	{
		if (uiBytes < sizeof(TPacketCGMessengerAddByVID))
			return -1;

		TPacketCGMessengerAddByVID* p2 = (TPacketCGMessengerAddByVID*)c_pData;
		LPCHARACTER ch_companion = CHARACTER_MANAGER::instance().Find(p2->vid);

		if (!ch_companion)
			return sizeof(TPacketCGMessengerAddByVID);

		if (ch->IsObserverMode())
			return sizeof(TPacketCGMessengerAddByVID);

		if (ch_companion->IsBlockMode(BLOCK_MESSENGER_INVITE))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방이 메신져 추가 거부 상태입니다."));
			return sizeof(TPacketCGMessengerAddByVID);
		}

		LPDESC d = ch_companion->GetDesc();

		if (!d)
			return sizeof(TPacketCGMessengerAddByVID);

		if (ch->GetGMLevel() == GM_PLAYER && ch_companion->GetGMLevel() != GM_PLAYER)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<메신져> 운영자는 메신져에 추가할 수 없습니다."));
			return sizeof(TPacketCGMessengerAddByVID);
		}

#if defined(__MESSENGER_BLOCK_SYSTEM__)
		if (MessengerManager::instance().IsBlocked(ch->GetName(), ch_companion->GetName()))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Unblock %s to continue."), ch_companion->GetName());
			return sizeof(TPacketCGMessengerAddByVID);
		}
		else if (MessengerManager::instance().IsBlocked(ch_companion->GetName(), ch->GetName()))
		{
#if defined(ENABLE_TEXT_PACKET)
			ch->ChatPacketNew(CHAT_TYPE_INFO, 15, "%s", ch_companion->GetName());
#endif
			return sizeof(TPacketCGMessengerAddByVID);
		}
#endif

		if (ch->GetDesc() == d) // 자신은 추가할 수 없다.
			return sizeof(TPacketCGMessengerAddByVID);

		MessengerManager::instance().RequestToAdd(ch, ch_companion);
		//MessengerManager::instance().AddToList(ch->GetName(), ch_companion->GetName());
	}
	return sizeof(TPacketCGMessengerAddByVID);

	case MESSENGER_SUBHEADER_CG_ADD_BY_NAME:
	{
		if (uiBytes < CHARACTER_NAME_MAX_LEN)
			return -1;

		char name[CHARACTER_NAME_MAX_LEN + 1];
		strlcpy(name, c_pData, sizeof(name));

		if (ch->GetGMLevel() == GM_PLAYER && gm_get_level(name) != GM_PLAYER)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<메신져> 운영자는 메신져에 추가할 수 없습니다."));
			return CHARACTER_NAME_MAX_LEN;
		}

		LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(name);

		if (!tch)
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s 님은 접속되 있지 않습니다."), name);
		else
		{
			if (tch == ch) // 자신은 추가할 수 없다.
				return CHARACTER_NAME_MAX_LEN;

#if defined(__MESSENGER_BLOCK_SYSTEM__)
			if (MessengerManager::instance().IsBlocked(ch->GetName(), tch->GetName()))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Unblock %s to continue."), tch->GetName());
				return CHARACTER_NAME_MAX_LEN;
			}
			else if (MessengerManager::instance().IsBlocked(tch->GetName(), ch->GetName()))
			{
#if defined(ENABLE_TEXT_PACKET)
				ch->ChatPacketNew(CHAT_TYPE_INFO, 15, "%s", tch->GetName());
#endif
				return CHARACTER_NAME_MAX_LEN;
			}
#endif

			if (tch->IsBlockMode(BLOCK_MESSENGER_INVITE) == true)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방이 메신져 추가 거부 상태입니다."));
			}
			else
			{
				// 메신저가 캐릭터단위가 되면서 변경
				MessengerManager::instance().RequestToAdd(ch, tch);
				//MessengerManager::instance().AddToList(ch->GetName(), tch->GetName());
			}
		}
	}
	return CHARACTER_NAME_MAX_LEN;

	case MESSENGER_SUBHEADER_CG_REMOVE:
	{
		if (uiBytes < CHARACTER_NAME_MAX_LEN)
			return -1;

		char char_name[CHARACTER_NAME_MAX_LEN + 1];
		strlcpy(char_name, c_pData, sizeof(char_name));
		MessengerManager::instance().RemoveFromList(ch->GetName(), char_name);
#if defined(ENABLE_COMMON_CHANGES)
		MessengerManager::instance().RemoveFromList(char_name, ch->GetName());
#endif
	}
	return CHARACTER_NAME_MAX_LEN;

#if defined(__MESSENGER_BLOCK_SYSTEM__)
	case MESSENGER_SUBHEADER_CG_BLOCK_ADD_BY_VID:
	{
		if (uiBytes < sizeof(TPacketCGMessengerAddBlockByVID))
			return -1;

		TPacketCGMessengerAddBlockByVID* p2 = (TPacketCGMessengerAddBlockByVID*)c_pData;
		LPCHARACTER ch_companion = CHARACTER_MANAGER::instance().Find(p2->vid);

		if (!ch_companion)
			return sizeof(TPacketCGMessengerAddBlockByVID);

		if (ch->IsObserverMode())
			return sizeof(TPacketCGMessengerAddBlockByVID);

		LPDESC d = ch_companion->GetDesc();

		if (!d)
			return sizeof(TPacketCGMessengerAddByVID);

		LPCHARACTER pkPartner = ch->GetMarryPartner();
		if (pkPartner)
		{
			if (ch_companion->GetName() == pkPartner->GetName())
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot block your spouse."));
				return sizeof(TPacketCGMessengerAddBlockByVID);
			}
		}

		if (MessengerManager::instance().IsBlocked(ch->GetName(), ch_companion->GetName()))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Remove %s from your friends list to continue."), ch_companion->GetName());
			return sizeof(TPacketCGMessengerAddBlockByVID);
		}

		if (MessengerManager::instance().IsBlocked(ch->GetName(), ch_companion->GetName()))
		{
#if defined(ENABLE_TEXT_PACKET)
			ch->ChatPacketNew(CHAT_TYPE_INFO, 19, "%s", ch_companion->GetName());
#endif
			return sizeof(TPacketCGMessengerAddBlockByVID);
		}

		if (ch->GetGMLevel() == GM_PLAYER && ch_companion->GetGMLevel() != GM_PLAYER && !test_server)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot block this player."));
			return sizeof(TPacketCGMessengerAddByVID);
		}

		if (ch->GetDesc() == d)
			return sizeof(TPacketCGMessengerAddBlockByVID);

		MessengerManager::instance().AddToBlockList(ch->GetName(), ch_companion->GetName());
	}
	return sizeof(TPacketCGMessengerAddBlockByVID);

	case MESSENGER_SUBHEADER_CG_BLOCK_ADD_BY_NAME:
	{
		if (uiBytes < CHARACTER_NAME_MAX_LEN)
			return -1;

		char name[CHARACTER_NAME_MAX_LEN + 1];
		strlcpy(name, c_pData, sizeof(name));

		if (gm_get_level(name) != GM_PLAYER && !test_server)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot block this player."));
			return CHARACTER_NAME_MAX_LEN;
		}

		LPCHARACTER pkChr = CHARACTER_MANAGER::instance().FindPC(name);
		if (pkChr == ch)
			return CHARACTER_NAME_MAX_LEN;

		LPDESC pkDesc = NULL;
		if (!pkChr)
		{
			CCI* pkCCI = P2P_MANAGER::instance().Find(name);
			if (pkCCI)
				pkDesc = pkCCI->pkDesc;
		}
		else
			pkDesc = pkChr->GetDesc();

		if (!pkDesc)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s is not online."), name);
			return CHARACTER_NAME_MAX_LEN;
		}
		else
		{
			LPCHARACTER pkPartner = ch->GetMarryPartner();
			if (pkPartner)
			{
				if (pkPartner->GetName() == name)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot block your spouse."));
					return CHARACTER_NAME_MAX_LEN;
				}
			}

			if (MessengerManager::instance().IsBlocked(ch->GetName(), name))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Remove %s from your friends list to continue."), name);
				return CHARACTER_NAME_MAX_LEN;
			}

			if (MessengerManager::instance().IsBlocked(ch->GetName(), name))
			{
#if defined(ENABLE_TEXT_PACKET)
				ch->ChatPacketNew(CHAT_TYPE_INFO, 19, "%s", name);
#endif
				return CHARACTER_NAME_MAX_LEN;
			}

			MessengerManager::instance().AddToBlockList(ch->GetName(), name);
		}
	}
	return CHARACTER_NAME_MAX_LEN;

	case MESSENGER_SUBHEADER_CG_BLOCK_REMOVE:
	{
		if (uiBytes < CHARACTER_NAME_MAX_LEN)
			return -1;

		char char_name[CHARACTER_NAME_MAX_LEN + 1];
		strlcpy(char_name, c_pData, sizeof(char_name));

		if (!MessengerManager::instance().IsBlocked(ch->GetName(), char_name))
			return CHARACTER_NAME_MAX_LEN;

		MessengerManager::instance().RemoveFromBlockList(ch->GetName(), char_name);
	}
	return CHARACTER_NAME_MAX_LEN;
#endif

	default:
		sys_err("CInputMain::Messenger : Unknown subheader %d : %s", p->subheader, ch->GetName());
		break;
	}

	return 0;
}

int CInputMain::Shop(LPCHARACTER ch, const char* data, size_t uiBytes)
{
	TPacketCGShop* p = (TPacketCGShop*)data;

	if (uiBytes < sizeof(TPacketCGShop))
		return -1;

	if (test_server)
		sys_log(0, "CInputMain::Shop() ==> SubHeader %d", p->subheader);

	const char* c_pData = data + sizeof(TPacketCGShop);
	uiBytes -= sizeof(TPacketCGShop);

	switch (p->subheader)
	{
	case SHOP_SUBHEADER_CG_END:
		sys_log(1, "INPUT: %s SHOP: END", ch->GetName());
		CShopManager::instance().StopShopping(ch);
		return 0;

	case SHOP_SUBHEADER_CG_BUY:
	{
		if (uiBytes < sizeof(BYTE) + sizeof(BYTE))
			return -1;

		BYTE bPos = *(c_pData + 1);
		sys_log(1, "INPUT: %s SHOP: BUY %d", ch->GetName(), bPos);
		CShopManager::instance().Buy(ch, bPos);
		return (sizeof(BYTE) + sizeof(BYTE));
	}

	case SHOP_SUBHEADER_CG_SELL:
	{
		if (uiBytes < sizeof(BYTE))
			return -1;

		BYTE pos = *c_pData;

		sys_log(0, "INPUT: %s SHOP: SELL", ch->GetName());
		CShopManager::instance().Sell(ch, pos);
		return sizeof(BYTE);
	}

	case SHOP_SUBHEADER_CG_SELL2:
	{
		const TPacketCGShopSell* p = reinterpret_cast<const TPacketCGShopSell*>(c_pData);

		sys_log(0, "INPUT: %s SHOP: SELL2", ch->GetName());

		CShopManager::instance().Sell(ch, p->wPos, p->wCount, p->bType);
		return sizeof(TPacketCGShopSell);
	}

	default:
		sys_err("CInputMain::Shop : Unknown subheader %d : %s", p->subheader, ch->GetName());
		break;
	}

	return 0;
}

void CInputMain::OnClick(LPCHARACTER ch, const char* data)
{
	struct command_on_click* pinfo = (struct command_on_click*)data;
	LPCHARACTER victim;

	if ((victim = CHARACTER_MANAGER::instance().Find(pinfo->vid)))
		victim->OnClick(ch);
	else if (test_server)
	{
		sys_err("CInputMain::OnClick %s.Click.NOT_EXIST_VID[%d]", ch->GetName(), pinfo->vid);
	}
}

void CInputMain::Exchange(LPCHARACTER ch, const char* data)
{
	struct command_exchange* pinfo = (struct command_exchange*)data;
	LPCHARACTER to_ch = NULL;

	if (!ch->CanHandleItem())
		return;

	int iPulse = thecore_pulse();

	if ((to_ch = CHARACTER_MANAGER::instance().Find(pinfo->arg1)))
	{
		if (iPulse - to_ch->GetSafeboxLoadTime() < PASSES_PER_SEC(g_nPortalLimitTime))
		{
			to_ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("거래 후 %d초 이내에 창고를 열수 없습니다."), g_nPortalLimitTime);
			return;
		}

		if (true == to_ch->IsDead())
		{
			return;
		}
	}

	sys_log(0, "CInputMain()::Exchange() SubHeader %d ", pinfo->sub_header);

	if (iPulse - ch->GetSafeboxLoadTime() < PASSES_PER_SEC(g_nPortalLimitTime))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("거래 후 %d초 이내에 창고를 열수 없습니다."), g_nPortalLimitTime);
		return;
	}

	switch (pinfo->sub_header)
	{
	case EXCHANGE_SUBHEADER_CG_START: // arg1 == vid of target character
		if (!ch->GetExchange())
		{
			if ((to_ch = CHARACTER_MANAGER::instance().Find(pinfo->arg1)))
			{
				//MONARCH_LIMIT
				/*
				if (to_ch->IsMonarch() || ch->IsMonarch())
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("군주와는 거래를 할수가 없습니다"), g_nPortalLimitTime);
					return;
				}
				//END_MONARCH_LIMIT
				*/
				if (iPulse - ch->GetSafeboxLoadTime() < PASSES_PER_SEC(g_nPortalLimitTime))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("창고를 연후 %d초 이내에는 거래를 할수 없습니다."), g_nPortalLimitTime);

					if (test_server)
						ch->ChatPacket(CHAT_TYPE_INFO, "[TestOnly][Safebox]Pulse %d LoadTime %d PASS %d", iPulse, ch->GetSafeboxLoadTime(), PASSES_PER_SEC(g_nPortalLimitTime));
					return;
				}

				if (iPulse - to_ch->GetSafeboxLoadTime() < PASSES_PER_SEC(g_nPortalLimitTime))
				{
					to_ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("창고를 연후 %d초 이내에는 거래를 할수 없습니다."), g_nPortalLimitTime);

					if (test_server)
						to_ch->ChatPacket(CHAT_TYPE_INFO, "[TestOnly][Safebox]Pulse %d LoadTime %d PASS %d", iPulse, to_ch->GetSafeboxLoadTime(), PASSES_PER_SEC(g_nPortalLimitTime));
					return;
				}

				if ((uint64_t)(ch->GetGold()) >= g_MaxGold)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("액수가 20억 냥을 초과하여 거래를 할수가 없습니다.."));

					sys_err("[OVERFLOG_GOLD] START (%u) id %u name %s ", ch->GetGold(), ch->GetPlayerID(), ch->GetName());
					return;
				}

#if defined(ENABLE_CHEQUE_SYSTEM)
				if (ch->GetCheque() > CHEQUE_MAX)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("액수가 20억 냥을 초과하여 거래를 할수가 없습니다.."));

					sys_err("[OVERFLOW_CHEQUE] START (%lld) id %u name %s ", ch->GetCheque(), ch->GetPlayerID(), ch->GetName());
					return;
				}
#endif

				if (to_ch->IsPC())
				{
					if (quest::CQuestManager::instance().GiveItemToPC(ch->GetPlayerID(), to_ch))
					{
						sys_log(0, "Exchange canceled by quest %s %s", ch->GetName(), to_ch->GetName());
						return;
					}
				}

				if (ch->GetMyShop() || ch->IsOpenSafebox() || ch->GetShopOwner() || ch->IsCubeOpen()
#if defined(__MAILBOX__)
 || ch->GetMailBox()
#endif
#ifdef ENABLE_AURA_SYSTEM
						|| ch->IsAuraRefineWindowOpen()
#endif
					)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("다른 거래중일경우 개인상점을 열수가 없습니다."));
					return;
				}

				ch->ExchangeStart(to_ch);
			}
		}
		break;

	case EXCHANGE_SUBHEADER_CG_ITEM_ADD: // arg1 == position of item, arg2 == position in exchange window
		if (ch->GetExchange())
		{
			if (ch->GetExchange()->GetCompany()->GetAcceptStatus() != true)
#if defined(ITEM_CHECKINOUT_UPDATE)
					ch->GetExchange()->AddItem(pinfo->Pos, pinfo->arg2, pinfo->SelectPosAuto);
#else
					ch->GetExchange()->AddItem(pinfo->Pos, pinfo->arg2);
#endif
		}
		break;

	case EXCHANGE_SUBHEADER_CG_ITEM_DEL: // arg1 == position of item
		if (ch->GetExchange())
		{
			if (ch->GetExchange()->GetCompany()->GetAcceptStatus() != true)
				ch->GetExchange()->RemoveItem(pinfo->arg1);
		}
		break;

	case EXCHANGE_SUBHEADER_CG_ELK_ADD: // arg1 == amount of gold
		if (ch->GetExchange())
		{
			const uint64_t nTotalGold = static_cast<long long>(ch->GetExchange()->GetCompany()->GetOwner()->GetGold()) + static_cast<long long>(pinfo->arg1);

			if (g_MaxGold <= nTotalGold)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방의 총금액이 20억 냥을 초과하여 거래를 할수가 없습니다.."));

				sys_err("[OVERFLOW_GOLD] ELK_ADD (%lld) id %u name %s ",
					ch->GetExchange()->GetCompany()->GetOwner()->GetGold(),
					ch->GetExchange()->GetCompany()->GetOwner()->GetPlayerID(),
					ch->GetExchange()->GetCompany()->GetOwner()->GetName());

				return;
			}

			if (ch->GetExchange()->GetCompany()->GetAcceptStatus() != true)
				ch->GetExchange()->AddGold(pinfo->arg1);
		}
		break;

#if defined(ENABLE_CHEQUE_SYSTEM)
	case EXCHANGE_SUBHEADER_CG_CHEQUE_ADD: // arg1 == amount of cheque
		if (ch->GetExchange())
		{
			const int64_t nTotalCheque = static_cast<int64_t>(ch->GetExchange()->GetCompany()->GetOwner()->GetCheque()) + static_cast<int64_t>(pinfo->arg1);

			if (CHEQUE_MAX <= nTotalCheque)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방의 총금액이 20억 냥을 초과하여 거래를 할수가 없습니다.."));

				sys_err("[OVERFLOW_CHEQUE] CHEQUE_ADD (%u) id %u name %s ",
					ch->GetExchange()->GetCompany()->GetOwner()->GetCheque(),
					ch->GetExchange()->GetCompany()->GetOwner()->GetPlayerID(),
					ch->GetExchange()->GetCompany()->GetOwner()->GetName());

				return;
			}

			if (ch->GetExchange()->GetCompany()->GetAcceptStatus() != true)
				ch->GetExchange()->AddCheque(pinfo->arg1);
		}
		break;
#endif

	case EXCHANGE_SUBHEADER_CG_ACCEPT: // arg1 == not used
		if (ch->GetExchange())
		{
			sys_log(0, "CInputMain()::Exchange() ==> ACCEPT ");
			ch->GetExchange()->Accept(true);
		}

		break;

	case EXCHANGE_SUBHEADER_CG_CANCEL: // arg1 == not used
		if (ch->GetExchange())
			ch->GetExchange()->Cancel();
		break;
	}
}

void CInputMain::Position(LPCHARACTER ch, const char* data)
{
	struct command_position* pinfo = (struct command_position*)data;

	switch (pinfo->position)
	{
	case POSITION_GENERAL:
		ch->Standup();
		break;

	case POSITION_SITTING_CHAIR:
		ch->Sitdown(0);
		break;

	case POSITION_SITTING_GROUND:
		ch->Sitdown(1);
		break;
	}
}

static const int ComboSequenceBySkillLevel[3][8] =
{
	//	0	1	2	3	4	5	6	7
	{	14,	15,	16,	17,	0,	0,	0,	0	},
	{	14,	15,	16,	18,	20,	0,	0,	0	},
	{	14,	15,	16,	18,	19,	17,	0,	0	},
};

#define COMBO_HACK_ALLOWABLE_MS 100

bool CheckComboHack(LPCHARACTER ch, BYTE bArg, DWORD dwTime, bool CheckSpeedHack)
{
	if (!gHackCheckEnable)
		return false;

	// 죽거나 기절 상태에서는 공격할 수 없으므로, skip한다.
	// 이렇게 하지 말고, CHRACTER::CanMove()에 
	// if (IsStun() || IsDead()) return false;
	// 를 추가하는게 맞다고 생각하나,
	// 이미 다른 부분에서 CanMove()는 IsStun(), IsDead()과
	// 독립적으로 체크하고 있기 때문에 수정에 의한 영향을
	// 최소화하기 위해 이렇게 땜빵 코드를 써놓는다.
	if (ch->IsStun() || ch->IsDead())
		return false;
	int ComboInterval = dwTime - ch->GetLastComboTime();
	int HackScalar = 0; // 기본 스칼라 단위 1
#if 0
	sys_log(0, "COMBO: %s arg:%u seq:%u delta:%d checkspeedhack:%d",
		ch->GetName(), bArg, ch->GetComboSequence(), ComboInterval - ch->GetValidComboInterval(), CheckSpeedHack);
#endif
	// bArg 14 ~ 21번 까지 총 8콤보 가능
	// 1. 첫 콤보(14)는 일정 시간 이후에 반복 가능
	// 2. 15 ~ 21번은 반복 불가능
	// 3. 차례대로 증가한다.
	if (bArg == 14)
	{
		if (CheckSpeedHack && ComboInterval > 0 && ComboInterval < ch->GetValidComboInterval() - COMBO_HACK_ALLOWABLE_MS)
		{
			// FIXME 첫번째 콤보는 이상하게 빨리 올 수가 있어서 300으로 나눔 -_-;
			// 다수의 몬스터에 의해 다운되는 상황에서 공격을 하면
			// 첫번째 콤보가 매우 적은 인터벌로 들어오는 상황 발생.
			// 이로 인해 콤보핵으로 튕기는 경우가 있어 다음 코드 비 활성화.
			//HackScalar = 1 + (ch->GetValidComboInterval() - ComboInterval) / 300;

			//sys_log(0, "COMBO_HACK: 2 %s arg:%u interval:%d valid:%u atkspd:%u riding:%s",
			//	ch->GetName(),
			//	bArg,
			//	ComboInterval,
			//	ch->GetValidComboInterval(),
			//	ch->GetPoint(POINT_ATT_SPEED),
			//	ch->IsRiding() ? "yes" : "no"
			//);
		}

		ch->SetComboSequence(1);
		ch->SetValidComboInterval((int)(ani_combo_speed(ch, 1) / (ch->GetPoint(POINT_ATT_SPEED) / 100.f)));
		ch->SetLastComboTime(dwTime);
	}
	else if (bArg > 14 && bArg < 22)
	{
		int idx = MIN(2, ch->GetComboIndex());

		if (ch->GetComboSequence() > 5) // 현재 6콤보 이상은 없다.
		{
			HackScalar = 1;
			ch->SetValidComboInterval(300);
			sys_log(0, "COMBO_HACK: 5 %s combo_seq:%d", ch->GetName(), ch->GetComboSequence());
		}
		// 자객 쌍수 콤보 예외처리
		else if (bArg == 21 &&
			idx == 2 &&
			ch->GetComboSequence() == 5 &&
			ch->GetJob() == JOB_ASSASSIN &&
			ch->GetWear(WEAR_WEAPON) &&
			ch->GetWear(WEAR_WEAPON)->GetSubType() == WEAPON_DAGGER
			)
			ch->SetValidComboInterval(300);
		else if (bArg == 21 &&
			idx == 2 &&
			ch->GetComboSequence() == 5 &&
			ch->GetJob() == JOB_WOLFMAN &&
			ch->GetWear(WEAR_WEAPON) &&
			ch->GetWear(WEAR_WEAPON)->GetSubType() == WEAPON_CLAW
			)
			ch->SetValidComboInterval(300);
		else if (ComboSequenceBySkillLevel[idx][ch->GetComboSequence()] != bArg)
		{
			HackScalar = 1;
			ch->SetValidComboInterval(300);

			sys_log(0, "COMBO_HACK: 3 %s arg:%u valid:%u combo_idx:%d combo_seq:%d",
				ch->GetName(),
				bArg,
				ComboSequenceBySkillLevel[idx][ch->GetComboSequence()],
				idx,
				ch->GetComboSequence());
		}
		else
		{
			if (CheckSpeedHack && ComboInterval < ch->GetValidComboInterval() - COMBO_HACK_ALLOWABLE_MS)
			{
				HackScalar = 1 + (ch->GetValidComboInterval() - ComboInterval) / 100;

				sys_log(0, "COMBO_HACK: 2 %s arg:%u interval:%d valid:%u atkspd:%u riding:%s",
					ch->GetName(),
					bArg,
					ComboInterval,
					ch->GetValidComboInterval(),
					ch->GetPoint(POINT_ATT_SPEED),
					ch->IsRiding() ? "yes" : "no");
			}

			// 말을 탔을 때는 15번 ~ 16번을 반복한다
			//if (ch->IsHorseRiding())
			if (ch->IsRiding())
				ch->SetComboSequence(ch->GetComboSequence() == 1 ? 2 : 1);
			else
				ch->SetComboSequence(ch->GetComboSequence() + 1);

			ch->SetValidComboInterval((int)(ani_combo_speed(ch, bArg - 13) / (ch->GetPoint(POINT_ATT_SPEED) / 100.f)));
			ch->SetLastComboTime(dwTime);
		}
	}
	else if (bArg == 13) // 기본 공격 (둔갑(Polymorph)했을 때 온다)
	{
		if (CheckSpeedHack && ComboInterval > 0 && ComboInterval < ch->GetValidComboInterval() - COMBO_HACK_ALLOWABLE_MS)
		{
			// 다수의 몬스터에 의해 다운되는 상황에서 공격을 하면
			// 첫번째 콤보가 매우 적은 인터벌로 들어오는 상황 발생.
			// 이로 인해 콤보핵으로 튕기는 경우가 있어 다음 코드 비 활성화.
			//HackScalar = 1 + (ch->GetValidComboInterval() - ComboInterval) / 100;

			//sys_log(0, "COMBO_HACK: 6 %s arg:%u interval:%d valid:%u atkspd:%u",
			//	ch->GetName(),
			//	bArg,
			//	ComboInterval,
			//	ch->GetValidComboInterval(),
			//	ch->GetPoint(POINT_ATT_SPEED)
			//);
		}

		if (ch->GetRaceNum() >= MAIN_RACE_MAX_NUM)
		{
			// POLYMORPH_BUG_FIX

			// DELETEME
			/*
			const CMotion * pkMotion = CMotionManager::instance().GetMotion(ch->GetRaceNum(), MAKE_MOTION_KEY(MOTION_MODE_GENERAL, MOTION_NORMAL_ATTACK));

			if (!pkMotion)
				sys_err("cannot find motion by race %u", ch->GetRaceNum());
			else
			{
				// 정상적 계산이라면 1000.f를 곱해야 하지만 클라이언트가 애니메이션 속도의 90%에서
				// 다음 애니메이션 블렌딩을 허용하므로 900.f를 곱한다.
				int k = (int) (pkMotion->GetDuration() / ((float) ch->GetPoint(POINT_ATT_SPEED) / 100.f) * 900.f);
				ch->SetValidComboInterval(k);
				ch->SetLastComboTime(dwTime);
			}
			*/
			float normalAttackDuration = CMotionManager::instance().GetNormalAttackDuration(ch->GetRaceNum());
			int k = (int)(normalAttackDuration / ((float)ch->GetPoint(POINT_ATT_SPEED) / 100.f) * 900.f);
			ch->SetValidComboInterval(k);
			ch->SetLastComboTime(dwTime);
			// END_OF_POLYMORPH_BUG_FIX
		}
		else
		{
			// 말이 안되는 콤보가 왔다 해커일 가능성?
			//if (ch->GetDesc()->DelayedDisconnect(number(2, 9)))
			//{
			//	LogManager::instance().HackLog("Hacker", ch);
			//	sys_log(0, "HACKER: %s arg %u", ch->GetName(), bArg);
			//}

			// 위 코드로 인해, 폴리모프를 푸는 중에 공격 하면,
			// 가끔 핵으로 인식하는 경우가 있다.

			// 자세히 말혀면,
			// 서버에서 poly 0를 처리했지만,
			// 클라에서 그 패킷을 받기 전에, 몹을 공격. <- 즉, 몹인 상태에서 공격.
			//
			// 그러면 클라에서는 서버에 몹 상태로 공격했다는 커맨드를 보내고 (arg == 13)
			//
			// 서버에서는 race는 인간인데 공격형태는 몹인 놈이다! 라고 하여 핵체크를 했다.

			// 사실 공격 패턴에 대한 것은 클라이언트에서 판단해서 보낼 것이 아니라,
			// 서버에서 판단해야 할 것인데... 왜 이렇게 해놨을까...
			// by rtsummit
		}
	}
	else
	{
		// 말이 안되는 콤보가 왔다 해커일 가능성?
		if (ch->GetDesc()->DelayedDisconnect(number(2, 9)))
		{
			LogManager::instance().HackLog("Hacker", ch);
			sys_log(0, "HACKER: %s arg %u", ch->GetName(), bArg);
		}

		HackScalar = 10;
		ch->SetValidComboInterval(300);
	}

	if (HackScalar)
	{
		// 말에 타거나 내렸을 때 1.5초간 공격은 핵으로 간주하지 않되 공격력은 없게 하는 처리
		if (get_dword_time() - ch->GetLastMountTime() > 1500)
			ch->IncreaseComboHackCount(1 + HackScalar);

		ch->SkipComboAttackByTime(ch->GetValidComboInterval());
	}

	return HackScalar;
}

void CInputMain::Move(LPCHARACTER ch, const char* data)
{
	if (!ch->CanMove())
		return;

	struct command_move* pinfo = (struct command_move*)data;

	if (pinfo->bFunc >= FUNC_MAX_NUM && !(pinfo->bFunc & 0x80))
	{
		sys_err("invalid move type: %s", ch->GetName());
		return;
	}
#ifdef ENABLE_CHECK_GHOSTMODE
	if (ch->IsPC() && ch->IsDead())
	{
		if (ch->GetGhostmodeCount() <= 10)
		{
			ch->AddGhostmodeCount();
		}
		else
		{
			LPDESC d = ch->GetDesc();
			if (d)
			{
				if (d->DelayedDisconnect(3))
				{
					LogManager::instance().HackLog("Ghostmode", ch);
					ch->ResetGhostmodeCount();
				}
			}
			return;
		}
	}
	else
	{
		ch->ResetGhostmodeCount();
	}
#endif
#ifdef ENABLE_CHECK_WALLHACK
	if (ch->IsPC())
	{
		if (!SECTREE_MANAGER::instance().IsMovablePosition(ch->GetMapIndex(), pinfo->lX, pinfo->lY))
		{
			if (ch->GetWallhackCount() <= 5)
			{
				ch->AddWallhackCount();
			}
			else
			{
				LPDESC d = ch->GetDesc();
				if (d)
				{
					if (d->DelayedDisconnect(3))
					{
						LogManager::instance().HackLog("Wallhack", ch);
						ch->ResetWallhackCount();
					}
				}
				return;
			}
		}
		else
		{
			ch->ResetWallhackCount();
		}
	}
#endif
	/*
	enum EMoveFuncType
	{
		FUNC_WAIT,
		FUNC_MOVE,
		FUNC_ATTACK,
		FUNC_COMBO,
		FUNC_MOB_SKILL,
		_FUNC_SKILL,
		FUNC_MAX_NUM,
		FUNC_SKILL = 0x80,
	};
	*/

	// 텔레포트 핵 체크

	//if (!test_server) // 2012.05.15 김용욱 : 테섭에서 (무적상태로) 다수 몬스터 상대로 다운되면서 공격시 콤보핵으로 죽는 문제가 있었다.
	{
		const float fDist = DISTANCE_SQRT((ch->GetX() - pinfo->lX) / 100, (ch->GetY() - pinfo->lY) / 100);
		if (((false == ch->IsRiding() && fDist > 750) || fDist > 999) && OXEVENT_MAP_INDEX != ch->GetMapIndex())
		{
			sys_log(0, "MOVE: %s trying to move too far (dist: %.1fm) Riding(%d)", ch->GetName(), fDist, ch->IsRiding());

			ch->Show(ch->GetMapIndex(), ch->GetX(), ch->GetY(), ch->GetZ());
			ch->Stop();
			return;
		}

		//
		// 스피드핵(SPEEDHACK) Check
		//
		int dwCurTime = get_dword_time();
		// 시간을 Sync하고 7초 후 부터 검사한다. (20090702 이전엔 5초였음)
		bool CheckSpeedHack = (false == ch->GetDesc()->IsHandshaking() && dwCurTime - ch->GetDesc()->GetClientTime() > 7000);

		if (CheckSpeedHack)
		{
			int iDelta = (int)(dwCurTime - pinfo->dwTime);
			int iServerDelta = (int)(dwCurTime - ch->GetDesc()->GetClientTime());

			iDelta = (int)(dwCurTime - pinfo->dwTime);

			// 시간이 늦게간다. 일단 로그만 해둔다. 진짜 이런 사람들이 많은지 체크해야함. TODO
			if (iDelta >= 30000)
			{
				sys_log(0, "SPEEDHACK: slow timer name %s delta %d", ch->GetName(), iDelta);
				ch->GetDesc()->DelayedDisconnect(3);
			}
			// 1초에 20msec 빨리 가는거 까지는 이해한다.
			else if (iDelta < -(iServerDelta / 50))
			{
				sys_log(0, "SPEEDHACK: DETECTED! %s (delta %d %d)", ch->GetName(), iDelta, iServerDelta);
				ch->GetDesc()->DelayedDisconnect(3);
			}
		}

		//
		// 콤보핵 및 스피드핵 체크
		//
		if (pinfo->bFunc == FUNC_COMBO && g_bCheckMultiHack)
		{
			CheckComboHack(ch, pinfo->bArg, pinfo->dwTime, CheckSpeedHack); // 콤보 체크
		}
	}

#if defined(ENABLE_PVP_TOURNAMENT)
	if (ch->GetMapIndex() == PVP_DUEL_MAP_INDEX)
	{
		if (CPvPDuel::Instance().PvPPullFunction(ch))
			return;
	}
#endif

	if (pinfo->bFunc == FUNC_MOVE)
	{
		if (ch->GetLimitPoint(POINT_MOV_SPEED) == 0)
			return;

		ch->SetRotation(pinfo->bRot * 5); // 중복 코드
		ch->ResetStopTime(); // ""


		ch->Goto(pinfo->lX, pinfo->lY);
	}
	else
	{
		if (pinfo->bFunc == FUNC_ATTACK || pinfo->bFunc == FUNC_COMBO)
			ch->OnMove(true);
		else if (pinfo->bFunc & FUNC_SKILL)
		{
			const int MASK_SKILL_MOTION = 0x7F;
			unsigned int motion = pinfo->bFunc & MASK_SKILL_MOTION;

			if (!ch->IsUsableSkillMotion(motion))
			{
				const char* name = ch->GetName();
				unsigned int job = ch->GetJob();
				unsigned int group = ch->GetSkillGroup();

				char szBuf[256];
				snprintf(szBuf, sizeof(szBuf), "SKILL_HACK: name=%s, job=%d, group=%d, motion=%d", name, job, group, motion);
				LogManager::Instance().HackLog(szBuf, ch->GetDesc()->GetAccountTable().login, ch->GetName(), ch->GetDesc()->GetHostName());
				sys_log(0, "%s", szBuf);

				if (test_server)
				{
					ch->GetDesc()->DelayedDisconnect(number(2, 8));
					ch->ChatPacket(CHAT_TYPE_INFO, szBuf);
				}
				else
				{
					ch->GetDesc()->DelayedDisconnect(number(150, 500));
				}
			}

			ch->OnMove();
		}

		ch->SetRotation(pinfo->bRot * 5); // 중복 코드
		ch->ResetStopTime(); // ""

#ifdef ENABLE_SYNC_HACK_FIX
		if (OXEVENT_MAP_INDEX != ch->GetMapIndex())
		{
			int fDist = DISTANCE_SQRT((ch->GetX() - pinfo->lX) / 100, (ch->GetY() - pinfo->lY) / 100);
			ch->SetSyncPosition(pinfo->lX, pinfo->lY);
			int il = 0;
			
			if ((false == ch->IsRiding() && fDist > 750) || fDist > 999)
			{
				ch->Show(ch->GetMapIndex(), ch->GetX(), ch->GetY(), ch->GetZ());
				return;
			}
			
			if (((false == ch->IsRiding() && fDist > 750) || fDist > 999))
				il = DISTANCE_SQRT((ch->GetX() - pinfo->lX) / 100, (ch->GetY() - pinfo->lY) / 100) * 3;

			ch->CheckSyncPosition() ? il += 1 : il += 3;
			ch->SetSyncCount(il);
		}
#endif

		ch->Move(pinfo->lX, pinfo->lY);
		ch->Stop();
		ch->StopStaminaConsume();
	}

	TPacketGCMove pack;

	pack.bHeader = HEADER_GC_MOVE;
	pack.bFunc = pinfo->bFunc;
	pack.bArg = pinfo->bArg;
	pack.bRot = pinfo->bRot;
	pack.dwVID = ch->GetVID();
	pack.lX = pinfo->lX;
	pack.lY = pinfo->lY;
	pack.dwTime = pinfo->dwTime;
	pack.dwDuration = (pinfo->bFunc == FUNC_MOVE) ? ch->GetCurrentMoveDuration() : 0;

	ch->PacketAround(&pack, sizeof(TPacketGCMove), ch);
	/*
		if (pinfo->dwTime == 10653691) // 디버거 발견
		{
			if (ch->GetDesc()->DelayedDisconnect(number(15, 30)))
				LogManager::instance().HackLog("Debugger", ch);

		}
		else if (pinfo->dwTime == 10653971) // Softice 발견
		{
			if (ch->GetDesc()->DelayedDisconnect(number(15, 30)))
				LogManager::instance().HackLog("Softice", ch);
		}
	*/
	/*
	sys_log(0,
			"MOVE: %s Func:%u Arg:%u Pos:%dx%d Time:%u Dist:%.1f",
			ch->GetName(),
			pinfo->bFunc,
			pinfo->bArg,
			pinfo->lX / 100,
			pinfo->lY / 100,
			pinfo->dwTime,
			fDist);
	*/
}

#if defined(__SKILL_COLOR_SYSTEM__)
void CInputMain::SetSkillColor(LPCHARACTER ch, const char* pcData)
{
	if (!ch)
		return;

	TPacketCGSkillColor* CGPacket = (TPacketCGSkillColor*)pcData;

	if (CGPacket->bSkillSlot >= ESkillColorLength::MAX_SKILL_COUNT)
		return;

	DWORD dwData[ESkillColorLength::MAX_SKILL_COUNT + ESkillColorLength::MAX_BUFF_COUNT][ESkillColorLength::MAX_EFFECT_COUNT];
	memcpy(dwData, ch->GetSkillColor(), sizeof(dwData));

	dwData[CGPacket->bSkillSlot][0] = CGPacket->dwCol1;
	dwData[CGPacket->bSkillSlot][1] = CGPacket->dwCol2;
	dwData[CGPacket->bSkillSlot][2] = CGPacket->dwCol3;
	dwData[CGPacket->bSkillSlot][3] = CGPacket->dwCol4;
	dwData[CGPacket->bSkillSlot][4] = CGPacket->dwCol5;

	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have changed the color of your skill."));

	ch->SetSkillColor(dwData[0]);

	TSkillColor GDPacket;
	memcpy(GDPacket.dwSkillColor, dwData, sizeof(dwData));
	GDPacket.dwPlayerID = ch->GetPlayerID();
	db_clientdesc->DBPacketHeader(HEADER_GD_SKILL_COLOR_SAVE, 0, sizeof(TSkillColor));
	db_clientdesc->Packet(&GDPacket, sizeof(TSkillColor));
}
#endif

void CInputMain::Attack(LPCHARACTER ch, const BYTE header, const char* data)
{
	if (NULL == ch)
		return;

	struct type_identifier
	{
		BYTE header;
		BYTE type;
	};

	const struct type_identifier* const type = reinterpret_cast<const struct type_identifier*>(data);

	if (type->type > 0)
	{
		if (false == ch->CanUseSkill(type->type))
		{
			return;
		}

		switch (type->type)
		{
		case SKILL_GEOMPUNG:
		case SKILL_SANGONG:
		case SKILL_YEONSA:
		case SKILL_KWANKYEOK:
		case SKILL_HWAJO:
		case SKILL_GIGUNG:
		case SKILL_PABEOB:
		case SKILL_MARYUNG:
		case SKILL_TUSOK:
#if defined(__9TH_SKILL__)
		case SKILL_PUNGLOEPO:
		case SKILL_ILGWANGPYO:
		case SKILL_MABEOBAGGWI:
		case SKILL_METEO:
#endif
		case SKILL_MAHWAN:
		case SKILL_BIPABU:
		case SKILL_NOEJEON:
		case SKILL_CHAIN:
		case SKILL_HORSE_WILDATTACK_RANGE:
			if (HEADER_CG_SHOOT != type->header)
			{
				if (test_server)
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Attack :name[%s] Vnum[%d] can't use skill by attack(warning)"), type->type);
				return;
			}
			break;
		}
	}

	switch (header)
	{
	case HEADER_CG_ATTACK:
	{
		if (NULL == ch->GetDesc())
			return;

		const TPacketCGAttack* const packMelee = reinterpret_cast<const TPacketCGAttack*>(data);

		ch->GetDesc()->AssembleCRCMagicCube(packMelee->bCRCMagicCubeProcPiece, packMelee->bCRCMagicCubeFilePiece);

		LPCHARACTER victim = CHARACTER_MANAGER::instance().Find(packMelee->dwVID);

		if (NULL == victim || ch == victim)
			return;

		switch (victim->GetCharType())
		{
		case CHAR_TYPE_NPC:
		case CHAR_TYPE_WARP:
		case CHAR_TYPE_GOTO:
		case CHAR_TYPE_HORSE:
		case CHAR_TYPE_PET:
		case CHAR_TYPE_PET_PAY:
			return;
		}

		if (packMelee->bType > 0)
		{
			if (false == ch->CheckSkillHitCount(packMelee->bType, victim->GetVID()))
			{
				return;
			}
		}

		ch->Attack(victim, packMelee->bType);
	}
	break;

	case HEADER_CG_SHOOT:
	{
		const TPacketCGShoot* const packShoot = reinterpret_cast<const TPacketCGShoot*>(data);

		ch->Shoot(packShoot->bType);
	}
	break;
	}
}

int CInputMain::SyncPosition(LPCHARACTER ch, const char* c_pcData, size_t uiBytes)
{
	const TPacketCGSyncPosition* pinfo = reinterpret_cast<const TPacketCGSyncPosition*>(c_pcData);

	if (uiBytes < pinfo->wSize)
		return -1;

	int iExtraLen = pinfo->wSize - sizeof(TPacketCGSyncPosition);

	if (iExtraLen < 0)
	{
		sys_err("invalid packet length (len %d size %u buffer %u)", iExtraLen, pinfo->wSize, uiBytes);
		ch->GetDesc()->SetPhase(PHASE_CLOSE);
		return -1;
	}

	if (0 != (iExtraLen % sizeof(TPacketCGSyncPositionElement)))
	{
		sys_err("invalid packet length %d (name: %s)", pinfo->wSize, ch->GetName());
		return iExtraLen;
	}

	int iCount = iExtraLen / sizeof(TPacketCGSyncPositionElement);

	if (iCount <= 0)
		return iExtraLen;

	static const int nCountLimit = 16;

	if (iCount > nCountLimit)
	{
		// LogManager::instance().HackLog( "SYNC_POSITION_HACK", ch );
		sys_err("Too many SyncPosition Count(%d) from Name(%s)", iCount, ch->GetName());
		// ch->GetDesc()->SetPhase(PHASE_CLOSE);
		// return -1;
		iCount = nCountLimit;
	}

	TEMP_BUFFER tbuf;
	LPBUFFER lpBuf = tbuf.getptr();

	TPacketGCSyncPosition* pHeader = (TPacketGCSyncPosition*)buffer_write_peek(lpBuf);
	buffer_write_proceed(lpBuf, sizeof(TPacketGCSyncPosition));

	const TPacketCGSyncPositionElement* e =
		reinterpret_cast<const TPacketCGSyncPositionElement*>(c_pcData + sizeof(TPacketCGSyncPosition));

	timeval tvCurTime;
	gettimeofday(&tvCurTime, NULL);

	for (int i = 0; i < iCount; ++i, ++e)
	{
		LPCHARACTER victim = CHARACTER_MANAGER::instance().Find(e->dwVID);

		if (!victim)
			continue;

		switch (victim->GetCharType())
		{
		case CHAR_TYPE_NPC:
		case CHAR_TYPE_WARP:
		case CHAR_TYPE_GOTO:
		case CHAR_TYPE_HORSE:
		case CHAR_TYPE_PET:
		case CHAR_TYPE_PET_PAY:
			continue;
		}

		// 소유권 검사
		if (!victim->SetSyncOwner(ch))
			continue;

		const float fDistWithSyncOwner = DISTANCE_SQRT((victim->GetX() - ch->GetX()) / 100, (victim->GetY() - ch->GetY()) / 100);
		static const float fLimitDistWithSyncOwner = 2500.f + 1000.f;
		// victim과의 거리가 2500 + a 이상이면 핵으로 간주.
		// 거리 참조 : 클라이언트의 __GetSkillTargetRange, __GetBowRange 함수
		// 2500 : 스킬 proto에서 가장 사거리가 긴 스킬의 사거리, 또는 활의 사거리
		// a = POINT_BOW_DISTANCE 값... 인데 실제로 사용하는 값인지는 잘 모르겠음. 아이템이나 포션, 스킬, 퀘스트에는 없는데...
		// 그래도 혹시나 하는 마음에 버퍼로 사용할 겸해서 1000.f 로 둠...
		if (fDistWithSyncOwner > fLimitDistWithSyncOwner)
		{
			// g_iSyncHackLimitCount번 까지는 봐줌.
			if (ch->GetSyncHackCount() < g_iSyncHackLimitCount)
			{
				ch->SetSyncHackCount(ch->GetSyncHackCount() + 1);
				continue;
			}
			else
			{
				LogManager::instance().HackLog("SYNC_POSITION_HACK", ch);

				sys_err("Too far SyncPosition DistanceWithSyncOwner(%f)(%s) from Name(%s) CH(%d,%d) VICTIM(%d,%d) SYNC(%d,%d)",
					fDistWithSyncOwner, victim->GetName(), ch->GetName(), ch->GetX(), ch->GetY(), victim->GetX(), victim->GetY(),
					e->lX, e->lY);

				ch->GetDesc()->SetPhase(PHASE_CLOSE);

				return -1;
			}
		}

		const float fDist = DISTANCE_SQRT((victim->GetX() - e->lX) / 100, (victim->GetY() - e->lY) / 100);
		static const long g_lValidSyncInterval = 100 * 1000; // 100ms
		const timeval& tvLastSyncTime = victim->GetLastSyncTime();
		timeval* tvDiff = timediff(&tvCurTime, &tvLastSyncTime);

		// SyncPosition을 악용하여 타유저를 이상한 곳으로 보내는 핵 방어하기 위하여,
		// 같은 유저를 g_lValidSyncInterval ms 이내에 다시 SyncPosition하려고 하면 핵으로 간주.
		if (tvDiff->tv_sec == 0 && tvDiff->tv_usec < g_lValidSyncInterval)
		{
			// g_iSyncHackLimitCount번 까지는 봐줌.
			if (ch->GetSyncHackCount() < g_iSyncHackLimitCount)
			{
				ch->SetSyncHackCount(ch->GetSyncHackCount() + 1);
				continue;
			}
			else
			{
				LogManager::instance().HackLog("SYNC_POSITION_HACK", ch);

				sys_err("Too often SyncPosition Interval(%ldms)(%s) from Name(%s) VICTIM(%d,%d) SYNC(%d,%d)",
					tvDiff->tv_sec * 1000 + tvDiff->tv_usec / 1000, victim->GetName(), ch->GetName(), victim->GetX(), victim->GetY(),
					e->lX, e->lY);

				ch->GetDesc()->SetPhase(PHASE_CLOSE);

				return -1;
			}
		}
		else if (fDist > 25.0f)
		{
			LogManager::instance().HackLog("SYNC_POSITION_HACK", ch);

			sys_err("Too far SyncPosition Distance(%f)(%s) from Name(%s) CH(%d,%d) VICTIM(%d,%d) SYNC(%d,%d)",
				fDist, victim->GetName(), ch->GetName(), ch->GetX(), ch->GetY(), victim->GetX(), victim->GetY(),
				e->lX, e->lY);

			ch->GetDesc()->SetPhase(PHASE_CLOSE);

			return -1;
		}
		else
		{
			victim->SetLastSyncTime(tvCurTime);
			victim->Sync(e->lX, e->lY);
			buffer_write(lpBuf, e, sizeof(TPacketCGSyncPositionElement));
		}
	}

	if (buffer_size(lpBuf) != sizeof(TPacketGCSyncPosition))
	{
		pHeader->bHeader = HEADER_GC_SYNC_POSITION;
		pHeader->wSize = buffer_size(lpBuf);

		ch->PacketAround(buffer_read_peek(lpBuf), buffer_size(lpBuf), ch);
	}

	return iExtraLen;
}

void CInputMain::FlyTarget(LPCHARACTER ch, const char* pcData, BYTE bHeader)
{
	TPacketCGFlyTargeting* p = (TPacketCGFlyTargeting*)pcData;
	ch->FlyTarget(p->dwTargetVID, p->x, p->y, bHeader);
}

void CInputMain::UseSkill(LPCHARACTER ch, const char* pcData)
{
	TPacketCGUseSkill* p = (TPacketCGUseSkill*)pcData;
	ch->UseSkill(p->dwVnum, CHARACTER_MANAGER::instance().Find(p->dwVID));
}

void CInputMain::ScriptButton(LPCHARACTER ch, const void* c_pData)
{
	TPacketCGScriptButton* p = (TPacketCGScriptButton*)c_pData;
	sys_log(0, "QUEST ScriptButton pid %d idx %u", ch->GetPlayerID(), p->idx);

	quest::PC* pc = quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID());
	if (pc && pc->IsConfirmWait())
	{
		quest::CQuestManager::instance().Confirm(ch->GetPlayerID(), quest::CONFIRM_TIMEOUT);
	}
	else if (p->idx & 0x80000000)
	{
		quest::CQuestManager::Instance().QuestInfo(ch->GetPlayerID(), p->idx & 0x7fffffff);
	}
	else
	{
		quest::CQuestManager::Instance().QuestButton(ch->GetPlayerID(), p->idx);
	}
}

void CInputMain::ScriptAnswer(LPCHARACTER ch, const void* c_pData)
{
	TPacketCGScriptAnswer* p = (TPacketCGScriptAnswer*)c_pData;
	sys_log(0, "QUEST ScriptAnswer pid %d answer %d", ch->GetPlayerID(), p->answer);

	if (p->answer > 250) // 다음 버튼에 대한 응답으로 온 패킷인 경우
	{
		quest::CQuestManager::Instance().Resume(ch->GetPlayerID());
	}
	else // 선택 버튼을 골라서 온 패킷인 경우
	{
		quest::CQuestManager::Instance().Select(ch->GetPlayerID(), p->answer);
	}
}

// SCRIPT_SELECT_ITEM
void CInputMain::ScriptSelectItem(LPCHARACTER ch, const void* c_pData)
{
	TPacketCGScriptSelectItem* p = (TPacketCGScriptSelectItem*)c_pData;
	sys_log(0, "QUEST ScriptSelectItem pid %d answer %d", ch->GetPlayerID(), p->selection);
	quest::CQuestManager::Instance().SelectItem(ch->GetPlayerID(), p->selection);
}
// END_OF_SCRIPT_SELECT_ITEM

void CInputMain::QuestInputString(LPCHARACTER ch, const void* c_pData)
{
	TPacketCGQuestInputString* p = (TPacketCGQuestInputString*)c_pData;

	char msg[65];
	strlcpy(msg, p->msg, sizeof(msg));
	sys_log(0, "QUEST InputString pid %u msg %s", ch->GetPlayerID(), msg);

	quest::CQuestManager::Instance().Input(ch->GetPlayerID(), msg);
}

void CInputMain::QuestConfirm(LPCHARACTER ch, const void* c_pData)
{
	TPacketCGQuestConfirm* p = (TPacketCGQuestConfirm*)c_pData;
	LPCHARACTER ch_wait = CHARACTER_MANAGER::instance().FindByPID(p->requestPID);
	if (p->answer)
		p->answer = quest::CONFIRM_YES;
	sys_log(0, "QuestConfirm from %s pid %u name %s answer %d", ch->GetName(), p->requestPID, (ch_wait) ? ch_wait->GetName() : "", p->answer);
	if (ch_wait)
	{
		quest::CQuestManager::Instance().Confirm(ch_wait->GetPlayerID(), (quest::EQuestConfirmType) p->answer, ch->GetPlayerID());
	}
}

void CInputMain::Target(LPCHARACTER ch, const char* pcData)
{
	TPacketCGTarget* p = (TPacketCGTarget*)pcData;

	building::LPOBJECT pkObj = building::CManager::instance().FindObjectByVID(p->dwVID);

	if (pkObj)
	{
		TPacketGCTarget pckTarget;
		pckTarget.header = HEADER_GC_TARGET;
		pckTarget.dwVID = p->dwVID;
		ch->GetDesc()->Packet(&pckTarget, sizeof(TPacketGCTarget));
	}
	else
		ch->SetTarget(CHARACTER_MANAGER::instance().Find(p->dwVID));
}

void CInputMain::Warp(LPCHARACTER ch, const char* pcData)
{
	ch->WarpEnd();
}

void CInputMain::SafeboxCheckin(LPCHARACTER ch, const char* c_pData)
{
	if (quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID())->IsRunning() == true)
		return;

	TPacketCGSafeboxCheckin* p = (TPacketCGSafeboxCheckin*)c_pData;

	if (!ch->CanHandleItem())
		return;

	CSafebox* pkSafebox = ch->GetSafebox();
	LPITEM pkItem = ch->GetItem(p->ItemPos);

	if (!pkSafebox || !pkItem)
		return;

#if defined(ITEM_CHECKINOUT_UPDATE)
	if (p->SelectPosAuto)
	{
		int AutoPos = pkSafebox->GetEmptySafebox(pkItem->GetSize());
		if (AutoPos == -1)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<SAFEBOX> You don't have enough space."));
			return;
		}
		p->bSafePos = AutoPos;
	}
#endif

	if (pkItem->GetType() == ITEM_BELT && pkItem->IsEquipped() && CBeltInventoryHelper::IsExistItemInBeltInventory(ch))
	{
		if (!CBeltInventoryHelper::ClearBelt(ch))
			return;
	}

	if (pkItem->GetCell() >= INVENTORY_MAX_NUM && IS_SET(pkItem->GetFlag(), ITEM_FLAG_IRREMOVABLE))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 창고로 옮길 수 없는 아이템 입니다."));
		return;
	}

	if (!pkSafebox->IsEmpty(p->bSafePos, pkItem->GetSize()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 옮길 수 없는 위치입니다."));
		return;
	}

	if (pkItem->GetVnum() == UNIQUE_ITEM_SAFEBOX_EXPAND)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 이 아이템은 넣을 수 없습니다."));
		return;
	}

	if (IS_SET(pkItem->GetAntiFlag(), ITEM_ANTIFLAG_SAFEBOX))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 이 아이템은 넣을 수 없습니다."));
		return;
	}

	if (pkItem->IsEquipped())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Storeroom] An item that is currently being used cannot be stored."));
		return;
	}

	if (true == pkItem->isLocked())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 이 아이템은 넣을 수 없습니다."));
		return;
	}

	if (ITEM_BELT == pkItem->GetType() && CBeltInventoryHelper::IsExistItemInBeltInventory(ch))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("벨트 인벤토리에 아이템이 존재하면 해제할 수 없습니다."));
		return;
	}

	pkItem->RemoveFromCharacter();
	if (!pkItem->IsDragonSoul())
		ch->SyncQuickslot(QUICKSLOT_TYPE_ITEM, p->ItemPos.cell, SHRT_MAX);

	pkSafebox->Add(p->bSafePos, pkItem);

#ifdef __GROWTH_PET_SYSTEM__
	if (pkItem->GetType() == ITEM_PET)
	{
		if (pkItem->GetSubType() == PET_UPBRINGING || pkItem->GetSubType() == PET_BAG)
		{
			LPGROWTH_PET pPet = ch->GetGrowthPet(pkItem->GetSocket(2));
			if (pPet)
				pkSafebox->AddPet(pPet);
		}
	}
#endif

	char szHint[128];
	snprintf(szHint, sizeof(szHint), "%s %u", pkItem->GetName(), pkItem->GetCount());
	LogManager::instance().ItemLog(ch, pkItem, "SAFEBOX PUT", szHint);
}

void CInputMain::SafeboxCheckout(LPCHARACTER ch, const char* c_pData, bool bMall)
{
	TPacketCGSafeboxCheckout* p = (TPacketCGSafeboxCheckout*)c_pData;

	if (!ch->CanHandleItem())
		return;

	CSafebox* pkSafebox;

	if (bMall)
		pkSafebox = ch->GetMall();
	else
		pkSafebox = ch->GetSafebox();

	if (!pkSafebox)
		return;

	LPITEM pkItem = pkSafebox->Get(p->bSafePos);

	if (!pkItem)
		return;

	if (p->ItemPos.IsBeltInventoryPosition())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't move items from your storage into your belt inventory."));
		return;
	}

#if defined(__SPECIAL_INVENTORY_SYSTEM__)
	if (p->ItemPos.IsSkillBookInventoryPosition() && !pkItem->IsSkillBook())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can only move skill books into this inventory."));
		return;
	}

	if (p->ItemPos.IsUpgradeItemsInventoryPosition() && !pkItem->IsUpgradeItem())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can only move upgrade items into this inventory."));
		return;
	}

	if (p->ItemPos.IsStoneInventoryPosition() && !pkItem->IsStone())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can only move stones into this inventory."));
		return;
	}

	if (p->ItemPos.IsGiftBoxInventoryPosition() && !pkItem->IsGiftBox())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can only move chests into this inventory."));
		return;
	}
#endif

#if defined(ITEM_CHECKINOUT_UPDATE)
	if (p->SelectPosAuto)
	{
		int AutoPos = pkItem->IsDragonSoul() ? ch->GetEmptyDragonSoulInventory(pkItem) : ch->GetEmptyInventory(pkItem->GetSize());
		if (AutoPos == -1)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You don't have enough space."));
			return;
		}
		p->ItemPos = TItemPos(pkItem->IsDragonSoul() ? DRAGON_SOUL_INVENTORY : INVENTORY, AutoPos);
	}
#endif

	if (!ch->IsEmptyItemGrid(p->ItemPos, pkItem->GetSize()))
		return;

	// 아이템 몰에서 인벤으로 옮기는 부분에서 용혼석 특수 처리
	// (몰에서 만드는 아이템은 item_proto에 정의된대로 속성이 붙기 때문에,
	// 용혼석의 경우, 이 처리를 하지 않으면 속성이 하나도 붙지 않게 된다.)
	if (pkItem->IsDragonSoul())
	{
		if (bMall)
		{
			DSManager::instance().DragonSoulItemInitialize(pkItem);
		}

		if (DRAGON_SOUL_INVENTORY != p->ItemPos.window_type)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 옮길 수 없는 위치입니다."));
			return;
		}

		TItemPos DestPos = p->ItemPos;
		if (!DSManager::instance().IsValidCellForThisItem(pkItem, DestPos))
		{
			int iCell = ch->GetEmptyDragonSoulInventory(pkItem);
			if (iCell < 0)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 옮길 수 없는 위치입니다."));
				return;
			}
			DestPos = TItemPos(DRAGON_SOUL_INVENTORY, iCell);
		}

		pkSafebox->Remove(p->bSafePos);
		pkItem->AddToCharacter(ch, DestPos);
		ITEM_MANAGER::instance().FlushDelayedSave(pkItem);
	}
	else
	{
		if (DRAGON_SOUL_INVENTORY == p->ItemPos.window_type)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<창고> 옮길 수 없는 위치입니다."));
			return;
		}

		pkSafebox->Remove(p->bSafePos);
		if (bMall)
		{
			if (NULL == pkItem->GetProto())
			{
				sys_err("pkItem->GetProto() == NULL (id : %d)", pkItem->GetID());
				return;
			}
			// 100% 확률로 속성이 붙어야 하는데 안 붙어있다면 새로 붙힌다. ...............
			if (100 == pkItem->GetProto()->bAlterToMagicItemPct && 0 == pkItem->GetAttributeCount())
			{
				pkItem->AlterToMagicItem(number(40, 50), number(10, 15));
			}
		}
		pkItem->AddToCharacter(ch, p->ItemPos);
		ITEM_MANAGER::instance().FlushDelayedSave(pkItem);
#ifdef __GROWTH_PET_SYSTEM__
		if (pkItem->GetType() == ITEM_PET)
		{
			if (pkItem->GetSubType() == PET_UPBRINGING || pkItem->GetSubType() == PET_BAG)
				pkSafebox->RemovePet(pkItem);
		}
#endif
	}

	DWORD dwID = pkItem->GetID();
	db_clientdesc->DBPacketHeader(HEADER_GD_ITEM_FLUSH, 0, sizeof(DWORD));
	db_clientdesc->Packet(&dwID, sizeof(DWORD));

	char szHint[128];
	snprintf(szHint, sizeof(szHint), "%s %u", pkItem->GetName(), pkItem->GetCount());
	if (bMall)
		LogManager::instance().ItemLog(ch, pkItem, "MALL GET", szHint);
	else
		LogManager::instance().ItemLog(ch, pkItem, "SAFEBOX GET", szHint);
}

void CInputMain::SafeboxItemMove(LPCHARACTER ch, const char* data)
{
	struct command_item_move* pinfo = (struct command_item_move*)data;

	if (!ch->CanHandleItem())
		return;

	if (!ch->GetSafebox())
		return;

	ch->GetSafebox()->MoveItem(pinfo->Cell.cell, pinfo->CellTo.cell, pinfo->count);
}

#if defined(BL_REMOTE_SHOP)
static bool RemoteShopRestrictedMap(long lMapIndex)
{
	switch (lMapIndex)
	{
	case 113:
		return true;
	default:
		return false;
	}
}

void CInputMain::RemoteShop(LPCHARACTER ch, const char* data) {
	const TPacketCGRemoteShop* p = reinterpret_cast<const TPacketCGRemoteShop*>(data);

	if (RemoteShopRestrictedMap(ch->GetMapIndex())) {
		ch->ChatPacket(CHAT_TYPE_INFO, "<RemoteShop> You can not open in this map.");
		return;
	}

	if (ch->IsOpenSafebox() || ch->GetExchange() || ch->GetShopOwner() || ch->GetMyShop() || ch->IsCubeOpen()) {
		return;
	}

	const DWORD dwCurrentTime = get_dword_time(), dwLimitTime = ch->GetLastRemoteTime() + 3000; // 3 sec.
	if (dwCurrentTime < dwLimitTime) {
		ch->ChatPacket(CHAT_TYPE_INFO, "<RemoteShop> You have to wait %u sec.", MAX(1, (dwLimitTime - dwCurrentTime) / 1000));
		return;
	}

	static const/*expr*/ DWORD _arrShopVnum[] = { 3, 12, 4 };
	if (p->shop_index >= _countof(_arrShopVnum)) {
		sys_err("RemoteShop player(%s) unknown index(%d)", ch->GetName(), p->shop_index);
		return;
	}

	const DWORD dwShopVnum = _arrShopVnum[p->shop_index];
	LPSHOP pkShop = CShopManager::instance().Get(dwShopVnum);
	if (pkShop == NULL) {
		return;
	}

	if (!pkShop->AddGuest(ch, 0, false, 1)) {
		return;
	}

	ch->SetShopOwner(ch);
	ch->SetLastRemoteTime(get_dword_time());
	sys_log(0, "SHOP: START: %s", ch->GetName());
}
#endif

// PARTY_JOIN_BUG_FIX
void CInputMain::PartyInvite(LPCHARACTER ch, const char* c_pData)
{
	if (!ch)
		return;

	if (ch->GetArena())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("대련장에서 사용하실 수 없습니다."));
		return;
	}

	TPacketCGPartyInvite* p = (TPacketCGPartyInvite*)c_pData;

#if defined(ENABLE_PVP_TOURNAMENT)
	if (ch->GetMapIndex() == PVP_DUEL_MAP_INDEX)
	{
#if defined(ENABLE_TEXT_PACKET)
		ch->ChatPacketNew(CHAT_TYPE_INFO, 41, "");
#endif
		return;
	}
#endif

	LPCHARACTER pInvitee = CHARACTER_MANAGER::instance().Find(p->vid);

	if (!pInvitee || !ch->GetDesc() || !pInvitee->GetDesc())
	{
		sys_err("PARTY Cannot find invited character");
		return;
	}

#if defined(__MESSENGER_BLOCK_SYSTEM__)
	if (MessengerManager::instance().IsBlocked(ch->GetName(), pInvitee->GetName()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Unblock %s to continue."), pInvitee->GetName());
		return;
	}
	else if (MessengerManager::instance().IsBlocked(pInvitee->GetName(), ch->GetName()))
	{
#if defined(ENABLE_TEXT_PACKET)
		ch->ChatPacketNew(CHAT_TYPE_INFO, 15, "%s", pInvitee->GetName());
#endif
		return;
	}
#endif

	ch->PartyInvite(pInvitee);
}

void CInputMain::PartyInviteAnswer(LPCHARACTER ch, const char* c_pData)
{
	if (!ch)
		return;

	if (ch->GetArena())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("대련장에서 사용하실 수 없습니다."));
		return;
	}

	TPacketCGPartyInviteAnswer* p = (TPacketCGPartyInviteAnswer*)c_pData;

	LPCHARACTER pInviter = CHARACTER_MANAGER::instance().Find(p->leader_vid);

	// pInviter 가 ch 에게 파티 요청을 했었다.

	if (!pInviter || !pInviter->IsPC()) {
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 파티요청을 한 캐릭터를 찾을수 없습니다."));
		return;
	}

	if (ch->GetMapIndex() != pInviter->GetMapIndex() && p->accept) {
		ch->ChatPacket(CHAT_TYPE_INFO, "%s need to be in the same map as yours!", pInviter->GetName());
		pInviter->ChatPacket(CHAT_TYPE_INFO, "%s need to be in the same map as yours!", ch->GetName());
		return;
	}

	if (!p->accept) {
		pInviter->PartyInviteDeny(ch->GetPlayerID());
	}
	else {
		pInviter->PartyInviteAccept(ch);
	}
}
// END_OF_PARTY_JOIN_BUG_FIX

void CInputMain::PartySetState(LPCHARACTER ch, const char* c_pData)
{
	if (!CPartyManager::instance().IsEnablePCParty())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 서버 문제로 파티 관련 처리를 할 수 없습니다."));
		return;
	}

	TPacketCGPartySetState* p = (TPacketCGPartySetState*)c_pData;

	if (!ch->GetParty())
		return;

	if (ch->GetParty()->GetLeaderPID() != ch->GetPlayerID())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 리더만 변경할 수 있습니다."));
		return;
	}

	if (!ch->GetParty()->IsMember(p->pid))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 상태를 변경하려는 사람이 파티원이 아닙니다."));
		return;
	}

	DWORD pid = p->pid;
	sys_log(0, "PARTY SetRole pid %d to role %d state %s", pid, p->byRole, p->flag ? "on" : "off");

	switch (p->byRole)
	{
	case PARTY_ROLE_NORMAL:
		break;

	case PARTY_ROLE_ATTACKER:
	case PARTY_ROLE_TANKER:
	case PARTY_ROLE_BUFFER:
	case PARTY_ROLE_SKILL_MASTER:
	case PARTY_ROLE_HASTE:
	case PARTY_ROLE_DEFENDER:
		if (ch->GetParty()->SetRole(pid, p->byRole, p->flag))
		{
			TPacketPartyStateChange pack;
			pack.dwLeaderPID = ch->GetPlayerID();
			pack.dwPID = p->pid;
			pack.bRole = p->byRole;
			pack.bFlag = p->flag;
			db_clientdesc->DBPacket(HEADER_GD_PARTY_STATE_CHANGE, 0, &pack, sizeof(pack));
		}
		/*
		else
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 어태커 설정에 실패하였습니다."));
		*/
		break;

	default:
		sys_err("wrong byRole in PartySetState Packet name %s state %d", ch->GetName(), p->byRole);
		break;
	}
}

void CInputMain::PartyRemove(LPCHARACTER ch, const char* c_pData)
{
	if (ch->GetArena())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("대련장에서 사용하실 수 없습니다."));
		return;
	}

	if (!CPartyManager::instance().IsEnablePCParty())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 서버 문제로 파티 관련 처리를 할 수 없습니다."));
		return;
	}

	if (ch->GetDungeon())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 던전 안에서는 파티에서 추방할 수 없습니다."));
		return;
	}

#ifdef ENABLE_12ZI
	if (ch->GetZodiac() || (ch->GetMapIndex() >= 3550000 && ch->GetMapIndex() < 3560000))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Group] You cannot leave a group while you are in a Zodiac Temple."));
		return;
	}
#endif

	TPacketCGPartyRemove* p = (TPacketCGPartyRemove*)c_pData;

	if (!ch->GetParty())
		return;

	LPPARTY pParty = ch->GetParty();
	if (pParty->GetLeaderPID() == ch->GetPlayerID())
	{
		if (ch->GetDungeon())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 던젼내에서는 파티원을 추방할 수 없습니다."));
		}
		else
		{
			// leader can remove any member
			if (p->pid == ch->GetPlayerID() || pParty->GetMemberCount() == 2)
			{
				// party disband
				CPartyManager::instance().DeleteParty(pParty);
			}
			else
			{
				LPCHARACTER B = CHARACTER_MANAGER::instance().FindByPID(p->pid);
				if (B)
				{
					//pParty->SendPartyRemoveOneToAll(B);
					B->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 파티에서 추방당하셨습니다."));
					//pParty->Unlink(B);
					//CPartyManager::instance().SetPartyMember(B->GetPlayerID(), NULL);
				}
				pParty->Quit(p->pid);
			}
		}
	}
	else
	{
		// otherwise, only remove itself
		if (p->pid == ch->GetPlayerID())
		{
			if (ch->GetDungeon())
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 던젼내에서는 파티를 나갈 수 없습니다."));
			}
			else
			{
				if (pParty->GetMemberCount() == 2)
				{
					// party disband
					CPartyManager::instance().DeleteParty(pParty);
				}
				else
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 파티에서 나가셨습니다."));
					//pParty->SendPartyRemoveOneToAll(ch);
					pParty->Quit(ch->GetPlayerID());
					//pParty->SendPartyRemoveAllToOne(ch);
					//CPartyManager::instance().SetPartyMember(ch->GetPlayerID(), NULL);
				}
			}
		}
		else
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 다른 파티원을 탈퇴시킬 수 없습니다."));
		}
	}
}

void CInputMain::AnswerMakeGuild(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGAnswerMakeGuild* p = (TPacketCGAnswerMakeGuild*)c_pData;

	if (ch->GetGold() < 200000) {
		return;
	}
#if defined(ENABLE_COMMON_CHANGES)
	else if (ch->GetLevel() < 40) {
		return;
	}
#endif

	if (get_global_time() - ch->GetQuestFlag("guild_manage.new_disband_time") <
		CGuildManager::instance().GetDisbandDelay())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 해산한 후 %d일 이내에는 길드를 만들 수 없습니다."),
			quest::CQuestManager::instance().GetEventFlag("guild_disband_delay"));
		return;
	}

	if (get_global_time() - ch->GetQuestFlag("guild_manage.new_withdraw_time") <
		CGuildManager::instance().GetWithdrawDelay())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 탈퇴한 후 %d일 이내에는 길드를 만들 수 없습니다."),
			quest::CQuestManager::instance().GetEventFlag("guild_withdraw_delay"));
		return;
	}

	if (ch->GetGuild())
		return;

	CGuildManager& gm = CGuildManager::instance();

	TGuildCreateParameter cp;
	memset(&cp, 0, sizeof(cp));

	cp.master = ch;
	strlcpy(cp.name, p->guild_name, sizeof(cp.name));

	if (cp.name[0] == 0 || !check_name(cp.name))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("적합하지 않은 길드 이름 입니다."));
		return;
	}

	DWORD dwGuildID = gm.CreateGuild(cp);

	if (dwGuildID)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> [%s] 길드가 생성되었습니다."), cp.name);

		int GuildCreateFee;

		if (LC_IsBrazil())
		{
			GuildCreateFee = 500000;
		}
		else
		{
			GuildCreateFee = 200000;
		}

		ch->PointChange(POINT_GOLD, -GuildCreateFee);
		DBManager::instance().SendMoneyLog(MONEY_LOG_GUILD, ch->GetPlayerID(), -GuildCreateFee);

		char Log[128];
		snprintf(Log, sizeof(Log), "GUILD_NAME %s MASTER %s", cp.name, ch->GetName());
		LogManager::instance().CharLog(ch, 0, "MAKE_GUILD", Log);

		if (g_iUseLocale)
			ch->RemoveSpecifyItem(GUILD_CREATE_ITEM_VNUM, 1);
		//ch->SendGuildName(dwGuildID);
	}
	else
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드 생성에 실패하였습니다."));
}

void CInputMain::PartyUseSkill(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGPartyUseSkill* p = (TPacketCGPartyUseSkill*)c_pData;
	if (!ch->GetParty())
		return;

	if (ch->GetPlayerID() != ch->GetParty()->GetLeaderPID())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 파티 기술은 파티장만 사용할 수 있습니다."));
		return;
	}

	switch (p->bySkillIndex) {
		case PARTY_SKILL_HEAL: {
			ch->GetParty()->HealParty();
			break;
		}
		case PARTY_SKILL_WARP: {
			if (!IS_SUMMONABLE_ZONE(ch->GetMapIndex()) ||
				ch->GetDungeon() || 
				ch->GetWarMap()
			) {
#if defined(ENABLE_TEXT_PACKET)
				ch->ChatPacketNew(CHAT_TYPE_INFO, 32, "");
#endif
				break;
			}

			LPCHARACTER pch = CHARACTER_MANAGER::instance().Find(p->vid);
			if (pch && pch->IsPC()) {
/*
				if (pch->GetDungeon() || 
					pch->GetWarMap() || 
					pch->GetMapIndex() >= 10000
				) {
					return;
				}
*/

			if (ch->GetMapIndex() != pch->GetMapIndex()) {
#if defined(ENABLE_TEXT_PACKET)
				ch->ChatPacketNew(CHAT_TYPE_INFO, 33, "");
#endif
				break;
			}

				ch->GetParty()->SummonToLeader(pch->GetPlayerID());
			} else {
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 소환하려는 대상을 찾을 수 없습니다."));
			}

			break;
		}
		default: {
			break;
		}
	}
}

void CInputMain::PartyParameter(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGPartyParameter* p = (TPacketCGPartyParameter*)c_pData;
#ifdef ENABLE_EXP_GROUP_FIX
	if (ch->GetParty() && ch->GetParty()->GetLeaderPID() == ch->GetPlayerID())
#else
	if (ch->GetParty())
#endif
		ch->GetParty()->SetParameter(p->bDistributeMode);
}

size_t GetSubPacketSize(const GUILD_SUBHEADER_CG& header)
{
	switch (header)
	{
	case GUILD_SUBHEADER_CG_DEPOSIT_MONEY: return sizeof(int);
	case GUILD_SUBHEADER_CG_WITHDRAW_MONEY: return sizeof(int);
	case GUILD_SUBHEADER_CG_ADD_MEMBER: return sizeof(DWORD);
	case GUILD_SUBHEADER_CG_REMOVE_MEMBER: return sizeof(DWORD);
	case GUILD_SUBHEADER_CG_CHANGE_GRADE_NAME: return 10;
	case GUILD_SUBHEADER_CG_CHANGE_GRADE_AUTHORITY: return sizeof(BYTE) + sizeof(BYTE);
	case GUILD_SUBHEADER_CG_OFFER: return sizeof(DWORD);
	case GUILD_SUBHEADER_CG_CHARGE_GSP: return sizeof(int);
	case GUILD_SUBHEADER_CG_POST_COMMENT: return 1;
	case GUILD_SUBHEADER_CG_DELETE_COMMENT: return sizeof(DWORD);
	case GUILD_SUBHEADER_CG_REFRESH_COMMENT: return 0;
	case GUILD_SUBHEADER_CG_CHANGE_MEMBER_GRADE: return sizeof(DWORD) + sizeof(BYTE);
	case GUILD_SUBHEADER_CG_USE_SKILL: return sizeof(TPacketCGGuildUseSkill);
	case GUILD_SUBHEADER_CG_CHANGE_MEMBER_GENERAL: return sizeof(DWORD) + sizeof(BYTE);
	case GUILD_SUBHEADER_CG_GUILD_INVITE_ANSWER: return sizeof(DWORD) + sizeof(BYTE);
	}

	return 0;
}

int CInputMain::Guild(LPCHARACTER ch, const char* data, size_t uiBytes)
{
	if (uiBytes < sizeof(TPacketCGGuild))
		return -1;

	const TPacketCGGuild* p = reinterpret_cast<const TPacketCGGuild*>(data);
	const char* c_pData = data + sizeof(TPacketCGGuild);

	uiBytes -= sizeof(TPacketCGGuild);

	const GUILD_SUBHEADER_CG SubHeader = static_cast<GUILD_SUBHEADER_CG>(p->subheader);
	const size_t SubPacketLen = GetSubPacketSize(SubHeader);

	if (uiBytes < SubPacketLen)
	{
		return -1;
	}

	CGuild* pGuild = ch->GetGuild();

	if (NULL == pGuild)
	{
		if (SubHeader != GUILD_SUBHEADER_CG_GUILD_INVITE_ANSWER)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드에 속해있지 않습니다."));
			return SubPacketLen;
		}
	}

	switch (SubHeader)
	{
	case GUILD_SUBHEADER_CG_DEPOSIT_MONEY:
	{
		// by mhh : 길드자금은 당분간 넣을 수 없다.
		return SubPacketLen;

		const int gold = MIN(*reinterpret_cast<const int*>(c_pData), __deposit_limit());

		if (gold < 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 잘못된 금액입니다."));
			return SubPacketLen;
		}

		if (ch->GetGold() < gold)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 가지고 있는 돈이 부족합니다."));
			return SubPacketLen;
		}

		pGuild->RequestDepositMoney(ch, gold);
	}
	return SubPacketLen;

	case GUILD_SUBHEADER_CG_WITHDRAW_MONEY:
	{
		// by mhh : 길드자금은 당분간 뺄 수 없다.
		return SubPacketLen;

		const int gold = MIN(*reinterpret_cast<const int*>(c_pData), 500000);

		if (gold < 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 잘못된 금액입니다."));
			return SubPacketLen;
		}

		pGuild->RequestWithdrawMoney(ch, gold);
	}
	return SubPacketLen;

	case GUILD_SUBHEADER_CG_ADD_MEMBER:
	{
		const DWORD vid = *reinterpret_cast<const DWORD*>(c_pData);
		LPCHARACTER newmember = CHARACTER_MANAGER::instance().Find(vid);

		if (!newmember)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 그러한 사람을 찾을 수 없습니다."));
			return SubPacketLen;
		}

		if (!ch->IsPC() || !newmember->IsPC())
			return SubPacketLen;

#if defined(__MESSENGER_BLOCK_SYSTEM__)
		if (MessengerManager::instance().IsBlocked(ch->GetName(), newmember->GetName()))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Unblock %s to continue."), newmember->GetName());
			return SubPacketLen;
		}
		else if (MessengerManager::instance().IsBlocked(newmember->GetName(), ch->GetName()))
		{
#if defined(ENABLE_TEXT_PACKET)
			ch->ChatPacketNew(CHAT_TYPE_INFO, 15, "%s", newmember->GetName());
#endif
			return SubPacketLen;
		}
#endif

		if (newmember->GetQuestFlag("change_guild_master.be_other_member") > get_global_time())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 아직 가입할 수 없는 캐릭터입니다"));
			return SubPacketLen;
		}

		pGuild->Invite(ch, newmember);
	}
	return SubPacketLen;

	case GUILD_SUBHEADER_CG_REMOVE_MEMBER:
	{
		if (pGuild->UnderAnyWar() != 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드전 중에는 길드원을 탈퇴시킬 수 없습니다."));
			return SubPacketLen;
		}

		const DWORD pid = *reinterpret_cast<const DWORD*>(c_pData);
		const TGuildMember* m = pGuild->GetMember(ch->GetPlayerID());

		if (NULL == m)
			return -1;

		LPCHARACTER member = CHARACTER_MANAGER::instance().FindByPID(pid);

		if (member)
		{
			if (member->GetGuild() != pGuild)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 상대방이 같은 길드가 아닙니다."));
				return SubPacketLen;
			}

			if (!pGuild->HasGradeAuth(m->grade, GUILD_AUTH_REMOVE_MEMBER))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드원을 강제 탈퇴 시킬 권한이 없습니다."));
				return SubPacketLen;
			}

			member->SetQuestFlag("guild_manage.new_withdraw_time", get_global_time());
			pGuild->RequestRemoveMember(member->GetPlayerID());

			if (LC_IsBrazil() == true)
			{
				DBManager::instance().Query("REPLACE INTO guild_invite_limit VALUES(%d, %d)", pGuild->GetID(), get_global_time());
			}
		}
		else
		{
			if (!pGuild->HasGradeAuth(m->grade, GUILD_AUTH_REMOVE_MEMBER))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드원을 강제 탈퇴 시킬 권한이 없습니다."));
				return SubPacketLen;
			}

			if (pGuild->RequestRemoveMember(pid))
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드원을 강제 탈퇴 시켰습니다."));
			else
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 그러한 사람을 찾을 수 없습니다."));
		}
	}
	return SubPacketLen;

	case GUILD_SUBHEADER_CG_CHANGE_GRADE_NAME:
	{
		char gradename[GUILD_GRADE_NAME_MAX_LEN + 1];
		strlcpy(gradename, c_pData + 1, sizeof(gradename));

		const TGuildMember* m = pGuild->GetMember(ch->GetPlayerID());

		if (NULL == m)
			return -1;

		if (m->grade != GUILD_LEADER_GRADE)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 직위 이름을 변경할 권한이 없습니다."));
		}
		else if (*c_pData == GUILD_LEADER_GRADE)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드장의 직위 이름은 변경할 수 없습니다."));
		}
		else if (!check_name(gradename))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 적합하지 않은 직위 이름 입니다."));
		}
		else
		{
			pGuild->ChangeGradeName(*c_pData, gradename);
		}
	}
	return SubPacketLen;

	case GUILD_SUBHEADER_CG_CHANGE_GRADE_AUTHORITY:
	{
		const TGuildMember* m = pGuild->GetMember(ch->GetPlayerID());

		if (NULL == m)
			return -1;

		if (m->grade != GUILD_LEADER_GRADE)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 직위 권한을 변경할 권한이 없습니다."));
		}
		else if (*c_pData == GUILD_LEADER_GRADE)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드장의 권한은 변경할 수 없습니다."));
		}
		else
		{
			pGuild->ChangeGradeAuth(*c_pData, *(c_pData + 1));
		}
	}
	return SubPacketLen;

	case GUILD_SUBHEADER_CG_OFFER:
	{
		DWORD offer = *reinterpret_cast<const DWORD*>(c_pData);

		if (pGuild->GetLevel() >= GUILD_MAX_LEVEL && LC_IsHongKong() == false)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드가 이미 최고 레벨입니다."));
		}
		else
		{
			offer /= 100;
			offer *= 100;

			if (pGuild->OfferExp(ch, offer))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> %u의 경험치를 투자하였습니다."), offer);
			}
			else
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 경험치 투자에 실패하였습니다."));
			}
		}
	}
	return SubPacketLen;

	case GUILD_SUBHEADER_CG_CHARGE_GSP:
	{
		const int offer = *reinterpret_cast<const int*>(c_pData);
		const int gold = offer * 100;

		if (offer < 0 || gold < offer || gold < 0 || ch->GetGold() < gold)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 돈이 부족합니다."));
			return SubPacketLen;
		}

		if (!pGuild->ChargeSP(ch, offer))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 용신력 회복에 실패하였습니다."));
		}
	}
	return SubPacketLen;

	case GUILD_SUBHEADER_CG_POST_COMMENT:
	{
		const size_t length = *c_pData;

		if (length > GUILD_COMMENT_MAX_LEN)
		{
			// 잘못된 길이.. 끊어주자.
			sys_err("POST_COMMENT: %s comment too long (length: %u)", ch->GetName(), length);
			ch->GetDesc()->SetPhase(PHASE_CLOSE);
			return -1;
		}

		if (uiBytes < 1 + length)
			return -1;

		const TGuildMember* m = pGuild->GetMember(ch->GetPlayerID());

		if (NULL == m)
			return -1;

		if (length && !pGuild->HasGradeAuth(m->grade, GUILD_AUTH_NOTICE) && *(c_pData + 1) == '!')
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 공지글을 작성할 권한이 없습니다."));
		}
		else
		{
			std::string str(c_pData + 1, length);
			pGuild->AddComment(ch, str);
		}

		return (1 + length);
	}

	case GUILD_SUBHEADER_CG_DELETE_COMMENT:
	{
		const DWORD comment_id = *reinterpret_cast<const DWORD*>(c_pData);

		pGuild->DeleteComment(ch, comment_id);
	}
	return SubPacketLen;

	case GUILD_SUBHEADER_CG_REFRESH_COMMENT:
		pGuild->RefreshComment(ch);
		return SubPacketLen;

	case GUILD_SUBHEADER_CG_CHANGE_MEMBER_GRADE:
	{
		const DWORD pid = *reinterpret_cast<const DWORD*>(c_pData);
		const BYTE grade = *(c_pData + sizeof(DWORD));
		const TGuildMember* m = pGuild->GetMember(ch->GetPlayerID());

		if (NULL == m)
			return -1;

		if (m->grade != GUILD_LEADER_GRADE)
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 직위를 변경할 권한이 없습니다."));
		else if (ch->GetPlayerID() == pid)
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드장의 직위는 변경할 수 없습니다."));
		else if (grade == 1)
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드장으로 직위를 변경할 수 없습니다."));
		else
			pGuild->ChangeMemberGrade(pid, grade);
	}
	return SubPacketLen;

	case GUILD_SUBHEADER_CG_USE_SKILL:
	{
		const TPacketCGGuildUseSkill* p = reinterpret_cast<const TPacketCGGuildUseSkill*>(c_pData);

		pGuild->UseSkill(p->dwVnum, ch, p->dwPID);
	}
	return SubPacketLen;

	case GUILD_SUBHEADER_CG_CHANGE_MEMBER_GENERAL:
	{
		const DWORD pid = *reinterpret_cast<const DWORD*>(c_pData);
		const BYTE is_general = *(c_pData + sizeof(DWORD));
		const TGuildMember* m = pGuild->GetMember(ch->GetPlayerID());

		if (NULL == m)
			return -1;

		if (m->grade != GUILD_LEADER_GRADE)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 장군을 지정할 권한이 없습니다."));
		}
		else
		{
			if (!pGuild->ChangeMemberGeneral(pid, is_general))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 더이상 장수를 지정할 수 없습니다."));
			}
		}
	}
	return SubPacketLen;

	case GUILD_SUBHEADER_CG_GUILD_INVITE_ANSWER:
	{
		const DWORD guild_id = *reinterpret_cast<const DWORD*>(c_pData);
		const BYTE accept = *(c_pData + sizeof(DWORD));

		CGuild* g = CGuildManager::instance().FindGuild(guild_id);

		if (g)
		{
			if (accept)
				g->InviteAccept(ch);
			else
				g->InviteDeny(ch->GetPlayerID());
		}
	}
	return SubPacketLen;

	}

	return 0;
}

void CInputMain::Fishing(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGFishing* p = (TPacketCGFishing*)c_pData;
	ch->SetRotation(p->dir * 5);
	ch->fishing();
	return;
}

void CInputMain::ItemGive(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGGiveItem* p = (TPacketCGGiveItem*)c_pData;
	LPCHARACTER to_ch = CHARACTER_MANAGER::instance().Find(p->dwTargetVID);

	if (to_ch)
		ch->GiveItem(to_ch, p->ItemPos);
	else
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("아이템을 건네줄 수 없습니다."));
}

#ifdef ENABLE_CSHIELD
void CInputMain::Hack(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGHack* p = (TPacketCGHack*)c_pData;

	if (!p || !ch)
		return;

	LogManager::instance().CShieldLog(ch, p->errorcode);
}
#else
void CInputMain::Hack(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGHack* p = (TPacketCGHack*)c_pData;

	char buf[sizeof(p->szBuf)];
	strlcpy(buf, p->szBuf, sizeof(buf));

	sys_err("HACK_DETECT: %s %s", ch->GetName(), buf);

	ch->GetDesc()->SetPhase(PHASE_CLOSE);
}
#endif

#if defined(__MYSHOP_DECO__)
void CInputMain::MyShopDeco(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGMyShopDeco* p = (TPacketCGMyShopDeco*)c_pData;

	if (!ch)
		return;

	// NOTE : We will check for the KashmirBundle when the packet is sent to server
	// .
	if (ch->CountSpecifyItem(KASHMIR_BUNDLE) <= 0)
	{
		sys_err("MyShopDeco : Kashmir Bundle not found (%s).", ch->GetName());
		return;
	}

	if (p->dwPolyVnum >= 30000 && p->dwPolyVnum <= 30008)
	{
		ch->SetMyShopDecoType(p->bType);
		ch->SetMyShopDecoPolyVnum(p->dwPolyVnum);
		ch->SetMyShopTabs(MYSHOP_MAX_TABS);
		ch->ChatPacket(CHAT_TYPE_COMMAND, "MyPrivShopOpen %d %d", TRUE, MYSHOP_MAX_TABS);
	}
	else
	{
		sys_err("MyShopDeco : Unkown PolyVnum");
		return;
	}
}
#endif

int CInputMain::MyShop(LPCHARACTER ch, const char* c_pData, size_t uiBytes)
{
	TPacketCGMyShop* p = (TPacketCGMyShop*)c_pData;
	int iExtraLen = p->wCount * sizeof(TShopItemTable);

	if (uiBytes < sizeof(TPacketCGMyShop) + iExtraLen)
		return -1;

	if ((uint64_t)(ch->GetGold()) >= g_MaxGold)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("소유 돈이 20억냥을 넘어 거래를 핼수가 없습니다."));
		sys_log(0, "MyShop ==> OverFlow Gold id %u name %s ", ch->GetPlayerID(), ch->GetName());
		return (iExtraLen);
	}

#if defined(ENABLE_CHEQUE_SYSTEM)
	if (ch->GetCheque() >= CHEQUE_MAX)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("소유 돈이 20억냥을 넘어 거래를 핼수가 없습니다."));
		sys_log(0, "MyShop ==> OverFlow Cheque id %u name %s ", ch->GetPlayerID(), ch->GetName());
		return (iExtraLen);
	}
#endif

	if (ch->IsStun() || ch->IsDead())
		return (iExtraLen);

	if (ch->GetExchange() || ch->IsOpenSafebox() || ch->GetShopOwner() || ch->IsCubeOpen()
#if defined(__MAILBOX__)
 || ch->GetMailBox()
#endif
#ifdef ENABLE_AURA_SYSTEM
		|| ch->IsAuraRefineWindowOpen()
#endif
		)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("다른 거래중일경우 개인상점을 열수가 없습니다."));
		return (iExtraLen);
	}

	sys_log(0, "MyShop count %d", p->wCount);
	ch->OpenMyShop(p->szSign, (TShopItemTable*)(c_pData + sizeof(TPacketCGMyShop)), p->wCount);
	return (iExtraLen);
}

#ifdef ENABLE_SHOW_CHEST_DROP
void CInputMain::ChestDropInfo(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGChestDropInfo* p = (TPacketCGChestDropInfo*) c_pData;

	if(p->wInventoryCell >= INVENTORY_AND_EQUIP_SLOT_MAX)
		return;
	
	LPITEM pkItem = ch->GetInventoryItem(p->wInventoryCell);

	if (!pkItem)
		return;
	
	std::vector<TChestDropInfoTable> vec_ItemList;
	ITEM_MANAGER::instance().GetChestItemList(pkItem->GetVnum(), vec_ItemList);

	if (vec_ItemList.size() == 0) {
		return;
	}

	TPacketGCChestDropInfo packet;
	packet.bHeader = HEADER_GC_CHEST_DROP_INFO;
	packet.wSize = sizeof(packet) + sizeof(TChestDropInfoTable) * vec_ItemList.size();
	packet.dwChestVnum = pkItem->GetVnum();

	ch->GetDesc()->BufferedPacket(&packet, sizeof(packet));
	ch->GetDesc()->Packet(&vec_ItemList[0], sizeof(TChestDropInfoTable) * vec_ItemList.size());
}
#endif

void CInputMain::Refine(LPCHARACTER ch, const char* c_pData)
{
	const TPacketCGRefine* p = reinterpret_cast<const TPacketCGRefine*>(c_pData);

	if (ch->GetExchange() || ch->IsOpenSafebox() || ch->GetShopOwner() || ch->GetMyShop() || ch->IsCubeOpen()
#if defined(__MAILBOX__)
 || ch->GetMailBox()
#endif
#ifdef ENABLE_AURA_SYSTEM
		|| ch->IsAuraRefineWindowOpen()
#endif
		)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("창고,거래창등이 열린 상태에서는 개량을 할수가 없습니다"));
		ch->ClearRefineMode();
		return;
	}

	if (p->type == 255)
	{
		// DoRefine Cancel
		ch->ClearRefineMode();
		return;
	}

	if (p->pos >= INVENTORY_MAX_NUM)
	{
		ch->ClearRefineMode();
		return;
	}

	LPITEM item = ch->GetInventoryItem(p->pos);

	if (!item)
	{
		ch->ClearRefineMode();
		return;
	}

	ch->SetRefineTime();

	if (p->type == REFINE_TYPE_NORMAL)
	{
		sys_log(0, "refine_type_noraml");
		ch->DoRefine(item);
	}
	else if (p->type == REFINE_TYPE_SCROLL || p->type == REFINE_TYPE_HYUNIRON || p->type == REFINE_TYPE_MUSIN || p->type == REFINE_TYPE_BDRAGON
#if defined(__STONE_OF_BLESS__)
		|| p->type == REFINE_TYPE_STONE_OF_BLESS
#endif
#if defined(__SOUL_SYSTEM__)
		|| p->type == REFINE_TYPE_SOUL_EVOLVE
		|| p->type == REFINE_TYPE_SOUL_AWAKE
#endif
		)
	{
		sys_log(0, "refine_type_scroll, ...");
		ch->DoRefineWithScroll(item);
	}
	else if (p->type == REFINE_TYPE_MONEY_ONLY)
	{
		const LPITEM item = ch->GetInventoryItem(p->pos);

		if (NULL != item)
		{
			if (500 <= item->GetRefineSet())
			{
				LogManager::instance().HackLog("DEVIL_TOWER_REFINE_HACK", ch);
			}
			else
			{
				if (ch->GetQuestFlag("deviltower_zone.can_refine"))
				{
#if defined(ENABLE_COMMON_CHANGES)
					if (ch->DoRefine(item, true)) {
						ch->SetQuestFlag("deviltower_zone.can_refine", 0);
					}
#else
					ch->DoRefine(item, true);
					ch->SetQuestFlag("deviltower_zone.can_refine", 0);
#endif
				}
				else
				{
					ch->ChatPacket(CHAT_TYPE_INFO, "사귀 타워 완료 보상은 한번까지 사용가능합니다.");
				}
			}
		}
	}

	ch->ClearRefineMode();
}

#if defined(__ACCE_COSTUME_SYSTEM__)
void CInputMain::Acce(LPCHARACTER pkChar, const char* c_pData)
{
	quest::PC* pPC = quest::CQuestManager::instance().GetPCForce(pkChar->GetPlayerID());
	if (pPC->IsRunning())
		return;

	TPacketAcce* sPacket = (TPacketAcce*)c_pData;
	switch (sPacket->subheader)
	{
	case ACCE_SUBHEADER_CG_CLOSE:
	{
		pkChar->CloseAcce();
	}
	break;
	case ACCE_SUBHEADER_CG_ADD:
	{
		pkChar->AddAcceMaterial(sPacket->tPos, sPacket->bPos);
	}
	break;
	case ACCE_SUBHEADER_CG_REMOVE:
	{
		pkChar->RemoveAcceMaterial(sPacket->bPos);
	}
	break;
	case ACCE_SUBHEADER_CG_REFINE:
	{
		pkChar->RefineAcceMaterials();
	}
	break;
	default:
		break;
	}
}
#endif

#if defined(__CHANGE_LOOK_SYSTEM__)
void CInputMain::ChangeLook(LPCHARACTER pkChar, const char* c_pData)
{
	quest::PC* pPC = quest::CQuestManager::instance().GetPCForce(pkChar->GetPlayerID());
	if (pPC->IsRunning())
		return;

	TPacketChangeLook* sPacket = (TPacketChangeLook*)c_pData;
	switch (sPacket->bSubHeader)
	{
	case SUBHEADER_CHANGE_LOOK_CLOSE:
		pkChar->ChangeLookWindow(false);
		break;

	case SUBHEADER_CHANGE_LOOK_ADD:
		pkChar->AddChangeLookMaterial(sPacket->tPos, sPacket->bPos);
		break;

	case SUBHEADER_CHANGE_LOOK_REMOVE:
		pkChar->RemoveChangeLookMaterial(sPacket->bPos);
		break;

	case SUBHEADER_CHANGE_LOOK_REFINE:
		pkChar->RefineChangeLookMaterials();
		break;

	default:
		break;
	}
}
#endif

#if defined(__SEND_TARGET_INFO__)
void CInputMain::TargetInfoLoad(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGTargetInfoLoad* p = (TPacketCGTargetInfoLoad*)c_pData;
	TPacketGCTargetInfo GCPacket;
	GCPacket.bHeader = HEADER_GC_TARGET_INFO;

	LPCHARACTER m_pkChrTarget = CHARACTER_MANAGER::instance().Find(p->dwVID);

	if (!ch || !m_pkChrTarget)
		return;

	if (m_pkChrTarget->IsMonster() || m_pkChrTarget->IsStone())
	{
		MonsterItemDropMap MonsterItemDropMap;
		MonsterItemDropMap.clear();

		ITEM_MANAGER::instance().GetMonsterItemDropMap(m_pkChrTarget, ch, MonsterItemDropMap);
		if (!MonsterItemDropMap.empty())
		{
			int iCount = 0;
			for (auto item : MonsterItemDropMap)
			{
				GCPacket.bIndex = iCount++;
				GCPacket.dwVID = m_pkChrTarget->GetVID();
				GCPacket.dwRaceVnum = m_pkChrTarget->GetRaceNum();
				GCPacket.dwVnum = item.first;
				GCPacket.wCount = item.second;
				ch->GetDesc()->Packet(&GCPacket, sizeof(GCPacket));
			}
		}
	}
	ch->ChatPacket(CHAT_TYPE_COMMAND, "RefreshMonsterDropInfo %d", m_pkChrTarget->GetRaceNum());
}
#endif


void CInputMain::ChangeLanguage(LPCHARACTER ch, BYTE bLanguage)
{
	if (!ch)
		return;

	if (!ch->GetDesc())
		return;

	if (ch->GetLanguage() == bLanguage)
		return;

	if (bLanguage > LOCALE_YMIR && bLanguage < LOCALE_MAX_NUM)
	{
		TRequestChangeLanguage packet;
		packet.dwAID = ch->GetDesc()->GetAccountTable().id;
		packet.bLanguage = bLanguage;

		db_clientdesc->DBPacketHeader(HEADER_GD_REQUEST_CHANGE_LANGUAGE, 0, sizeof(TRequestChangeLanguage));
		db_clientdesc->Packet(&packet, sizeof(packet));

		ch->ChangeLanguage(bLanguage);
	}
}

#if defined(__SKILLBOOK_COMB_SYSTEM__)
bool CInputMain::SkillBookCombination(LPCHARACTER ch, TItemPos(&CombItemGrid)[SKILLBOOK_COMB_SLOT_MAX], BYTE bAction)
{
	if (!ch->GetDesc())
		return false;

	// if (CombItemGrid.empty())
		// return false;

	if (ch->GetExchange() || ch->GetShop() || ch->GetMyShop() || ch->IsOpenSafebox() || ch->IsCubeOpen())
		return false;

	if (bAction != 2/*COMBI_START*/)
		return false;

	std::set <LPITEM> set_items;
	for (int i = 0; i < SKILLBOOK_COMB_SLOT_MAX; i++)
	{
		LPITEM pItem = ch->GetItem(CombItemGrid[i]);
		if (pItem)
		{
			if (pItem->GetType() != ITEM_SKILLBOOK)
				return false;

			set_items.insert(pItem);
		}
	}

	if (set_items.empty())
		return false;

	if (ch->GetGold() < SKILLBOOK_COMB_COST)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You don't have enough Yang to trade books with me."));
		return false;
	}

	for (std::set <LPITEM>::iterator it = set_items.begin(); it != set_items.end(); ++it)
	{
		LPITEM pItem = *it;
		if (pItem)
		{
			pItem->SetCount(pItem->GetCount() - 1);
			//pItem->RemoveFromCharacter();
			//M2_DESTROY_ITEM(pItem);
		}
	}

	DWORD dwBooks[JOB_MAX_NUM][2/*SKILL_GROUPS*/][2] = {
		{ // 0 - Warrior
			{50401, 50406}, // Skill Group 1
			{50416, 50421}, // Skill Group 2
		},
		{ // 1 - Ninja
			{50431, 50436}, // Skill Group 1
			{50446, 50451}, // Skill Group 2
		},
		{ // 2 - Sura
			{50461, 50466}, // Skill Group 1
			{50476, 50481}, // Skill Group 2
		},
		{ // 3 - Shaman
			{50491, 50496}, // Skill Group 1
			{50506, 50511}, // Skill Group 2
		},
		{ // 4 - Wolfman
			{50530, 50535}, // Skill Group 1
			{0, 0}, // Skill Group 2
		},
	};

	ch->PointChange(POINT_GOLD, -SKILLBOOK_COMB_COST);

	if (ch->GetSkillGroup() != 0)
	{
		DWORD dwMinRandomBook = dwBooks[ch->GetJob()][ch->GetSkillGroup() - 1][0];
		DWORD dwMaxRandomBook = dwBooks[ch->GetJob()][ch->GetSkillGroup() - 1][1];

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<> dis(dwMinRandomBook, dwMaxRandomBook);
		DWORD dwRandomBook = dis(gen);

		ch->AutoGiveItem(dwRandomBook, 1);
	}
	else
		ch->AutoGiveItem(50300, 1);

	return true;
}
#endif

void CInputMain::WhisperDetails(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGWhisperDetails* CGWhisperDetails = (TPacketCGWhisperDetails*)c_pData;

	if (!*CGWhisperDetails->name)
		return;

	TPacketGCWhisperDetails GCWhisperDetails;
	GCWhisperDetails.header = HEADER_GC_WHISPER_DETAILS;
	strncpy(GCWhisperDetails.name, CGWhisperDetails->name, sizeof(GCWhisperDetails.name) - 1);

	BYTE bLanguage = LOCALE_DEFAULT;

	LPCHARACTER pkChr = CHARACTER_MANAGER::instance().FindPC(CGWhisperDetails->name);

	if (!pkChr)
	{
		LPDESC pkDesc = NULL;
		CCI* pkCCI = P2P_MANAGER::instance().Find(CGWhisperDetails->name);

		if (pkCCI)
		{
			pkDesc = pkCCI->pkDesc;
			if (pkDesc)
				bLanguage = pkCCI->bLanguage;
		}
	}
	else
	{
		bLanguage = pkChr->GetLanguage();
	}

	GCWhisperDetails.bLanguage = bLanguage;
	ch->GetDesc()->Packet(&GCWhisperDetails, sizeof(GCWhisperDetails));
}

#if defined(__MAILBOX__)
#if defined(ENABLE_COMMON_CHANGES)
#include "MailBox.h"

static char ___name[CHARACTER_NAME_MAX_LEN * 2 + 1];
static char ___title[25 * 2 + 1];
static char ___message[100 * 2 + 1];
#endif

void CInputMain::MailboxWrite(LPCHARACTER ch, const char* c_pData)
{
	const auto* p = reinterpret_cast<const TPacketCGMailboxWrite*>(c_pData);
	if (p == nullptr)
		return;

	CMailBox* mail = ch->GetMailBox();
	if (mail == nullptr)
		return;

#if defined(ENABLE_COMMON_CHANGES)
	DBManager::instance().EscapeString(___name, sizeof(___name), p->szName, strlen(p->szName));
	DBManager::instance().EscapeString(___title, sizeof(___title), p->szTitle, strlen(p->szTitle));
	DBManager::instance().EscapeString(___message, sizeof(___message), p->szMessage, strlen(p->szMessage));

	if (strcmp(___name, p->szName) != 0) {
		sys_err("The player: %s (pid: %u) is trying to use SQL vulnerability (%s - %s).", ch->GetName(),
																						  ch->GetPlayerID(),
																						  ___name,
																						  p->szName);
		mail->AlertSQL(0);
		return;
	} else if (strcmp(___title, p->szTitle) != 0) {
		sys_err("The player: %s (pid: %u) is trying to use SQL vulnerability (%s - %s).", ch->GetName(),
																						  ch->GetPlayerID(),
																						  ___title,
																						  p->szTitle);
		mail->AlertSQL(1);
		return;
	} else if (strcmp(___message, p->szMessage) != 0) {
		sys_err("The player: %s (pid: %u) is trying to use SQL vulnerability (%s - %s).", ch->GetName(),
																						  ch->GetPlayerID(),
																						  ___message,
																						  p->szMessage);
		mail->AlertSQL(2);
		return;
	}

	mail->Write(___name, ___title, ___message, p->pos, p->iYang, p->iWon);
#else
	mail->Write(p->szName, p->szTitle, p->szMessage, p->pos, p->iYang, p->iWon);
#endif
}

void CInputMain::MailboxConfirm(LPCHARACTER ch, const char* c_pData)
{
	const auto* p = reinterpret_cast<const TPacketCGMailboxWriteConfirm*>(c_pData);
	if (p == nullptr)
		return;

	CMailBox* mail = ch->GetMailBox();
	if (mail == nullptr)
		return;

#if defined(ENABLE_COMMON_CHANGES)
	DBManager::instance().EscapeString(___name, sizeof(___name), p->szName, strlen(p->szName));
	if (strcmp(___name, p->szName) != 0) {
		sys_err("The player: %s (pid: %u) is trying to use SQL vulnerability (%s - %s).", ch->GetName(),
																						  ch->GetPlayerID(),
																						  ___name,
																						  p->szName);
		mail->AlertSQL(0);
		return;
	}
#endif

	mail->CheckPlayer(p->szName);
}

void CInputMain::MailboxProcess(LPCHARACTER ch, const char* c_pData)
{
	const auto* p = reinterpret_cast<const TPacketMailboxProcess*>(c_pData);
	if (p == nullptr)
		return;

	CMailBox* mail = ch->GetMailBox();
	if (mail == nullptr)
		return;

	switch (p->bSubHeader)
	{
	case CMailBox::EMAILBOX_CG::MAILBOX_CG_CLOSE:
		ch->SetMailBox(nullptr);
		break;
	case CMailBox::EMAILBOX_CG::MAILBOX_CG_DELETE:
		mail->DeleteMail(p->bArg1, false);
		break;
	case CMailBox::EMAILBOX_CG::MAILBOX_CG_ALL_DELETE:
		mail->DeleteAllMails();
		break;
	case CMailBox::EMAILBOX_CG::MAILBOX_CG_GET_ITEMS:
		mail->GetItem(p->bArg1, false);
		break;
	case CMailBox::EMAILBOX_CG::MAILBOX_CG_ALL_GET_ITEMS:
		mail->GetAllItems();
		break;
	case CMailBox::EMAILBOX_CG::MAILBOX_CG_ADD_DATA:
		mail->AddData(p->bArg1, p->bArg2);
		break;
	default:
		sys_err("CInputMain::MailboxProcess Unknown SubHeader (ch: %s) (%d)", ch->GetName(), p->bSubHeader);
		break;
	}
}
#endif

#ifdef __GROWTH_PET_SYSTEM__
void CInputMain::PetHatch(LPCHARACTER ch, const char* c_pData)
{
	const TPacketCGPetHatch* p = reinterpret_cast<const TPacketCGPetHatch*>(c_pData);

	if (!ch || !ch->GetDesc())
		return;

	CGrowthPetManager::Instance().EggHatch(ch, p->name, p->eggPos);
}

void CInputMain::PetWindow(LPCHARACTER ch, const char* c_pData)
{
	const TPacketCGPetWindow* p = reinterpret_cast<const TPacketCGPetWindow*>(c_pData);

	if (!ch || !ch->GetDesc())
		return;

	switch (p->window)
	{
		case PET_WINDOW_HATCH:
			ch->SetPetHatchWindow(p->state);
			break;

		case PET_WINDOW_NAME_CHANGE:
			ch->SetPetChangeNameWindow(p->state);
			break;
	}
}

void CInputMain::PetWindowType(LPCHARACTER ch, const char* c_pData)
{
	const TPacketCGPetWindowType* p = reinterpret_cast<const TPacketCGPetWindowType*>(c_pData);

	if (!ch || !ch->GetDesc())
		return;

	BYTE bWindowType = p->type;

	if (bWindowType == ch->GetPetWindowType())
		return;

	if (bWindowType == PET_WINDOW_ATTR_CHANGE || bWindowType == PET_WINDOW_PRIMIUM_FEEDSTUFF)
	{

		if ((ch->GetExchange() || ch->IsOpenSafebox() || ch->GetShopOwner()) || ch->IsCubeOpen())
			return;
	}

	ch->SetPetWindowType(bWindowType);

	TPacketGCPet packet;
	packet.header = HEADER_GC_PET;
	packet.subheader = SUBHEADER_PET_WINDOW_TYPE_INFO + bWindowType;

	ch->GetDesc()->Packet(&packet, sizeof(packet));
}

void CInputMain::PetFeed(LPCHARACTER ch, const char* c_pData)
{
	const TPacketCGPetFeed* p = reinterpret_cast<const TPacketCGPetFeed*>(c_pData);
	
	if (!ch || !ch->GetDesc())
		return;

	if (ch->GetActiveGrowthPet())
		ch->GetActiveGrowthPet()->Feed(p);
	else
		ch->ChatPacket(CHAT_TYPE_INFO, "Activate your pet first.");
}

void CInputMain::PetDetermine(LPCHARACTER ch, const char* c_pData)
{
	const TPacketCGPetDetermine* p = reinterpret_cast<const TPacketCGPetDetermine*>(c_pData);

	if (!ch || !ch->GetDesc())
		return;

	LPITEM pDetermineItem = ch->GetInventoryItem(p->determinePos.cell);
	if (!pDetermineItem)
		return;

	if (pDetermineItem->GetType() != ITEM_PET || pDetermineItem->GetSubType() != PET_ATTR_DETERMINE)
		return;

	if (!ch->GetActiveGrowthPet())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Activate your pet first.");
		return;
	}

	ch->GetActiveGrowthPet()->AttrDetermine(pDetermineItem);
}

void CInputMain::PetAttrChange(LPCHARACTER ch, const char* c_pData)
{
	const TPacketCGPetAttrChange* p = reinterpret_cast<const TPacketCGPetAttrChange*>(c_pData);

	if (!ch || !ch->GetDesc())
		return;

	LPITEM pUpBringingItem = ch->GetInventoryItem(p->upBringingPos.cell);
	if (!pUpBringingItem)
		return;

	if (pUpBringingItem->GetType() != ITEM_PET || pUpBringingItem->GetSubType() != PET_UPBRINGING)
		return;

	LPITEM pChangeAttrItem = ch->GetInventoryItem(p->attrChangePos.cell);
	if (!pChangeAttrItem)
		return;

	if (pChangeAttrItem->GetType() != ITEM_PET || pChangeAttrItem->GetSubType() != PET_ATTR_CHANGE)
		return;

	LPGROWTH_PET pPet = ch->GetGrowthPet(pUpBringingItem->GetSocket(2));
	if (!pPet)
		return;

	pPet->AttrChange(pUpBringingItem, pChangeAttrItem);
}

void CInputMain::PetRevive(LPCHARACTER ch, const char* c_pData)
{
	const TPacketCGPetRevive* p = reinterpret_cast<const TPacketCGPetRevive*>(c_pData);

	if (!ch || !ch->GetDesc())
		return;

	LPITEM pUpBringingItem = ch->GetInventoryItem(p->upBringingPos.cell);
	if (!pUpBringingItem)
		return;

	if (pUpBringingItem->GetType() != ITEM_PET || pUpBringingItem->GetSubType() != PET_UPBRINGING)
		return;

	LPGROWTH_PET pPet = ch->GetGrowthPet(pUpBringingItem->GetSocket(2));
	if (!pPet)
		return;

	pPet->PremiumFeed(p, pUpBringingItem);
}

void CInputMain::PetLearnSkill(LPCHARACTER ch, const char* c_pData)
{
	const TPacketCGPetLearnSkill* p = reinterpret_cast<const TPacketCGPetLearnSkill*>(c_pData);

	if (!ch || !ch->GetDesc())
		return;

	LPITEM pSkillBookItem = ch->GetInventoryItem(p->learnSkillPos.cell);
	if (!pSkillBookItem)
		return;

	if (pSkillBookItem->GetType() != ITEM_PET || pSkillBookItem->GetSubType() != PET_SKILL)
		return;

	if (!ch->GetActiveGrowthPet())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Activate your pet first.");
		return;
	}

	ch->GetActiveGrowthPet()->LearnSkill(p->slotIndex, pSkillBookItem);
}

void CInputMain::PetSkillUpgrade(LPCHARACTER ch, const char* c_pData)
{
	const TPacketCGPetSkillUpgrade* p = reinterpret_cast<const TPacketCGPetSkillUpgrade*>(c_pData);

	if (!ch->GetActiveGrowthPet())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Activate your pet first.");
		return;
	}

	ch->GetActiveGrowthPet()->SkillUpgrade(p->slotIndex);
}

void CInputMain::PetDeleteSkill(LPCHARACTER ch, const char* c_pData)
{
	const TPacketCGPetDeleteSkill* p = reinterpret_cast<const TPacketCGPetDeleteSkill*>(c_pData);

	if (!ch || !ch->GetDesc())
		return;

	LPITEM pDeleteSkillItem = ch->GetInventoryItem(p->deleteSkillPos.cell);
	if (!pDeleteSkillItem)
		return;

	if (pDeleteSkillItem->GetType() != ITEM_PET || pDeleteSkillItem->GetSubType() != PET_SKILL_DEL_BOOK)
		return;

	if (!ch->GetActiveGrowthPet())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Activate your pet first.");
		return;
	}

	ch->GetActiveGrowthPet()->DeleteSkill(p->slotIndex, pDeleteSkillItem);
}

void CInputMain::PetDeleteAllSkill(LPCHARACTER ch, const char* c_pData)
{
	const TPacketCGPetDeleteAllSkill* p = reinterpret_cast<const TPacketCGPetDeleteAllSkill*>(c_pData);

	if (!ch || !ch->GetDesc())
		return;

	LPITEM pDeleteAllSkillItem = ch->GetInventoryItem(p->deleteAllSkillPos.cell);
	if (!pDeleteAllSkillItem)
		return;

	if (pDeleteAllSkillItem->GetType() != ITEM_PET || pDeleteAllSkillItem->GetSubType() != PET_SKILL_ALL_DEL_BOOK)
		return;

	if (!ch->GetActiveGrowthPet())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Activate your pet first.");
		return;
	}

	ch->GetActiveGrowthPet()->DeleteAllSkill(pDeleteAllSkillItem);
}

void CInputMain::PetNameChange(LPCHARACTER ch, const char* c_pData)
{
	const TPacketCGPetNameChange* p = reinterpret_cast<const TPacketCGPetNameChange*>(c_pData);

	if (!ch || !ch->GetDesc())
		return;

	LPITEM pNameChangeItem = ch->GetInventoryItem(p->changeNamePos.cell);
	if (!pNameChangeItem)
		return;

	if (pNameChangeItem->GetType() != ITEM_PET || pNameChangeItem->GetSubType() != PET_NAME_CHANGE)
		return;

	LPITEM pUpBringingItem = ch->GetInventoryItem(p->upBringingPos.cell);
	if (!pUpBringingItem)
		return;

	if (pUpBringingItem->GetType() != ITEM_PET || pUpBringingItem->GetSubType() != PET_UPBRINGING)
		return;

	LPGROWTH_PET pPet = ch->GetGrowthPet(pUpBringingItem->GetSocket(2));
	if (!pPet)
		return;

	if (pPet->GetOwner() != ch)
		return;

	pPet->NameChange(p->name, pNameChangeItem, pUpBringingItem);
}
#endif


#ifdef ENABLE_AURA_SYSTEM
size_t GetAuraSubPacketLength(const EPacketCGAuraSubHeader& SubHeader)
{
	switch (SubHeader)
	{
		case AURA_SUBHEADER_CG_REFINE_CHECKIN:
			return sizeof(TSubPacketCGAuraRefineCheckIn);
		case AURA_SUBHEADER_CG_REFINE_CHECKOUT:
			return sizeof(TSubPacketCGAuraRefineCheckOut);
		case AURA_SUBHEADER_CG_REFINE_ACCEPT:
			return sizeof(TSubPacketCGAuraRefineAccept);
		case AURA_SUBHEADER_CG_REFINE_CANCEL:
			return 0;
	}

	return 0;
}

int CInputMain::Aura(LPCHARACTER ch, const char* data, size_t uiBytes)
{
	if (!ch)
		return -1;

	if (uiBytes < sizeof(TPacketCGAura))
		return -1;

	const TPacketCGAura* pinfo = reinterpret_cast<const TPacketCGAura*>(data);
	const char* c_pData = data + sizeof(TPacketCGAura);

	uiBytes -= sizeof(TPacketCGAura);

	const EPacketCGAuraSubHeader SubHeader = static_cast<EPacketCGAuraSubHeader>(pinfo->bSubHeader);
	const size_t SubPacketLength = GetAuraSubPacketLength(SubHeader);
	if (uiBytes < SubPacketLength)
	{
		sys_err("invalid aura subpacket length (sublen %d size %u buffer %u)", SubPacketLength, sizeof(TPacketCGAura), uiBytes);
		return -1;
	}

	switch (SubHeader)
	{
		case AURA_SUBHEADER_CG_REFINE_CHECKIN:
			{
				const TSubPacketCGAuraRefineCheckIn* sp = reinterpret_cast<const TSubPacketCGAuraRefineCheckIn*>(c_pData);
				ch->AuraRefineWindowCheckIn(sp->byAuraRefineWindowType, sp->AuraCell, sp->ItemCell);
			}
			return SubPacketLength;
		case AURA_SUBHEADER_CG_REFINE_CHECKOUT:
			{
				const TSubPacketCGAuraRefineCheckOut* sp = reinterpret_cast<const TSubPacketCGAuraRefineCheckOut*>(c_pData);
				ch->AuraRefineWindowCheckOut(sp->byAuraRefineWindowType, sp->AuraCell);
			}
			return SubPacketLength;
		case AURA_SUBHEADER_CG_REFINE_ACCEPT:
			{
				const TSubPacketCGAuraRefineAccept* sp = reinterpret_cast<const TSubPacketCGAuraRefineAccept*>(c_pData);
				ch->AuraRefineWindowAccept(sp->byAuraRefineWindowType);
			}
			return SubPacketLength;
		case AURA_SUBHEADER_CG_REFINE_CANCEL:
			{
				ch->AuraRefineWindowClose();
			}
			return SubPacketLength;
	}

	return 0;
}
#endif

#ifdef __ENABLE_NEW_OFFLINESHOP__
#include "new_offlineshop.h"
#include "new_offlineshop_manager.h"
template <class T>
bool CanDecode(T* p, int buffleft) {
	return buffleft >= (int)sizeof(T);
}

template <class T>
const char* Decode(T*& pObj, const char* data, int* pbufferLeng = nullptr, int* piBufferLeft=nullptr)
{
	pObj = (T*) data;

	if(pbufferLeng)
		*pbufferLeng += sizeof(T);

	if(piBufferLeft)
		*piBufferLeft -= sizeof(T);

	return data + sizeof(T);
}

int OfflineshopPacketCreateNewShop(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGShopCreate* pack = nullptr;
	if(!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra=0;
	data = Decode(pack, data, &iExtra, &iBufferLeft);

	offlineshop::TShopInfo& rShopInfo = pack->shop;
#ifdef __ENABLE_NEW_OFFLINESHOP_FIX_FLOOD__
	//fix flooding
	if (rShopInfo.dwCount > 500 || rShopInfo.dwCount == 0) {
		sys_err("%s tried to open a shop with 500+ items.", ch->GetName());
		return -1;
	}
#endif

	std::vector<offlineshop::TShopItemInfo> vec;
	vec.reserve(rShopInfo.dwCount);

	offlineshop::TShopItemInfo* pItem=nullptr;


	for (DWORD i = 0; i < rShopInfo.dwCount; ++i)
	{
		if(!CanDecode(pItem, iBufferLeft))
			return -1;

		data = Decode(pItem, data, &iExtra, &iBufferLeft);
		vec.push_back(*pItem);
	}


	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if(!rManager.RecvShopCreateNewClientPacket(ch, rShopInfo, vec))
		offlineshop::SendChatPacket(ch, offlineshop::CHAT_PACKET_CANNOT_CREATE_SHOP);

	return iExtra;
}


int OfflineshopPacketChangeShopName(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGShopChangeName* pack = nullptr;
	if(!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra=0;
	data = Decode(pack,data, &iExtra, &iBufferLeft);

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if(!rManager.RecvShopChangeNameClientPacket(ch, pack->szName))
		offlineshop::SendChatPacket(ch, offlineshop::CHAT_PACKET_CANNOT_CHANGE_NAME);

	return iExtra;
}


int OfflineshopPacketForceCloseShop(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if(!rManager.RecvShopForceCloseClientPacket(ch))
		offlineshop::SendChatPacket(ch, offlineshop::CHAT_PACKET_CANNOT_FORCE_CLOSE);

	return 0;
}


int OfflineshopPacketRequestShopList(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	rManager.RecvShopRequestListClientPacket(ch);
	return 0;
}


int OfflineshopPacketOpenShop(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGShopOpen* pack = nullptr;
	if(!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra=0;
	data = Decode(pack,data, &iExtra, &iBufferLeft);

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if(!rManager.RecvShopOpenClientPacket(ch,pack->dwOwnerID))
		offlineshop::SendChatPacket(ch, offlineshop::CHAT_PACKET_CANNOT_OPEN_SHOP);

	return iExtra;
}


int OfflineshopPacketOpenShowOwner(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if(!rManager.RecvShopOpenMyShopClientPacket(ch))
		offlineshop::SendChatPacket(ch, offlineshop::CHAT_PACKET_CANNOT_OPEN_SHOP_OWNER);

	return 0;
}

#if defined(ENABLE_IKARUS_OFFSHOP_FIXES)
int OfflineshopPacketBuyItem(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGShopBuyItem* pack = nullptr;
	if(!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra=0;
	data = Decode(pack,data, &iExtra, &iBufferLeft);

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if(!rManager.RecvShopBuyItemClientPacket(ch , pack->dwOwnerID, pack->dwItemID, pack->bIsSearch, pack->TotalPriceSeen)) //patch seen price check
		offlineshop::SendChatPacket(ch, offlineshop::CHAT_PACKET_CANNOT_BUY_ITEM);
		//offlineshop::SendChatPacket(ch, offlineshop::CHAT_PACKET_CANNOT_OPEN_SHOP);

	return iExtra;
}
#else
int OfflineshopPacketBuyItem(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGShopBuyItem* pack = nullptr;
	if(!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra=0;
	data = Decode(pack,data, &iExtra, &iBufferLeft);

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if(!rManager.RecvShopBuyItemClientPacket(ch , pack->dwOwnerID, pack->dwItemID, pack->bIsSearch))
		offlineshop::SendChatPacket(ch, offlineshop::CHAT_PACKET_CANNOT_BUY_ITEM);
		//offlineshop::SendChatPacket(ch, offlineshop::CHAT_PACKET_CANNOT_OPEN_SHOP);

	return iExtra;
}
#endif

int OfflineshopPacketAddItem(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGAddItem* pack = nullptr;
	if(!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra=0;
	data = Decode(pack,data, &iExtra, &iBufferLeft);
	
	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if(!rManager.RecvShopAddItemClientPacket(ch, pack->pos, pack->price))
		offlineshop::SendChatPacket(ch, offlineshop::CHAT_PACKET_CANNOT_ADD_ITEM);

	return iExtra;
}


int OfflineshopPacketRemoveItem(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGRemoveItem* pack = nullptr;
	if(!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra=0;
	data = Decode(pack,data, &iExtra, &iBufferLeft);

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if(!rManager.RecvShopRemoveItemClientPacket(ch, pack->dwItemID))
		offlineshop::SendChatPacket(ch, offlineshop::CHAT_PACKET_CANNOT_REMOVE_ITEM);

	return iExtra;
}


int OfflineshopPacketEditItem(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGEditItem* pack = nullptr;
	if(!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra=0;
	data = Decode(pack,data, &iExtra, &iBufferLeft);

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if(!rManager.RecvShopEditItemClientPacket(ch, pack->dwItemID, pack->price))
		offlineshop::SendChatPacket(ch, offlineshop::CHAT_PACKET_CANNOT_EDIT_ITEM);

	return iExtra;
}


int OfflineshopPacketFilterRequest(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGFilterRequest* pack = nullptr;
	if(!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra=0;
	data = Decode(pack,data, &iExtra, &iBufferLeft);

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if(!rManager.RecvShopFilterRequestClientPacket(ch, pack->filter))
		offlineshop::SendChatPacket(ch, offlineshop::CHAT_PACKET_CANNOT_FILTER);

	return iExtra;
}


int OfflineshopPacketCreateOffer(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGOfferCreate* pack = nullptr;
	if(!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra=0;
	data = Decode(pack,data, &iExtra, &iBufferLeft);

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if(!rManager.RecvShopCreateOfferClientPacket(ch, pack->offer))
		offlineshop::SendChatPacket(ch, offlineshop::CHAT_PACKET_CANNOT_CREATE_OFFER);

	return iExtra;
}


int OfflineshopPacketAcceptOffer(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGOfferAccept* pack = nullptr;
	if(!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra=0;
	data = Decode(pack,data, &iExtra, &iBufferLeft);

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if(!rManager.RecvShopAcceptOfferClientPacket(ch, pack->dwOfferID))
		offlineshop::SendChatPacket(ch, offlineshop::CHAT_PACKET_CANNOT_ACCEPT_OFFER);

	return iExtra;
}



int OfflineshopPacketOfferCancel(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGOfferCancel* pack = nullptr;
	if(!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra=0;
	data = Decode(pack,data, &iExtra, &iBufferLeft);

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if(!rManager.RecvShopCancelOfferClientPacket(ch, pack->dwOfferID, pack->dwOwnerID))
		offlineshop::SendChatPacket(ch, offlineshop::CHAT_PACKET_CANNOT_ACCEPT_OFFER);

	return iExtra;
}


int OfflineshopPacketOfferListRequest(LPCHARACTER ch)
{
	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	rManager.RecvOfferListRequestPacket(ch);
	return 0;
}


int OfflineshopPacketOpenSafebox(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if(!rManager.RecvShopSafeboxOpenClientPacket(ch))
		offlineshop::SendChatPacket(ch, offlineshop::CHAT_PACKET_CANNOT_OPEN_SAFEBOX);

	return 0;
}


int OfflineshopPacketCloseBoard(LPCHARACTER ch)
{
	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	rManager.RecvCloseBoardClientPacket(ch);
	return 0;
}


int OfflineshopPacketGetItemSafebox(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGShopSafeboxGetItem* pack = nullptr;
	if(!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra=0;
	data = Decode(pack,data, &iExtra, &iBufferLeft);


	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if(!rManager.RecvShopSafeboxGetItemClientPacket(ch, pack->dwItemID))
		offlineshop::SendChatPacket(ch, offlineshop::CHAT_PACKET_CANNOT_SAFEBOX_GET_ITEM);

	return iExtra;

}


int OfflineshopPacketGetValutesSafebox(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGShopSafeboxGetValutes* pack = nullptr;
	if(!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra=0;
	data = Decode(pack,data, &iExtra, &iBufferLeft);

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if(!rManager.RecvShopSafeboxGetValutesClientPacket(ch, pack->valutes))
		offlineshop::SendChatPacket(ch, offlineshop::CHAT_PACKET_CANNOT_SAFEBOX_GET_VALUTES);

	return iExtra;
}


int OfflineshopPacketCloseSafebox(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if(!rManager.RecvShopSafeboxCloseClientPacket(ch))
		offlineshop::SendChatPacket(ch, offlineshop::CHAT_PACKET_CANNOT_SAFEBOX_CLOSE);

	return 0;
}


int OfflineshopPacketListRequest(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if(!rManager.RecvAuctionListRequestClientPacket(ch))
		offlineshop::SendChatPacket(ch, offlineshop::CHAT_PACKET_AUCTION_CANNOT_SEND_LIST);

	return 0;
}



int OfflineshopPacketOpenAuctionRequest(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGAuctionOpenRequest* pack = nullptr;
	if(!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra=0;
	data = Decode(pack,data, &iExtra, &iBufferLeft);

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if(!rManager.RecvAuctionOpenRequestClientPacket(ch, pack->dwOwnerID))
		offlineshop::SendChatPacket(ch, offlineshop::CHAT_PACKET_AUCTION_CANNOT_OPEN_AUCTION);

	return iExtra;
}



int OfflineshopPacketOpenMyAuctionRequest(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if(!rManager.RecvMyAuctionOpenRequestClientPacket(ch))
		offlineshop::SendChatPacket(ch, offlineshop::CHAT_PACKET_AUCTION_CANNOT_SEND_LIST);

	return 0;
}



int OfflineshopPacketCreateAuction(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGAuctionCreate* pack = nullptr;
	if(!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra=0;
	data = Decode(pack,data, &iExtra, &iBufferLeft);

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if(!rManager.RecvAuctionCreateClientPacket(ch, pack->dwDuration, pack->init_price, pack->pos))
		offlineshop::SendChatPacket(ch, offlineshop::CHAT_PACKET_AUCTION_CANNOT_CREATE_AUCTION);

	return iExtra;
}



int OfflineshopPacketAddOffer(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGAuctionAddOffer* pack = nullptr;
	if(!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra=0;
	data = Decode(pack,data, &iExtra, &iBufferLeft);

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	if(!rManager.RecvAuctionAddOfferClientPacket(ch, pack->dwOwnerID, pack->price))
		offlineshop::SendChatPacket(ch, offlineshop::CHAT_PACKET_AUCTION_CANNOT_ADD_OFFER);

	return iExtra;
}



int OfflineshopPacketExitFromAuction(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGAuctionExitFrom* pack = nullptr;
	if(!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra=0;
	data = Decode(pack,data, &iExtra, &iBufferLeft);

	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	rManager.RecvAuctionExitFromAuction(ch, pack->dwOwnerID);
	return iExtra;
}


#ifdef ENABLE_NEW_SHOP_IN_CITIES
int OfflineshopPacketClickEntity(LPCHARACTER ch, const char* data, int iBufferLeft)
{
	offlineshop::TSubPacketCGShopClickEntity* pack = nullptr;
	if(!CanDecode(pack, iBufferLeft))
		return -1;

	int iExtra=0;
	data = Decode(pack, data, &iExtra, &iBufferLeft);

	
	offlineshop::CShopManager& rManager = offlineshop::GetManager();
	rManager.RecvShopClickEntity(ch, pack->dwShopVID);
	return iExtra;
}

#endif



int OfflineshopPacket(const char* data , LPCHARACTER ch, long iBufferLeft)
{
	if ((uint32_t)(iBufferLeft) < sizeof(TPacketCGNewOfflineShop))
		return -1;

	TPacketCGNewOfflineShop* pPack=nullptr;
	iBufferLeft -= sizeof(TPacketCGNewOfflineShop);
	data = Decode(pPack, data);

	

	switch (pPack->bSubHeader)
	{
	
	case offlineshop::SUBHEADER_CG_SHOP_CREATE_NEW:				return /*sizeof(TPacketCGNewOfflineShop) +*/ OfflineshopPacketCreateNewShop(ch,data,iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_CHANGE_NAME:			return /*sizeof(TPacketCGNewOfflineShop) +*/ OfflineshopPacketChangeShopName(ch,data,iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_FORCE_CLOSE:			return /*sizeof(TPacketCGNewOfflineShop) +*/ OfflineshopPacketForceCloseShop(ch,data,iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_REQUEST_SHOPLIST:		return /*sizeof(TPacketCGNewOfflineShop) +*/ OfflineshopPacketRequestShopList(ch,data,iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_OPEN:					return /*sizeof(TPacketCGNewOfflineShop) +*/ OfflineshopPacketOpenShop(ch,data,iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_OPEN_OWNER:				return /*sizeof(TPacketCGNewOfflineShop) +*/ OfflineshopPacketOpenShowOwner(ch,data,iBufferLeft);
	
	case offlineshop::SUBHEADER_CG_SHOP_BUY_ITEM:				return /*sizeof(TPacketCGNewOfflineShop) +*/ OfflineshopPacketBuyItem(ch, data , iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_ADD_ITEM:				return /*sizeof(TPacketCGNewOfflineShop) +*/ OfflineshopPacketAddItem(ch,data,iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_REMOVE_ITEM:			return /*sizeof(TPacketCGNewOfflineShop) +*/ OfflineshopPacketRemoveItem(ch,data,iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_EDIT_ITEM:				return /*sizeof(TPacketCGNewOfflineShop) +*/ OfflineshopPacketEditItem(ch,data,iBufferLeft);
	
	case offlineshop::SUBHEADER_CG_SHOP_FILTER_REQUEST:			return /*sizeof(TPacketCGNewOfflineShop) +*/ OfflineshopPacketFilterRequest(ch,data,iBufferLeft);
	
	case offlineshop::SUBHEADER_CG_SHOP_OFFER_CREATE:			return /*sizeof(TPacketCGNewOfflineShop) +*/ OfflineshopPacketCreateOffer(ch,data,iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_OFFER_ACCEPT:			return /*sizeof(TPacketCGNewOfflineShop) +*/ OfflineshopPacketAcceptOffer(ch,data,iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_REQUEST_OFFER_LIST:		return /*sizeof(TPacketCGNewOfflineShop) +*/ OfflineshopPacketOfferListRequest(ch);
	case offlineshop::SUBHEADER_CG_SHOP_OFFER_CANCEL:			return /*sizeof(TPacketCGNewOfflineShop) +*/ OfflineshopPacketOfferCancel(ch, data,iBufferLeft);

	case offlineshop::SUBHEADER_CG_SHOP_SAFEBOX_OPEN:			return /*sizeof(TPacketCGNewOfflineShop) +*/ OfflineshopPacketOpenSafebox(ch,data,iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_SAFEBOX_GET_ITEM:		return /*sizeof(TPacketCGNewOfflineShop) +*/ OfflineshopPacketGetItemSafebox(ch,data,iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_SAFEBOX_GET_VALUTES:	return /*sizeof(TPacketCGNewOfflineShop) +*/ OfflineshopPacketGetValutesSafebox(ch,data,iBufferLeft);
	case offlineshop::SUBHEADER_CG_SHOP_SAFEBOX_CLOSE:			return /*sizeof(TPacketCGNewOfflineShop) +*/ OfflineshopPacketCloseSafebox(ch,data,iBufferLeft);

	case offlineshop::SUBHEADER_CG_AUCTION_LIST_REQUEST:		return /*sizeof(TPacketCGNewOfflineShop) +*/ OfflineshopPacketListRequest(ch, data, iBufferLeft);
	case offlineshop::SUBHEADER_CG_AUCTION_OPEN_REQUEST:		return /*sizeof(TPacketCGNewOfflineShop) +*/ OfflineshopPacketOpenAuctionRequest(ch, data, iBufferLeft);
	case offlineshop::SUBHEADER_CG_MY_AUCTION_OPEN_REQUEST:		return /*sizeof(TPacketCGNewOfflineShop) +*/ OfflineshopPacketOpenMyAuctionRequest(ch, data, iBufferLeft);
	case offlineshop::SUBHEADER_CG_CREATE_AUCTION:				return /*sizeof(TPacketCGNewOfflineShop) +*/ OfflineshopPacketCreateAuction(ch, data, iBufferLeft);
	case offlineshop::SUBHEADER_CG_AUCTION_ADD_OFFER:			return /*sizeof(TPacketCGNewOfflineShop) +*/ OfflineshopPacketAddOffer(ch, data, iBufferLeft);
	case offlineshop::SUBHEADER_CG_EXIT_FROM_AUCTION:			return /*sizeof(TPacketCGNewOfflineShop) +*/ OfflineshopPacketExitFromAuction(ch, data, iBufferLeft);

	case offlineshop::SUBHEADER_CG_CLOSE_BOARD:					return /*sizeof(TPacketCGNewOfflineshop) +*/ OfflineshopPacketCloseBoard(ch);
#ifdef ENABLE_NEW_SHOP_IN_CITIES
	case offlineshop::SUBHEADER_CG_CLICK_ENTITY:				return /*sizeof(TPacketCGNewOfflineshop) +*/ OfflineshopPacketClickEntity(ch, data, iBufferLeft);
#endif


	default:
		sys_err("UNKNOWN SUBHEADER %d ",pPack->bSubHeader);
		return -1;
	}

}
#endif

#ifdef ENABLE_CUBE_RENEWAL_WORLDARD
void CInputMain::CubeRenewalSend(LPCHARACTER ch, const char* data)
{
	struct packet_send_cube_renewal * pinfo = (struct packet_send_cube_renewal *) data;
	switch (pinfo->subheader)
	{
		case CUBE_RENEWAL_SUB_HEADER_MAKE_ITEM:
		{

			int index_item = pinfo->index_item;
			int count_item = pinfo->count_item;
			int index_item_improve = pinfo->index_item_improve;

			Cube_Make(ch,index_item,count_item,index_item_improve);
		}
		break;

		case CUBE_RENEWAL_SUB_HEADER_CLOSE:
		{
			Cube_close(ch);
		}
		break;
	}
}
#endif

#ifdef ENABLE_ANTI_MULTIPLE_FARM
#include "HAntiMultipleFarm.h"

auto CInputMain::RecvAntiFarmUpdateStatus(LPCHARACTER ch, const char* data, size_t uiBytes) -> int
{
	const TSendAntiFarmInfo* p = reinterpret_cast<const TSendAntiFarmInfo*>(data);

	if (uiBytes < sizeof(TSendAntiFarmInfo))
		return -1;
	
	LPDESC d = nullptr;
	if (!ch || !(d = ch->GetDesc()))
		return -1;

	const char* c_pData = data + sizeof(TSendAntiFarmInfo);
	uiBytes -= sizeof(TSendAntiFarmInfo);

	switch (p->subheader)
	{
		case AF_SH_SEND_STATUS_UPDATE:
		{
			size_t extraLen = (sizeof(DWORD) * MULTIPLE_FARM_MAX_ACCOUNT);
			if (uiBytes < extraLen)
				return -1;
			
			std::vector<DWORD> v_dwPIDS;
			for (uint8_t i = 0; i < MULTIPLE_FARM_MAX_ACCOUNT; ++i)
				v_dwPIDS.emplace_back(*reinterpret_cast<const DWORD*>(c_pData + (sizeof(DWORD) * i)));
			
			std::string sMAIf = d->GetLoginMacAdress();
			CAntiMultipleFarm::instance().SendBlockDropStatusChange(sMAIf, v_dwPIDS);
			
			{ //send p2p update
				CAntiMultipleFarm::TP2PChangeDropStatus dataPacket(HEADER_GG_ANTI_FARM);
				strlcpy(dataPacket.cMAIf, sMAIf.c_str(), sizeof(dataPacket.cMAIf));
				for (uint8_t i = 0; i < v_dwPIDS.size(); ++i)
					dataPacket.dwPIDs[i] = v_dwPIDS[i];
				P2P_MANAGER::instance().Send(&dataPacket, sizeof(CAntiMultipleFarm::TP2PChangeDropStatus));
			}
			
			return extraLen;
		}
	}

	return 0;
}
#endif

#if defined(ENABLE_EXTENDED_BATTLE_PASS)
int32_t CInputMain::ReciveExtBattlePassActions(LPCHARACTER ch, const char* data, size_t uiBytes) {
	TPacketCGExtBattlePassAction* p = (TPacketCGExtBattlePassAction*)data;

	if (uiBytes < sizeof(TPacketCGExtBattlePassAction)) {
		return -1;
	}

	const char* c_pData = data + sizeof(TPacketCGExtBattlePassAction);
	uiBytes -= sizeof(TPacketCGExtBattlePassAction);

	switch (p->bAction) {
		case 1: {
			CBattlePassManager::instance().BattlePassRequestOpen(ch);
			return 0;
		}
		case 2: {
			if (get_dword_time() < ch->GetLastReciveExtBattlePassOpenRanking()) {
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("BATTLEPASS_NEXT_REFRESH_RANKLIST_TIME"), ((ch->GetLastReciveExtBattlePassOpenRanking() - get_dword_time()) / 1000) + 1 );
				return 0;
			}

			ch->SetLastReciveExtBattlePassOpenRanking(get_dword_time() + 10000);

			for (uint8_t bBattlePassType = 1; bBattlePassType <= 3 ; ++bBattlePassType) {
				uint8_t bBattlePassID;
				if (bBattlePassType == 1) {
					bBattlePassID = CBattlePassManager::instance().GetNormalBattlePassID();
				} else if (bBattlePassType == 2) {
					bBattlePassID = CBattlePassManager::instance().GetPremiumBattlePassID();
					if (bBattlePassID != ch->GetExtBattlePassPremiumID()) {
						continue;
					}
				} else if (bBattlePassType == 3) {
					bBattlePassID = CBattlePassManager::instance().GetEventBattlePassID();
				}

				std::unique_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("SELECT player_name, battlepass_type+0, battlepass_id, "
																			   "UNIX_TIMESTAMP(start_time), UNIX_TIMESTAMP(end_time) FROM "
																			   "`player`.`battlepass_playerindex` WHERE battlepass_type=%d "
																			   "and battlepass_id=%d and battlepass_completed=1 and "
																			   "not player_name LIKE '[%%' "
																			   "ORDER BY (UNIX_TIMESTAMP(end_time)-UNIX_TIMESTAMP(start_time)) ASC LIMIT 40"
																			   , bBattlePassType, bBattlePassID));
				if (pMsg->uiSQLErrno) {
					return 0;
				}

				MYSQL_ROW row;
				while ((row = mysql_fetch_row(pMsg->Get()->pSQLResult))) {
					TPacketGCExtBattlePassRanking pack;
					pack.bHeader = HEADER_GC_EXT_BATTLE_PASS_SEND_RANKING;
					strlcpy(pack.szPlayerName, row[0], sizeof(pack.szPlayerName));
					pack.bBattlePassType = std::atoi(row[1]);
					pack.bBattlePassID = std::atoll(row[2]);
					pack.dwStartTime = std::atoll(row[3]);
					pack.dwEndTime = std::atoll(row[4]);
					ch->GetDesc()->Packet(&pack, sizeof(pack));
				}
			}

			break;
		}
		case 10: {
			CBattlePassManager::instance().BattlePassRequestReward(ch, 1);
			return 0;
		}
		case 11: {
			CBattlePassManager::instance().BattlePassRequestReward(ch, 2);
			return 0;
		}
		case 12: {
			CBattlePassManager::instance().BattlePassRequestReward(ch, 3);
			return 0;
		}
		default: {
			break;
		}
	}

	return 0;
}

int32_t CInputMain::ReciveExtBattlePassPremiumItem(LPCHARACTER ch, const char* data, size_t uiBytes) {
	TPacketCGExtBattlePassSendPremiumItem* p = (TPacketCGExtBattlePassSendPremiumItem*)data;
	if (uiBytes < sizeof(TPacketCGExtBattlePassSendPremiumItem)) {
		return -1;
	}

	const char* c_pData = data + sizeof(TPacketCGExtBattlePassSendPremiumItem);
	uiBytes -= sizeof(TPacketCGExtBattlePassSendPremiumItem);

	LPITEM item = ch->GetInventoryItem(p->iSlotIndex);
	if (item != NULL && item->GetVnum() == 93100) {
		ch->PointChange(POINT_BATTLE_PASS_PREMIUM_ID, CBattlePassManager::instance().GetPremiumBattlePassID());
		CBattlePassManager::instance().BattlePassRequestOpen(ch);
		item->SetCount(item->GetCount() - 1);
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("BATTLEPASS_NOW_IS_ACTIVATED_PREMIUM_BATTLEPASS_OWN"));
	}

	return 0;
}
#endif

#ifdef ENABLE_CSHIELD
void CInputMain::CShieldReceive(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGCShield* p = (TPacketCGCShield*)c_pData;

	if (!ch || !c_pData)
		return;

	if (strlen(p->key) < 64)
	{
		LPDESC d = ch->GetDesc();
		if (d)
			d->SetPhase(PHASE_CLOSE);
		return;
	}

	if (!ch->GetCShield()->GetCShieldInit())
		ch->GetCShield()->SetCShieldIinit();
	ch->GetCShield()->SetKeyStored(p->key);
}
#endif
#ifdef ENABLE_CAPTCHA
void CInputMain::CaptchaReceive(LPCHARACTER ch, const char* c_pData)
{
	TPacketCGCaptcha* p = (TPacketCGCaptcha*)c_pData;

	if (!ch || !c_pData)
		return;

	LPDESC d = ch->GetDesc();

	if (!d)
		return;

	if (p->captchaResult != ch->GetCShield()->GetCaptchaResult())
	{
		if (ch->GetCShield()->GetCaptchaUnsolvedCount() == 3)
		{
			LogManager::instance().CShieldLog(ch, 10010);
			return;
		}

		ch->GetCShield()->SetCaptchaResult(number(100, 999));

		TPacketGCCaptcha packet_re;
		packet_re.header = HEADER_GC_CAPTCHA;
		strlcpy(packet_re.captchaImage, ch->GetCShield()->GenerateCaptcha(ch->GetCShield()->GetCaptchaResult()).c_str(), sizeof(packet_re.captchaImage));
		d->Packet(&packet_re, sizeof(TPacketGCCaptcha));

		ch->GetCShield()->SetCaptchaUnsolvedCount(ch->GetCShield()->GetCaptchaUnsolvedCount() + 1);
		ch->ChatPacket(CHAT_TYPE_NOTICE, LC_TEXT("Wrong captcha."));
		return;
	}

	TPacketGCCaptcha packet_f;
	packet_f.header = HEADER_GC_CAPTCHA;
	strlcpy(packet_f.captchaImage, "1000", sizeof(packet_f.captchaImage));
	d->Packet(&packet_f, sizeof(TPacketGCCaptcha));

#ifdef ENABLE_RANDOM_CAPTCHA
	DWORD randomCaptchaTime = number(-10, 10) + ENABLE_CAPTCHA;
	DWORD captchaTime = randomCaptchaTime < 10 ? 10 : randomCaptchaTime;
	ch->GetCShield()->SetCaptchaTime(time(0) + captchaTime * 60);
#else
	ch->GetCShield()->SetCaptchaTime(time(0) + (ENABLE_CAPTCHA * 60));
#endif
	ch->GetCShield()->SetCaptchaCount(0);
	ch->GetCShield()->SetCaptchaUnsolvedCount(0);
	ch->GetCShield()->SetCaptchaSessionCount(0);
	ch->GetCShield()->SetCaptchaAfterLoginTime(0);

	ch->ChatPacket(CHAT_TYPE_NOTICE, LC_TEXT("Captcha solved."));
}
#endif

#if defined(__DUNGEON_INFO_SYSTEM__)
void CInputMain::DungeonInfo(LPCHARACTER ch, const char* c_pData)
{
	DungeonInfo::Packet::CGInfo* CGPacket = (DungeonInfo::Packet::CGInfo*)c_pData;

	switch (CGPacket->bySubHeader)
	{
	case DungeonInfo::EAction::CLOSE:
	{
		ch->SetDungeonInfoOpen(false);
		ch->StopDungeonInfoReloadEvent();
	}
	break;

	case DungeonInfo::EAction::OPEN:
	{
		if (!ch->IsDungeonInfoOpen())
			CDungeonInfoManager::Instance().SendInfo(ch);
	}
	break;

	case DungeonInfo::EAction::WARP:
	{
		if (ch->IsDungeonInfoOpen())
			CDungeonInfoManager::Instance().Warp(ch, CGPacket->byIndex);
	}
	break;

	case DungeonInfo::EAction::RANK:
	{
		if (ch->IsDungeonInfoOpen())
			CDungeonInfoManager::Instance().Ranking(ch, CGPacket->byIndex, CGPacket->byRankType);
	}
	break;

	}
}
#endif

int CInputMain::Analyze(LPDESC d, BYTE bHeader, const char* c_pData)
{
	LPCHARACTER ch;

	if (!(ch = d->GetCharacter()))
	{
		sys_err("no character on desc");
		d->SetPhase(PHASE_CLOSE);
#if defined(ENABLE_ANTI_FLOOD)
		return -1;
#else
		return 0;
#endif
	}

#if defined(ENABLE_ANTI_FLOOD)
	if (bHeader != HEADER_CG_ITEM_USE &&
		bHeader != HEADER_CG_ITEM_PICKUP &&
		bHeader != HEADER_CG_MOVE
	) {
		if (thecore_pulse() > ch->GetAntiFloodPulse(FLOOD_PACKETS) + PASSES_PER_SEC(1)) {
			ch->SetAntiFloodCount(FLOOD_PACKETS, 0);
			ch->SetAntiFloodPulse(FLOOD_PACKETS, thecore_pulse());
		}

		if (ch->IncreaseAntiFloodCount(FLOOD_PACKETS) >= 300) {
			const LPDESC d = ch->GetDesc();
			if (d) {
				d->SetPhase(PHASE_CLOSE);
				sys_err("Player: %s host(%s) header(%d) is trying to flood with the packets.",
						ch->GetName(), inet_ntoa(d->GetAddr().sin_addr), bHeader);
				return -1;
			}
		}
	}
#endif

	int iExtraLen = 0;

	if (test_server && bHeader != HEADER_CG_MOVE)
		sys_log(0, "CInputMain::Analyze() ==> Header [%d] ", bHeader);

	switch (bHeader)
	{
#if defined(ENABLE_EXTENDED_BATTLE_PASS)
	case HEADER_CG_EXT_BATTLE_PASS_ACTION: {
		if ((iExtraLen = ReciveExtBattlePassActions(ch, c_pData, m_iBufferLeft)) < 0) {
			return -1;
		}

		break;
	}
	case HEADER_CG_EXT_SEND_BP_PREMIUM_ITEM: {
		if ((iExtraLen = ReciveExtBattlePassPremiumItem(ch, c_pData, m_iBufferLeft)) < 0) {
			return -1;
		}

		break;
	}
#endif
	case HEADER_CG_PONG:
		Pong(d);
		break;

	case HEADER_CG_TIME_SYNC:
		Handshake(d, c_pData);
		break;

	case HEADER_CG_CHAT:
		if (test_server)
		{
			char* pBuf = (char*)c_pData;
			sys_log(0, "%s", pBuf + sizeof(TPacketCGChat));
		}

		if ((iExtraLen = Chat(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;

		break;

	case HEADER_CG_WHISPER:
		if ((iExtraLen = Whisper(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;

	case HEADER_CG_MOVE:
		Move(ch, c_pData);
		break;

	case HEADER_CG_CHARACTER_POSITION:
		Position(ch, c_pData);
		break;

	case HEADER_CG_ITEM_USE:
		if (!ch->IsObserverMode())
			ItemUse(ch, c_pData);
		break;
	case HEADER_CG_ITEM_DROP:
		if (!ch->IsObserverMode())
		{
			ItemDrop(ch, c_pData);
		}
		break;

	case HEADER_CG_ITEM_DROP2:
		if (!ch->IsObserverMode())
			ItemDrop2(ch, c_pData);
		break;

#if defined(__NEW_DROP_DIALOG__)
	case HEADER_CG_ITEM_DESTROY:
		if (!ch->IsObserverMode())
			ItemDestroy(ch, c_pData);
		break;
#endif

	case HEADER_CG_ITEM_MOVE:
		if (!ch->IsObserverMode())
			ItemMove(ch, c_pData);
		break;

#ifdef ENABLE_SELL_ITEM
		case HEADER_CG_ITEM_SELL:
			if (!ch->IsObserverMode())
				ItemSell(ch, c_pData);
			break;
#endif

	case HEADER_CG_ITEM_PICKUP:
		if (!ch->IsObserverMode())
			ItemPickup(ch, c_pData);
		break;

	case HEADER_CG_ITEM_USE_TO_ITEM:
		if (!ch->IsObserverMode())
			ItemToItem(ch, c_pData);
		break;

	case HEADER_CG_ITEM_GIVE:
		if (!ch->IsObserverMode())
			ItemGive(ch, c_pData);
		break;

	case HEADER_CG_EXCHANGE:
		if (!ch->IsObserverMode())
			Exchange(ch, c_pData);
		break;

	case HEADER_CG_ATTACK:
	case HEADER_CG_SHOOT:
		if (!ch->IsObserverMode())
		{
			Attack(ch, bHeader, c_pData);
		}
		break;

	case HEADER_CG_USE_SKILL:
		if (!ch->IsObserverMode())
			UseSkill(ch, c_pData);
		break;

#if defined(__SKILL_COLOR_SYSTEM__)
	case HEADER_CG_SKILL_COLOR:
		SetSkillColor(ch, c_pData);
		break;
#endif

	case HEADER_CG_QUICKSLOT_ADD:
		QuickslotAdd(ch, c_pData);
		break;

	case HEADER_CG_QUICKSLOT_DEL:
		QuickslotDelete(ch, c_pData);
		break;

	case HEADER_CG_QUICKSLOT_SWAP:
		QuickslotSwap(ch, c_pData);
		break;

	case HEADER_CG_SHOP:
		if ((iExtraLen = Shop(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;

	case HEADER_CG_MESSENGER:
		if ((iExtraLen = Messenger(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;

	case HEADER_CG_ON_CLICK:
		OnClick(ch, c_pData);
		break;

	case HEADER_CG_SYNC_POSITION:
		if ((iExtraLen = SyncPosition(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;

	case HEADER_CG_ADD_FLY_TARGETING:
	case HEADER_CG_FLY_TARGETING:
		FlyTarget(ch, c_pData, bHeader);
		break;

	case HEADER_CG_SCRIPT_BUTTON:
		ScriptButton(ch, c_pData);
		break;

		// SCRIPT_SELECT_ITEM
	case HEADER_CG_SCRIPT_SELECT_ITEM:
		ScriptSelectItem(ch, c_pData);
		break;
		// END_OF_SCRIPT_SELECT_ITEM

	case HEADER_CG_SCRIPT_ANSWER:
		ScriptAnswer(ch, c_pData);
		break;

	case HEADER_CG_QUEST_INPUT_STRING:
		QuestInputString(ch, c_pData);
		break;

	case HEADER_CG_QUEST_CONFIRM:
		QuestConfirm(ch, c_pData);
		break;

	case HEADER_CG_REQUEST_EVENT_QUEST:
		RequestEventQuest(ch, c_pData);
		break;

	case HEADER_CG_TARGET:
		Target(ch, c_pData);
		break;

	case HEADER_CG_WARP:
		Warp(ch, c_pData);
		break;

	case HEADER_CG_SAFEBOX_CHECKIN:
		SafeboxCheckin(ch, c_pData);
		break;

	case HEADER_CG_SAFEBOX_CHECKOUT:
		SafeboxCheckout(ch, c_pData, false);
		break;

	case HEADER_CG_SAFEBOX_ITEM_MOVE:
		SafeboxItemMove(ch, c_pData);
		break;

#if defined(BL_REMOTE_SHOP)
	case HEADER_CG_REMOTE_SHOP:
		RemoteShop(ch, c_pData);
		break;
#endif

	case HEADER_CG_MALL_CHECKOUT:
		SafeboxCheckout(ch, c_pData, true);
		break;

	case HEADER_CG_PARTY_INVITE:
		PartyInvite(ch, c_pData);
		break;

	case HEADER_CG_PARTY_REMOVE:
		PartyRemove(ch, c_pData);
		break;

	case HEADER_CG_PARTY_INVITE_ANSWER:
		PartyInviteAnswer(ch, c_pData);
		break;

	case HEADER_CG_PARTY_SET_STATE:
		PartySetState(ch, c_pData);
		break;

	case HEADER_CG_PARTY_USE_SKILL:
		PartyUseSkill(ch, c_pData);
		break;

	case HEADER_CG_PARTY_PARAMETER:
		PartyParameter(ch, c_pData);
		break;

	case HEADER_CG_ANSWER_MAKE_GUILD:
		AnswerMakeGuild(ch, c_pData);
		break;

	case HEADER_CG_GUILD:
		if ((iExtraLen = Guild(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;

	case HEADER_CG_FISHING:
		Fishing(ch, c_pData);
		break;

	case HEADER_CG_HACK:
		Hack(ch, c_pData);
		break;

	case HEADER_CG_MYSHOP:
		if ((iExtraLen = MyShop(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;

#if defined(__MYSHOP_DECO__)
	case HEADER_CG_MYSHOP_DECO:
		MyShopDeco(ch, c_pData);
		break;
#endif

	case HEADER_CG_REFINE:
		Refine(ch, c_pData);
		break;

#ifdef ENABLE_CSHIELD
		case HEADER_CG_CSHIELD:
			CShieldReceive(ch, c_pData);
			break;
#endif
#ifdef ENABLE_CAPTCHA
		case HEADER_CG_CAPTCHA:
			CaptchaReceive(ch, c_pData);
			break;
#endif


#ifdef ENABLE_FEATURES_OXEVENT
	case HEADER_CG_OXEVENT_MANAGER:
		COXEventManager::instance().Manager(ch, c_pData);
		break;
#endif
	case HEADER_CG_CLIENT_VERSION:
		Version(ch, c_pData);
		break;

	case HEADER_CG_DRAGON_SOUL_REFINE: {
		TPacketCGDragonSoulRefine* p = reinterpret_cast <TPacketCGDragonSoulRefine*>((void*)c_pData);
		switch (p->bSubType) {
			case DS_SUB_HEADER_CLOSE: {
				ch->DragonSoul_RefineWindow_Close();
				break;
			}
			case DS_SUB_HEADER_DO_REFINE_GRADE: {
				DSManager::instance().DoRefineGrade(ch, p->ItemGrid);
				break;
			}
			case DS_SUB_HEADER_DO_REFINE_STEP: {
				DSManager::instance().DoRefineStep(ch, p->ItemGrid);
				break;
			}
			case DS_SUB_HEADER_DO_REFINE_STRENGTH:
			{
				DSManager::instance().DoRefineStrength(ch, p->ItemGrid);
				break;
			}
#if defined(ENABLE_DS_CHANGE_ATTR)
			case DS_SUB_HEADER_DO_CHANGEATTR: {
				DSManager::instance().DoChangeAttr(ch, p->ItemGrid);
				break;
			}
#endif
			default: {
				break;
			}
		}

		break;
	}
#if defined(ENABLE_DS_REFINE_ALL)
	case HEADER_CG_DRAGON_SOUL_REFINE_ALL: {
		TPacketDragonSoulRefineAll* p = reinterpret_cast <TPacketDragonSoulRefineAll*>((void*)c_pData);
		DSManager::instance().DoRefineAll(ch, p->subheader, p->type, p->grade);
		break;
	}
#endif
#ifdef ENABLE_CUBE_RENEWAL_WORLDARD
		case HEADER_CG_CUBE_RENEWAL:
			CubeRenewalSend(ch, c_pData);
			break;
#endif

#ifdef ENABLE_SWITCHBOT
	case HEADER_CG_SWITCHBOT:
		if ((iExtraLen = Switchbot(ch, c_pData, m_iBufferLeft)) < 0)
		{
			return -1;
		}
		break;
#endif

#ifdef ENABLE_ANTI_MULTIPLE_FARM
	case HEADER_CG_ANTI_FARM: // packet receive for the header CG_Anti_farm
		if ((iExtraLen = RecvAntiFarmUpdateStatus(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;
#endif

#ifdef __EVENT_MANAGER__
	case HEADER_CG_REQUEST_EVENT_DATA:
		RequestEventData(ch, c_pData);
		break;
#endif
#ifdef __ENABLE_NEW_OFFLINESHOP__
		case HEADER_CG_NEW_OFFLINESHOP:
			if((iExtraLen = OfflineshopPacket(c_pData, ch, m_iBufferLeft))< 0)
				return -1;
			break;
#endif
#if defined(__SEND_TARGET_INFO__)
	case HEADER_CG_TARGET_INFO_LOAD:
	{
		TargetInfoLoad(ch, c_pData);
	}
	break;
#endif

#ifdef ENABLE_SHOW_CHEST_DROP
		case HEADER_CG_CHEST_DROP_INFO:
			ChestDropInfo(ch, c_pData);
			break;
#endif

#if defined(__ACCE_COSTUME_SYSTEM__)
	case HEADER_CG_ACCE:
		Acce(ch, c_pData);
		break;
#endif

#if defined(__CHANGE_LOOK_SYSTEM__)
	case HEADER_CG_CHANGE_LOOK:
	{
		ChangeLook(ch, c_pData);
	}
	break;
#endif

#if defined(__MINI_GAME_CATCH_KING__)
	case HEADER_CG_MINI_GAME_CATCH_KING:
		if ((iExtraLen = CMiniGameManager::instance().MiniGameCatchKing(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;
		break;
#endif

#if defined(__PRIVATE_SHOP_SEARCH_SYSTEM__)
	case HEADER_CG_SHOP_SEARCH:
		if (!ch->IsObserverMode())
			ShopSearch(ch, c_pData, false);
		break;

	case HEADER_CG_SHOP_SEARCH_SUB:
		if (!ch->IsObserverMode())
			ShopSearch(ch, c_pData, true);
		break;

	case HEADER_CG_SHOP_SEARCH_BUY:
		if (!ch->IsObserverMode())
			ShopSearchBuy(ch, c_pData);
		break;
#endif

	case HEADER_CG_CHANGE_LANGUAGE:
	{
		TPacketChangeLanguage* p = reinterpret_cast <TPacketChangeLanguage*>((void*)c_pData);
		ChangeLanguage(ch, p->bLanguage);
	}
	break;

#if defined(__SKILLBOOK_COMB_SYSTEM__)
	case HEADER_CG_SKILLBOOK_COMB:
	{
		TPacketCGSkillBookCombination* p = reinterpret_cast <TPacketCGSkillBookCombination*>((void*)c_pData);
		SkillBookCombination(ch, p->CombItemGrid, p->bAction);
	}
	break;
#endif

	case HEADER_CG_WHISPER_DETAILS:
		WhisperDetails(ch, c_pData);
		break;
#ifdef __GROWTH_PET_SYSTEM__
	case HEADER_CG_PET_HATCH:
		PetHatch(ch, c_pData);
		break;

	case HEADER_CG_PET_WINDOW:
		PetWindow(ch, c_pData);
		break;

	case HEADER_CG_PET_WINDOW_TYPE:
		PetWindowType(ch, c_pData);
		break;

	case HEADER_CG_PET_NAME_CHANGE:
		PetNameChange(ch, c_pData);
		break;

	case HEADER_CG_PET_FEED:
		PetFeed(ch, c_pData);
		break;

	case HEADER_CG_PET_DETERMINE:
		PetDetermine(ch, c_pData);
		break;

	case HEADER_CG_PET_ATTR_CHANGE:
		PetAttrChange(ch, c_pData);
		break;

	case HEADER_CG_PET_REVIVE:
		PetRevive(ch, c_pData);
		break;

	case HEADER_CG_PET_LEARN_SKILL:
		PetLearnSkill(ch, c_pData);
		break;

	case HEADER_CG_PET_SKILL_UPGRADE:
		PetSkillUpgrade(ch, c_pData);
		break;

	case HEADER_CG_PET_DELETE_SKILL:
		PetDeleteSkill(ch, c_pData);
		break;

	case HEADER_CG_PET_DELETE_ALL_SKILL:
		PetDeleteAllSkill(ch, c_pData);
		break;
#endif
#ifdef ENABLE_AURA_SYSTEM
		case HEADER_CG_AURA:
			if ((iExtraLen = Aura(ch, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;
#endif


#if defined(__MAILBOX__)
	case HEADER_CG_MAILBOX_WRITE:
		MailboxWrite(ch, c_pData);
		break;

	case HEADER_CG_MAILBOX_WRITE_CONFIRM:
		MailboxConfirm(ch, c_pData);
		break;

	case HEADER_CG_MAILBOX_PROCESS:
		MailboxProcess(ch, c_pData);
		break;
#endif

#ifdef ENABLE_SONITEX_OFFLIENSHOP
		case HEADER_CG_PRIVATE_SHOP:
		{
			TPacketCGPrivateShop* p = (TPacketCGPrivateShop*)c_pData;
			c_pData += sizeof(TPacketCGPrivateShop);

			switch (p->bSubHeader)
			{
				case SUBHEADER_CG_PRIVATE_SHOP_BUILD:
				{
					if ((iExtraLen = PrivateShopBuild(ch, c_pData, m_iBufferLeft)) < 0)
						return -1;
				} break;

				case SUBHEADER_CG_PRIVATE_SHOP_CLOSE:
				{
					PrivateShopClose(ch);
				} break;

				case SUBHEADER_CG_PRIVATE_SHOP_PANEL_OPEN:
				{
					PrivateShopPanelOpen(ch);
				} break;

				case SUBHEADER_CG_PRIVATE_SHOP_PANEL_CLOSE:
				{
					PrivateShopPanelClose(ch);
				} break;

				case SUBHEADER_CG_PRIVATE_SHOP_START:
				{
					if ((iExtraLen = PrivateShopStart(ch, c_pData, m_iBufferLeft)) < 0)
						return -1;
				} break;

				case SUBHEADER_CG_PRIVATE_SHOP_END:
				{
					PrivateShopEnd(ch);
				} break;

				case SUBHEADER_CG_PRIVATE_SHOP_BUY:
				{
					if ((iExtraLen = PrivateShopBuy(ch, c_pData, m_iBufferLeft)) < 0)
						return -1;
				} break;

				case SUBHEADER_CG_PRIVATE_SHOP_WITHDRAW:
				{
					PrivateShopWithdraw(ch);
				} break;

				case SUBHEADER_CG_PRIVATE_SHOP_MODIFY:
				{
					PrivateShopModify(ch);
				} break;

				case SUBHEADER_CG_PRIVATE_SHOP_ITEM_PRICE_CHANGE:
				{
					if ((iExtraLen = PrivateShopItemPriceChange(ch, c_pData, m_iBufferLeft)) < 0)
						return -1;
				} break;

				case SUBHEADER_CG_PRIVATE_SHOP_ITEM_MOVE:
				{
					if ((iExtraLen = PrivateShopItemMove(ch, c_pData, m_iBufferLeft)) < 0)
						return -1;
				} break;

				case SUBHEADER_CG_PRIVATE_SHOP_ITEM_CHECKIN:
				{
					if ((iExtraLen = PrivateShopItemCheckin(ch, c_pData, m_iBufferLeft)) < 0)
						return -1;
				} break;

				case SUBHEADER_CG_PRIVATE_SHOP_ITEM_CHECKOUT:
				{
					if ((iExtraLen = PrivateShopItemCheckout(ch, c_pData, m_iBufferLeft)) < 0)
						return -1;
				} break;

				case SUBHEADER_CG_PRIVATE_SHOP_TITLE_CHANGE:
				{
					if ((iExtraLen = PrivateShopTitleChange(ch, c_pData, m_iBufferLeft)) < 0)
						return -1;
				} break;

				case SUBHEADER_CG_PRIVATE_SHOP_SEARCH_CLOSE:
				{
					PrivateShopSearchClose(ch);
				} break;

				case SUBHEADER_CG_PRIVATE_SHOP_SEARCH:
				{
					if ((iExtraLen = PrivateShopSearch(ch, c_pData, m_iBufferLeft)) < 0)
						return -1;
				} break;

				case SUBHEADER_CG_PRIVATE_SHOP_SEARCH_BUY:
				{
					if ((iExtraLen = PrivateShopSearchBuy(ch, c_pData, m_iBufferLeft)) < 0)
						return -1;
				} break;
			}
		} break;
#endif
#if defined(ENABLE_INGAME_WIKI)
		case InGameWiki::HEADER_CG_WIKI:
			{
				RecvWikiPacket(ch, c_pData);
				break;
			}
#endif

#if defined(ENABLE_MULTI_ITEM_USE)
	case HEADER_CG_MULTI_ITEM_USE: {
		if (!ch->IsObserverMode()) {
			MultiItemUse(ch, c_pData);
		}

		break;
	}
#endif
#if defined(__DUNGEON_INFO_SYSTEM__)
		case DungeonInfo::Packet::HEADER_CG_DUNGEON_INFO:
			DungeonInfo(ch, c_pData);
			break;
#endif
	}

	return (iExtraLen);
}

int CInputDead::Analyze(LPDESC d, BYTE bHeader, const char* c_pData)
{
	LPCHARACTER ch;

	if (!(ch = d->GetCharacter()))
	{
		sys_err("no character on desc");
#if defined(ENABLE_ANTI_FLOOD)
		return -1;
#else
		return 0;
#endif
	}

	int iExtraLen = 0;

	switch (bHeader)
	{
	case HEADER_CG_PONG:
		Pong(d);
		break;

	case HEADER_CG_TIME_SYNC:
		Handshake(d, c_pData);
		break;

	case HEADER_CG_CHAT:
		if ((iExtraLen = Chat(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;

		break;

	case HEADER_CG_WHISPER:
		if ((iExtraLen = Whisper(ch, c_pData, m_iBufferLeft)) < 0)
			return -1;

		break;

	case HEADER_CG_HACK:
		Hack(ch, c_pData);
		break;
	default:
#if defined(ENABLE_ANTI_FLOOD)
		return -1;
#else
		return 0;
#endif
	}

	return (iExtraLen);
}

#ifdef __EVENT_MANAGER__
void CInputMain::RequestEventQuest(LPCHARACTER ch, const void* c_pData)
{
	TPacketCGRequestEventQuest* p = (TPacketCGRequestEventQuest*)c_pData;

	if (ch && ch->GetDesc())
		quest::CQuestManager::instance().RequestEventQuest(p->szName, ch->GetPlayerID());
}

void CInputMain::RequestEventData(LPCHARACTER ch, const char* c_pData)
{
	if (ch && ch->GetDesc())
	{
		const TPacketCGRequestEventData* p = reinterpret_cast<const TPacketCGRequestEventData*>(c_pData);
		CEventManager::Instance().SendEventInfo(ch, p->bMonth);
	}
}
#endif

#ifdef ENABLE_SWITCHBOT
int CInputMain::Switchbot(LPCHARACTER ch, const char* data, size_t uiBytes)
{
	const TPacketCGSwitchbot* p = reinterpret_cast<const TPacketCGSwitchbot*>(data);

	if (uiBytes < sizeof(TPacketCGSwitchbot))
	{
		return -1;
	}

	const char* c_pData = data + sizeof(TPacketCGSwitchbot);
	uiBytes -= sizeof(TPacketCGSwitchbot);

	switch (p->subheader)
	{
	case SUBHEADER_CG_SWITCHBOT_START:
	{
		size_t extraLen = sizeof(TSwitchbotAttributeAlternativeTable) * SWITCHBOT_ALTERNATIVE_COUNT;
		if (uiBytes < extraLen)
		{
			return -1;
		}

		std::vector<TSwitchbotAttributeAlternativeTable> vec_alternatives;

		for (BYTE alternative = 0; alternative < SWITCHBOT_ALTERNATIVE_COUNT; ++alternative)
		{
			const TSwitchbotAttributeAlternativeTable* pAttr = reinterpret_cast<const TSwitchbotAttributeAlternativeTable*>(c_pData);
			c_pData += sizeof(TSwitchbotAttributeAlternativeTable);

			vec_alternatives.emplace_back(*pAttr);
		}

		CSwitchbotManager::Instance().Start(ch->GetPlayerID(), p->slot, vec_alternatives);
		return extraLen;
	}

	case SUBHEADER_CG_SWITCHBOT_STOP:
	{
		CSwitchbotManager::Instance().Stop(ch->GetPlayerID(), p->slot);
		return 0;
	}
	}

	return 0;
}
#endif

#ifdef ENABLE_SONITEX_OFFLIENSHOP
int CInputMain::PrivateShopBuild(LPCHARACTER ch, const char* c_pData, size_t uiBytes)
{
	TPacketCGPrivateShopBuild* p = (TPacketCGPrivateShopBuild*)c_pData;
	c_pData += sizeof(TPacketCGPrivateShopBuild);

	// Size of the remaining packet
	int iExtraLen = sizeof(TPacketCGPrivateShopBuild) + p->wItemCount * sizeof(TPrivateShopItem);

	// No items have been passed
	if (uiBytes < (sizeof(TPacketCGPrivateShop) + iExtraLen))
		return -1;

	if (!ch || !ch->GetDesc())
		return iExtraLen;

	if (ch && ch->m_pkTimedEvent)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Your logout has been cancelled.");
		event_cancel(&ch->m_pkTimedEvent);

		return iExtraLen;
	}

	if (ch->IsStun() || ch->IsDead())
		return iExtraLen;

	if (ch->GetExchange() || ch->IsOpenSafebox() || ch->GetShopOwner() || ch->IsCubeOpen())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot open a private shop while another window is open."));
		return iExtraLen;
	}

	ch->BuildPrivateShop(p->szTitle, p->dwPolyVnum, p->bTitleType, p->bPageCount, p->wItemCount, (TPrivateShopItem*)c_pData);

	return iExtraLen;
}

void CInputMain::PrivateShopClose(LPCHARACTER ch)
{
	if (!ch || !ch->GetDesc())
		return;

	if (ch && ch->m_pkTimedEvent)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Your logout has been cancelled.");
		event_cancel(&ch->m_pkTimedEvent);

		return;
	}

	if (ch->IsStun() || ch->IsDead())
		return;

	if (ch->GetExchange() || ch->IsOpenSafebox() ||ch->IsCubeOpen() || ch->GetShop())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "You cannot close a private shop while another window is open.");
		return;
	}

	if (!ch->IsPrivateShopOwner() || !ch->IsEditingPrivateShop())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You do not have an open private shop."));
		return;
	}

	BYTE bSubHeader = PRIVATE_SHOP_GD_SUBHEADER_CLOSE;
	DWORD dwPID = ch->GetPlayerID();

	db_clientdesc->DBPacketHeader(HEADER_GD_PRIVATE_SHOP, ch->GetDesc()->GetHandle(), sizeof(BYTE) + sizeof(DWORD));
	db_clientdesc->Packet(&bSubHeader, sizeof(BYTE));
	db_clientdesc->Packet(&dwPID, sizeof(DWORD));
}

void CInputMain::PrivateShopPanelOpen(LPCHARACTER ch)
{
	if (!ch || !ch->GetDesc())
		return;

	if (ch->GetExchange() || ch->IsOpenSafebox() || ch->GetShopOwner() || ch->IsCubeOpen())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot open a private shop while another window is open."));
		return;
	}

	if (ch->IsDead() || ch->IsStun()) {
		return;
	}

	if (ch->IsEditingPrivateShop())
		return;

	ch->OpenPrivateShopPanel();
}

void CInputMain::PrivateShopPanelClose(LPCHARACTER ch)
{
	if (!ch)
		return;

	if (ch->IsStun() || ch->IsDead())
		return;

	ch->ClosePrivateShopPanel();
}

int CInputMain::PrivateShopStart(LPCHARACTER ch, const char* c_pData, size_t uiBytes)
{
	const DWORD dwVID = *reinterpret_cast<const DWORD*>(c_pData);
	int iExtraLen = sizeof(DWORD);

	// No data has been passed
	if (uiBytes < iExtraLen)
		return -1;

	if (!ch || !ch->GetDesc())
		return iExtraLen;

	if (ch && ch->m_pkTimedEvent)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Your logout has been cancelled.");
		event_cancel(&ch->m_pkTimedEvent);

		return iExtraLen;
	}

#ifdef __GROWTH_PET_SYSTEM__
	if (ch->GetPetWindowType() == PET_WINDOW_ATTR_CHANGE || ch->GetPetWindowType() == PET_WINDOW_PRIMIUM_FEEDSTUFF)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "While modifying your pet's stats, you cannot trade, sell items, use the storeroom etc.");
		return iExtraLen;
	}
#endif

	if ((ch->GetExchange() || ch->IsOpenSafebox() || ch->GetMyShop() || ch->GetShopOwner()) || ch->IsCubeOpen())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't use a private shop now."));
		return iExtraLen;
	}

	if (ch->IsStun() || ch->IsDead()) {
		return iExtraLen;
	}

	LPPRIVATE_SHOP pPrivateShop = CPrivateShopManager::Instance().GetPrivateShop(dwVID);
	if (!pPrivateShop)
		return iExtraLen;

	if (pPrivateShop->GetID() == ch->GetPlayerID())
	{
		if (!ch->IsEditingPrivateShop())
			ch->OpenPrivateShopPanel();

		return iExtraLen;
	}

	if (pPrivateShop == ch->GetViewingPrivateShop())
		return iExtraLen;

	if (ch->IsEditingPrivateShop())
		ch->ClosePrivateShopPanel();

	if (ch->GetViewingPrivateShop())
		ch->GetViewingPrivateShop()->RemoveShopViewer(ch);

	pPrivateShop->AddShopViewer(ch);

	return iExtraLen;
}

void CInputMain::PrivateShopEnd(LPCHARACTER ch)
{
	if (!ch)
		return;

	CPrivateShopManager::Instance().StopShopping(ch);
}

int CInputMain::PrivateShopBuy(LPCHARACTER ch, const char* c_pData, size_t uiBytes)
{
	const WORD wPos = *reinterpret_cast<const WORD*>(c_pData);
	int iExtraLen = sizeof(WORD);

	// No data has been passed
	if (uiBytes < iExtraLen)
		return -1;

	if (!ch || !ch->GetDesc())
		return iExtraLen;

	if (ch->m_pkTimedEvent)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Your logout has been cancelled.");
		event_cancel(&ch->m_pkTimedEvent);

		return iExtraLen;
	}

	if (ch->IsStun() || ch->IsDead())
		return iExtraLen;

	if (!ch->GetViewingPrivateShop())
		return iExtraLen;
	
	if (!ch->GetViewingPrivateShop()->GetItem(wPos))
		return iExtraLen;

	// Packet to db
	BYTE bSubHeader = PRIVATE_SHOP_GD_SUBHEADER_BUY_REQUEST;

	TPacketGDPrivateShopBuyRequest subPacket;
	subPacket.dwCustomerPID = ch->GetPlayerID();
	subPacket.dwShopID = ch->GetViewingPrivateShop()->GetID();
	subPacket.wPos = wPos;
	subPacket.llGoldBalance = ch->GetGold();
	subPacket.dwChequeBalance = ch->GetCheque();
	subPacket.llGoldPrice = ch->GetViewingPrivateShop()->GetItem(wPos)->GetGoldPrice();
	subPacket.dwChequePrice = ch->GetViewingPrivateShop()->GetItem(wPos)->GetChequePrice();

	db_clientdesc->DBPacketHeader(HEADER_GD_PRIVATE_SHOP, ch->GetDesc()->GetHandle(), sizeof(BYTE) + sizeof(TPacketGDPrivateShopBuyRequest));
	db_clientdesc->Packet(&bSubHeader, sizeof(BYTE));
	db_clientdesc->Packet(&subPacket, sizeof(TPacketGDPrivateShopBuyRequest));

	return iExtraLen;
}

void CInputMain::PrivateShopWithdraw(LPCHARACTER ch)
{
	if (!ch)
		return;

	if (ch->m_pkTimedEvent)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Your logout has been cancelled.");
		event_cancel(&ch->m_pkTimedEvent);

		return;
	}

	if (ch->IsStun() || ch->IsDead())
		return;

	if (!ch->IsPrivateShopOwner() || !ch->IsEditingPrivateShop())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You do not have an open private shop."));
		return;
	}

	if ((ch->GetPrivateShopTable()->llGold + ch->GetGold()) > g_MaxGold)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot exchange as this would exceed the maximum amount of Yang."));
		return;
	}

	if ((ch->GetPrivateShopTable()->dwCheque + ch->GetCheque()) > CHEQUE_MAX)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot exchange as this would exceed the maximum amount of Won."));
		return;
	}

	BYTE bSubHeader = PRIVATE_SHOP_GD_SUBHEADER_WITHDRAW_REQUEST;
	DWORD dwPID = ch->GetPlayerID();
	db_clientdesc->DBPacketHeader(HEADER_GD_PRIVATE_SHOP, ch->GetDesc()->GetHandle(), sizeof(BYTE) + sizeof(DWORD));
	db_clientdesc->Packet(&bSubHeader, sizeof(BYTE));
	db_clientdesc->Packet(&dwPID, sizeof(DWORD));
}

void CInputMain::PrivateShopModify(LPCHARACTER ch)
{
	if (!ch)
		return;

	if (ch->IsStun() || ch->IsDead())
		return;

	if (!ch->IsPrivateShopOwner() || !ch->IsEditingPrivateShop())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You do not have an open private shop."));
		return;
	}

	BYTE bSubHeader = PRIVATE_SHOP_GD_SUBHEADER_MODIFY_REQUEST;
	DWORD dwPID = ch->GetPlayerID();
	db_clientdesc->DBPacketHeader(HEADER_GD_PRIVATE_SHOP, ch->GetDesc()->GetHandle(), sizeof(BYTE) + sizeof(DWORD));
	db_clientdesc->Packet(&bSubHeader, sizeof(BYTE));
	db_clientdesc->Packet(&dwPID, sizeof(DWORD));
}

int CInputMain::PrivateShopItemPriceChange(LPCHARACTER ch, const char* c_pData, size_t uiBytes)
{
	TPacketCGPrivateShopItemPriceChange* p = (TPacketCGPrivateShopItemPriceChange*)c_pData;

	int iExtraLen = sizeof(TPacketCGPrivateShopItemPriceChange);

	// No data has been passed
	if (uiBytes < iExtraLen)
		return -1;

	if (!ch)
		return iExtraLen;

	if (ch->IsStun() || ch->IsDead())
		return iExtraLen;

	if (ch->m_pkTimedEvent)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Your logout has been cancelled.");
		event_cancel(&ch->m_pkTimedEvent);

		return iExtraLen;
	}

	if (!ch->IsPrivateShopOwner() || !ch->IsEditingPrivateShop())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You do not have an open private shop."));
		return iExtraLen;
	}

	if (!ch->CanModifyPrivateShop())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot manage private shop's content while it is not in a modifying state."));
		return iExtraLen;
	}

	const TPlayerPrivateShopItem* pPrivateShopItem = ch->GetPrivateShopItem(p->wPos);
	if (!pPrivateShopItem)
		return iExtraLen;

	if ((ch->GetPrivateShopTotalGold() + ch->GetGold() - pPrivateShopItem->llGold + p->llGold) > g_MaxGold)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot exchange as this would exceed the maximum amount of Yang."));
		return iExtraLen;
	}

	if ((ch->GetPrivateShopTotalCheque() + ch->GetCheque() - pPrivateShopItem->dwCheque + p->dwCheque) > CHEQUE_MAX)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot exchange as this would exceed the maximum amount of Won."));
		return iExtraLen;
	}

	BYTE bSubHeader = PRIVATE_SHOP_GD_SUBHEADER_ITEM_PRICE_CHANGE_REQUEST;

	TPacketPrivateShopItemPriceChange subPacket;
	subPacket.dwShopID = ch->GetPlayerID();
	subPacket.wPos = p->wPos;
	subPacket.llGold = p->llGold;
	subPacket.dwCheque = p->dwCheque;

	db_clientdesc->DBPacketHeader(HEADER_GD_PRIVATE_SHOP, ch->GetDesc()->GetHandle(), sizeof(BYTE) + sizeof(TPacketPrivateShopItemPriceChange));
	db_clientdesc->Packet(&bSubHeader, sizeof(BYTE));
	db_clientdesc->Packet(&subPacket, sizeof(TPacketPrivateShopItemPriceChange));

	return iExtraLen;
}

int CInputMain::PrivateShopItemMove(LPCHARACTER ch, const char* c_pData, size_t uiBytes)
{
	TPacketCGPrivateShopItemMove* p = (TPacketCGPrivateShopItemMove*)c_pData;

	int iExtraLen = sizeof(TPacketCGPrivateShopItemMove);

	// No data has been passed
	if (uiBytes < iExtraLen)
		return -1;

	if (!ch)
		return iExtraLen;

	if (ch->IsStun() || ch->IsDead())
		return iExtraLen;

	if (ch->m_pkTimedEvent)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Your logout has been cancelled.");
		event_cancel(&ch->m_pkTimedEvent);

		return iExtraLen;
	}

	if (!ch->IsPrivateShopOwner() || !ch->IsEditingPrivateShop())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You do not have an open private shop."));
		return iExtraLen;
	}

	if (!ch->CanModifyPrivateShop())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot manage private shop's content while it is not in a modifying state."));		return iExtraLen;
	}

	BYTE bSubHeader = PRIVATE_SHOP_GD_SUBHEADER_ITEM_MOVE_REQUEST;

	TPacketPrivateShopItemMove subPacket;
	subPacket.dwShopID = ch->GetPlayerID();
	subPacket.wPos = p->wPos;
	subPacket.wChangePos = p->wChangePos;

	db_clientdesc->DBPacketHeader(HEADER_GD_PRIVATE_SHOP, ch->GetDesc()->GetHandle(), sizeof(BYTE) + sizeof(TPacketPrivateShopItemMove));
	db_clientdesc->Packet(&bSubHeader, sizeof(BYTE));
	db_clientdesc->Packet(&subPacket, sizeof(TPacketPrivateShopItemMove));

	return iExtraLen;
}

int CInputMain::PrivateShopItemCheckin(LPCHARACTER ch, const char* c_pData, size_t uiBytes)
{
	TPacketCGPrivateShopItemCheckin* p = (TPacketCGPrivateShopItemCheckin*)c_pData;

	int iExtraLen = sizeof(TPacketCGPrivateShopItemCheckin);

	// No data has been passed
	if (uiBytes < iExtraLen)
		return -1;

	if (!ch)
		return iExtraLen;

	if (ch->IsStun() || ch->IsDead())
		return iExtraLen;

	if (ch->m_pkTimedEvent)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Your logout has been cancelled.");
		event_cancel(&ch->m_pkTimedEvent);

		return iExtraLen;
	}

	if (!ch->IsPrivateShopOwner() || !ch->IsEditingPrivateShop())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You do not have an open private shop."));
		return iExtraLen;
	}

	if (!ch->CanModifyPrivateShop())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot manage private shop's content while it is not in a modifying state."));		
		return iExtraLen;
	}

	if ((ch->GetPrivateShopTotalGold() + ch->GetGold() + p->llGold) > g_MaxGold)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot exchange as this would exceed the maximum amount of Yang."));
		return iExtraLen;
	}

	if ((ch->GetPrivateShopTotalCheque() + ch->GetCheque() + p->dwCheque) > CHEQUE_MAX)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot exchange as this would exceed the maximum amount of Won."));
		return iExtraLen;
	}

	LPITEM pItem = ch->GetItem(p->TSrcPos);
	if (!pItem)
		return iExtraLen;

	TPlayerPrivateShopItem t;
	t.dwID = pItem->GetID();
	t.wPos = 0;
	t.dwCount = pItem->GetCount();
	t.dwVnum = pItem->GetOriginalVnum();
	thecore_memcpy(t.alSockets, pItem->GetSockets(), sizeof(t.alSockets));
	thecore_memcpy(t.aAttr, pItem->GetAttributes(), sizeof(t.aAttr));
#if defined(__CHANGE_LOOK_SYSTEM__)
	t.transmutation = pItem->GetTransmutation();
#endif
	t.bSize = pItem->GetSize();
	t.llGold = p->llGold;
	t.dwCheque = p->dwCheque;
	t.dwOwner = pItem->GetOwner()->GetPlayerID();

	TPacketGDPrivateShopItemCheckin subPacket;
	subPacket.dwShopID = ch->GetPlayerID();
	subPacket.TItem = t;
	subPacket.iPos = p->iDstPos;

	BYTE bSubHeader = PRIVATE_SHOP_GD_SUBHEADER_ITEM_CHECKIN_REQUEST;
	db_clientdesc->DBPacketHeader(HEADER_GD_PRIVATE_SHOP, ch->GetDesc()->GetHandle(), sizeof(BYTE) + sizeof(TPacketGDPrivateShopItemCheckin));
	db_clientdesc->Packet(&bSubHeader, sizeof(BYTE));
	db_clientdesc->Packet(&subPacket, sizeof(TPacketGDPrivateShopItemCheckin));

	return iExtraLen;
}

int CInputMain::PrivateShopItemCheckout(LPCHARACTER ch, const char* c_pData, size_t uiBytes)
{
	TPacketCGPrivateShopItemCheckout* p = (TPacketCGPrivateShopItemCheckout*)c_pData;

	int iExtraLen = sizeof(TPacketCGPrivateShopItemCheckout);

	// No data has been passed
	if (uiBytes < iExtraLen)
		return -1;

	if (!ch)
		return iExtraLen;

	if (ch->IsStun() || ch->IsDead())
		return iExtraLen;

	if (ch->m_pkTimedEvent)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Your logout has been cancelled.");
		event_cancel(&ch->m_pkTimedEvent);

		return iExtraLen;
	}

	if (!ch->IsPrivateShopOwner() || !ch->IsEditingPrivateShop())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You do not have an open private shop."));
		return iExtraLen;
	}

	if (!ch->CanModifyPrivateShop())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot manage private shop's content while it is not in a modifying state."));		
		return iExtraLen;
	}

	const TPlayerPrivateShopItem* c_pShopItem = ch->GetPrivateShopItem(p->wSrcPos);
	if (!c_pShopItem)
	{
		sys_err("Cannot find item on position %d", p->wSrcPos);
		return iExtraLen;
	}

	const TItemTable* pItemTable = ITEM_MANAGER::Instance().GetTable(c_pShopItem->dwVnum);
	if (!pItemTable)
	{
		sys_err("Cannot find item table for item vnum %d", c_pShopItem->dwVnum);
		return iExtraLen;
	}

	BYTE bWindow;

	// Find a position if none was specified or the specified one is invalid
	if (pItemTable->bType == ITEM_DS)
	{
		if (p->iDstPos < 0) {
			return iExtraLen;
		}

		if (!ch->IsEmptyItemGrid(TItemPos(DRAGON_SOUL_INVENTORY, p->iDstPos), c_pShopItem->bSize)) {
			LPITEM pDragonSoulItem = ITEM_MANAGER::Instance().CreateItem(c_pShopItem->dwVnum);
			if (!pDragonSoulItem) {
				return iExtraLen;
			}

			int iPos = ch->GetEmptyDragonSoulInventory(pDragonSoulItem);
			M2_DESTROY_ITEM(pDragonSoulItem);

			if (iPos < 0)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You don't have enough space in your inventory."));
				return iExtraLen;
			}

			p->iDstPos = iPos;
		} else {
			LPITEM pDragonSoulItem = ITEM_MANAGER::Instance().CreateItem(c_pShopItem->dwVnum);
			if (!pDragonSoulItem) {
				return iExtraLen;
			}

			if (!DSManager::instance().IsValidCellForThisItem(pDragonSoulItem, TItemPos(DRAGON_SOUL_INVENTORY, p->iDstPos))) {
				M2_DESTROY_ITEM(pDragonSoulItem);
				return iExtraLen;
			}

			M2_DESTROY_ITEM(pDragonSoulItem);
		}

		bWindow = DRAGON_SOUL_INVENTORY;
	}
	else
	{
#if defined(__SPECIAL_INVENTORY_SYSTEM__)
		if (pItemTable->bType == ITEM_SKILLBOOK) {
			if (!(p->iDstPos >= SKILL_BOOK_INVENTORY_SLOT_START && p->iDstPos < SKILL_BOOK_INVENTORY_SLOT_END)) {
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can move book item just into books inventory!"));
				return iExtraLen;
			}
		}
		else if (ITEM_MANAGER::Instance().IsUpgradeItem(c_pShopItem->dwVnum)) {
			if (!(p->iDstPos >= UPGRADE_ITEMS_INVENTORY_SLOT_START && p->iDstPos < UPGRADE_ITEMS_INVENTORY_SLOT_END)) {
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can move upgrade item just into upgrades inventory!"));
				return iExtraLen;
			}
		}
		else if (pItemTable->bType == ITEM_METIN) {
			if (!(p->iDstPos >= STONE_INVENTORY_SLOT_START && p->iDstPos < STONE_INVENTORY_SLOT_END)) {
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can move stone item just into stones inventory!"));
				return iExtraLen;
			}
		}
		else if (pItemTable->bType == ITEM_GIFTBOX
#if defined(__GACHA_SYSTEM__)
 || pItemTable->bType == ITEM_GACHA
#endif
		) {
			if (!(p->iDstPos >= GIFT_BOX_INVENTORY_SLOT_START && p->iDstPos < GIFT_BOX_INVENTORY_SLOT_END)) {
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can move box items just into boxes inventory!"));
				return iExtraLen;
			}
		}
#endif
		else {
			if (p->iDstPos >= SKILL_BOOK_INVENTORY_SLOT_START) {
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can move normal items just into normal inventory!"));
				return iExtraLen;
			}
		}

		if (p->iDstPos < 0 || !ch->IsEmptyItemGrid(TItemPos(INVENTORY, p->iDstPos), c_pShopItem->bSize))
		{
			int iPos = ch->GetEmptyInventory(c_pShopItem->bSize);
			if (iPos < 0)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You don't have enough space in your inventory."));
				return iExtraLen;
			}

			p->iDstPos = iPos;
		}

		bWindow = INVENTORY;
	}

	// Packet to db
	BYTE bSubHeader = PRIVATE_SHOP_GD_SUBHEADER_ITEM_CHECKOUT_REQUEST;

	TPacketGDPrivateShopItemCheckout subPacket;
	subPacket.dwPID = ch->GetPlayerID();
	subPacket.wSrcPos = p->wSrcPos;
	subPacket.TDstPos.cell = p->iDstPos;
	subPacket.TDstPos.window_type = bWindow;
	subPacket.TItem = *c_pShopItem;

	db_clientdesc->DBPacketHeader(HEADER_GD_PRIVATE_SHOP, ch->GetDesc()->GetHandle(), sizeof(BYTE) + sizeof(TPacketGDPrivateShopItemCheckout));
	db_clientdesc->Packet(&bSubHeader, sizeof(BYTE));
	db_clientdesc->Packet(&subPacket, sizeof(TPacketGDPrivateShopItemCheckout));

	return iExtraLen;
}

int CInputMain::PrivateShopTitleChange(LPCHARACTER ch, const char* c_pData, size_t uiBytes)
{
	const char* c_szTitle = c_pData;
	int iExtraLen = PRIVATE_SHOP_TITLE_MAX_LEN + 1;

	// No data has been passed
	if (uiBytes < iExtraLen)
		return -1;

	if (!ch)
		return iExtraLen;

	if (ch->IsStun() || ch->IsDead())
		return iExtraLen;

	if (!ch->IsPrivateShopOwner() || !ch->IsEditingPrivateShop())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You do not have an open private shop."));
		return iExtraLen;
	}

	if (!ch->CanModifyPrivateShop())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot manage private shop's content while it is not in a modifying state."));		
		return iExtraLen;
	}

	if (strlen(c_szTitle) < PRIVATE_SHOP_TITLE_MIN_LEN)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The name entered is too short."));
		return iExtraLen;
	}

	BYTE bSubHeader = PRIVATE_SHOP_GD_SUBHEADER_TITLE_CHANGE_REQUEST;

	TPacketPrivateShopTitleChange subPacket;
	subPacket.dwPID = ch->GetPlayerID();
	strncpy(subPacket.szTitle, c_szTitle, PRIVATE_SHOP_TITLE_MAX_LEN);

	db_clientdesc->DBPacketHeader(HEADER_GD_PRIVATE_SHOP, ch->GetDesc()->GetHandle(), sizeof(BYTE) + sizeof(TPacketPrivateShopTitleChange));
	db_clientdesc->Packet(&bSubHeader, sizeof(BYTE));
	db_clientdesc->Packet(&subPacket, sizeof(TPacketPrivateShopTitleChange));

	return iExtraLen;
}

void CInputMain::PrivateShopSearchClose(LPCHARACTER ch)
{
	if (!ch)
		return;

	ch->CloseShopSearch();
}

int CInputMain::PrivateShopSearch(LPCHARACTER ch, const char* c_pData, size_t uiBytes)
{
	TPacketCGPrivateShopSearch* p = (TPacketCGPrivateShopSearch*)c_pData;

	int iExtraLen = sizeof(TPacketCGPrivateShopSearch);

	// No data has been passed
	if (uiBytes < iExtraLen)
		return -1;

	if (!ch)
		return iExtraLen;

	if (ch->IsStun() || ch->IsDead())
		return iExtraLen;

	CPrivateShopManager::Instance().SearchItem(ch->GetDesc(), p->dwVnum, p->bIgnoreRefineGrade, p->iJob, p->bItemType, p->bItemSubType, p->aAttr, p->llMaxGold, p->dwMaxCheque);

	TPacketGGPrivateShopItemSearch packet;
	packet.bHeader = HEADER_GG_PRIVATE_SHOP_ITEM_SEARCH;
	packet.dwCustomerID = ch->GetPlayerID();
	packet.dwCustomerPort = p2p_port;
	packet.dwVnum = p->dwVnum;
	packet.bIgnoreRefineGrade = p->bIgnoreRefineGrade;
	packet.iJob = p->iJob;
	packet.bItemSubType = p->bItemSubType;
	packet.bItemType = p->bItemType;
	packet.llMaxGold = p->llMaxGold;
	packet.dwMaxCheque = p->dwMaxCheque;
	thecore_memcpy(packet.aAttr, p->aAttr, sizeof(packet.aAttr));

	P2P_MANAGER::Instance().Send(&packet, sizeof(TPacketGGPrivateShopItemSearch));

	return iExtraLen;
}

int CInputMain::PrivateShopSearchBuy(LPCHARACTER ch, const char* c_pData, size_t uiBytes)
{
	TPacketCGPrivateShopSearchBuy* p = (TPacketCGPrivateShopSearchBuy*)c_pData;

	int iExtraLen = sizeof(TPacketCGPrivateShopSearchBuy);

	// No data has been passed
	if (uiBytes < iExtraLen)
		return -1;

	if (!ch)
		return iExtraLen;

	if (ch->IsStun() || ch->IsDead())
		return iExtraLen;

	if (!ch->IsShopSearch())
		return iExtraLen;

	if (p->dwShopID == ch->GetPlayerID())
		return iExtraLen;

	// Packet to db
	BYTE bSubHeader = PRIVATE_SHOP_GD_SUBHEADER_BUY_REQUEST;

	TPacketGDPrivateShopBuyRequest subPacket;
	subPacket.dwCustomerPID = ch->GetPlayerID();
	subPacket.dwShopID = p->dwShopID;
	subPacket.wPos = p->wPos;
	subPacket.llGoldBalance = ch->GetGold();
	subPacket.dwChequeBalance = ch->GetCheque();
	subPacket.llGoldPrice = p->llGoldPrice;
	subPacket.dwChequePrice = p->dwChequePrice;

	db_clientdesc->DBPacketHeader(HEADER_GD_PRIVATE_SHOP, ch->GetDesc()->GetHandle(), sizeof(BYTE) + sizeof(TPacketGDPrivateShopBuyRequest));
	db_clientdesc->Packet(&bSubHeader, sizeof(BYTE));
	db_clientdesc->Packet(&subPacket, sizeof(TPacketGDPrivateShopBuyRequest));

	return iExtraLen;
}
#endif

#if defined(ENABLE_INGAME_WIKI)
void CInputMain::RecvWikiPacket(LPCHARACTER ch, const char * c_pData)
{
	if (!ch || (ch && !ch->GetDesc()))
		return;
	
	if (!c_pData)
		return;
	
	InGameWiki::TCGWikiPacket * p = nullptr;
	if (!(p = (InGameWiki::TCGWikiPacket *) c_pData))
		return;
	
	InGameWiki::TGCWikiPacket pack;
	pack.set_data_type(!p->is_mob ? InGameWiki::LOAD_WIKI_ITEM : InGameWiki::LOAD_WIKI_MOB);
	pack.increment_data_size(WORD(sizeof(InGameWiki::TGCWikiPacket)));
	
	if (pack.is_data_type(InGameWiki::LOAD_WIKI_ITEM))
	{
		const std::vector<CommonWikiData::TWikiItemOriginInfo>& originVec = ITEM_MANAGER::Instance().GetItemOrigin(p->vnum);
		const std::vector<CSpecialItemInfo> _gV = ITEM_MANAGER::instance().GetWikiChestInfo(p->vnum);
		const std::vector<CommonWikiData::TWikiRefineInfo> _rV = ITEM_MANAGER::instance().GetWikiRefineInfo(p->vnum);
		const CommonWikiData::TWikiInfoTable* _wif = ITEM_MANAGER::instance().GetItemWikiInfo(p->vnum);
		
		if (!_wif)
			return;
		
		const size_t origin_size = originVec.size();
		const size_t chest_info_count = _wif->chest_info_count;
		const size_t refine_infos_count = _wif->refine_infos_count;
		const size_t buf_data_dize = sizeof(InGameWiki::TGCItemWikiPacket) +
								(origin_size * sizeof(CommonWikiData::TWikiItemOriginInfo)) +
								(chest_info_count * sizeof(CommonWikiData::TWikiChestInfo)) +
								(refine_infos_count * sizeof(CommonWikiData::TWikiRefineInfo));
		
		if (chest_info_count != _gV.size()) {
			sys_err("Item Vnum : %d || ERROR TYPE -> 1", p->vnum);
			return;
		}
		
		if (refine_infos_count != _rV.size()) {
			sys_err("Item Vnum : %d || ERROR TYPE -> 2", p->vnum);
			return;
		}
		
		pack.increment_data_size(WORD(buf_data_dize));
		
		TEMP_BUFFER buf;
		buf.write(&pack, sizeof(InGameWiki::TGCWikiPacket));
		
		InGameWiki::TGCItemWikiPacket data_packet;
		data_packet.mutable_wiki_info(*_wif);
		data_packet.set_origin_infos_count(origin_size);
		data_packet.set_vnum(p->vnum);
		data_packet.set_ret_id(p->ret_id);
		buf.write(&data_packet, sizeof(data_packet));
		
		{
			if (origin_size)
				for (int idx = 0; idx < (int)origin_size; ++idx)
					buf.write(&(originVec[idx]), sizeof(CommonWikiData::TWikiItemOriginInfo));
			
			if (chest_info_count > 0) {
				for (int idx = 0; idx < (int)chest_info_count; ++idx) {
					CommonWikiData::TWikiChestInfo write_struct(_gV[idx].vnum, _gV[idx].count);
					buf.write(&write_struct, sizeof(CommonWikiData::TWikiChestInfo));
				}
			}
			
			if (refine_infos_count > 0)
				for (int idx = 0; idx < (int)refine_infos_count; ++idx)
					buf.write(&(_rV[idx]), sizeof(CommonWikiData::TWikiRefineInfo));
		}
		
		ch->GetDesc()->Packet(buf.read_peek(), buf.size());
	}
	else
	{
		CMobManager::TMobWikiInfoVector& mobVec = CMobManager::instance().GetMobWikiInfo(p->vnum);
		const size_t _mobVec_size = mobVec.size();
		
		if (!_mobVec_size) {
			if (test_server)
				sys_log(0, "Mob Vnum: %d : || LOG TYPE -> 1", p->vnum);
			return;
		}
		
		const size_t buf_data_dize = (sizeof(InGameWiki::TGCMobWikiPacket) + (_mobVec_size * sizeof(CommonWikiData::TWikiMobDropInfo)));
		pack.increment_data_size(WORD(buf_data_dize));
		
		TEMP_BUFFER buf;
		buf.write(&pack, sizeof(InGameWiki::TGCWikiPacket));
		
		InGameWiki::TGCMobWikiPacket data_packet;
		data_packet.set_drop_info_count(_mobVec_size);
		data_packet.set_vnum(p->vnum);
		data_packet.set_ret_id(p->ret_id);
		buf.write(&data_packet, sizeof(InGameWiki::TGCMobWikiPacket));
		
		{
			if (_mobVec_size) {
				for (int idx = 0; idx < (int)_mobVec_size; ++idx) {
					CommonWikiData::TWikiMobDropInfo write_struct(mobVec[idx].vnum, mobVec[idx].count);
					buf.write(&write_struct, sizeof(CommonWikiData::TWikiMobDropInfo));
				}
			}
		}
		
		ch->GetDesc()->Packet(buf.read_peek(), buf.size());
	}
}
#endif

