#pragma once

#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

#include "Types.h"

void to_lowercase(char *str);

std::string to_lowercase(const std::string &str);

std::pair<std::vector<std::pair<int, const BenchFileInfo *>>, std::set<std::string>> get_file_data_pairs(
    std::vector<BenchFileInfo> &benches_files,
    char *filter_string);

bool load_rotaion_json(const std::filesystem::path &json_path, nlohmann::json &j);

bool load_skill_data_map(const std::filesystem::path &json_path, nlohmann::json &j);

std::string format_build_name(const std::string &raw_name);

std::vector<BenchFileInfo> get_bench_files(const std::filesystem::path &bench_path);

std::map<std::string, KeybindInfo> parse_xml_keybinds(const std::filesystem::path &xml_path);

bool DownloadFile(const std::string &url, const std::filesystem::path &outputPath);

bool ExtractZipFile(const std::filesystem::path &zipPath, const std::filesystem::path &extractPath);

void DownloadAndExtractDataAsync(const std::filesystem::path &addonPath);
