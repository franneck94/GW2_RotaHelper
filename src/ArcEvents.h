///----------------------------------------------------------------------------------------------------
/// Copyright (c) Raidcore.GG - Licensed under the MIT License.
///
/// Name         :  ArcEvents.h
/// Description  :  Contains the callbacks for ArcDPS.
///----------------------------------------------------------------------------------------------------

#ifndef ARCEVENTS_H
#define ARCEVENTS_H

#include "arcdps/ArcDPS.h"

///----------------------------------------------------------------------------------------------------
/// ArcEv Namespace
///----------------------------------------------------------------------------------------------------
namespace ArcEv
{
	bool OnCombatLocal(
		ArcDPS::CombatEvent* ev,
		ArcDPS::AgentShort* src,
		ArcDPS::AgentShort* dst,
		char* skillname,
		uint64_t id,
		uint64_t revision
	);

	bool OnCombat(
		const char* channel,
		ArcDPS::CombatEvent* ev,
		ArcDPS::AgentShort* src,
		ArcDPS::AgentShort* dst,
		char* skillname,
		uint64_t id, uint64_t
		revision
	);
}

#endif
