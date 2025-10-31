#pragma once

#include <cstdint>
#include <list>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "arcdps/ArcDPS.h"

struct EvCombatDataPersistent
{
    std::string SrcName;
    uintptr_t SrcID;
    uint32_t SrcProfession;
    uint32_t SrcSpecialization;
    std::string SkillName;
    uint64_t SkillID;
    uint64_t EventID;
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

struct SkillState
{
    bool is_current;
    bool is_last;
    bool is_auto_attack;
};

struct LogSkillInfo
{
    std::string name;
    std::string icon_url;
};

using LogSkillInfoMap = std::map<int, LogSkillInfo>;

using LogDataTypes = std::variant<int, float, bool, std::string>;

struct IntNode
{
    std::map<std::string, IntNode> children;
    std::optional<LogDataTypes> value;
};

enum class SkillType
{
    NONE,
    WEAPON_1,
    WEAPON_2,
    WEAPON_3,
    WEAPON_4,
    WEAPON_5,
    HEAL,
    UTILITY_1,
    UTILITY_2,
    UTILITY_3,
    ELITE,
    PROFESSION_1,
    PROFESSION_2,
    PROFESSION_3,
    PROFESSION_4,
    PROFESSION_5,
    PROFESSION_6,
    PROFESSION_7,
};

struct SkillData
{
    int icon_id;
    int skill_id;
    std::string name;
    int recharge_time;
    float recharge_time_with_alacrity;
    int cast_time;
    float cast_time_with_quickness;
    bool is_auto_attack;
    bool is_weapon_skill;
    bool is_utility_skill;
    bool is_elite_skill;
    bool is_heal_skill;
    bool is_profession_skill;
    SkillType skill_type;
};

struct RotationStep
{
    float time_of_cast;
    float duration_ms;
    SkillData skill_data;
    bool is_special_skill;
};

using RotationSteps = std::vector<RotationStep>;
using RotationStepsList = std::list<RotationStep>;
using SkillDataMap = std::map<int, SkillData>;

enum class ProfessionID : uint32_t
{
    UNKNOWN = 0,
    GUARDIAN = 1,
    WARRIOR = 2,
    ENGINEER = 3,
    RANGER = 4,
    THIEF = 5,
    ELEMENTALIST = 6,
    MESMER = 7,
    NECROMANCER = 8,
    REVENANT = 9,
};

enum class EliteSpecID : uint32_t
{
    // Elementalist
    Catalyst = 67,
    Evoker = 80,
    Tempest = 48,
    Weaver = 56,

    // Engineer
    Amalgam = 75,
    Holosmith = 57,
    Mechanist = 70,
    Scrapper = 43,

    // Guardian
    Dragonhunter = 27,
    Firebrand = 62,
    Luminary = 81,
    Willbender = 65,

    // Mesmer
    Chronomancer = 40,
    Mirage = 59,
    Troubadour = 73,
    Virtuoso = 66,

    // Necromancer
    Harbinger = 64,
    Reaper = 34,
    Ritualist = 76,
    Scourge = 60,

    // Ranger
    Druid = 5,
    Galeshot = 78,
    Soulbeast = 55,
    Untamed = 72,

    // Revenant
    Conduit = 79,
    Herald = 52,
    Renegade = 63,
    Vindicator = 69,

    // Thief
    Antiquary = 77,
    Daredevil = 7,
    Deadeye = 58,
    Specter = 71,

    // Warrior
    Berserker = 18,
    Bladesworn = 68,
    Paragon = 74,
    Spellbreaker = 61,

    // Default/Unknown
    Unknown = 0
};

struct MetaData
{
    std::string name;
    std::string url;
    std::string benchmark_type;
    std::string profession;
    ProfessionID profession_id;
    std::string elite_spec;
    EliteSpecID elite_spec_id;
    std::string build_type;
    std::string url_name;
    std::string dps_report_url;
    std::string html_file_path;
};

enum class Keys
{
    NONE,
    SHIFT = 2,
    TAB = 22,
    E = 69,
    LEFT_ALT = 202,
};

// Unchecked
enum class Modifiers
{
    SHIFT = 2,
    ALT = 4,
    CTRL = 6,
};
