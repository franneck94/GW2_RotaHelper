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
};
