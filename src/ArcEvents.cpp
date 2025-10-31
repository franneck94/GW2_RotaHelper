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
#include "FileUtils.h"
#include "MumbleUtils.h"
#include "Shared.h"
#include "Types.h"

namespace
{
constexpr static auto MIN_TIME_DIFF = 10U;

static auto is_first_cast_map = std::map<std::string, bool>{};

bool IsValidSelfData(const EvCombatData &combat_data)
{
    return combat_data.src->IsSelf && combat_data.src != nullptr &&
           combat_data.skillname != nullptr && combat_data.ev != nullptr &&
           combat_data.src->Name != nullptr;
}

bool IsValidCombatEvent(const EvCombatData &combat_data)
{
    const auto is_valid_self = IsValidSelfData(combat_data);

    return is_valid_self;
}

bool IsSkillFromBuild_IdBased(const EvCombatDataPersistent &combat_data)
{
#ifdef _DEBUG
    if (combat_data.SkillID == static_cast<uint64_t>(-1))
        return true;
#endif

    const auto &skill_data_map = Globals::RotationRun.skill_data_map;
    const auto found =
        skill_data_map.find(combat_data.SkillID) != skill_data_map.end();
    return found;
}

bool IsSkillFromBuild_NameBased(const EvCombatDataPersistent &combat_data)
{
    const auto &skill_data_map = Globals::RotationRun.skill_data_map;
    for (const auto &kv : skill_data_map)
    {
        if (kv.second.name == combat_data.SkillName)
            return true;
    }
    return false;
}

bool IsAnySkillFromBuild(const EvCombatDataPersistent &combat_data)
{
    return IsSkillFromBuild_NameBased(combat_data) ||
           IsSkillFromBuild_IdBased(combat_data);
}

std::chrono::steady_clock::time_point UpdateCastTime(
    std::map<std::string, std::chrono::steady_clock::time_point> &last_cast_map,
    const EvCombatDataPersistent &combat_data)
{
    const auto now = std::chrono::steady_clock::now();

    last_cast_map[combat_data.SkillName] = now;

    return now;
}

std::chrono::steady_clock::time_point GetLastCastTime(
    const EvCombatDataPersistent &combat_data)
{
    static auto last_cast_map =
        std::map<std::string, std::chrono::steady_clock::time_point>{};

    const auto it = last_cast_map.find(combat_data.SkillName);
    if (it != last_cast_map.end())
    {
        const auto time = it->second;
        UpdateCastTime(last_cast_map, combat_data);

        return time;
    }

    const auto now = UpdateCastTime(last_cast_map, combat_data);
    return now;
}

bool SKillCastIsTooEarlyWrtRechargeTime(
    const std::chrono::steady_clock::time_point &now,
    const EvCombatDataPersistent &combat_data,
    std::map<uint64_t, std::chrono::steady_clock::time_point>
        &skill_last_cast_times)
{
    auto last_cast_time = skill_last_cast_times[combat_data.SkillID];
    skill_last_cast_times[combat_data.SkillID] = now;

    const auto skill_data_map_it = Globals::RotationRun.skill_data_map.find(
        static_cast<int>(combat_data.SkillID));

    if (skill_data_map_it != Globals::RotationRun.skill_data_map.end())
    {
        auto current_profession = get_current_profession_name();
        auto profession_lower = to_lowercase(current_profession);

        auto is_mesmer_weapon_4 = false;
        auto is_berserker_f1 = false;

        if (profession_lower == "mesmer")
        {
            // TODO: For Chrono - CS reset
            is_mesmer_weapon_4 = RotationRunType::mesmer_weapon_4_skills.count(
                                     combat_data.SkillID) > 0;
        }
        else if (profession_lower == "warrior")
        {
            is_berserker_f1 = RotationRunType::berserker_f1_skills.count(
                                  combat_data.SkillID) > 0;
        }

        if (!is_mesmer_weapon_4 && !is_berserker_f1)
        {
            const auto &skill_data = skill_data_map_it->second;
            const auto recharge_time_s = skill_data.recharge_time;
            const auto recharge_time_w_alac_s =
                static_cast<int>(recharge_time_s * 0.8f);

            const auto cast_time_diff = now - last_cast_time;
            const auto cast_time_diff_s =
                std::chrono::duration_cast<std::chrono::seconds>(cast_time_diff)
                    .count();
            const auto recharge_duration_s =
                std::chrono::seconds(recharge_time_w_alac_s);

            if (cast_time_diff_s < recharge_time_w_alac_s * 0.7 &&
                recharge_time_w_alac_s > 0 &&
                !skill_data.is_auto_attack) // XXX: Hacky
                return true;
        }
    }

    return false;
}

bool IsNotTheSameCast(const EvCombatDataPersistent &combat_data)
{
    static auto skill_last_cast_times =
        std::map<uint64_t, std::chrono::steady_clock::time_point>{};

    const auto now = std::chrono::steady_clock::now();
    const auto last_cast_time = GetLastCastTime(combat_data);
    const auto is_not_same_cast =
        (now - last_cast_time) > std::chrono::milliseconds(MIN_TIME_DIFF);

    if (is_first_cast_map.find(combat_data.SkillName) ==
        is_first_cast_map.end())
    {
        is_first_cast_map[combat_data.SkillName] = true;
        return true;
    }

    if (skill_last_cast_times.find(combat_data.SkillID) !=
        skill_last_cast_times.end())
    {
        if (SKillCastIsTooEarlyWrtRechargeTime(now,
                                               combat_data,
                                               skill_last_cast_times))
            return false;
    }
    else
    {
        skill_last_cast_times[combat_data.SkillID] = now;
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

    auto combat_data = EvCombatData{ev, src, dst, skillname, id, revision};

    if (IsValidCombatEvent(combat_data))
    {
        const auto data = EvCombatDataPersistent{
            .SrcName = std::string(combat_data.src->Name),
            .SrcID = combat_data.src->ID,
            .SrcProfession = combat_data.src->Profession,
            .SrcSpecialization = combat_data.src->Specialization,
            .SkillName = std::string(combat_data.skillname),
            .SkillID = combat_data.ev->SkillID,
            .EventID = combat_data.id,
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
