///----------------------------------------------------------------------------------------------------
/// Copyright (c) Raidcore.GG - Licensed under the MIT License.
///
/// Name         :  ArcEvents.cpp
/// Description  :  Contains the callbacks for ArcDPS.
///----------------------------------------------------------------------------------------------------

#include <algorithm>
#include <atomic>
#include <chrono>
#include <deque>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <utility>

#include "ArcEvents.h"

#include "Defines.h"
#include "Settings.h"
#include "Shared.h"
#include "SkillData.h"
#include "Types.h"
#include "TypesUtils.h"

namespace
{
constexpr auto MIN_DUPLICATE_EVENT_INTERVAL = std::chrono::milliseconds(200);
constexpr size_t MAX_PENDING_EVENTS = 512;

static auto is_first_cast_map = std::map<SkillID, bool>{};
static auto authoritative_event_times =
    std::map<std::pair<SkillID, SkillEventKind>, std::chrono::steady_clock::time_point>{};
static auto observed_skill_events = std::map<SkillID, SkillEventKind>{};
static auto observed_skill_times = std::map<SkillID, std::chrono::steady_clock::time_point>{};
std::atomic<bool> accept_combat_events = false;
std::mutex pending_events_mutex;
std::deque<EvCombatDataPersistent> pending_events;

bool IsEquivalentSkillID(const SkillID expected, const SkillID actual)
{
    if (expected == actual)
        return true;

    const auto expected_mapping = SkillRuleData::special_mapping_skills.find(expected);
    if (expected_mapping != SkillRuleData::special_mapping_skills.end() && expected_mapping->second == actual)
        return true;

    const auto actual_mapping = SkillRuleData::special_mapping_skills.find(actual);
    return actual_mapping != SkillRuleData::special_mapping_skills.end() && actual_mapping->second == expected;
}

template <typename Value>
const Value *FindEquivalentSkillValue(const std::map<SkillID, Value> &values, const SkillID skill_id)
{
    const auto exact = values.find(skill_id);
    if (exact != values.end())
        return &exact->second;

    const auto forward_mapping = SkillRuleData::special_mapping_skills.find(skill_id);
    if (forward_mapping != SkillRuleData::special_mapping_skills.end())
    {
        const auto mapped = values.find(forward_mapping->second);
        if (mapped != values.end())
            return &mapped->second;
    }

    for (const auto &[source, target] : SkillRuleData::special_mapping_skills)
    {
        if (target != skill_id)
            continue;

        const auto mapped = values.find(source);
        if (mapped != values.end())
            return &mapped->second;
    }

    return nullptr;
}

// The current ArcDPS EVTC format keeps the 64-byte cbtevent layout but adds
// animation events as new state-change values. The pinned Raidcore header does
// not expose these names yet, so keep the documented numeric values here until
// that header catches up.
constexpr uint8_t CBTS_COMBAT_COMPAT = 0;
constexpr uint8_t CBTS_ANIMATION_START_COMPAT = 67;
constexpr uint8_t CBTS_ANIMATION_STOP_COMPAT = 68;
constexpr uint8_t CBTR_SKILL_CAST_COMPAT = 11;
constexpr uint8_t ACTV_START_COMPAT = 1;
constexpr uint8_t ACTV_MINIMUM_COMPAT = 3;
constexpr uint8_t ACTV_CANCEL_COMPAT = 4;
constexpr uint8_t ACTV_RESET_COMPAT = 5;
constexpr uint8_t ACTV_NO_DATA_COMPAT = 6;

static_assert(sizeof(ArcDPS::CombatEvent) == 64, "Unexpected ArcDPS cbtevent layout");

constexpr bool IsSuccessfulAnimationStop(const uint8_t activation)
{
    return activation == ACTV_MINIMUM_COMPAT || activation == ACTV_RESET_COMPAT ||
           activation == ACTV_NO_DATA_COMPAT;
}

constexpr std::optional<SkillEventKind> ClassifySkillEvent(const ArcDPS::CombatEvent &event)
{
    if (event.IsStatechange == CBTS_ANIMATION_START_COMPAT)
        return std::nullopt;

    if (event.IsStatechange == CBTS_ANIMATION_STOP_COMPAT)
    {
        if (IsSuccessfulAnimationStop(event.IsActivation))
            return SkillEventKind::ANIMATION_COMPLETE;
        return std::nullopt;
    }

    if (event.IsStatechange != CBTS_COMBAT_COMPAT)
        return std::nullopt;

    // Older ArcDPS revisions encoded animation lifecycle events in
    // IsActivation while leaving IsStatechange at zero.
    if (event.IsActivation != 0)
    {
        if (event.IsActivation == ACTV_START_COMPAT || event.IsActivation == ACTV_CANCEL_COMPAT)
            return std::nullopt;
        if (IsSuccessfulAnimationStop(event.IsActivation))
            return SkillEventKind::ANIMATION_COMPLETE;
        return std::nullopt;
    }

    if (event.Result == CBTR_SKILL_CAST_COMPAT)
        return SkillEventKind::SKILL_CAST;

    return SkillEventKind::DAMAGE;
}

constexpr bool ValidateEventClassifier()
{
    auto event = ArcDPS::CombatEvent{};

    event.IsStatechange = CBTS_ANIMATION_START_COMPAT;
    if (ClassifySkillEvent(event).has_value())
        return false;

    event.IsStatechange = CBTS_ANIMATION_STOP_COMPAT;
    event.IsActivation = ACTV_RESET_COMPAT;
    if (ClassifySkillEvent(event) != SkillEventKind::ANIMATION_COMPLETE)
        return false;

    event.IsActivation = ACTV_CANCEL_COMPAT;
    if (ClassifySkillEvent(event).has_value())
        return false;

    event = ArcDPS::CombatEvent{};
    event.Result = CBTR_SKILL_CAST_COMPAT;
    if (ClassifySkillEvent(event) != SkillEventKind::SKILL_CAST)
        return false;

    event = ArcDPS::CombatEvent{};
    return ClassifySkillEvent(event) == SkillEventKind::DAMAGE;
}

static_assert(ValidateEventClassifier(), "ArcDPS event classifier regression");

bool IsValidCombatEvent(const EvCombatData &combat_data)
{
    return combat_data.src != nullptr && combat_data.src->IsSelf && combat_data.ev != nullptr;
}

bool IsValidSkillID(const EvCombatDataPersistent &combat_data)
{
    const auto &rotation_steps = Globals::RotationRun.all_rotation_steps;
    return std::any_of(rotation_steps.begin(), rotation_steps.end(), [&combat_data](const RotationStep &step) {
        return IsEquivalentSkillID(step.skill_data.skill_id, combat_data.SkillID);
    });
}

bool IsAnySkillFromBuild(const EvCombatDataPersistent &combat_data)
{
    if (SkillRuleData::skills_to_not_track.find(combat_data.SkillID) != SkillRuleData::skills_to_not_track.end())
        return false;

    // Damage events with Buff set are condition/buff ticks, not direct skill activations.
    return IsValidSkillID(combat_data) &&
           (combat_data.EventKind != SkillEventKind::DAMAGE || combat_data.IsBuff == 0);
}

bool IsMultiHitSkill(const std::chrono::steady_clock::time_point &now,
                     const EvCombatDataPersistent &combat_data,
                     const bool repeated_authoritative_event)
{
    const auto last_cast_time = Globals::SkillLastTimeCast[combat_data.SkillID];
    const auto skill_data_map_it = Globals::RotationRun.skill_data_map.find(combat_data.SkillID);
    if (skill_data_map_it == Globals::RotationRun.skill_data_map.end())
        return false;

    const auto &skill_data = skill_data_map_it->second;
    const auto cast_time_diff = now - last_cast_time;
    if (cast_time_diff < MIN_DUPLICATE_EVENT_INTERVAL)
    {
        Globals::IsSameCast = true;
        return true;
    }

    // Repeated authoritative events of the same kind can represent ammunition
    // skills cast again before their normal recharge has completed.
    if (repeated_authoritative_event)
    {
        Globals::IsSameCast = false;
        Globals::SkillLastTimeCast[combat_data.SkillID] = now;
        return false;
    }

    const auto cast_time_diff_s = std::chrono::duration<float>(cast_time_diff).count();
    const auto is_same_alac_based = skill_data.recharge_time_with_alacrity > 0
                                        ? cast_time_diff_s < skill_data.recharge_time_with_alacrity * 0.90F
                                        : false;
    const auto is_same_quick_based = skill_data.cast_time_with_quickness > 0
                                         ? cast_time_diff_s < skill_data.cast_time_with_quickness * 0.75F
                                         : false;

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

        Globals::IsSameCast = false;
        Globals::SkillLastTimeCast[combat_data.SkillID] = now;
        return false;
    }

