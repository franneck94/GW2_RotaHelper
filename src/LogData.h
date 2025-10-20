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

    std::set<std::string> skills_to_drop = {
        "Bloodstone Fervor",
        "Blutstein",
        "Waffen Wechsel",
        "Weapon Swap",
        "Relic of",
        "Relikt des",
        "Mushroom King",
        " Kit",
        "Dodge",
        "Ranger Pet",
        "Cascading Corruption",
        "Mirage Cloak",
    };

    std::set<std::string> special_to_gray_out = {
        "Attunement",
        "Detonate",
        "Approaching Doom",
        "Bladesong Distortion",
        "Distortion",
        "Chapter 1:",
        "Chapter 2:",
        "Chapter 3:",
        "Chapter 4:",
    };

    std::set<std::string> special_to_remove_duplicates = {
        "Rushing Justice",
    };
};
