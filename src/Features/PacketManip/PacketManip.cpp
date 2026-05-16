#include "PacketManip.h"

inline bool CPacketManip::WillTimeOut() {
	INetChannel* iNetChan = I::EngineClient->GetNetChannelInfo();
	if (!iNetChan) {
		return false;
	}

	return iNetChan->m_nChokedPackets >= 21;
}

inline bool CPacketManip::AACheck(CUserCmd* pCmd) {
	INetChannel* iNetChan = I::EngineClient->GetNetChannelInfo();
	CBaseEntity* pLocal = g_EntityCache.GetLocal();
	if (!iNetChan || !pLocal) {
		return false;
	}

	return !iNetChan->m_nChokedPackets && F::AntiAim.ShouldAntiAim(pLocal);
}

inline void CPacketManip::RunFakeLag(CUserCmd* pCmd, bool* pSendPacket, const EHANDLE& nOldGroundInt, const int nOldFlags) {
	F::FakeLag.Run(pCmd, pSendPacket, nOldGroundInt, nOldFlags);
}

void CPacketManip::CreateMove(CUserCmd* pCmd, bool* pSendPacket, const EHANDLE& nOldGroundInt, const int nOldFlags) {
	static bool bSilentChokedLastTick = false;

	const bool bTickbaseActive = G::ShouldShift || G::Teleporting || G::Recharging || Vars::Misc::CL_Move::SEnabled.Value;
	if (!bTickbaseActive) {
		*pSendPacket = true;
	}

	const bool bSilentAttack = pCmd && (pCmd->buttons & (IN_ATTACK | IN_ATTACK2 | IN_ATTACK3));

	//prevent overchoking by just not running anything below if we believe it will cause us to time out
	if (!WillTimeOut() && !bTickbaseActive) {
		//anti aim will no longer set pSendPacket to false/true
		if (AACheck(pCmd)) {
			*pSendPacket = false;
			F::AntiAim.Run(pCmd, pSendPacket);
			return;
		}

		if (G::SilentTime) {
			// Only choke one silent shot tick, then force the packet out on the next command.
			// Choking every silent command indefinitely makes fire look client-side and drops trigger / autoshoot shots.
			if (bSilentAttack && !bSilentChokedLastTick) {
				*pSendPacket = false;
				bSilentChokedLastTick = true;
				F::AntiAim.Run(pCmd, pSendPacket);
				return;
			}

			bSilentChokedLastTick = false;
			*pSendPacket = true;
			F::AntiAim.Run(pCmd, pSendPacket);
			return;
		}

		bSilentChokedLastTick = false;
		//F::AntiAim.Run(pCmd, pSendPacket);
		RunFakeLag(pCmd, pSendPacket, nOldGroundInt, nOldFlags);
	}
	else if (!G::SilentTime) {
		bSilentChokedLastTick = false;
	}
	
	if (G::ShouldShift || G::Teleporting) {
		*pSendPacket = G::ShiftedTicks <= 1;
	}

	if (!bTickbaseActive) {
		return F::AntiAim.Run(pCmd, pSendPacket);
	}
}
