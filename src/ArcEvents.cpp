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
#include "SkillData.h"
#include "Types.h"
#include "TypesUtils.h"

namespace
{
constexpr static auto MIN_TIME_DIFF = 10U;

static auto is_first_cast_map = std::map<SkillID, bool>{};

bool IsValidSelfData(const EvCombatData &combat_data)
{
    return combat_data.src->IsSelf && combat_data.src != nullptr && combat_data.skillname != nullptr &&
           combat_data.ev != nullptr && combat_data.src->Name != nullptr;
}

bool IsValidCombatEvent(const EvCombatData &combat_data)
{
    const auto is_valid_self = IsValidSelfData(combat_data);

    return is_valid_self;
}

bool IsValidSkillID(const EvCombatDataPersistent &combat_data)
{
    const auto &skill_data_map = Globals::RotationRun.skill_data_map;
    const auto found = skill_data_map.find(combat_data.SkillID) != skill_data_map.end();
    return found;
}

bool IsNoCondiTick(const EvCombatDataPersistent &combat_data)
{
    static const auto condi_names = std::set<std::string>{
        "Bleeding",
        "Burning",
        "Torment",
        "Confusion",
        "Poisened",
    };

    const auto not_found = condi_names.find(combat_data.SkillName) == condi_names.end();
    return not_found;
}

bool IsAnySkillFromBuild(const EvCombatDataPersistent &combat_data)
{
    if (SkillRuleData::skills_to_not_track.find(combat_data.SkillID) != SkillRuleData::skills_to_not_track.end())
        return false;

    return IsValidSkillID(combat_data) && IsNoCondiTick(combat_data);
}

std::chrono::steady_clock::time_point GetLastCastTime(const std::chrono::steady_clock::time_point &now,
                                                      const EvCombatDataPersistent &combat_data)
{
    const auto it = Globals::SkillLastTimeCast.find(combat_data.SkillID);
    if (it != Globals::SkillLastTimeCast.end())
    {
        return it->second;
    }

    return now;
}

bool IsMultiHitSkill(const std::chrono::steady_clock::time_point &now, const EvCombatDataPersistent &combat_data)
{
    auto last_cast_time = Globals::SkillLastTimeCast[combat_data.SkillID];

    const auto skill_data_map_it = Globals::RotationRun.skill_data_map.find(combat_data.SkillID);
    if (skill_data_map_it == Globals::RotationRun.skill_data_map.end())
        return false;

    const auto &skill_data = skill_data_map_it->second;
    const auto recharge_time_w_alac_s = skill_data.recharge_time_with_alacrity;
    const auto cast_time_w_quick_s = skill_data.cast_time_with_quickness;

    const auto cast_time_diff = now - last_cast_time;
    const auto cast_time_diff_s = std::chrono::duration<float>(cast_time_diff).count();
    const auto recharge_duration_s = std::chrono::seconds(static_cast<int>(recharge_time_w_alac_s));

    const auto is_same_alac_based =
        recharge_time_w_alac_s > 0 ? cast_time_diff_s < recharge_time_w_alac_s * 0.90 : false;
    const auto is_same_quick_based = cast_time_w_quick_s > 0 ? cast_time_diff_s < cast_time_w_quick_s * 0.75 : false;

    const auto is_reset_like_skill =
        SkillRuleData::reset_like_skill.find(combat_data.SkillID) != SkillRuleData::reset_like_skill.end();
    const auto is_profession_reset_like_skill = SkillRuleData::IsProfessionResetLikeSKill(combat_data.SkillID);

    if (is_profession_reset_like_skill || is_reset_like_skill)
    {
        if (is_same_quick_based)
        {
            Globals::IsSameCast = true;
            return true;
        }
        else
        {
            Globals::IsSameCast = false;
            Globals::SkillLastTimeCast[combat_data.SkillID] = now;
            return false;
        }
    }

    if (is_same_alac_based || is_same_quick_based)
    {
        Globals::IsSameCast = true;
        return true;
    }
    else
    {
        Globals::IsSameCast = false;
    }

    Globals::SkillLastTimeCast[combat_data.SkillID] = now;
    return false;
}

bool IsSameCast(const EvCombatDataPersistent &combat_data)
{
    const auto now = std::chrono::steady_clock::now();

    if (is_first_cast_map.find(combat_data.SkillID) == is_first_cast_map.end())
    {
        is_first_cast_map[combat_data.SkillID] = true;
        return false;
    }

    if (Globals::SkillLastTimeCast.find(combat_data.SkillID) != Globals::SkillLastTimeCast.end())
    {
        if (IsMultiHitSkill(now, combat_data))
            return true;
        else
            Globals::SkillLastTimeCast[combat_data.SkillID] = now;
    }

    Globals::SkillLastTimeCast[combat_data.SkillID] = now;
    return false;
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

    if (channel == nullptr || ev == nullptr || src == nullptr || dst == nullptr || skillname == nullptr)
        return false;

    auto combat_data = EvCombatData{ev, src, dst, skillname, id, revision};

    if (!IsValidCombatEvent(combat_data))
        return false;

    const auto data = EvCombatDataPersistent{
        .SrcName = std::string(combat_data.src->Name),
        .SrcID = combat_data.src->ID,
        .SrcProfession = combat_data.src->Profession,
        .SrcSpecialization = combat_data.src->Specialization,
        .SkillName = std::string(combat_data.skillname),
        .SkillID = SafeConvertToSkillID(combat_data.ev->SkillID),
        .EventID = combat_data.id,
    };

    if (data.SkillID == SkillID::UNKNOWN_SKILL || !IsAnySkillFromBuild(data))
        return false;

    Globals::LastArcEventSkillName = data.SkillName;

    if (IsSameCast(data))
        return false;

    Globals::Render.skill_activation_callback(data);

    return true;
}
} // namespace ArcEv
