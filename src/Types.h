#include <cstdint>
#include <string>

#include "arcdps/ArcDPS.h"

struct EvCombatDataPersistent
{
    std::string SrcName;
    uintptr_t SrcID;
    uint32_t SrcProfession;
    uint32_t SrcSpecialization;
    std::string DstName;
    uintptr_t DstID;
    uint32_t DstProfession;
    uint32_t DstSpecialization;
    std::string SkillName;
    uint64_t SkillID;
};

struct EvCombatData
{
    ArcDPS::CombatEvent *ev;
    ArcDPS::AgentShort *src;
    ArcDPS::AgentShort *dst;
    char *skillname;
    uint64_t id;
    uint64_t revision;
};

struct EvAgentUpdate
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
