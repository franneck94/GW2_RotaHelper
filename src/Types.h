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

struct SkillInfo
{
    std::string name;
    std::string icon_url;
};

using SkillInfoMap = std::map<int, SkillInfo>;
using LogDataTypes = std::variant<int, float, bool, std::string>;

struct IntNode
{
    std::map<std::string, IntNode> children;
    std::optional<LogDataTypes> value;
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
};

struct RotationStep
{
    float time_of_cast;
    float duration_ms;
    SkillData skill_data;
    bool is_special_skill;
};

using RotationInfoVec = std::vector<RotationStep>;
using RotationInfoList = std::list<RotationStep>;
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

inline ProfessionID string_to_profession(const std::string &profession_name)
{
    auto lower_name = profession_name;
    std::transform(lower_name.begin(),
                   lower_name.end(),
                   lower_name.begin(),
                   ::tolower);

    if (lower_name == "guardian")
        return ProfessionID::GUARDIAN;
    if (lower_name == "warrior")
        return ProfessionID::WARRIOR;
    if (lower_name == "engineer")
        return ProfessionID::ENGINEER;
    if (lower_name == "ranger")
        return ProfessionID::RANGER;
    if (lower_name == "thief")
        return ProfessionID::THIEF;
    if (lower_name == "elementalist")
        return ProfessionID::ELEMENTALIST;
    if (lower_name == "mesmer")
        return ProfessionID::MESMER;
    if (lower_name == "necromancer")
        return ProfessionID::NECROMANCER;
    if (lower_name == "revenant")
        return ProfessionID::REVENANT;

    return ProfessionID::UNKNOWN;
}

inline EliteSpecID string_to_elite_spec(const std::string &spec_name)
{
    auto lower_name = spec_name;
    std::transform(lower_name.begin(),
                   lower_name.end(),
                   lower_name.begin(),
                   ::tolower);

    // Elementalist
    if (lower_name == "catalyst")
        return EliteSpecID::Catalyst;
    if (lower_name == "evoker")
        return EliteSpecID::Evoker;
    if (lower_name == "tempest")
        return EliteSpecID::Tempest;
    if (lower_name == "weaver")
        return EliteSpecID::Weaver;

    // Engineer
    if (lower_name == "amalgam")
        return EliteSpecID::Amalgam;
    if (lower_name == "holosmith")
        return EliteSpecID::Holosmith;
    if (lower_name == "mechanist")
        return EliteSpecID::Mechanist;
    if (lower_name == "scrapper")
        return EliteSpecID::Scrapper;

    // Guardian
    if (lower_name == "dragonhunter")
        return EliteSpecID::Dragonhunter;
    if (lower_name == "firebrand")
        return EliteSpecID::Firebrand;
    if (lower_name == "luminary")
        return EliteSpecID::Luminary;
    if (lower_name == "willbender")
        return EliteSpecID::Willbender;

    // Mesmer
    if (lower_name == "chronomancer")
        return EliteSpecID::Chronomancer;
    if (lower_name == "mirage")
        return EliteSpecID::Mirage;
    if (lower_name == "troubadour")
        return EliteSpecID::Troubadour;
    if (lower_name == "virtuoso")
        return EliteSpecID::Virtuoso;

    // Necromancer
    if (lower_name == "harbinger")
        return EliteSpecID::Harbinger;
    if (lower_name == "reaper")
        return EliteSpecID::Reaper;
    if (lower_name == "ritualist")
        return EliteSpecID::Ritualist;
    if (lower_name == "scourge")
        return EliteSpecID::Scourge;

    // Ranger
    if (lower_name == "druid")
        return EliteSpecID::Druid;
    if (lower_name == "galeshot")
        return EliteSpecID::Galeshot;
    if (lower_name == "soulbeast")
        return EliteSpecID::Soulbeast;
    if (lower_name == "untamed")
        return EliteSpecID::Untamed;

    // Revenant
    if (lower_name == "conduit")
        return EliteSpecID::Conduit;
    if (lower_name == "herald")
        return EliteSpecID::Herald;
    if (lower_name == "renegade")
        return EliteSpecID::Renegade;
    if (lower_name == "vindicator")
        return EliteSpecID::Vindicator;

    // Thief
    if (lower_name == "antiquary")
        return EliteSpecID::Antiquary;
    if (lower_name == "daredevil")
        return EliteSpecID::Daredevil;
    if (lower_name == "deadeye")
        return EliteSpecID::Deadeye;
    if (lower_name == "specter" || lower_name == "spectre")
        return EliteSpecID::Specter;

    // Warrior
    if (lower_name == "berserker")
        return EliteSpecID::Berserker;
    if (lower_name == "bladesworn")
        return EliteSpecID::Bladesworn;
    if (lower_name == "paragon")
        return EliteSpecID::Paragon;
    if (lower_name == "spellbreaker")
        return EliteSpecID::Spellbreaker;

    return EliteSpecID::Unknown;
}

