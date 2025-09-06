///----------------------------------------------------------------------------------------------------
/// Copyright (c) Raidcore.GG - Licensed under the MIT License.
///
/// Name         :  ArcEvents.cpp
/// Description  :  Contains the callbacks for ArcDPS.
///----------------------------------------------------------------------------------------------------

#include "ArcEvents.h"

#include "ExtEvents.h"

namespace ArcEv
{
	void OnCombatSquad(
		ArcDPS::CombatEvent* ev,
		ArcDPS::AgentShort* src,
		ArcDPS::AgentShort* dst,
		char* skillname,
		uint64_t id,
		uint64_t revision
	)
	{
		OnCombat("EV_ARCDPS_COMBATEVENT_SQUAD_RAW", ev, src, dst, skillname, id, revision);
	}

	void OnCombatLocal(
		ArcDPS::CombatEvent* ev,
		ArcDPS::AgentShort* src,
		ArcDPS::AgentShort* dst,
		char* skillname,
		uint64_t id,
		uint64_t revision
	)
	{
		OnCombat("EV_ARCDPS_COMBATEVENT_LOCAL_RAW", ev, src, dst, skillname, id, revision);
	}

	void OnCombat(
		const char* channel,
		ArcDPS::CombatEvent* ev,
		ArcDPS::AgentShort* src,
		ArcDPS::AgentShort* dst,
		char* skillname,
		uint64_t id, uint64_t
		revision
	)
	{
		if (APIDefs == nullptr) { return; }

		EvCombatData evCbtData
		{
			ev,
			src,
			dst,
			skillname,
			id,
			revision
		};

		APIDefs->Events.Raise(channel, &evCbtData);

		Ext::AgentUpdate(&evCbtData);
	}
}
