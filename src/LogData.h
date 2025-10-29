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
#include <variant>
#include <vector>

#include "nlohmann/json.hpp"

#include "Types.h"

using json = nlohmann::json;

struct SkillRules
{
    const std::set<std::string> &skills_substr_weapon_swap_like;
    const std::set<std::string> &skills_match_weapon_swap_like;
    const std::set<std::string> &skills_substr_to_drop;
    const std::set<std::string> &skills_match_to_drop;
    const std::set<std::string> &special_substr_to_gray_out;
    const std::set<std::string> &special_match_to_gray_out;
    const std::set<std::string> &special_substr_to_remove_duplicates;
};

class RotationRunType
{
public:
    void load_data(const std::filesystem::path &json_path,
                   const std::filesystem::path &img_path);

    void pop_bench_rotation_queue();
    std::tuple<int, int, size_t> get_current_rotation_indices() const;
    RotationStep get_rotation_skill(const size_t idx) const;
    void restart_rotation();
    void reset_rotation();
    bool is_current_run_done() const;

    std::list<std::future<void>> futures;
    LogSkillInfoMap log_skill_info_map;
    RotationSteps all_rotation_steps;
    RotationStepsList todo_rotation_steps;
    SkillDataMap skill_data_map;
    MetaData meta_data;

    const static inline std::set<std::string> skills_substr_weapon_swap_like = {
        "Weapon Swap",
        // GUARDIAN
        // WARRIOR
        // ENGINEER
        " Kit",
        // RANGER
        "Ranger Pet",
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

    const static inline std::set<std::string> skills_match_weapon_swap_like = {
        // GUARDIAN
        // WARRIOR
        // ENGINEER
        "Flamethrower",
        // RANGER
        // THIEF
        // ELEMENTALIST
        // MESMER
        // NECROMANCER
        // REVENANT
    };

    const static inline std::set<std::string> skills_substr_to_drop = {
        "Bloodstone Fervor",
        "Relic of",
        "Mushroom King",
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

    const static inline std::set<std::string> skills_match_to_drop = {
        // GUARDIAN
        // WARRIOR
        // ENGINEER
        "Focused Devastation",
        "Explosive Entrance",
        "Spark Revolver",
        "Fire Rocket Barrage",
        "Orbital Command Strike",
        "Bomb",
        // RANGER
        "Unleashed Overbearing Smash (Leap)",
        // THIEF
        // ELEMENTALIST
        "Sunspot",
        "Earthen Blast",
        // MESMER
        "Mirage Cloak",
        // NECROMANCER
        "Approaching Doom",
        "Chilling Nova",
        "Cascading Corruption",
        // REVENANT
        "Invoke Torment",
    };

    const static inline std::set<std::string> special_substr_to_gray_out = {
        "Dodge",
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
        // RANGER
        // THIEF
        // ELEMENTALIST
        // MESMER
        "Distortion",
        // NECROMANCER
        // REVENANT
    };

    const static inline std::set<std::string> special_match_to_gray_out = {
        // GUARDIAN
        "Zealot's Flame",
        // WARRIOR
        "Sheathe Gunsaber",
        "Unsheathe Gunsaber",
        "Dragon Trigger",
        "Flow Stabilizer",
        "Overcharged Cartridges",
        "Tactical Reload",
        "Arcing Slice",
        "Blood Reckoning",
        // ENGINEER
        "Devastator",       // TODO
        "Overcharged Shot", // TODO
        "Spark Revolver",
        "Core Reactor Shot",
        "Jade Mortar",
        "Rocket Punch",
        "Rolling Smash",
        "Discharge Array",
        "Sky Circus",
        "Radiant Arc",
        // RANGER
        // THIEF
        "Prepare Thousand Needles",
        "Spider Venom",
        "Assassin's Signet",
        "Signet of Malice",
        // ELEMENTALIST
        "Earthquake",
        "Fire Shield",
        "Signet of Restoration",
        // MESMER
        "Signet of Domination",
        "Signet of Midnight",
        "Signet of Inspiration",
        "Signet of Illusions",
        "Signet of the Ether",
        "Jaunt",
        "Crystal Sands",
        // NECROMANCER
        "Plague Signet",
        "Elixir of Promise",
        "Distress",
        "Garish Pillar",
        "Sandstorm Shroud",
        "Perforate",
        "Soul Shards",
        "Nightfall",
        // REVENANT
        "Legendary Entity Stance",
        "Legendary Alliance Stance",
        "Legendary Renegade Stance",
        "Legendary Demon Stance",
        "Legendary Dwarf Stance",
        "Legendary Centaur Stance",
        "Legendary Assassin Stance",
        "Legendary Dragon Stance",
        "Facet of Chaos",
    };

    const static inline std::set<std::string>
        special_substr_to_remove_duplicates = {
            "Rushing Justice",
            "Devastator", // TODO
    };

    static inline const std::set<uint64_t> berserker_f1_skills = {
        14353, // Eviscerate (Axe)
        14367, // Flurry (Sword)
        14375, // Arcing Slice (Greatsword)
        14387, // Earthshaker (Hammer)
        14396, // Kill Shot (Rifle)
        14414, // Skull Crack (Mace)
        14443, // Whirling Strike (Spear)
        14469, // Forceful Shot (Speargun)
        14506, // Combustive Shot (Longbow)
        29644, // Gun Flame (Rifle)
        29679, // Skull Grinder (Mace)
        29852, // Arc Divider (Greatsword)
        29923, // Scorched Earth (Longbow)
        30682, // Flaming Flurry (Sword)
        30851, // Decapitate (Axe)
        30879, // Rupturing Smash (Hammer)
        30989, // Burning Shackles (Speargun)
        31048, // Wild Whirl (Spear)
        45252, // Breaching Strike (Dagger)
        62745, // Unsheathe Gunsaber (None)
        62861, // Sheathe Gunsaber (None)
        69290, // Slicing Maelstrom (Dagger)
        71875, // Rampart Splitter (Staff)
        71922, // Path to Victory (Staff)
        72911, // Harrier's Toss (Spear)
        73103  // Wild Throw (Spear)
    };

    static inline const std::set<uint64_t> mesmer_weapon_4_skills = {
        10175, // Phantasmal Duelist (Pistol)
        10186, // Temporal Curtain (Focus)
        10221, // Phantasmal Berserker (Greatsword)
        10280, // Illusionary Riposte (Sword)
        10285, // The Prestige (Torch)
        10325, // Slipstream (Spear)
        10328, // Phantasmal Whaler (Trident)
        10331, // Chaos Armor (Staff)
        10358, // Counter Blade (Sword)
        10363, // Into the Void (Focus)
        29649, // Deja Vu (Shield)
        30769, // Echo of Memory (Shield)
        72007, // Phantasmal Sharpshooter (Rifle)
        72946  // Phantasmal Lancer (Spear)
    };
};
