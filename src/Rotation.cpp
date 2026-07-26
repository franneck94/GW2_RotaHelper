#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

#include "ArcEvents.h"
#include "LogData.h"
#include "KeyboardCapture.h"
#include "MumbleUtils.h"
#include "Rotation.h"
#include "Settings.h"
#include "Shared.h"
#include "SkillData.h"
#include "Types.h"
#include "TypesUtils.h"

namespace
{
bool IsSkillAutoAttack(const SkillID queried_skill_id, const std::string &skill_name, const SkillDataMap &skill_data_map)
{
    auto it = skill_data_map.find(queried_skill_id);

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
                            const EvCombatDataPersistent &current_casted_skill,
                            const RotationLogType &rotation_run)
{
    const auto future_is_auto_attack = n_th_future_rota_skill.skill_data.is_auto_attack;
    if (!future_is_auto_attack)
        return false;

    const auto actual_casted_skill_is_auto_attack =
        IsSkillAutoAttack(current_casted_skill.SkillID, current_casted_skill.SkillName, rotation_run.skill_data_map);

    const auto n_th_future_skill_weapon_type = n_th_future_rota_skill.skill_data.weapon_type;
    const auto actual_skill_data =
        SkillRuleData::GetDataByID(current_casted_skill.SkillID, rotation_run.skill_data_map);
    const auto actual_casted_skill_weapon_type = actual_skill_data.weapon_type;
    const auto same_weapon_type = n_th_future_skill_weapon_type == actual_casted_skill_weapon_type;

    return future_is_auto_attack && actual_casted_skill_is_auto_attack && same_weapon_type;
}

bool IsSpecialMappingSkill(const EvCombatDataPersistent &current_casted_skill,
                           const RotationStep &n_th_future_rota_skill)
{
    const auto expected_id = n_th_future_rota_skill.skill_data.skill_id;
    const auto expected_mapping = SkillRuleData::special_mapping_skills.find(expected_id);
    if (expected_mapping != SkillRuleData::special_mapping_skills.end() &&
        expected_mapping->second == current_casted_skill.SkillID)
        return true;

    const auto actual_mapping = SkillRuleData::special_mapping_skills.find(current_casted_skill.SkillID);
    if (actual_mapping != SkillRuleData::special_mapping_skills.end() && actual_mapping->second == expected_id)
        return true;

    return false;
}

bool CheckTheNextNskills(const EvCombatDataPersistent &current_casted_skill,
                         const RotationStep &n_th_future_rota_skill,
                         const uint32_t window_length,
                         const bool accept_other_aa,
                         RotationLogType &rotation_run)
{
    const auto is_special_mapping = IsSpecialMappingSkill(current_casted_skill, n_th_future_rota_skill);

    const auto is_match =
        (((n_th_future_rota_skill.skill_data.skill_id == current_casted_skill.SkillID) || is_special_mapping));

    const auto is_any_other_auto_attack =
        !is_match && accept_other_aa &&
        IsOtherValidAutoAttack(n_th_future_rota_skill, current_casted_skill, rotation_run);

    if (is_match || is_any_other_auto_attack)
    {
        const auto pop_count = std::min<size_t>(window_length, rotation_run.missing_rotation_steps.size());
        for (size_t i = 0; i < pop_count; ++i)
            rotation_run.missing_rotation_steps.pop_front();
    }

    return is_match || is_any_other_auto_attack;
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

bool TryAdvanceTimedGreySkill(RotationLogType &rotation_run, SkillDetectionTimers &timers)
{
    if (rotation_run.missing_rotation_steps.empty())
        return false;

    const auto &current_step = rotation_run.missing_rotation_steps.front();
    auto cast_time_ms = 150.0F;
    if (current_step.skill_data.cast_time_with_quickness > 0)
        cast_time_ms = std::max(cast_time_ms, current_step.skill_data.cast_time_with_quickness * 1000.0f);

    const auto now = std::chrono::steady_clock::now();
    const auto time_since_last_pop_ms =
        std::chrono::duration<float, std::milli>(now - timers.time_of_last_pop).count();
    if (!IsRotationStepGreyedOut(current_step, rotation_run) || time_since_last_pop_ms <= cast_time_ms)
        return false;

    rotation_run.missing_rotation_steps.pop_front();
    timers.time_of_last_pop = now;
    return true;
}

RotaSkillWindow _GetRotaWindowFromRotationSteps(RotationLogType &rotation_run, SkillDetectionTimers &timers)
{
    auto rota_window = RotaSkillWindow{};

    if (rotation_run.missing_rotation_steps.empty())
        return rota_window;

    (void)TryAdvanceTimedGreySkill(rotation_run, timers);
    if (rotation_run.missing_rotation_steps.empty())
        return rota_window;

    auto it = rotation_run.missing_rotation_steps.begin();
    rota_window.curr_rota_skill = *it;

    it = rotation_run.missing_rotation_steps.begin();
    ++it;
    if (it != rotation_run.missing_rotation_steps.end())
    {
        rota_window.next_rota_skill = *it;
        ++it;
    }
    if (it != rotation_run.missing_rotation_steps.end())
    {
        rota_window.next_next_rota_skill = *it;
        ++it;
    }
    if (it != rotation_run.missing_rotation_steps.end())
    {
        rota_window.next_next_next_rota_skill = *it;
    }

    return rota_window;
}

// t+1 future skill
bool DoCheckForNextSkill(const RotaSkillWindow &rota_window, const SkillDetectionTimers &timers)
{
    constexpr static auto min_time_for_next_s = 0.3f;
    const auto time_since_aa_skip_s = GetTimeSinceInSeconds(timers.time_of_last_aa_skip);
    const auto time_since_last_next_skill_check_s = GetTimeSinceInSeconds(timers.time_of_last_next_skill_check);

    const auto is_valid_for_aa = (time_since_aa_skip_s > 3 || !rota_window.next_rota_skill.skill_data.is_auto_attack);
    const auto is_valid_overall =
        (time_since_last_next_skill_check_s > min_time_for_next_s || timers.is_first_check_for_next);
    return is_valid_overall && is_valid_for_aa;
}

// t+2 future skill
bool DoCheckForNextNextSkill(const RotaSkillWindow &rota_window, const SkillDetectionTimers &timers)
{
    constexpr static auto min_time_for_next_next_s = 0.6f;
    const auto time_since_last_next_next_skill_check_s =
        GetTimeSinceInSeconds(timers.time_of_last_next_next_skill_check);

    const auto is_valid_skill = (rota_window.next_next_rota_skill.is_special_skill ||
                                 !rota_window.next_next_rota_skill.skill_data.is_auto_attack);
    const auto is_timer_reached =
        (time_since_last_next_next_skill_check_s > min_time_for_next_next_s || timers.is_first_check_for_next_next);
    return is_valid_skill && is_timer_reached;
}

// t+3 future skill
bool DoCheckForNextNextNextSkill(const RotaSkillWindow &rota_window, const SkillDetectionTimers &timers)
{
    constexpr static auto min_time_for_next_next_next_s = 1.2f;
    const auto time_since_last_next_next_next_skill_check_s =
        GetTimeSinceInSeconds(timers.time_of_last_next_next_next_skill_check);

    const auto is_valid_skill = (rota_window.next_next_next_rota_skill.is_special_skill ||
                                 !rota_window.next_next_next_rota_skill.skill_data.is_auto_attack);
    const auto is_timer_reached = (time_since_last_next_next_next_skill_check_s > min_time_for_next_next_next_s ||
                                   timers.is_first_check_for_next_next_next);
    return (is_valid_skill && is_timer_reached);
}

RotaSkillWindow GetRotaSkillWindow(RotationLogType &rotation_run, SkillDetectionTimers &timers)
{
    auto rota_window = _GetRotaWindowFromRotationSteps(rotation_run, timers);

    rota_window.check_for_next_skill = DoCheckForNextSkill(rota_window, timers);

    if (!rota_window.check_for_next_skill)
        return rota_window;

    rota_window.check_for_next_next_skill = DoCheckForNextNextSkill(rota_window, timers);

    if (!rota_window.check_for_next_next_skill)
        return rota_window;

    rota_window.check_for_next_next_next_skill = DoCheckForNextNextNextSkill(rota_window, timers);

    return rota_window;
}

bool ModifierMatches(const KeyPressEvent &event, Modifiers modifier)
{
    switch (modifier)
    {
    case Modifiers::NONE:
        return !event.shift && !event.control && !event.alt;
    case Modifiers::SHIFT:
        return event.shift && !event.control && !event.alt;
    case Modifiers::CTRL:
    case Modifiers::RCTRL:
        return event.control && !event.shift && !event.alt;
    case Modifiers::ALT:
    case Modifiers::RALT:
        return event.alt && !event.shift && !event.control;
    default:
        return false;
    }
}

} // namespace

SkillSlot GetExpectedInputSlot(const RotationStep &step, const RotationLogType &rotation_run)
{
    const auto mapped_skill_name = [&rotation_run](const int icon_id) -> std::string {
        const auto it = rotation_run.log_skill_info_map.find(icon_id);
        return it != rotation_run.log_skill_info_map.end() ? it->second.name : std::string{};
    };

    if (step.skill_data.name == mapped_skill_name(rotation_run.skill_key_mapping.skill_7))
        return SkillSlot::UTILITY_1;
    if (step.skill_data.name == mapped_skill_name(rotation_run.skill_key_mapping.skill_8))
        return SkillSlot::UTILITY_2;
    if (step.skill_data.name == mapped_skill_name(rotation_run.skill_key_mapping.skill_9))
        return SkillSlot::UTILITY_3;
    return step.skill_data.skill_slot;
}

bool IsRotationStepDetectable(const RotationStep &step, const RotationLogType &rotation_run)
{
    if (Settings::CustomGreySkills.contains(static_cast<uint32_t>(step.skill_data.skill_id)))
        return false;

    if (ArcEv::HasObservedSkill(step.skill_data.skill_id))
        return true;

    return Settings::UseSkillEvents && KeyboardCapture::GetInstance().IsInitialized() &&
           GetExpectedInputSlot(step, rotation_run) != SkillSlot::NONE;
}

bool IsRotationStepGreyedOut(const RotationStep &step, const RotationLogType &rotation_run)
{
    const auto is_configured_grey =
        Settings::CustomGreySkills.contains(static_cast<uint32_t>(step.skill_data.skill_id));
    return (step.is_special_skill || is_configured_grey) && !IsRotationStepDetectable(step, rotation_run);
}

void KeypressSkillDetectionLogic(RotationLogType &rotation_run, SkillDetectionTimers &timers)
{
    const auto key_presses = KeyboardCapture::GetInstance().ConsumeKeyPresses();

    if (!Settings::UseSkillEvents || key_presses.empty())
        return;

    if (Globals::MumbleData &&
        (!Globals::MumbleData->Context.IsGameFocused || Globals::MumbleData->Context.IsTextboxFocused))
    {
        return;
    }

    (void)TryAdvanceTimedGreySkill(rotation_run, timers);

    for (size_t key_index = 0; key_index < key_presses.size(); ++key_index)
    {
        const auto &key_press = key_presses[key_index];
        const auto windows_key = static_cast<WindowsKeys>(key_press.virtual_key);
        const auto gw2_key = windows_key_to_keys_enum(windows_key);
        if (gw2_key == Keys::NONE)
            continue;

        auto detected_skill_slot = SkillSlot::NONE;
        std::string detected_action_name;

        if (!key_press.shift && !key_press.control && !key_press.alt)
            default_gw2key_to_skillslot_mapping(gw2_key, detected_skill_slot, detected_action_name);

        for (const auto &[action_name, keybind_info] : Globals::RenderData.keybinds)
        {
            if (keybind_info.button == gw2_key && keybind_info.device == Device::KEYBOARD &&
                ModifierMatches(key_press, keybind_info.modifier))
            {
                detected_action_name = action_name;
                detected_skill_slot = str_to_default_skillslot(action_name);
                break;
            }
        }

        if (detected_skill_slot == SkillSlot::NONE || rotation_run.missing_rotation_steps.empty())
            continue;

        const auto &expected_step = rotation_run.missing_rotation_steps.front();
        if (Settings::CustomGreySkills.contains(static_cast<uint32_t>(expected_step.skill_data.skill_id)))
            continue;
        if (GetExpectedInputSlot(expected_step, rotation_run) != detected_skill_slot)
            continue;

        if (ArcEv::WasSkillObservedRecently(expected_step.skill_data.skill_id, std::chrono::milliseconds(300)))
        {
            continue;
        }

        auto detected_event = EvCombatDataPersistent{};
        detected_event.SkillName = expected_step.skill_data.name;
        detected_event.SkillID = expected_step.skill_data.skill_id;
        detected_event.EventKind = SkillEventKind::KEYPRESS;
        Globals::LastKeyPressSkillID = detected_event.SkillID;
        Globals::LastKeyPressSkillTime = std::chrono::steady_clock::now();
        if (key_index + 1 < key_presses.size())
        {
            KeyboardCapture::GetInstance().RequeueKeyPresses(
                std::vector<KeyPressEvent>{key_presses.begin() + static_cast<std::ptrdiff_t>(key_index + 1),
                                           key_presses.end()});
        }
        Globals::Render.skill_activation_callback(std::move(detected_event));
        return;
    }
}

/**
 * @brief This is the main logic for skill detection against the rotation steps.
 *
 * It checks if the currently casted skill from combat log matches the expected skill
 * in the rotation steps. It uses a windowing approach to look ahead in the rotation
 * steps to find matches, allowing for some flexibility in skill order.
 * We look into the current and the 3 skills afterwards in the rotation steps.
 *
 * Auto attacks are only valid for current and next skill in the rotation steps, if the
 * weapon type matches.
 */
void SkillDetectionLogic(uint32_t &num_skills_wo_match,
                         std::chrono::steady_clock::time_point &time_since_last_match,
                         SkillDetectionTimers &timers,
                         RotationLogType &rotation_run,
                         const EvCombatDataPersistent &current_casted_skill)
{
    const auto duration_since_last_match = GetTimeSinceInSeconds(time_since_last_match);
    const auto curr_casted_is_auto_attack = IsSkillAutoAttack(current_casted_skill.SkillID,
                                                              current_casted_skill.SkillName,
                                                              Globals::RotationRun.skill_data_map);

    if (num_skills_wo_match == 0)
        time_since_last_match = std::chrono::steady_clock::now();

    auto rota_window = GetRotaSkillWindow(rotation_run, timers);
    const auto num_special_skills_in_window = (rota_window.curr_rota_skill.is_special_skill ? 1 : 0) +
                                              (rota_window.next_rota_skill.is_special_skill ? 1 : 0) +
                                              (rota_window.next_next_rota_skill.is_special_skill ? 1 : 0) +
                                              (rota_window.next_next_next_rota_skill.is_special_skill ? 1 : 0);
    if (CheckTheNextNskills(current_casted_skill, rota_window.curr_rota_skill, 1, true, rotation_run))
    {
        ResetSkillDetectionData(timers, num_skills_wo_match);

        timers.is_first_check_for_next = false;
        return;
    }

    if (!curr_casted_is_auto_attack)
    {
        const auto current_casted_is_profession_reset_like_skill =
            SkillRuleData::IsProfessionResetLikeSKill(current_casted_skill.SkillID);

        if (current_casted_is_profession_reset_like_skill)
            return;

        if (rota_window.check_for_next_skill &&
            CheckTheNextNskills(current_casted_skill, rota_window.next_rota_skill, 2, true, rotation_run))
        {
            ResetSkillDetectionData(timers, num_skills_wo_match);

            if (rota_window.next_rota_skill.skill_data.is_auto_attack &&
                rota_window.next_next_rota_skill.skill_data.is_auto_attack)
                timers.time_of_last_aa_skip = std::chrono::steady_clock::now();

            timers.is_first_check_for_next_next = false;
            return;
        }

        if (num_special_skills_in_window >= 1 && rota_window.check_for_next_next_skill &&
            CheckTheNextNskills(current_casted_skill, rota_window.next_next_rota_skill, 3, true, rotation_run))
        {
            ResetSkillDetectionData(timers, num_skills_wo_match);

            timers.is_first_check_for_next_next = false;
            return;
        }

        if (num_special_skills_in_window >= 2 && rota_window.check_for_next_next_next_skill &&
            CheckTheNextNskills(current_casted_skill, rota_window.next_next_next_rota_skill, 4, true, rotation_run))
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
            const auto is_exact_match = rota_skill.skill_data.skill_id == current_casted_skill.SkillID ||
                                        IsSpecialMappingSkill(current_casted_skill, rota_skill);
            const auto is_auto_attack_match =
                !is_exact_match && IsOtherValidAutoAttack(rota_skill, current_casted_skill, rotation_run);

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
