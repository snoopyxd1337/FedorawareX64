#include "../Hooks.h"

#include "../../Features/Misc/Misc.h"
#include "../../Features/ChatInfo/ChatInfo.h"

#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "../../Features/Visuals/Visuals.h"
#include "../../Features/NoSpread/NoSpread.h"
#include "../../Features/Auto/AutoUber/AutoUber.h"
#include "../../Utils/FunctionalDebug/FunctionalDebug.h"

static int anti_balance_attempts = 0;
static std::string previous_name;

const static std::string CLEAR_MSG("?\nServer:\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
								   "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
								   "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
								   "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
								   "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
								   "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
								   "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");

const static std::vector<std::string> BAD_WORDS{ "cheat", "hack", "bot", "aim", "esp", "kick", "hax", "script" };

static std::string clr({ '\x7', '0', 'D', '9', '2', 'F', 'F' });
static std::string yellow({ '\x7', 'C', '8', 'A', '9', '0', '0' }); //C8A900
static std::string white({ '\x7', 'F', 'F', 'F', 'F', 'F', 'F' }); //FFFFFF
static std::string green({ '\x7', '3', 'A', 'F', 'F', '4', 'D' }); //3AFF4D

MAKE_HOOK(BaseClientDLL_DispatchUserMessage, Utils::GetVFuncPtr(I::BaseClientDLL, 36), bool, __fastcall,
		  void* ecx, UserMessageType type, bf_read& msgData)
{
	FDBG_SCOPE("Hook.BaseClientDLL_DispatchUserMessage");

	if (!ecx)
	{
		FDBG_SKIP("Hook.BaseClientDLL_DispatchUserMessage", "ecx null");
		return false;
	}

	if (!msgData.m_pData)
	{
		FDBG_SKIP("UserMessage.Features", "msgData.m_pData null");
		return Hook.Original<FN>()(ecx, type, msgData);
	}

	const auto bufData = reinterpret_cast<const char*>(msgData.m_pData);
	msgData.SetAssertOnOverflow(false);

	FDBG_CALL("Feature.ChatInfo.UserMessage", F::ChatInfo.UserMessage(type, msgData));
	msgData.Seek(0);

	switch (type)
	{
		case SayText2:
		{
			const int nbl = msgData.GetNumBytesLeft();
			if (nbl < 5 || nbl >= 256)
			{
				break;
			}

			const int entIdx = msgData.ReadByte();
			msgData.Seek(8);
			char typeBuffer[256], nameBuffer[256], msgBuffer[256];
			if (msgData.GetNumBytesLeft() == 0) { break; }
			msgData.ReadString(typeBuffer, 256);
			if (msgData.GetNumBytesLeft() == 0) { break; }
			msgData.ReadString(nameBuffer, 256);
			if (msgData.GetNumBytesLeft() == 0) { break; }
			msgData.ReadString(msgBuffer, 256);

			std::string chatType(typeBuffer);
			std::string playerName(nameBuffer);
			std::string chatMessage(msgBuffer);

			FDBG_CALL("Feature.RSChat.PushChat", F::RSChat.PushChat(I::ClientEntityList->GetClientEntity(entIdx), Utils::ConvertUtf8ToWide(chatMessage)));

			// F::RSChat.PushChat(I::ClientEntityList->GetClientEntity(entIdx), chatMessage);
			/*if (Vars::Misc::ChatCensor.Value)
			{
				PlayerInfo_t senderInfo{};
				if (I::Engine->GetPlayerInfo(entIdx, &senderInfo))
				{
					if (entIdx == I::Engine->GetLocalPlayer()) { break; }
					if (G::IsIgnored(senderInfo.friendsID) || g_EntityCache.IsFriend(entIdx))
					{
						break;
					}

					const std::vector<std::string> toReplace = { " ", "4", "3", "0", "6", "5", "7", "@", ".", ",", "-", "!" };
					const std::vector<std::string> replaceWith = { "", "a", "e", "o", "g", "s", "t", "a", "", "", "", "i" };

					for (std::vector<int>::size_type i = 0; i != toReplace.size(); i++)
					{
						boost::replace_all(chatMessage, toReplace[i], replaceWith[i]);
					}

					for (auto& word : BAD_WORDS)
					{
						if (boost::contains(chatMessage, word))
						{
							const std::string cmd = "say_team \"" + CLEAR_MSG + "\"";
							I::Engine->ServerCmd(cmd.c_str(), true);
							I::ClientMode->m_pChatElement->ChatPrintf(0, tfm::format("%s[FeD] \x3 %s\x1 wrote\x3 %s", clr, playerName, chatMessage).c_str());
							break;
						}
					}
				}
			}*/

			break;
		}

		case VoiceSubtitle:
		{
			const int iEntityID = msgData.ReadByte();
			const int iVoiceMenu = msgData.ReadByte();
			const int iCommandID = msgData.ReadByte();

			if (iVoiceMenu == 1 && iCommandID == 6)
			{
				FDBG_CALL("Feature.AutoUber.AddMedicCaller", F::AutoUber.AddMedicCaller(iEntityID));
			}

			break;
		}

		case TextMsg:
		{
			bool parsedPerf = false;
			FDBG_CALL("Feature.NoSpread.ParsePlayerPerf", parsedPerf = F::NoSpread.ParsePlayerPerf(msgData));
			if (parsedPerf) {
				return true;
			}

			if (Vars::Misc::AntiAutobal.Value && msgData.GetNumBitsLeft() > 35)
			{
				const INetChannel* server = I::EngineClient->GetNetChannelInfo();
				const std::string data(bufData);

				if (data.find("TeamChangeP") != std::string::npos && g_EntityCache.GetLocal())
				{
					const std::string serverName(server->GetAddress());
					if (serverName != previous_name)
					{
						previous_name = serverName;
						anti_balance_attempts = 0;
					}
					if (anti_balance_attempts < 2)
					{
						FDBG_CALL("UserMessage.AntiAutobal.Retry", I::EngineClient->ClientCmd_Unrestricted("retry"));
					}
					else
					{
						FDBG_CALL("UserMessage.AntiAutobal.PartyChat", I::EngineClient->ClientCmd_Unrestricted(
							"tf_party_chat \"I will be autobalanced in 3 seconds\""));
					}
					anti_balance_attempts++;
				}
			}
			break;
		}

		case VGUIMenu:
		{
			const auto szMenu = reinterpret_cast<const char*>(msgData.m_pData);
			FW_TRACE((std::string("[VGUI] menu=")
				+ szMenu
				+ " removeMOTD=" + std::to_string(Vars::Visuals::RemoveMOTD.Value)
				+ " autoJoin=" + std::to_string(Vars::Misc::AutoJoin.Value)).c_str());

			// Remove MOTD
			if (Vars::Visuals::RemoveMOTD.Value || Vars::Misc::AutoJoin.Value)
			{
				if (strcmp(szMenu, "info") == 0)
				{
					FW_TRACE("[VGUI] closing MOTD");
					FDBG_CALL("UserMessage.VGUI.CloseMOTD", I::EngineClient->ClientCmd_Unrestricted("closedwelcomemenu"));
					return true;
				}
			}

			// Autojoin team / class
			const int autoJoin = Vars::Misc::AutoJoin.Value;
			if (autoJoin > 0)
			{
				if (strcmp(szMenu, "team") == 0)
				{
					FW_TRACE("[VGUI] autojoin autoteam requested");
					FDBG_CALL("UserMessage.VGUI.AutoTeam", I::EngineClient->ClientCmd_Unrestricted("autoteam"));
					break;
				}

				if (autoJoin <= 9 && strncmp(szMenu, "class_", 6) == 0)
				{
					static std::string classNames[] = { "scout", "soldier", "pyro", "demoman", "heavyweapons", "engineer", "medic", "sniper", "spy" };
					FW_TRACE(("[VGUI] autojoin class requested: " + classNames[autoJoin - 1]).c_str());
					FDBG_CALL("UserMessage.VGUI.JoinClass", I::EngineClient->ClientCmd_Unrestricted(std::string("join_class").append(" ").append(classNames[autoJoin - 1]).c_str()));
					break;
				}
			}

			break;
		}

		case ForcePlayerViewAngles:
		{
			return Vars::Visuals::PreventForcedAngles.Value ? true : Hook.Original<FN>()(ecx, type, msgData);
		}

		case SpawnFlyingBird:
		case PlayerGodRayEffect:
		case PlayerTauntSoundLoopStart:
		case PlayerTauntSoundLoopEnd:
		{
			return Vars::Visuals::RemoveTaunts.Value ? true : Hook.Original<FN>()(ecx, type, msgData);
		}

		case Shake:
		case Fade:
		case Rumble:
		{
			return Vars::Visuals::RemoveScreenEffects.Value ? true : Hook.Original<FN>()(ecx, type, msgData);
		}
	}

	msgData.Seek(0);
	bool result = false;
	FDBG_CALL("UserMessage.Original", result = Hook.Original<FN>()(ecx, type, msgData));
	return result;
}
