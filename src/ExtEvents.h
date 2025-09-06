///----------------------------------------------------------------------------------------------------
/// Copyright (c) Raidcore.GG - Licensed under the MIT License.
///
/// Name         :  ExtEvents.h
/// Description  :  Contains the logic for extended events.
///----------------------------------------------------------------------------------------------------

#ifndef EXTEVENTS_H
#define EXTEVENTS_H

#include <Windows.h>

#include "arcdps/ArcDPS.h"
#include "Shared.h"

#define EV_ACCOUNT_NAME          "EV_ACCOUNT_NAME"
#define EV_ARCDPS_SELF_JOIN      "EV_ARCDPS_SELF_JOIN"
#define EV_ARCDPS_SELF_LEAVE     "EV_ARCDPS_SELF_LEAVE"
#define EV_ARCDPS_SQUAD_JOIN     "EV_ARCDPS_SQUAD_JOIN"
#define EV_ARCDPS_SQUAD_LEAVE    "EV_ARCDPS_SQUAD_LEAVE"
#define EV_ARCDPS_TARGET_CHANGED "EV_ARCDPS_TARGET_CHANGED"

#define EV_REQUEST_ACCOUNT_NAME         "EV_REQUEST_ACCOUNT_NAME"
#define EV_REPLAY_ARCDPS_SELF_JOIN      "EV_REPLAY_ARCDPS_SELF_JOIN"
#define EV_REPLAY_ARCDPS_SQUAD_JOIN     "EV_REPLAY_ARCDPS_SQUAD_JOIN"
#define EV_REPLAY_ARCDPS_TARGET_CHANGED "EV_REPLAY_ARCDPS_TARGET_CHANGED"

///----------------------------------------------------------------------------------------------------
/// Ext Namespace
///----------------------------------------------------------------------------------------------------
namespace Ext
{
	///----------------------------------------------------------------------------------------------------
	/// AgentUpdate:
	/// 	Processes agent updates from ArcDPS.
	///----------------------------------------------------------------------------------------------------
	void AgentUpdate(EvCombatData* evCbtData);

	///----------------------------------------------------------------------------------------------------
	/// OnAccountNameRequest:
	/// 	Triggers an event (EV_ACCOUNT_NAME) to share the account name of your own user.
	///----------------------------------------------------------------------------------------------------
	void OnAccountNameRequest(void* eventArgs);

	///----------------------------------------------------------------------------------------------------
	/// OnSelfRequest:
	/// 	Triggers an event (EV_ARCDPS_SELF_JOIN) to share the self agent data by ArcDPS.
	///----------------------------------------------------------------------------------------------------
	void OnSelfRequest(void* eventArgs);

	///----------------------------------------------------------------------------------------------------
	/// OnSquadRequest:
	/// 	Triggers events (EV_ARCDPS_SQUAD_JOIN) for each squad member to share agent data by ArcDPS.
	///----------------------------------------------------------------------------------------------------
	void OnSquadRequest(void* eventArgs);

	///----------------------------------------------------------------------------------------------------
	/// OnTargetRequest:
	/// 	Triggers an event (EV_ARCDPS_TARGET_CHANGED) to share the last target as reported by ArcDPS.
	///----------------------------------------------------------------------------------------------------
	void OnTargetRequest(void* eventArgs);
}

#endif
