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
    SkillInfoMap skill_info_map;
    RotationInfoVec rotation_vector;
    RotationInfoList bench_rotation_list;
    SkillDataMap skill_data;

    MetaData meta_data;

    const static inline std::set<std::string> skills_substr_to_drop = {
        "Bloodstone Fervor",
        "Blutstein",
        "Waffen Wechsel",
        "Weapon Swap",
        "Relic of",
        "Relikt des",
        "Mushroom King",
        "Dodge",
        // GUARDIAN
        "Fire Jurisdiction",
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
        // GUARDIAN
        "Chapter 1:",
        "Chapter 2:",
        "Chapter 3:",
        "Chapter 4:",
        // WARRIOR
        // ENGINEER
        "Detonate",
        // RANGER
        // THIEF
        // ELEMENTALIST
        // MESMER
        "Distortion",
        // NECROMANCER
        "Harbinger Shroud",
        "Reaper's Shroud",
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
        "Devastator",        // TODO
        "Overcharged Shot",  // TODO
        "Spark Revolver",    // XXX
        "Core Reactor Shot", // XXX
        "Jade Mortar",       // XXX
        "Rocket Punch",      // XXX
        "Rolling Smash",
        "Discharge Array",
        "Sky Circus",
        "Radiant Arc",
        // RANGER
        // THIEF
        // ELEMENTALIST
        "Earthquake",
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
        // REVENANT
    };

    const static inline std::set<std::string>
        special_substr_to_remove_duplicates = {
            "Rushing Justice",
    };
};