    if (is_same_alac_based || is_same_quick_based)
    {
        Globals::IsSameCast = true;
        return true;
    }

    Globals::IsSameCast = false;
    Globals::SkillLastTimeCast[combat_data.SkillID] = now;
    return false;
}

bool IsSameCast(const EvCombatDataPersistent &combat_data)
{
    const auto now = std::chrono::steady_clock::now();
    auto repeated_authoritative_event = false;
    if (combat_data.EventKind != SkillEventKind::DAMAGE)
    {
        const auto key = std::make_pair(combat_data.SkillID, combat_data.EventKind);
        const auto previous = authoritative_event_times.find(key);
        repeated_authoritative_event =
            previous != authoritative_event_times.end() && now - previous->second >= MIN_DUPLICATE_EVENT_INTERVAL;
        authoritative_event_times[key] = now;
    }

    if (is_first_cast_map.find(combat_data.SkillID) == is_first_cast_map.end())
    {
        is_first_cast_map[combat_data.SkillID] = true;
        Globals::SkillLastTimeCast[combat_data.SkillID] = now;
        return false;
    }

    if (Globals::SkillLastTimeCast.find(combat_data.SkillID) != Globals::SkillLastTimeCast.end() &&
        IsMultiHitSkill(now, combat_data, repeated_authoritative_event))
        return true;

    Globals::SkillLastTimeCast[combat_data.SkillID] = now;
    return false;
}
} // namespace

