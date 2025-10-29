///----------------------------------------------------------------------------------------------------
/// Copyright (c) Raidcore.GG - Licensed under the MIT License.
///
/// Name         :  ArcEvents.cpp
/// Description  :  Contains the callbacks for ArcDPS.
///----------------------------------------------------------------------------------------------------

#include <chrono>
#include <fstream>
#include <iomanip>
#include <map>
#include <string>

#include "ArcEvents.h"

#include "Defines.h"
#include "Shared.h"
#include "Types.h"

namespace
{
constexpr static auto MIN_TIME_DIFF = 10U;

static auto is_first_cast_map = std::map<std::string, bool>{};

bool IsValidSelfData(const EvCombatData &evCbtData)
{
    return evCbtData.src->IsSelf && evCbtData.src != nullptr &&
           evCbtData.skillname != nullptr && evCbtData.ev != nullptr &&
           evCbtData.src->Name != nullptr;
}

bool IsValidCombatEvent(const EvCombatData &evCbtData)
{
    const auto is_valid_self = IsValidSelfData(evCbtData);

    return is_valid_self;
}

bool IsSkillFromBuild_IdBased(const EvCombatDataPersistent &evCbtData)
{
#ifdef _DEBUG
    if (evCbtData.SkillID == static_cast<uint64_t>(-1))
        return true;
#endif

    const auto &skill_data_map = Globals::RotationRun.skill_data_map;
    const auto found = skill_data_map.find(evCbtData.SkillID) != skill_data_map.end();
    return found;
}

bool IsSkillFromBuild_NameBased(const EvCombatDataPersistent &evCbtData)
{
    const auto &skill_data_map = Globals::RotationRun.skill_data_map;
    for (const auto &kv : skill_data_map)
    {
        if (kv.second.name == evCbtData.SkillName)
            return true;
    }
    return false;
}

bool IsAnySkillFromBuild(const EvCombatDataPersistent &evCbtData)
{
    return IsSkillFromBuild_NameBased(evCbtData) ||
           IsSkillFromBuild_IdBased(evCbtData);
}

std::chrono::steady_clock::time_point UpdateCastTime(
    std::map<std::string, std::chrono::steady_clock::time_point> &last_cast_map,
    const EvCombatDataPersistent &evCbtData)
{
    const auto now = std::chrono::steady_clock::now();

    last_cast_map[evCbtData.SkillName] = now;

    return now;
}

std::chrono::steady_clock::time_point GetLastCastTime(
    const EvCombatDataPersistent &evCbtData)
{
    static auto last_cast_map =
        std::map<std::string, std::chrono::steady_clock::time_point>{};

    const auto it = last_cast_map.find(evCbtData.SkillName);
    if (it != last_cast_map.end())
    {
        const auto time = it->second;
        UpdateCastTime(last_cast_map, evCbtData);

        return time;
    }

    const auto now = UpdateCastTime(last_cast_map, evCbtData);
    return now;
}

bool IsNotTheSameCast(const EvCombatDataPersistent &evCbtData)
{
    const auto now = std::chrono::steady_clock::now();
    const auto last_cast_time = GetLastCastTime(evCbtData);
    const auto is_not_same_cast =
        (now - last_cast_time) > std::chrono::milliseconds(MIN_TIME_DIFF);

    if (is_first_cast_map.find(evCbtData.SkillName) == is_first_cast_map.end())
    {
        is_first_cast_map[evCbtData.SkillName] = true;
        return true;
    }

    return is_not_same_cast;
}
}; // namespace

namespace ArcEv
{
void ResetSkillCastTracking()
{
    is_first_cast_map.clear();
}

void OnCombatLocal(void *data)
{
    if (data == nullptr)
        return;

    auto *combat_data = static_cast<EvCombatData *>(data);
    if (combat_data == nullptr)
        return;

    OnCombat("EV_ARCDPS_COMBATEVENT_LOCAL_RAW",
             combat_data->ev,
             combat_data->src,
             combat_data->dst,
             combat_data->skillname,
             combat_data->id,
             combat_data->revision);
}

bool OnCombat(const char *channel,
              ArcDPS::CombatEvent *ev,
              ArcDPS::AgentShort *src,
              ArcDPS::AgentShort *dst,
              char *skillname,
              uint64_t id,
              uint64_t revision)
{
#ifdef GW2_NEXUS_ADDON
    if (Globals::APIDefs == nullptr)
        return false;
#endif

    if (channel == nullptr || ev == nullptr || src == nullptr ||
        dst == nullptr || skillname == nullptr)
        return false;

    auto evCbtData = EvCombatData{ev, src, dst, skillname, id, revision};

    if (IsValidCombatEvent(evCbtData))
    {
        const auto data = EvCombatDataPersistent{
            .SrcName = std::string(evCbtData.src->Name),
            .SrcID = evCbtData.src->ID,
            .SrcProfession = evCbtData.src->Profession,
            .SrcSpecialization = evCbtData.src->Specialization,
            .SkillName = std::string(evCbtData.skillname),
            .SkillID = evCbtData.ev->SkillID,
            .EventID = evCbtData.id,
        };

        if (Globals::RotationRun.log_skill_info_map.empty() ||
            IsAnySkillFromBuild(data))
        {
            if (IsNotTheSameCast(data))
            {
                Globals::Render.skill_activation_callback(true, data);

                return true;
            }
        }
    }

    return false;
}
} // namespace ArcEv
