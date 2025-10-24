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
    RotationInfo get_rotation_skill(const size_t idx) const;
    void restart_rotation();
    void reset_rotation();
    bool is_current_run_done() const;

    std::list<std::future<void>> futures;
    SkillInfoMap skill_info_map;
    RotationInfoVec rotation_vector;
    RotationInfoList bench_rotation_list;
    SkillDataMap skill_data;

    MetaData meta_data;

    const static inline std::set<std::string> skills_to_drop = {
        "Bloodstone Fervor",
        "Blutstein",
        "Waffen Wechsel",
        "Weapon Swap",
        "Relic of",
        "Relikt des",
        "Mushroom King",
        "Dodge",
        // GUARDIAN
        // WARRIOR
        // ENGINEER
        " Kit",
        "Focused Devastation",
        "Explosive Entrance",
        // RANGER
        "Ranger Pet",
        "Unleashed Overbearing Smash (Leap)",
        // THIEF
        // ELEMENTALIST
        "Attunement",
        "Sunspot",
        "Earthen Blast",
        // MESMER
        "Mirage Cloak",
        // NECROMANCER
        "Approaching Doom",
        "Cascading Corruption",
        // REVENANT
        "Invoke Torment",
    };

    const static inline std::set<std::string> special_to_gray_out = {
        // GUARDIAN
        "Chapter 1:",
        "Chapter 2:",
        "Chapter 3:",
        "Chapter 4:",
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
        "Detonate",
        "Devastator", // TODO
        "Rolling Smash",
        "Discharge Array",
        "Sky Circus",
        // RANGER
        // THIEF
        // ELEMENTALIST
        "Earthquake",
        // MESMER
        "Bladesong Distortion",
        "Distortion",
        "Signet of Domination",
        "Signet of Midnight",
        "Signet of Inspiration",
        "Signet of Illusions",
        "Signet of the Ether",
        "Jaunt",
        "Crystal Sands",
        // NECROMANCER
        "Harbinger Shroud",
        "Reaper Shroud",
        "Blood Is Power",
        "Plague Signet",
        // REVENANT
    };

    const static inline std::set<std::string> special_to_remove_duplicates = {
        "Rushing Justice",
    };
};
