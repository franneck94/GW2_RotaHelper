///----------------------------------------------------------------------------------------------------
/// Copyright (c) Raidcore.GG - Licensed under the MIT License.
///
/// Name         :  ArcEvents.h
/// Description  :  Contains the callbacks for ArcDPS.
///----------------------------------------------------------------------------------------------------

#ifndef ARCEVENTS_H
#define ARCEVENTS_H

#include <chrono>

#include "arcdps/ArcDPS.h"
#include "SkillIDs.h"
#include "Types.h"

///----------------------------------------------------------------------------------------------------
/// ArcEv Namespace
///----------------------------------------------------------------------------------------------------
namespace ArcEv
{
void StartCombatEventProcessing();
void StopCombatEventProcessing();
void ProcessPendingCombatEvents();
void ResetSkillCastTracking();
void ResetObservedSkillCapabilities();
bool HasObservedSkill(SkillID skill_id);
bool WasSkillObservedRecently(SkillID skill_id, std::chrono::milliseconds interval);
const char *SkillEventKindName(SkillEventKind kind);

void OnCombatLocal(void *data);

bool OnCombat(const char *channel,
              ArcDPS::CombatEvent *ev,
              ArcDPS::AgentShort *src,
              ArcDPS::AgentShort *dst,
              char *skillname,
              uint64_t id,
              uint64_t revision);
} // namespace ArcEv

#endif
