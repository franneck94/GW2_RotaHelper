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
bool IsSkillAutoAttack(const SkillID skill_id, const std::string &skill_name, const SkillDataMap &skill_data_map)
{
    auto it = skill_data_map.find(skill_id);

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

bool IsOtherValidAutoAttack(const RotationStep &n_th_future_rota_skill,
                            const EvCombatDataPersistent &curr_actual_casted_skill,
                            const RotationLogType &rotation_run)
{
    const auto future_is_auto_attack = n_th_future_rota_skill.skill_data.is_auto_attack;
    const auto is_not_special_cast_time_skill =
        SkillRuleData::skill_cast_time_map.find(n_th_future_rota_skill.skill_data.skill_id) ==
        SkillRuleData::skill_cast_time_map.end();
    const auto actual_casted_skill_is_auto_attack = IsSkillAutoAttack(curr_actual_casted_skill.SkillID,
                                                                      curr_actual_casted_skill.SkillName,
                                                                      rotation_run.skill_data_map);

    const auto n_th_future_skill_weapon_type = n_th_future_rota_skill.skill_data.weapon_type;
    const auto actual_skill_data =
        SkillRuleData::GetDataByID(curr_actual_casted_skill.SkillID, rotation_run.skill_data_map);
    const auto actual_casted_skill_weapon_type = actual_skill_data.weapon_type;
    const auto same_weapon_type = n_th_future_skill_weapon_type == actual_casted_skill_weapon_type;

    return future_is_auto_attack && is_not_special_cast_time_skill && actual_casted_skill_is_auto_attack &&
           same_weapon_type;
}

bool IsSpecialMappingSkill(const EvCombatDataPersistent &curr_actual_casted_skill,
                           const RotationStep &n_th_future_rota_skill)
{
    if (SkillRuleData::special_mapping_skills.find(curr_actual_casted_skill.SkillID) !=
        SkillRuleData::special_mapping_skills.end())
    {
        const auto maped_skill_id = SkillRuleData::special_mapping_skills.at(curr_actual_casted_skill.SkillID);
        if (maped_skill_id == n_th_future_rota_skill.skill_data.skill_id)
            return true;
    }

    if (SkillRuleData ::special_mapping_skills.find(n_th_future_rota_skill.skill_data.skill_id) !=
        SkillRuleData::special_mapping_skills.end())
    {
        const auto maped_skill_id =
            SkillRuleData::special_mapping_skills.at(n_th_future_rota_skill.skill_data.skill_id);
        if (maped_skill_id == curr_actual_casted_skill.SkillID)
            return true;
    }

    return false;
}

bool CheckTheNextNskills(const EvCombatDataPersistent &curr_actual_casted_skill,
                         const RotationStep &n_th_future_rota_skill,
                         const uint32_t window_length,
                         RotationLogType &rotation_run)
{
    const auto is_special_mapping = IsSpecialMappingSkill(curr_actual_casted_skill, n_th_future_rota_skill);

    const auto is_match =
        (((n_th_future_rota_skill.skill_data.name == curr_actual_casted_skill.SkillName) || is_special_mapping));

    const auto is_any_other_auto_attack =
        !is_match && IsOtherValidAutoAttack(n_th_future_rota_skill, curr_actual_casted_skill, rotation_run);

    if (is_match || is_any_other_auto_attack)
    {
        for (uint32_t i = 0; i < window_length; ++i)
            rotation_run.missing_rotation_steps.pop_front();
    }

    return is_match;
}

void ResetSkillDetectionData(SkillDetectionTimers &timers, uint32_t &num_skills_wo_match)
{
    const auto now = std::chrono::steady_clock::now();
    timers.time_of_last_pop = now;
    timers.time_of_last_next_skill_check = now;
    timers.time_of_last_next_next_skill_check = now;
    timers.time_of_last_next_next_next_skill_check = now;
    num_skills_wo_match = 0U;
}

float GetTimeSinceInSeconds(const std::chrono::steady_clock::time_point &t0)
{
    const auto now = std::chrono::steady_clock::now();
    const auto time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - t0).count();
    return static_cast<float>(time_ms) / 1000.0f;
}

