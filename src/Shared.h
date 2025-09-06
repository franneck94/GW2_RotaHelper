#ifndef SHARED_H
#define SHARED_H

#include <string>

#include "arcdps/ArcDPS.h"
#include "mumble/Mumble.h"
#include "nexus/Nexus.h"
#include "rtapi/RTAPI.hpp"
#include "arcdps/ArcDPS.h"

extern AddonAPI *APIDefs;
extern NexusLinkData *NexusLink;
extern RTAPI::RealTimeData *RTAPIData;
extern const char *KB_TOGGLE_GW2RotaHelper;
extern const char *ADDON_NAME;
extern std::string AccountName;
extern ArcDPS::Exports ArcExports;

struct EvCombatData
{
    ArcDPS::CombatEvent *ev;
    ArcDPS::AgentShort *src;
    ArcDPS::AgentShort *dst;
    char *skillname;
    uint64_t id;
    uint64_t revision;
};

struct EvAgentUpdate // when ev is null
{
    char account[64];     // dst->name  = account name
    char character[64];   // src->name  = character name
    uintptr_t id;         // src->id    = agent id
    uintptr_t instanceId; // dst->id    = instance id (per map)
    uint32_t added;       // src->prof  = is new agent
    uint32_t target;      // src->elite = is new targeted agent
    uint32_t Self;        // dst->Self  = is Self
    uint32_t prof;        // dst->prof  = profession / core spec
    uint32_t elite;       // dst->elite = elite spec
    uint16_t team;        // src->team  = team
    uint16_t subgroup;    // dst->team  = subgroup
};

#endif
