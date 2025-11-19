#include <cstdint>
#include <string>

#include "LogData.h"
#include "MumbleUtils.h"
#include "Rotation.h"
#include "Settings.h"
#include "Shared.h"
#include "SkillData.h"
#include "Types.h"
#include "TypesUtils.h"

namespace
{
bool IsSkillAutoAttack(const uint64_t skill_id, const std::string &skill_name, const SkillDataMap &skill_data_map)
{
    auto it = skill_data_map.find(static_cast<int>(skill_id));

    if (it != skill_data_map.end())
        return it->second.is_auto_attack;

    for (const auto &[skill_id, skill_data] : skill_data_map)
    {
        if (skill_data.name == skill_name)
        {
            return skill_data.is_auto_attack;
        }
    }

    return false;
}

bool IsOtherValidAutoAttack(const RotationStep &future_rota_skill,
                            const EvCombatDataPersistent &skill_ev,
                            const RotationLogType &rotation_run)
{
    const auto future_is_aa = future_rota_skill.skill_data.is_auto_attack;
    const auto is_not_special_cast_time_skill =
        SkillRuleData::skill_cast_time_map.find(future_rota_skill.skill_data.name) ==
        SkillRuleData::skill_cast_time_map.end();
    const auto user_skill_is_aa = IsSkillAutoAttack(skill_ev.SkillID, skill_ev.SkillName, rotation_run.skill_data_map);

    return future_is_aa && is_not_special_cast_time_skill && user_skill_is_aa;
}

bool IsSpecialMappingSkill(const EvCombatDataPersistent &skill_ev, const RotationStep &future_rota_skill)
{
    if (SkillRuleData::special_mapping_skills.find(skill_ev.SkillName) != SkillRuleData::special_mapping_skills.end())
    {
        const auto mapped_name = SkillRuleData::special_mapping_skills.at(skill_ev.SkillName);
        if (mapped_name == future_rota_skill.skill_data.name)
            return true;
    }

    if (SkillRuleData ::special_mapping_skills.find(future_rota_skill.skill_data.name) !=
        SkillRuleData::special_mapping_skills.end())
    {
        const auto mapped_name = SkillRuleData::special_mapping_skills.at(future_rota_skill.skill_data.name);
        if (mapped_name == skill_ev.SkillName)
            return true;
    }

    return false;
}

bool CheckTheNextNskills(const EvCombatDataPersistent &skill_ev,
                         const RotationStep &future_rota_skill,
                         const uint32_t n,
                         const bool is_okay,
                         RotationLogType &rotation_run,
                         EvCombatDataPersistent &last_skill)
{
    const auto is_special_mapping = IsSpecialMappingSkill(skill_ev, future_rota_skill);
    const auto is_match =
        (((future_rota_skill.skill_data.name == skill_ev.SkillName) || is_special_mapping) && is_okay);
    const auto is_any_other_aa = !is_match && IsOtherValidAutoAttack(future_rota_skill, skill_ev, rotation_run);

    if (is_match || is_any_other_aa)
    {
        for (uint32_t i = 0; i < n; ++i)
            rotation_run.missing_rotation_steps.pop_front();

        last_skill = skill_ev;
    }

    return is_match;
}


void ResetSkillDetectionData(std::chrono::steady_clock::time_point &time_of_last_next_skill_check,
                             std::chrono::steady_clock::time_point &time_of_last_next_next_skill_check,
                             std::chrono::steady_clock::time_point &time_of_last_next_next_next_skill_check,
                             uint32_t &num_skills_wo_match)
{
    time_of_last_next_skill_check = std::chrono::steady_clock::now();
    time_of_last_next_next_skill_check = std::chrono::steady_clock::now();
    time_of_last_next_next_next_skill_check = std::chrono::steady_clock::now();
    num_skills_wo_match = 0U;
}

std::tuple<RotationStep, RotationStep, RotationStep, RotationStep> GetCurrAndNextRotaSkills(
    RotationLogType &rotation_run)
{
    static auto first_time_pop = true;
    static auto last_time_pop = std::chrono::steady_clock::now();
    const auto now = std::chrono::steady_clock::now();
    const auto time_since_last_pop = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_time_pop).count();

