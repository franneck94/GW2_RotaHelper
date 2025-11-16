#pragma once

#include <d3d11.h>

#include <filesystem>
#include <future>
#include <iostream>
#include <list>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "nlohmann/json.hpp"

#include "SkillIDs.h"
#include "Types.h"

using json = nlohmann::json;

class RotationLogType;

SkillState get_skill_state(const RotationLogType &rotation_run,
                           const std::vector<EvCombatDataPersistent> &played_rotation,
                           const size_t window_idx,
                           const size_t current_idx,
                           const bool is_auto_attack);

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
};

class RotationLogType
{
public:
    void load_data(const std::filesystem::path &json_path, const std::filesystem::path &img_path);

    void pop_bench_rotation_queue();
    std::tuple<int, int, size_t> get_current_rotation_indices() const;
    RotationStep get_rotation_skill(const size_t idx) const;
    void restart_rotation();
    void reset_rotation();
    bool is_current_run_done() const;

    std::list<std::future<void>> futures;
    LogSkillInfoMap log_skill_info_map;
    RotationSteps all_rotation_steps;
    RotationStepsList missing_rotation_steps;
    SkillDataMap skill_data_map;
    MetaData meta_data;

    const static inline std::set<std::string_view> skills_substr_weapon_swap_like = {
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

    const static inline std::set<std::string_view> skills_match_weapon_swap_like = {
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

    const static inline std::set<std::string_view> skills_substr_to_drop = {
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
        // MESMER
        // NECROMANCER
        // REVENANT
    };

    const static inline std::set<std::string_view> skills_match_to_drop = {
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

    const static inline std::set<std::string_view> special_substr_to_gray_out = {
        // GUARDIAN
        "Chapter 1:",
        "Chapter 2:",
        "Chapter 3:",
        "Chapter 4:",
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
        // MESMER
        "Distortion",
        // NECROMANCER
        // REVENANT
    };

    const static inline std::set<std::string_view> special_match_to_gray_out = {
        "Dodge",
        // GUARDIAN
        "Zealot's Flame",
        "Rushing Justice",
        "Symbol of Punishment",
        "Flowing Resolve",
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
        // THIEF
        "Prepare Thousand Needles",
        "Spider Venom",
        "Assassin's Signet",
        "Signet of Malice",
        // ELEMENTALIST
        "Earthquake",
        "Fire Shield",
        "Signet of Restoration",
        "Flame Barrage",
        "Dragon's Tooth",
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
        // REVENANT
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
    };

    const static inline std::set<std::string_view> special_substr_to_remove_duplicates = {
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
    };

    const static inline std::set<std::string_view> easy_mode_drop_match = {
        "Sky Circus",
        "Spark Revolver",
        "Core Reactor Shot",
        "Jade Mortar",
        "Rocket Punch",
        "Rolling Smash",
        "Barrier Burst",
        "Crisis Zone",
        "Power Spike",
    };

    const static inline SkillRules skill_rules = SkillRules{
        skills_substr_weapon_swap_like,
        skills_match_weapon_swap_like,
        skills_substr_to_drop,
        skills_match_to_drop,
        special_substr_to_gray_out,
        special_match_to_gray_out,
        special_substr_to_remove_duplicates,
        easy_mode_drop_match,
    };

    static inline const std::set<uint64_t> berserker_f1_skills = {
        static_cast<uint64_t>(SkillID::EVISCERATE),      // Eviscerate (Axe)
        static_cast<uint64_t>(SkillID::EARTHSHAKER),     // Earthshaker (Hammer)
        static_cast<uint64_t>(SkillID::ARC_DIVIDER),     // Arc Divider (Greatsword)
        static_cast<uint64_t>(SkillID::FLAMING_FLURRY),  // Flaming Flurry (Sword)
        static_cast<uint64_t>(SkillID::DECAPITATE),      // Decapitate (Axe)
        static_cast<uint64_t>(SkillID::RUPTURING_SMASH), // Rupturing Smash (Hammer)
        static_cast<uint64_t>(SkillID::WILD_THROW),      // Wild Throw (Spear)
        static_cast<uint64_t>(SkillID::SCORCHED_EARTH),  // Scorched Earth (LB)
    };

    static inline const std::set<uint64_t> mesmer_weapon_4_skills = {
        static_cast<uint64_t>(SkillID::PHANTASMAL_DUELIST),      // Phantasmal Duelist (Pistol)
        static_cast<uint64_t>(SkillID::TEMPORAL_CURTAIN),        // Temporal Curtain (Focus)
        static_cast<uint64_t>(SkillID::PHANTASMAL_BERSERKER),    // Phantasmal Berserker (Greatsword)
        static_cast<uint64_t>(SkillID::ILLUSIONARY_RIPOSTE),     // Illusionary Riposte (Sword)
        static_cast<uint64_t>(SkillID::THE_PRESTIGE),            // The Prestige (Torch)
        static_cast<uint64_t>(SkillID::SLIPSTREAM),              // Slipstream (Spear)
        static_cast<uint64_t>(SkillID::PHANTASMAL_WHALER),       // Phantasmal Whaler (Trident)
        static_cast<uint64_t>(SkillID::CHAOS_ARMOR),             // Chaos Armor (Staff)
        static_cast<uint64_t>(SkillID::COUNTER_BLADE),           // Counter Blade (Sword)
        static_cast<uint64_t>(SkillID::INTO_THE_VOID),           // Into the Void (Focus)
        static_cast<uint64_t>(SkillID::DEJA_VU),                 // Deja Vu (Shield)
        static_cast<uint64_t>(SkillID::ECHO_OF_MEMORY),          // Echo of Memory (Shield)
        static_cast<uint64_t>(SkillID::PHANTASMAL_SHARPSHOOTER), // 72007 - Phantasmal Sharpshooter (Rifle)
        static_cast<uint64_t>(SkillID::PHANTASMAL_LANCER)        // 72946 - Phantasmal Lancer (Spear)
    };

    static inline const std::set<uint64_t> reset_like_skill = {
        static_cast<uint64_t>(SkillID::CRUSHING_BLOW),
        static_cast<uint64_t>(SkillID::REFRACTION_CUTTER),
        static_cast<uint64_t>(SkillID::REFRACTION_CUTTER_1),
        static_cast<uint64_t>(SkillID::MIND_THE_GAP),
    };

    const static inline std::map<std::string_view, float> skill_cast_time_map = {
        {"Essence Blast", 0.75f},       // rit shroud aa
        {"Life Rend", 0.5f},            // reaper shroud aa
        {"Life Slash", 0.5f},           // reaper shroud aa
        {"Life Reap", 0.5f},            // reaper shroud aa
        {"Tainted Bolts", 0.5f},        // harbinger shroud aa
        {"Hail of Justice", 1.25f},     // guard pistol 4
        {"Cleansing Flame", 3.25f},     // guard torch 5
        {"Rapid Fire", 2.25f},          // ranger lb 2
        {"Barrage", 2.25f},             // ranger lb 5
        {"Weakening Whirlwind", 0.75f}, // thief staff 3
        {"Double Tap", 0.75f},          // thief rilfe 3
        {"Three Round Burst", 1.0f},    // thief rilfe 3
        {"Wild Throw", 3.25f},          // warrrior spear f1
        {"Whirling Defense", 3.25f},    // ranger axe 5
        {"Rifle Burst", 0.5f},          // engi rifle 1
        {"Spatial Surge", 1.0f},        // mesmer gs 1
        {"Flying Cutter", 0.5f},        // mesmer sw 1
        {"Flaming Flurry", 3.25f},      // warrior sw f1
        {"Scorched Earth", 3.25f},      // warrior lb f1
        {"Overload Air", 4.0f},         // f1 air
        {"Overload Fire", 4.0f},        // f1 fire
        {"Flamestrike", 0.75f},         // f1 fire
    };
};
