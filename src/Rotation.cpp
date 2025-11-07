#include <cstdint>
#include <string>

#include "LogData.h"
#include "Rotation.h"
#include "Settings.h"
#include "Shared.h"
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

bool CheckTheNextNskills(const EvCombatDataPersistent &skill_ev,
                         const RotationStep &future_rota_skill,
                         const uint32_t n,
                         const bool is_okay,
                         RotationLogType &rotation_run,
                         EvCombatDataPersistent &last_skill)
{
    auto is_match = ((future_rota_skill.skill_data.name == skill_ev.SkillName) && is_okay);

    if (is_match)
    {
        for (uint32_t i = 0; i < n; ++i)
            rotation_run.todo_rotation_steps.pop_front();

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
    auto it = rotation_run.todo_rotation_steps.begin();
    RotationStep curr_rota_skill;
    RotationStep next_rota_skill;
    RotationStep next_next_rota_skill;
    RotationStep next_next_next_rota_skill;

    if (rotation_run.todo_rotation_steps.size() > 1)
    {
        curr_rota_skill = *it;

        while (curr_rota_skill.is_special_skill && rotation_run.todo_rotation_steps.size() > 2)
        {
            rotation_run.todo_rotation_steps.pop_front();
            it = rotation_run.todo_rotation_steps.begin();
            curr_rota_skill = *it;
        }

        ++it;
    }
    if (rotation_run.todo_rotation_steps.size() > 2)
    {
        next_rota_skill = *it;
        ++it;
    }
    if (rotation_run.todo_rotation_steps.size() > 3)
    {
        next_next_rota_skill = *it;
        ++it;
    }
    if (rotation_run.todo_rotation_steps.size() > 4)
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


    const auto check_for_next_skill = (time_span_since_aa_skip > 3 || !next_rota_skill.skill_data.is_auto_attack) &&
                                      time_since_last_next_skill_check > min_time_for_next_s;
    const auto check_for_next_next_skill =
        (next_next_rota_skill.is_special_skill || !next_next_rota_skill.skill_data.is_auto_attack) &&
        time_since_last_next_next_skill_check > min_time_for_next_next_s;
    const auto check_for_next_next_next_skill =
        (next_next_rota_skill.is_special_skill || !next_next_next_rota_skill.skill_data.is_auto_attack) &&
        time_since_last_next_next_next_skill_check > min_time_for_next_next_next_s;

    if (CheckTheNextNskills(skill_ev, curr_rota_skill, 1, true, rotation_run, last_skill))
    {
        ResetSkillDetectionData(time_of_last_next_skill_check,
                                time_of_last_next_next_skill_check,
                                time_of_last_next_next_next_skill_check,
                                num_skills_wo_match);
        return;
    }

    if (!Settings::StrictModeForSkillDetection)
    {

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

            return;
        }

        if (!curr_is_auto_attack &&
            CheckTheNextNskills(skill_ev, next_next_rota_skill, 3, check_for_next_next_skill, rotation_run, last_skill))
        {
            ResetSkillDetectionData(time_of_last_next_skill_check,
                                    time_of_last_next_next_skill_check,
                                    time_of_last_next_next_next_skill_check,
                                    num_skills_wo_match);
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
            return;
        }
    }

    if (!curr_is_auto_attack)
        ++num_skills_wo_match;

    if (num_skills_wo_match > 5)
    {
        if (curr_rota_skill.skill_data.is_auto_attack || curr_is_auto_attack)
            return;

        if (duration_since_last_match < 10)
            return;

        for (auto it = rotation_run.todo_rotation_steps.begin(); it != rotation_run.todo_rotation_steps.end(); ++it)
        {
            const auto diff = std::distance(it, rotation_run.todo_rotation_steps.begin());
            if (diff > 6)
                return;

            const auto rota_skill = *it;
            if (rota_skill.skill_data.name == skill_ev.SkillName)
            {
                while (rotation_run.todo_rotation_steps.begin() != it)
                    rotation_run.todo_rotation_steps.pop_front();

                rotation_run.todo_rotation_steps.pop_front();

                last_skill = skill_ev;
                num_skills_wo_match = 0U;
                time_since_last_match = now;
                return;
            }
        }
    }
}
