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
	///----------------------------------------------------------------------------------------------------
	/// OnCombatSquad:
	/// 	Receives arcdps combat callbacks. (squad)
	///----------------------------------------------------------------------------------------------------
	void OnCombatSquad(
		ArcDPS::CombatEvent* ev,
		ArcDPS::AgentShort* src,
		ArcDPS::AgentShort* dst,
		char* skillname,
		uint64_t id,
		uint64_t revision
	);
	
	///----------------------------------------------------------------------------------------------------
	/// OnCombatLocal:
	/// 	Receives arcdps combat callbacks. (local)
	///----------------------------------------------------------------------------------------------------
	void OnCombatLocal(
		ArcDPS::CombatEvent* ev,
		ArcDPS::AgentShort* src,
		ArcDPS::AgentShort* dst,
		char* skillname,
		uint64_t id,
		uint64_t revision
	);
	
	///----------------------------------------------------------------------------------------------------
	/// OnCombat:
	/// 	Relays the combat event to Nexus.
	///----------------------------------------------------------------------------------------------------
	void OnCombat(
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
