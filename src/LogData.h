#pragma once

#include <d3d11.h>

#include <filesystem>
#include <future>
#include <iostream>
#include <map>
#include <optional>
#include <queue>
#include <string>
#include <variant>
#include <vector>

#include "nlohmann/json.hpp"

using json = nlohmann::json;

struct SkillInfo
{
    std::string name;
    std::string icon_url;
    bool trait_proc;
    bool gear_proc;
};

using SkillInfoMap = std::map<int, SkillInfo>;
using LogDataTypes = std::variant<int, float, bool, std::string>;

struct IntNode
{
    std::map<std::string, IntNode> children;
    std::optional<LogDataTypes> value;
};

enum class RotationStatus
{
    UNKNOWN,
    REDUCED,
    CANCEL,
    FULL,
    INSTANT
};

struct RotationInfo
{
    int skill_id;
    float cast_time;
    float duration_ms;
    float unk;
    std::string skill_name;
    RotationStatus status;
};

using RotationInfoVec = std::vector<RotationInfo>;
using RotationInfoQueue = std::queue<RotationInfo>;
using TextureMap = std::unordered_map<int, ID3D11ShaderResourceView *>;

class RotationRun
{
public:
    void load_data(const std::filesystem::path &json_path, const std::filesystem::path &img_path, ID3D11Device *pd3dDevice);

    void print_rotation_info() const;

    void print_skill_info() const;

    void pop_bench_rotation_queue();

    std::tuple<int, int, size_t> get_current_rotation_indices() const;

    RotationInfo get_rotation_skill(const size_t idx) const;

    void restart_rotation();

    bool is_current_run_done() const;

    std::queue<std::future<void>> futures;
    SkillInfoMap skill_info_map;
    RotationInfoVec rotation_vector;
    RotationInfoQueue bench_rotation_queue;
};