RotaSkillWindow GetRotaSkillWindow(RotationLogType &rotation_run, SkillDetectionTimers &timers)
{
    const auto now = std::chrono::steady_clock::now();
    const auto time_since_last_pop_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - timers.time_of_last_pop).count();

    auto it = rotation_run.missing_rotation_steps.begin();
    auto rota_window = RotaSkillWindow{};

    if (rotation_run.missing_rotation_steps.size() > 1)
    {
        rota_window.curr_rota_skill = *it;

        const auto skill_id = rota_window.curr_rota_skill.skill_data.skill_id;
        auto cast_time_ms = 150.0F;
        if (SkillRuleData::grey_skill_cast_time_map.find(skill_id) != SkillRuleData::grey_skill_cast_time_map.end())
        {
            // wait for 75% of cast time of greyed out skill until next is "current"
            cast_time_ms = max(cast_time_ms, SkillRuleData::grey_skill_cast_time_map.at(skill_id) * 1000.0f * 0.75f);
        }

        // FIX: time_since_last_pop_ms makes only sense when there was already a pop, we should rather reset the timer when a valid skill (non gray) was executed
        const auto can_pop = (time_since_last_pop_ms > cast_time_ms);
        if (rota_window.curr_rota_skill.is_special_skill && can_pop)
        {
            rotation_run.missing_rotation_steps.pop_front();
            it = rotation_run.missing_rotation_steps.begin();
            rota_window.curr_rota_skill = *it;

            timers.time_of_last_pop = std::chrono::steady_clock::now();
        }

        ++it;
    }

    if (rotation_run.missing_rotation_steps.size() > 2)
    {
        rota_window.next_rota_skill = *it;
        ++it;
    }
    if (rotation_run.missing_rotation_steps.size() > 3)
    {
        rota_window.next_next_rota_skill = *it;
        ++it;
    }
    if (rotation_run.missing_rotation_steps.size() > 4)
    {
        rota_window.next_next_next_rota_skill = *it;
        ++it;
    }

    constexpr static auto min_time_for_next_s = 0.3f;
    constexpr static auto min_time_for_next_next_s = 0.6f;
    constexpr static auto min_time_for_next_next_next_s = 1.2f;

    const auto time_since_last_next_skill_check = GetTimeSinceInSeconds(timers.time_of_last_next_skill_check);
    const auto time_since_last_next_next_skill_check = GetTimeSinceInSeconds(timers.time_of_last_next_next_skill_check);
    const auto time_since_last_next_next_next_skill_check =
        GetTimeSinceInSeconds(timers.time_of_last_next_next_next_skill_check);
    const auto time_since_aa_skip = GetTimeSinceInSeconds(timers.time_of_last_aa_skip);

    rota_window.check_for_next_skill =
        (time_since_aa_skip > 3 || !rota_window.next_rota_skill.skill_data.is_auto_attack) &&
        (time_since_last_next_skill_check > min_time_for_next_s || timers.is_first_check_for_next);

    if (!rota_window.check_for_next_skill)
        return rota_window;

    rota_window.check_for_next_next_skill =
        ((rota_window.next_next_rota_skill.is_special_skill ||
          !rota_window.next_next_rota_skill.skill_data.is_auto_attack) &&
         (time_since_last_next_next_skill_check > min_time_for_next_next_s || timers.is_first_check_for_next_next));

    if (!rota_window.check_for_next_next_skill)
        return rota_window;

    rota_window.check_for_next_next_next_skill =
        ((rota_window.next_next_rota_skill.is_special_skill ||
          !rota_window.next_next_next_rota_skill.skill_data.is_auto_attack) &&
         (time_since_last_next_next_next_skill_check > min_time_for_next_next_next_s ||
          timers.is_first_check_for_next_next_next));

    return rota_window;
}
} // namespace

void SkillDetectionLogic(uint32_t &num_skills_wo_match,
                         std::chrono::steady_clock::time_point &time_since_last_match,
                         RotationLogType &rotation_run,
                         const EvCombatDataPersistent &curr_actual_casted_skill)
{
    static auto timers = SkillDetectionTimers{};

    const auto duration_since_last_match = GetTimeSinceInSeconds(time_since_last_match);
    const auto curr_casted_is_auto_attack = IsSkillAutoAttack(curr_actual_casted_skill.SkillID,
                                                              curr_actual_casted_skill.SkillName,
                                                              Globals::RotationRun.skill_data_map);

    if (num_skills_wo_match == 0)
        time_since_last_match = std::chrono::steady_clock::now();

    auto rota_window = GetRotaSkillWindow(rotation_run, timers);

    if (CheckTheNextNskills(curr_actual_casted_skill, rota_window.curr_rota_skill, 1, rotation_run))
    {
        ResetSkillDetectionData(timers, num_skills_wo_match);

        timers.is_first_check_for_next = false;
        return;
    }

    const auto still_look_into_in_strict_mode =
        ((Settings::StrictModeForSkillDetection && rota_window.curr_rota_skill.is_special_skill) ||
         !Settings::StrictModeForSkillDetection);

    if (still_look_into_in_strict_mode && !curr_casted_is_auto_attack)
    {
        const auto curr_actual_casted_is_profession_reset_like_skill =
            SkillRuleData::IsProfessionResetLikeSKill(curr_actual_casted_skill.SkillID);

        if (curr_actual_casted_is_profession_reset_like_skill)
            return;

        if (rota_window.check_for_next_skill &&
            CheckTheNextNskills(curr_actual_casted_skill, rota_window.next_rota_skill, 2, rotation_run))
        {
            ResetSkillDetectionData(timers, num_skills_wo_match);

            if (rota_window.next_rota_skill.skill_data.is_auto_attack &&
                rota_window.next_next_rota_skill.skill_data.is_auto_attack)
                timers.time_of_last_aa_skip = std::chrono::steady_clock::now();

            timers.is_first_check_for_next_next = false;
            return;
        }

        if (rota_window.check_for_next_next_skill &&
            CheckTheNextNskills(curr_actual_casted_skill, rota_window.next_next_rota_skill, 3, rotation_run))
        {
            ResetSkillDetectionData(timers, num_skills_wo_match);

            timers.is_first_check_for_next_next = false;
            return;
        }

        if (rota_window.check_for_next_next_next_skill &&
            CheckTheNextNskills(curr_actual_casted_skill, rota_window.next_next_next_rota_skill, 4, rotation_run))
        {
            ResetSkillDetectionData(timers, num_skills_wo_match);

            timers.is_first_check_for_next_next_next = false;
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
            const auto is_exact_match = (rota_skill.skill_data.name == curr_actual_casted_skill.SkillName);
            const auto is_auto_attack_match = !is_exact_match && (rota_skill.skill_data.is_auto_attack &&
                                                                  IsSkillAutoAttack(curr_actual_casted_skill.SkillID,
                                                                                    curr_actual_casted_skill.SkillName,
                                                                                    rotation_run.skill_data_map));

            if (is_exact_match || is_auto_attack_match)
            {
                while (rotation_run.missing_rotation_steps.begin() != it)
                    rotation_run.missing_rotation_steps.pop_front();

                rotation_run.missing_rotation_steps.pop_front();

                num_skills_wo_match = 0U;
                time_since_last_match = std::chrono::steady_clock::now();
                return;
            }
        }
    }
}