    auto it = rotation_run.missing_rotation_steps.begin();
    auto curr_rota_skill = RotationStep{};
    auto next_rota_skill = RotationStep{};
    auto next_next_rota_skill = RotationStep{};
    auto next_next_next_rota_skill = RotationStep{};

    if (rotation_run.missing_rotation_steps.size() > 1)
    {
        curr_rota_skill = *it;

        if (curr_rota_skill.is_special_skill && (time_since_last_pop > 150 || first_time_pop))
        {
            rotation_run.missing_rotation_steps.pop_front();
            it = rotation_run.missing_rotation_steps.begin();
            curr_rota_skill = *it;

            last_time_pop = std::chrono::steady_clock::now();
            first_time_pop = false;
        }

        ++it;
    }

    if (rotation_run.missing_rotation_steps.size() > 2)
    {
        next_rota_skill = *it;
        ++it;
    }
    if (rotation_run.missing_rotation_steps.size() > 3)
    {
        next_next_rota_skill = *it;
        ++it;
    }
    if (rotation_run.missing_rotation_steps.size() > 4)
    {
        next_next_next_rota_skill = *it;
        ++it;
    }

    return std::make_tuple(curr_rota_skill, next_rota_skill, next_next_rota_skill, next_next_next_rota_skill);
}

float GetTimeSinceInSeconds(const std::chrono::steady_clock::time_point &t0,
                            const std::chrono::steady_clock::time_point &now)
{
    const auto time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - t0).count();
    return static_cast<float>(time_ms) / 1000.0f;
}
} // namespace