inline std::string profession_to_string(ProfessionID profession_id)
{
    switch (profession_id)
    {
    case ProfessionID::GUARDIAN:
        return "Guardian";
    case ProfessionID::WARRIOR:
        return "Warrior";
    case ProfessionID::ENGINEER:
        return "Engineer";
    case ProfessionID::RANGER:
        return "Ranger";
    case ProfessionID::THIEF:
        return "Thief";
    case ProfessionID::ELEMENTALIST:
        return "Elementalist";
    case ProfessionID::MESMER:
        return "Mesmer";
    case ProfessionID::NECROMANCER:
        return "Necromancer";
    case ProfessionID::REVENANT:
        return "Revenant";
    case ProfessionID::UNKNOWN:
    default:
        return "Unknown";
    }
}

inline std::string elite_spec_to_string(EliteSpecID elite_spec_id)
{
    switch (elite_spec_id)
    {
    // Elementalist
    case EliteSpecID::Catalyst:
        return "Catalyst";
    case EliteSpecID::Evoker:
        return "Evoker";
    case EliteSpecID::Tempest:
        return "Tempest";
    case EliteSpecID::Weaver:
        return "Weaver";

    // Engineer
    case EliteSpecID::Amalgam:
        return "Amalgam";
    case EliteSpecID::Holosmith:
        return "Holosmith";
    case EliteSpecID::Mechanist:
        return "Mechanist";
    case EliteSpecID::Scrapper:
        return "Scrapper";

    // Guardian
    case EliteSpecID::Dragonhunter:
        return "Dragonhunter";
    case EliteSpecID::Firebrand:
        return "Firebrand";
    case EliteSpecID::Luminary:
        return "Luminary";
    case EliteSpecID::Willbender:
        return "Willbender";

    // Mesmer
    case EliteSpecID::Chronomancer:
        return "Chronomancer";
    case EliteSpecID::Mirage:
        return "Mirage";
    case EliteSpecID::Troubadour:
        return "Troubadour";
    case EliteSpecID::Virtuoso:
        return "Virtuoso";

    // Necromancer
    case EliteSpecID::Harbinger:
        return "Harbinger";
    case EliteSpecID::Reaper:
        return "Reaper";
    case EliteSpecID::Ritualist:
        return "Ritualist";
    case EliteSpecID::Scourge:
        return "Scourge";

    // Ranger
    case EliteSpecID::Druid:
        return "Druid";
    case EliteSpecID::Galeshot:
        return "Galeshot";
    case EliteSpecID::Soulbeast:
        return "Soulbeast";
    case EliteSpecID::Untamed:
        return "Untamed";

    // Revenant
    case EliteSpecID::Conduit:
        return "Conduit";
    case EliteSpecID::Herald:
        return "Herald";
    case EliteSpecID::Renegade:
        return "Renegade";
    case EliteSpecID::Vindicator:
        return "Vindicator";

    // Thief
    case EliteSpecID::Antiquary:
        return "Antiquary";
    case EliteSpecID::Daredevil:
        return "Daredevil";
    case EliteSpecID::Deadeye:
        return "Deadeye";
    case EliteSpecID::Specter:
        return "Specter";

    // Warrior
    case EliteSpecID::Berserker:
        return "Berserker";
    case EliteSpecID::Bladesworn:
        return "Bladesworn";
    case EliteSpecID::Paragon:
        return "Paragon";
    case EliteSpecID::Spellbreaker:
        return "Spellbreaker";

    // Default/Unknown
    case EliteSpecID::Unknown:
    default:
        return "Unknown";
    }
}

inline std::vector<std::string> get_elite_specs_for_profession(
    ProfessionID profession)
{
    std::vector<std::string> elite_specs;

    switch (profession)
    {
    case ProfessionID::GUARDIAN:
        elite_specs = {"dragonhunter", "firebrand", "willbender", "luminary"};
        break;
    case ProfessionID::WARRIOR:
        elite_specs = {"berserker", "spellbreaker", "bladesworn", "paragon"};
        break;
    case ProfessionID::ENGINEER:
        elite_specs = {"scrapper", "holosmith", "mechanist", "amalgam"};
        break;
    case ProfessionID::RANGER:
        elite_specs = {"druid", "soulbeast", "untamed", "galeshot"};
        break;
    case ProfessionID::THIEF:
        elite_specs = {"daredevil", "deadeye", "specter", "antiquary"};
        break;
    case ProfessionID::ELEMENTALIST:
        elite_specs = {"tempest", "weaver", "catalyst", "evoker"};
        break;
    case ProfessionID::MESMER:
        elite_specs = {"chronomancer", "mirage", "virtuoso", "troubadour"};
        break;
    case ProfessionID::NECROMANCER:
        elite_specs = {"reaper", "scourge", "harbinger", "ritualist"};
        break;
    case ProfessionID::REVENANT:
        elite_specs = {"herald", "renegade", "vindicator", "conduit"};
        break;
    default:
        break;
    }

    return elite_specs;
}
