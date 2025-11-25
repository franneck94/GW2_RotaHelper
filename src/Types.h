#pragma once

#include <cstdint>
#include <filesystem>
#include <list>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "arcdps/ArcDPS.h"

#include "Defines.h"
#include "SkillIDs.h"

struct EvCombatDataPersistent
{
    std::string SrcName;
    uintptr_t SrcID;
    uint32_t SrcProfession;
    uint32_t SrcSpecialization;
    std::string SkillName;
    SkillID SkillID;
    uint64_t EventID;
    bool RepeatedSkill;
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

struct BenchFileInfo
{
    std::filesystem::path full_path;
    std::filesystem::path relative_path;
    std::string display_name;
    bool is_directory_header;

    BenchFileInfo(const std::filesystem::path &full, const std::filesystem::path &relative, bool is_header = false);
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

enum class SkillSlot
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

enum class WeaponType
{
    NONE = 0,
    GREATSWORD = 1,
    HAMMER = 2,
    LONGBOW = 3,
    RIFLE = 4,
    SHORTBOW = 5,
    STAFF = 6,
    SPEAR = 7,
    TRIDENT = 8,
    HARPOON_GUN = 9,
    AXE = 10,
    DAGGER = 11,
    MACE = 12,
    PISTOL = 13,
    SCEPTER = 14,
    SWORD = 15,
    FOCUS = 16,
    SHIELD = 17,
    TORCH = 18,
    WARHORN = 19,
};

struct SkillData
{
    int icon_id;
    SkillID skill_id;
    std::string name;
    float recharge_time;
    float recharge_time_with_alacrity;
    float cast_time;
    float cast_time_with_quickness;
    bool is_auto_attack;
    bool is_weapon_skill;
    bool is_utility_skill;
    bool is_elite_skill;
    bool is_heal_skill;
    bool is_profession_skill;
    SkillSlot skill_type;
    WeaponType weapon_type;
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
using SkillDataMap = std::map<SkillID, SkillData>;

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
    LEFT_CTRL = 1,
    LEFT_SHIFT = 2,
    CAPS = 5,
    ZIRUMFLEX = 17,
    TAB = 22,
    LEFT_ARROW = 29,
    RIGHT_ARROW = 30,
    F1 = 32,
    F2 = 33,
    F3 = 34,
    F4 = 35,
    F5 = 36,
    F6 = 37,
    F7 = 38,
    ONE = 49,
    TWO = 50,
    THREE = 51,
    FOUR = 52,
    FIVE = 53,
    SIX = 54,
    SEVEN = 55,
    EIGHT = 56,
    NINE = 57,
    ZERO = 48,
    A = 65,
    E = 69,
    B = 66,
    C = 67,
    D = 68,
    F = 70,
    G = 71,
    H = 72,
    I = 73,
    J = 74,
    K = 75,
    L = 76,
    M = 77,
    N = 78,
    O = 79,
    Q = 81,
    W = 87,
    R = 82,
    T = 84,
    Y = 89,
    U = 85,
    P = 80,
    S = 83,
    Z = 90,
    X = 88,
    V = 86,
    NUM_ADD = 91,
    NUM_1 = 96,
    NUM_2 = 97,
    NUM_3 = 98,
    NUM_4 = 99,
    NUM_5 = 100,
    NUM_6 = 101,
    NUM_7 = 102,
    NUM_8 = 103,
    NUM_9 = 104,
    NUM_RET = 105,
    LEFT_ALT = 202,
};

enum class Modifiers
{
    NONE = 0,
    SHIFT = 2,
    ALT = 4,
    CTRL = 6,
};

struct KeybindInfo
{
    std::string action_name;
    Keys button = Keys::NONE;
    Modifiers modifier = Modifiers::NONE;
};

struct SkillRules
{
    const std::set<std::string_view> &skills_substr_weapon_swap_like;
    const std::set<std::string_view> &skills_match_weapon_swap_like;
    const std::set<std::string_view> &skills_substr_to_drop;
    const std::set<std::string_view> &skills_match_to_drop;
    const std::set<std::string_view> &special_substr_to_gray_out;
    const std::set<std::string_view> &special_match_to_gray_out;
    const std::set<std::string_view> &special_substr_to_remove_duplicates;
    const std::set<std::string_view> &easy_mode_drop_match;
    const std::map<std::string_view, std::set<SkillID>> &class_map_special_match_to_gray_out;
    const std::map<std::string_view, std::set<SkillID>> &class_map_easy_mode_drop_match;
};

enum class DownloadState : uint8_t
{
    NOT_STARTED,
    STARTED,
    FINISHED,
    FAILED,
    NO_UPDATE_NEEDED,
};

struct RotaSkillWindow
{
    RotationStep curr_rota_skill;
    RotationStep next_rota_skill;
    bool check_for_next_skill;
    RotationStep next_next_rota_skill;
    bool check_for_next_next_skill;
    RotationStep next_next_next_rota_skill;
    bool check_for_next_next_next_skill;
};

struct SkillDetectionTimers
{
    std::chrono::steady_clock::time_point time_of_last_next_skill_check = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point time_of_last_next_next_skill_check = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point time_of_last_next_next_next_skill_check = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point time_of_last_aa_skip = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point time_of_last_pop = std::chrono::steady_clock::now();
    bool is_first_check_for_next = true;
    bool is_first_check_for_next_next = true;
    bool is_first_check_for_next_next_next = true;
};