void SkillDetectionLogic(uint32_t &num_skills_wo_match,
                         std::chrono::steady_clock::time_point &time_since_last_match,
                         RotationLogType &rotation_run,
                         const EvCombatDataPersistent &skill_ev,
                         EvCombatDataPersistent &last_skill)
{
    constexpr static auto min_time_for_next_s = 0.3f;
    constexpr static auto min_time_for_next_next_s = 0.6f;
    constexpr static auto min_time_for_next_next_next_s = 1.2f;

    static auto is_first_check_for_next = true;
    static auto is_first_check_for_next_next = true;
    static auto is_first_check_for_next_next_next = true;

    static auto time_of_last_next_skill_check = std::chrono::steady_clock::now();
    static auto time_of_last_next_next_skill_check = std::chrono::steady_clock::now();
    static auto time_of_last_next_next_next_skill_check = std::chrono::steady_clock::now();
    static auto time_of_last_aa_skip = std::chrono::steady_clock::now();

    const auto now = std::chrono::steady_clock::now();
    const auto time_since_last_next_skill_check = GetTimeSinceInSeconds(time_of_last_next_skill_check, now);
    const auto time_since_last_next_next_skill_check = GetTimeSinceInSeconds(time_of_last_next_next_skill_check, now);
    const auto time_since_last_next_next_next_skill_check =
        GetTimeSinceInSeconds(time_of_last_next_next_next_skill_check, now);
    const auto time_span_since_aa_skip = GetTimeSinceInSeconds(time_of_last_aa_skip, now);
    const auto duration_since_last_match = GetTimeSinceInSeconds(time_since_last_match, now);

    const auto curr_is_auto_attack =
        IsSkillAutoAttack(skill_ev.SkillID, skill_ev.SkillName, Globals::RotationRun.skill_data_map);

    if (num_skills_wo_match == 0)
        time_since_last_match = std::chrono::steady_clock::now();

    auto [curr_rota_skill, next_rota_skill, next_next_rota_skill, next_next_next_rota_skill] =
        GetCurrAndNextRotaSkills(rotation_run);

    const auto check_for_next_skill =
        (time_span_since_aa_skip > 3 || !next_rota_skill.skill_data.is_auto_attack) &&
        (time_since_last_next_skill_check > min_time_for_next_s || is_first_check_for_next);
    const auto check_for_next_next_skill =
        check_for_next_skill &&
        ((next_next_rota_skill.is_special_skill || !next_next_rota_skill.skill_data.is_auto_attack) &&
         (time_since_last_next_next_skill_check > min_time_for_next_next_s || is_first_check_for_next_next));
    const auto check_for_next_next_next_skill =
        check_for_next_next_skill &
        ((next_next_rota_skill.is_special_skill || !next_next_next_rota_skill.skill_data.is_auto_attack) &&
         (time_since_last_next_next_next_skill_check > min_time_for_next_next_next_s ||
          is_first_check_for_next_next_next));

    if (CheckTheNextNskills(skill_ev, curr_rota_skill, 1, true, rotation_run, last_skill))
    {
        ResetSkillDetectionData(time_of_last_next_skill_check,
                                time_of_last_next_next_skill_check,
                                time_of_last_next_next_next_skill_check,
                                num_skills_wo_match);

        is_first_check_for_next = false;
        return;
    }

    if (!Settings::StrictModeForSkillDetection)
    {
        auto current_profession = get_current_profession_name();
        auto profession_lower = to_lowercase(current_profession);

        auto is_mesmer_weapon_4 = false;
        auto is_berserker_f1 = false;

        // TODO: For Chrono - CS reset
        if (profession_lower == "mesmer")
            is_mesmer_weapon_4 = SkillRuleData::mesmer_weapon_4_skills.count(skill_ev.SkillID) > 0;
        else if (profession_lower == "warrior")
            is_berserker_f1 = SkillRuleData::berserker_f1_skills.count(skill_ev.SkillID) > 0;

        if (is_mesmer_weapon_4 || is_berserker_f1)
            return;

        if (!curr_is_auto_attack && check_for_next_skill &&
            CheckTheNextNskills(skill_ev, next_rota_skill, 2, true, rotation_run, last_skill))
        {
            ResetSkillDetectionData(time_of_last_next_skill_check,
                                    time_of_last_next_next_skill_check,
                                    time_of_last_next_next_next_skill_check,
                                    num_skills_wo_match);

            if (next_rota_skill.skill_data.is_auto_attack && next_next_rota_skill.skill_data.is_auto_attack)
            {
                time_of_last_aa_skip = std::chrono::steady_clock::now();
            }

            is_first_check_for_next_next = false;
            return;
        }

        if (!curr_is_auto_attack &&
            CheckTheNextNskills(skill_ev, next_next_rota_skill, 3, check_for_next_next_skill, rotation_run, last_skill))
        {
            ResetSkillDetectionData(time_of_last_next_skill_check,
                                    time_of_last_next_next_skill_check,
                                    time_of_last_next_next_next_skill_check,
                                    num_skills_wo_match);

            is_first_check_for_next_next = false;
            return;
        }

        if (!curr_is_auto_attack && CheckTheNextNskills(skill_ev,
                                                        next_next_next_rota_skill,
                                                        4,
                                                        check_for_next_next_next_skill,
                                                        rotation_run,
                                                        last_skill))
        {
            ResetSkillDetectionData(time_of_last_next_skill_check,
                                    time_of_last_next_next_skill_check,
                                    time_of_last_next_next_next_skill_check,
                                    num_skills_wo_match);

            is_first_check_for_next_next_next = false;
            return;
        }
    }

    ++num_skills_wo_match;

    if (num_skills_wo_match > 6)
    {
        if (duration_since_last_match < 4)
            return;

        for (auto it = rotation_run.missing_rotation_steps.begin(); it != rotation_run.missing_rotation_steps.end();
             ++it)
        {
            const auto diff = std::distance(rotation_run.missing_rotation_steps.begin(), it);
            if (diff > 6)
                return;

            const auto rota_skill = *it;
            const auto is_exact_match = (rota_skill.skill_data.name == skill_ev.SkillName);
            const auto is_auto_attack_match =
                !is_exact_match &&
                (rota_skill.skill_data.is_auto_attack &&
                 IsSkillAutoAttack(skill_ev.SkillID, skill_ev.SkillName, rotation_run.skill_data_map));

            if (is_exact_match || is_auto_attack_match)
            {
                while (rotation_run.missing_rotation_steps.begin() != it)
                    rotation_run.missing_rotation_steps.pop_front();

                rotation_run.missing_rotation_steps.pop_front();

                last_skill = skill_ev;
                num_skills_wo_match = 0U;
                time_since_last_match = now;
                return;
            }
        }
    }
}