namespace ArcEv
{
void StartCombatEventProcessing()
{
    std::lock_guard<std::mutex> lock(pending_events_mutex);
    pending_events.clear();
    observed_skill_events.clear();
    observed_skill_times.clear();
    accept_combat_events = true;
}

void StopCombatEventProcessing()
{
    accept_combat_events = false;
    std::lock_guard<std::mutex> lock(pending_events_mutex);
    pending_events.clear();
}

void ProcessPendingCombatEvents()
{
    std::deque<EvCombatDataPersistent> events;
    {
        std::lock_guard<std::mutex> lock(pending_events_mutex);
        events.swap(pending_events);
    }

    for (auto &data : events)
    {
        data.SkillID = SafeConvertToSkillID(static_cast<uint64_t>(data.SkillID));
        if (data.SkillID == SkillID::UNKNOWN_SKILL || !IsAnySkillFromBuild(data))
            continue;

        const auto skill_it = Globals::RotationRun.skill_data_map.find(data.SkillID);
        if (data.SkillName.empty() && skill_it != Globals::RotationRun.skill_data_map.end())
            data.SkillName = skill_it->second.name;

        if (Settings::CustomGreySkills.contains(static_cast<uint32_t>(data.SkillID)))
            continue;

        if (data.EventKind != SkillEventKind::DAMAGE)
            observed_skill_events[data.SkillID] = data.EventKind;

        const auto since_keypress =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() -
                                                                  Globals::LastKeyPressSkillTime)
                .count();
        if (Settings::UseSkillEvents && data.SkillID == Globals::LastKeyPressSkillID && since_keypress >= 0 &&
            since_keypress < 250)
            continue;

        Globals::LastArcEventSkillName = data.SkillName;
        if (!IsSameCast(data))
        {
            observed_skill_times[data.SkillID] = std::chrono::steady_clock::now();
            Globals::Render.skill_activation_callback(std::move(data));
        }
    }
}

void ResetSkillCastTracking()
{
    is_first_cast_map.clear();
    authoritative_event_times.clear();
    observed_skill_times.clear();
    Globals::SkillLastTimeCast.clear();
    std::lock_guard<std::mutex> lock(pending_events_mutex);
    pending_events.clear();
}

void ResetObservedSkillCapabilities()
{
    observed_skill_events.clear();
    observed_skill_times.clear();
}

bool HasObservedSkill(const SkillID skill_id)
{
    return FindEquivalentSkillValue(observed_skill_events, skill_id) != nullptr;
}

bool WasSkillObservedRecently(const SkillID skill_id, const std::chrono::milliseconds interval)
{
    const auto *observed_time = FindEquivalentSkillValue(observed_skill_times, skill_id);
    return observed_time != nullptr && std::chrono::steady_clock::now() - *observed_time < interval;
}

const char *SkillEventKindName(const SkillEventKind kind)
{
    switch (kind)
    {
    case SkillEventKind::DAMAGE:
        return "damage";
    case SkillEventKind::SKILL_CAST:
        return "skill-cast";
    case SkillEventKind::ANIMATION_COMPLETE:
        return "animation-complete";
    case SkillEventKind::KEYPRESS:
        return "keypress";
    case SkillEventKind::UNKNOWN:
    default:
        return "unknown";
    }
}

void OnCombatLocal(void *data)
{
    if (data == nullptr)
        return;

    const auto *combat_data = static_cast<EvCombatData *>(data);
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

    if (!accept_combat_events.load() || channel == nullptr || ev == nullptr || src == nullptr)
        return false;

    const auto combat_data = EvCombatData{ev, src, dst, skillname, id, revision};
    if (!IsValidCombatEvent(combat_data))
        return false;

    const auto event_kind = ClassifySkillEvent(*combat_data.ev);
    if (!event_kind.has_value())
        return false;

    const auto data = EvCombatDataPersistent{
        .SrcName = combat_data.src->Name ? std::string(combat_data.src->Name) : std::string{},
        .SrcID = combat_data.src->ID,
        .SrcProfession = combat_data.src->Profession,
        .SrcSpecialization = combat_data.src->Specialization,
        .SkillName = combat_data.skillname ? std::string(combat_data.skillname) : std::string{},
        .SkillID = static_cast<SkillID>(combat_data.ev->SkillID),
        .EventID = combat_data.id,
        .RepeatedSkill = false,
        .EventKind = *event_kind,
        .IsBuff = combat_data.ev->Buff,
        .StateChange = combat_data.ev->IsStatechange,
        .Activation = combat_data.ev->IsActivation,
        .Result = combat_data.ev->Result,
    };

    {
        std::lock_guard<std::mutex> lock(pending_events_mutex);
        if (pending_events.size() >= MAX_PENDING_EVENTS)
            pending_events.pop_front();
        pending_events.push_back(data);
    }

    return true;
}
} // namespace ArcEv
