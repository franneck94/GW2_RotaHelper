#pragma once

#include <filesystem>
#include <set>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

#include "Types.h"

struct BenchFileInfo
{
    std::filesystem::path full_path;
    std::filesystem::path relative_path;
    std::string display_name;
    bool is_directory_header;

    BenchFileInfo(const std::filesystem::path &full,
                  const std::filesystem::path &relative,
                  bool is_header = false);
};

std::string to_lowercase(const std::string &str);

std::pair<std::vector<std::pair<int, const BenchFileInfo *>>,
          std::set<std::string>>
get_file_data_pairs(std::vector<BenchFileInfo> &benches_files,
                    std::string &filter_string);

bool load_rotaion_json(const std::filesystem::path &json_path,
                       nlohmann::json &j);

bool load_skill_data_map(const std::filesystem::path &json_path,
                         nlohmann::json &j);
