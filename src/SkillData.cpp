#include <map>
#include <set>
#include <string>
#include <string_view>

#include "MumbleUtils.h"
#include "SkillData.h"

#include "Types.h"

namespace SkillRuleData
{

const std::set<std::string_view> skills_substr_weapon_swap_like = {
    "Weapon Swap",
    // GUARDIAN
    // WARRIOR
    // ENGINEER
    " Kit",
    "Photon Forge",
    // RANGER
    "Ranger Pet",
    "Celestial Avatar",
    // THIEF
    // ELEMENTALIST
    "Attunement",
    // MESMER
    // NECROMANCER
    "Harbinger Shroud",
    "Reaper's Shroud",
    "Ritualist's Shroud",
    // REVENANT
};

const std::set<std::string_view> skills_match_weapon_swap_like = {
    // GUARDIAN
    // WARRIOR
    "Berserk",
    // ENGINEER
    "Flamethrower",
    "Elixir Gun",
    "Bomb Kit",
    "Grenade Kit",
    // RANGER
    "Summon Cyclone Bow",
    "Dismiss Cyclone Bow",
    "Unleash Ranger",
    "Unleash Pet",
    // THIEF
    // ELEMENTALIST
    // MESMER
    // NECROMANCER
    // REVENANT
    "Legendary Entity Stance",
    "Legendary Alliance Stance",
    "Legendary Alliance",
    "Legendary Renegade Stance",
    "Legendary Demon Stance",
    "Legendary Dwarf Stance",
    "Legendary Centaur Stance",
    "Legendary Assassin Stance",
    "Legendary Dragon Stance",
};

const std::set<std::string_view> skills_substr_to_drop = {
    "Bloodstone Fervor",
    "Relic of",
    "Relikt des ",
    "Mushroom King",
    "Superior Sigil",
    // GUARDIAN
    "Fire Jurisdiction",
    // WARRIOR
    // ENGINEER
    // RANGER
    // THIEF
    // ELEMENTALIST
    "Flame Explosion",
    // MESMER
    // NECROMANCER
    // REVENANT
};

const std::set<std::string_view> skills_match_to_drop = {
    "Doom",
    // GUARDIAN
    // WARRIOR
    "King of Fires",
    "Magebane Tether",
    // ENGINEER
    "Focused Devastation",
    "Explosive Entrance",
    "Spark Revolver",
    "Fire Rocket Barrage",
    "Orbital Command Strike",
    "Bomb",
    // RANGER
    "Wuthering Wind",
    "Unleashed Overbearing Smash (Leap)",
    "Quick Draw",
    // THIEF
    "Impaling Lotus",
    // ELEMENTALIST
    "Sunspot",
    "Earthen Blast",
    // MESMER
    "Mirage Cloak",
    "Syncopate",
    "Syncopate (Delay Wave)",
    // NECROMANCER
    "Approaching Doom",
    "Chilling Nova",
    "Cascading Corruption",
    "Explosive Growth",
    "Deathly Haste",
    // REVENANT
    "Invoke Torment",
    "Mistfire",
};

const std::set<std::string_view> special_substr_to_gray_out = {
    // GUARDIAN
    "Chapter 1:",
    "Chapter 2:",
    "Chapter 3:",
    "Chapter 4:",
    "Tome of ",
    " Radiant Forge",
    // WARRIOR
    // ENGINEER
    "Detonate",
    "Photon Forge",
    "Holoforge",
    "-Storm",
    // RANGER
    "Blood Moon",
    // THIEF
    "Shadow Meld",
    // ELEMENTALIST
    "Deploy Jade Sphere",
    "Zap",
    // MESMER
    "Distortion",
    // NECROMANCER
    // REVENANT
};

const std::set<std::string_view> special_match_to_gray_out = {
    "Dodge",
    // GUARDIAN
    "Zealot's Flame",
    "Rushing Justice",
    "Symbol of Punishment",
    "Flowing Resolve",
    "Jurisdiction",
    // WARRIOR
    "Sheathe Gunsaber",
    "Unsheathe Gunsaber",
    "Dragon Trigger",
    "Flow Stabilizer",
    "Overcharged Cartridges",
    "Tactical Reload",
    "Arcing Slice",
    "Blood Reckoning",
    "Outrage",
    "Chant of Action",
    "Signet of Fury",
    "Signet of Might",
    "Signet of Rage",
    "Berserk",
    "Flames of War",
    // ENGINEER
    "Devastator",
    "Flame Blast",
    "Air Blast",
    "Overcharged Shot",
    "Superconducting Signet",
    "Overclock Signet",
    "Lightning Rod",
    "Spark Revolver",
    "Core Reactor Shot",
    "Jade Mortar",
    "Rocket Punch",
    "Rolling Smash",
    "Discharge Array",
    "Sky Circus",
    "Radiant Arc",
    "Barrier Burst",
    "Crisis Zone",
    "Napalm",
    "Glue Shot",
    "Acid Bomb",
    "Offensive Protocol: Obliterate",
    "Offensive Protocol: Demolish",
    "Evolve",
    "Corona Burst",
    "Photon Blitz",
    "Holo Leap",
    // RANGER
    "Quarry's Peril",
    "Perfect Storm",
    "Mistral",
    "Pelt",
    "Path of Scars",
    "Rending Vines",
    "Enveloping Haze",
    "Flame Trap",
    "Frost Trap",
    "Venomous Outburst",
    "\"Sic 'Em!\"",
    "Sharpening Stone",
    "Sun Spirit",
    "Entangle",
    "Viper's Nest",
    "Seed of Life",
    "Lunar Impact",
    "Natural Convergence",
    "Sharpen Spines",
    "Vulture Stance",
    "Bumble",
    "Rain of Spikes",
    // THIEF
    "Prepare Thousand Needles",
    "Spider Venom",
    "Assassin's Signet",
    "Signet of Malice",
    "Skritt Scuffle",
    "Harrowing Storm",
    // ELEMENTALIST
    "Earthquake",
    "Fire Shield",
    "Signet of Restoration",
    "Flame Barrage",
    "Dragon's Tooth",
    "Fire Shield",
    "Sand Squall",
    // MESMER
    "Signet of Domination",
    "Signet of Midnight",
    "Signet of Inspiration",
    "Signet of Illusions",
    "Signet of the Ether",
    "Jaunt",
    "Crystal Sands",
    "Bladesong Sorrow",
    "Phantasmal Warden",
    "Bladeturn Requiem",
    "Bladecall",
    "Tale of the Soulkeeper",
    "Harmonious Harp",
    "Tale of the August Queen",
    "Phantasmal Disenchanter",
    "Continuum Split",
    "Continuum Shift",
    "Time Sink",
    "Phantasmal Berserker",
    "Phantasmal Lancer",
    // NECROMANCER
    "Plague Signet",
    "Elixir of Promise",
    "Garish Pillar",
    "Sandstorm Shroud",
    "Soul Shards",
    "Preservation",
    "Grasping Dark",
    "Innervate Wanderlust",
    "Innervate Anguish",
    "Distress",
    "Death's Charge",
    "Perforate",
    "Grasping Dark",
    "\"You Are All Weaklings!\"",
    "Haunt",
    "Plaguelands",
    "Nefarious Favor",
    "Terrify",
    "Infusing Terror",
    "Anguish",
    "Summon Spirits",
    "Wanderlust",
    // REVENANT
    "True Nature",
    "Facet of Chaos",
    "Abyssal Blitz",
    "Resist the Darkness",
    "Embrace the Darkness",
    "Razorclaw's Rage",
    "Cosmic Wisdom",
    "Energy Meld",
    "Spear of Archemorus",
    "Facet of Elements",
    "Facet of Darkness",
    "Facet of Strength",
    "Relinquish Power",
    "Orders from Above",
};

const std::set<std::string_view> special_substr_to_remove_duplicates = {
    "Rushing Justice",
    "Devastator",
    "Signet of Fury",
    "Offensive Protocol: Demolish",
    "Abyssal Blitz",
    "Legendary Entity Stance",
    "Legendary Alliance Stance",
    "Legendary Alliance",
    "Legendary Renegade Stance",
    "Legendary Demon Stance",
    "Legendary Dwarf Stance",
    "Legendary Centaur Stance",
    "Legendary Assassin Stance",
    "Legendary Dragon Stance",
    "Deathstrike",
    "Dodge",
    "Wolf's Onslaught",
    "Unleashed Overbearing Smash",
    "Jaunt",
    "Cry of Frustration",
    "Arcane Blast",
    "Twilight Combo",
    "Death Blossom",
    "Beguiling Haze",
};

const std::set<std::string_view> easy_mode_drop_match = {
    // ENGINEER
    "Sky Circus",
    "Spark Revolver",
    "Core Reactor Shot",
    "Jade Mortar",
    "Rocket Punch",
    "Rolling Smash",
    "Barrier Burst",
    "Crisis Zone",
    "Power Spike",
    "Discharge Array",
    // REVENANT
    "Abyssal Strike",
    "Abyssal Fire",
    // MESMER
    "Power Spike",
};

const SkillRules skill_rules = SkillRules{
    skills_substr_weapon_swap_like,
    skills_match_weapon_swap_like,
    skills_substr_to_drop,
    skills_match_to_drop,
    special_substr_to_gray_out,
    special_match_to_gray_out,
    special_substr_to_remove_duplicates,
    easy_mode_drop_match,
};

const std::set<SkillID> skills_to_not_track = {
    SkillID::RIFLE_BURST_GRENADE,
    SkillID::LIGHTNING_STRIKE,
};

const std::map<SkillID, SkillID> special_mapping_skills = {
    {SkillID::DEVASTATOR, SkillID::FOCUSED_DEVASTATION},
    {SkillID::FOCUSED_DEVASTATION, SkillID::DEVASTATOR},
    {SkillID::LIGHTNING_ROD, SkillID::ELECTRIC_ARTILLERY},
    {SkillID::ELECTRIC_ARTILLERY, SkillID::LIGHTNING_ROD},
};

const std::set<SkillID> berserker_f1_skills = {
    SkillID::EVISCERATE,
    SkillID::EARTHSHAKER,
    SkillID::ARC_DIVIDER,
    SkillID::FLAMING_FLURRY,
    SkillID::DECAPITATE,
    SkillID::RUPTURING_SMASH,
    SkillID::WILD_THROW,
    SkillID::SCORCHED_EARTH,
};

const std::set<SkillID> mesmer_weapon_4_skills = {
    SkillID::PHANTASMAL_DUELIST,
    SkillID::TEMPORAL_CURTAIN,
    SkillID::PHANTASMAL_BERSERKER,
    SkillID::ILLUSIONARY_RIPOSTE,
    SkillID::THE_PRESTIGE,
    SkillID::SLIPSTREAM,
    SkillID::PHANTASMAL_WHALER,
    SkillID::CHAOS_ARMOR,
    SkillID::COUNTER_BLADE,
    SkillID::INTO_THE_VOID,
    SkillID::DEJA_VU,
    SkillID::ECHO_OF_MEMORY,
    SkillID::PHANTASMAL_SHARPSHOOTER,
    SkillID::PHANTASMAL_LANCER,
};

const std::set<SkillID> reset_like_skill = {
    SkillID::CRUSHING_BLOW,
    SkillID::REFRACTION_CUTTER,
    SkillID::REFRACTION_CUTTER_1,
    SkillID::MIND_THE_GAP,
    SkillID::OVERLOAD_AIR,
    SkillID::OVERLOAD_FIRE,
    SkillID::OVERLOAD_WATER,
    SkillID::OVERLOAD_EARTH,
};

const std::map<SkillID, float> skill_cast_time_map = {
    {SkillID::ESSENCE_BLAST, 0.75f},    // rit shroud aa
    {SkillID::LIFE_REND, 0.5f},         // reaper shroud aa
    {SkillID::LIFE_SLASH, 0.5f},        // reaper shroud aa
    {SkillID::LIFE_REAP, 0.5f},         // reaper shroud aa
    {SkillID::TAINTED_BOLTS, 0.5f},     // harbinger shroud aa
    {SkillID::HAIL_OF_JUSTICE, 1.25f},  // guard pistol 4
    {SkillID::CLEANSING_FLAME, 3.25f},  // guard torch 5
    {SkillID::RAPID_FIRE, 2.25f},       // ranger lb 2
    {SkillID::BARRAGE, 2.25f},          // ranger lb 5
    {SkillID::WEAKENING_WHIRL, 0.75f},  // thief staff 3
    {SkillID::DOUBLE_TAP, 0.75f},       // thief rifle 3
    {SkillID::THREE_ROUND_BURST, 1.0f}, // thief rifle 3
    {SkillID::WILD_THROW, 3.25f},       // warrior spear f1
    {SkillID::WHIRLING_DEFENSE, 3.25f}, // ranger axe 5
    {SkillID::RIFLE_BURST, 0.5f},       // engi rifle 1
    {SkillID::SPATIAL_SURGE, 1.0f},     // mesmer gs 1
    {SkillID::FLYING_CUTTER, 0.5f},     // mesmer sw 1
    {SkillID::FLAMING_FLURRY, 3.25f},   // warrior sw f1
    {SkillID::SCORCHED_EARTH, 3.25f},   // warrior lb f1
    {SkillID::OVERLOAD_AIR, 4.0f},      // ele tempest f1 air
    {SkillID::OVERLOAD_FIRE, 4.0f},     // ele tempest f1 fire
    {SkillID::FLAMESTRIKE, 0.9f},       // ele scepter 1 fire
    {SkillID::SCORCHING_SHOT, 0.5f},    // ele earth pistol 1
    {SkillID::PIERCING_PEBBLE, 0.5f},   // ele fire pistol 1
    {SkillID::RIFT_SLASH, 0.5f},        // rev sword 1
    {SkillID::PUNCTURING_JAB, 0.5f},    // engi spear 1
    {SkillID::RENDING_STRIKE, 0.5f},    // engi spear 1
    {SkillID::AMPLIFYING_SLICE, 0.5f},  // engi spear 1
    {SkillID::CONDUIT_SURGE, 0.5f},     // engi spear 2
    {SkillID::LIGHTNING_ROD, 0.5f},     // engi spear 3
    {SkillID::ORB_OF_WRATH, 0.5f},     //
};

const std::map<SkillID, float> grey_skill_cast_time_map = {
    // GUARDIAN
    {SkillID::RUSHING_JUSTICE, 0.5f},
    {SkillID::SYMBOL_OF_PUNISHMENT, 0.25f},
    {SkillID::FLOWING_RESOLVE, 0.5f},
    {SkillID::JURISDICTION, 0.75f},     //
    // MESMER
    {SkillID::PHANTASMAL_BERSERKER, 0.75f},
    {SkillID::SIGNET_OF_THE_ETHER, 1.0f},
    // ENGINEER
    {SkillID::FLAME_BLAST, 0.10f}, // NOTE: lower than real
    {SkillID::NAPALM, 2.0f},       // NOTE: lower than real
    {SkillID::SUPERCONDUCTING_SIGNET, 0.75f},
    {SkillID::DEVASTATOR, 1.0f}, // XXX: differs from real cast time, fix this when skill is not shown double anymore
};

SkillData GetDataByID(const SkillID skill_id, const SkillDataMap &skill_data_map)
{
    auto it = skill_data_map.find(skill_id);

    if (it != skill_data_map.end())
        return it->second;

    return {};
}

bool IsProfessionResetLikeSKill(const SkillID skill_id)
{
    auto current_profession = get_current_profession_name();
    auto profession_lower = to_lowercase(current_profession);

    auto is_mesmer_weapon_4 = false;
    auto is_berserker_f1 = false;
    if (profession_lower == "mesmer")
        is_mesmer_weapon_4 =
            SkillRuleData::mesmer_weapon_4_skills.find(skill_id) != SkillRuleData::mesmer_weapon_4_skills.end();
    else if (profession_lower == "warrior")
        is_berserker_f1 = SkillRuleData::berserker_f1_skills.find(skill_id) != SkillRuleData::berserker_f1_skills.end();

    return is_mesmer_weapon_4 || is_berserker_f1;
}

} // namespace SkillRuleData
