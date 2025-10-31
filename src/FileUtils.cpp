#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

#include "FileUtils.h"
#include "MumbleUtils.h"
#include "TypesUtils.h"

BenchFileInfo::BenchFileInfo(const std::filesystem::path &full,
                             const std::filesystem::path &relative,
                             bool is_header)
    : full_path(full), relative_path(relative), is_directory_header(is_header)
{
    if (is_header)
    {
        display_name = "[+] " + relative.string();
    }
    else
    {
        auto filename = relative.filename().string();
        if (filename.ends_with("_v4.json"))
        {
            filename = filename.substr(0, filename.length() - 8);
        }
        display_name = "    " + filename;
    }
};

std::string to_lowercase(const std::string &str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::pair<std::vector<std::pair<int, const BenchFileInfo *>>,
          std::set<std::string>>
get_file_data_pairs(std::vector<BenchFileInfo> &benches_files,
                    std::string &filter_string)
{
    auto filtered_files = std::vector<std::pair<int, const BenchFileInfo *>>{};
    auto directories_with_matches = std::set<std::string>{};

    if (filter_string.empty())
    {
        // When filter is empty, filter by current character's profession
        const auto profession = get_current_profession_name();
        auto current_profession = to_lowercase(profession);

        if (current_profession.empty())
        {
            // If no profession available, show all files
            for (int n = 0; n < benches_files.size(); n++)
                filtered_files.emplace_back(n, &benches_files[n]);
        }
        else
        {
            // Get elite specs for this profession
            auto profession_id = string_to_profession(current_profession);
            auto elite_specs = get_elite_specs_for_profession(profession_id);

            // First pass: find files that match the profession or elite specs and collect their directories
            for (int n = 0; n < benches_files.size(); n++)
            {
                const auto &file_info = benches_files[n];

                if (!file_info.is_directory_header)
                {
                    auto display_lower = to_lowercase(file_info.display_name);
                    auto path_lower =
                        to_lowercase(file_info.relative_path.string());

                    bool matches = false;

                    // Check if file matches current profession
                    if ((display_lower.find(current_profession) !=
                         std::string::npos) ||
                        (path_lower.find(current_profession) !=
                         std::string::npos))
                    {
                        matches = true;
                    }

                    // Check if file matches any elite spec for this profession
                    if (!matches)
                    {
                        for (const auto &elite_spec : elite_specs)
                        {
                            if (display_lower.find(elite_spec) !=
                                    std::string::npos ||
                                path_lower.find(elite_spec) !=
                                    std::string::npos)
                            {
                                matches = true;
                                break;
                            }
                        }
                    }

                    if (matches)
                    {
                        const auto parent_dir =
                            file_info.relative_path.parent_path().string();
                        if (!parent_dir.empty() && parent_dir != ".")
                        {
                            directories_with_matches.insert(parent_dir);
                        }
                    }
                }
            }

            // Second pass: add directory headers and matching files to filtered list
            for (int n = 0; n < benches_files.size(); n++)
            {
                const auto &file_info = benches_files[n];

                if (file_info.is_directory_header)
                {
                    if (directories_with_matches.count(
                            file_info.relative_path.string()) > 0)
                    {
                        filtered_files.emplace_back(n, &file_info);
                    }
                }
                else
                {
                    auto display_lower = to_lowercase(file_info.display_name);
                    auto path_lower =
                        to_lowercase(file_info.relative_path.string());

                    bool matches = false;

                    // Check if file matches current profession
                    if (display_lower.find(current_profession) !=
                            std::string::npos ||
                        path_lower.find(current_profession) !=
                            std::string::npos)
                    {
                        matches = true;
                    }

                    // Check if file matches any elite spec for this profession
                    if (!matches)
                    {
                        for (const auto &elite_spec : elite_specs)
                        {
                            if (display_lower.find(elite_spec) !=
                                    std::string::npos ||
                                path_lower.find(elite_spec) !=
                                    std::string::npos)
                            {
                                matches = true;
                                break;
                            }
                        }
                    }

                    if (matches)
                    {
                        filtered_files.emplace_back(n, &file_info);
                    }
                }
            }
        }

        return std::make_pair(filtered_files, directories_with_matches);
    }

    // First pass: find all files that match and collect their directories
    for (int n = 0; n < benches_files.size(); n++)
    {
        const auto &file_info = benches_files[n];

        if (!file_info.is_directory_header)
        {
            auto display_lower = to_lowercase(file_info.display_name);

            if (display_lower.find(filter_string) != std::string::npos)
            {
                const auto parent_dir =
                    file_info.relative_path.parent_path().string();
                if (!parent_dir.empty() && parent_dir != ".")
                {
                    directories_with_matches.insert(parent_dir);
                }
            }
        }
    }

    // Second pass: add directory headers and matching files to filtered list
    for (int n = 0; n < benches_files.size(); n++)
    {
        const auto &file_info = benches_files[n];

        if (file_info.is_directory_header)
        {
            if (directories_with_matches.count(
                    file_info.relative_path.string()) > 0)
            {
                filtered_files.emplace_back(n, &file_info);
            }
        }
        else
        {
            auto display_lower = to_lowercase(file_info.display_name);

            if (display_lower.find(filter_string) != std::string::npos)
            {
                filtered_files.emplace_back(n, &file_info);
            }
        }
    }

    return std::make_pair(filtered_files, directories_with_matches);
}


bool load_rotaion_json(const std::filesystem::path &json_path,
                       nlohmann::json &j)
{
    try
    {
        auto file{std::ifstream{json_path}};
        if (!file.is_open())
        {
            std::cerr << "Error: Could not open rotation data file: "
                      << json_path << std::endl;
            return false;
        }

        auto j{nlohmann::json{}};
        file >> j;
    }
    catch (const nlohmann::json::exception &e)
    {
        std::cerr << "Error parsing rotation data JSON: " << e.what()
                  << std::endl;
        return false;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error loading rotation data: " << e.what() << std::endl;
        return false;
    }

    return true;
}

bool load_skill_data_map(const std::filesystem::path &json_path,
                         nlohmann::json &j)
{
    const auto skill_data_json =
        json_path.parent_path().parent_path().parent_path().parent_path() /
        "skills" / "gw2_skills_en.json";

    try
    {
        auto file2{std::ifstream{skill_data_json}};
        if (!file2.is_open())
        {
            std::cerr << "Warning: Could not open skill data file: "
                      << skill_data_json << std::endl;
            return false;
        }

        file2 >> j;
    }
    catch (const nlohmann::json::exception &e)
    {
        std::cerr << "Error parsing skill data JSON: " << e.what() << std::endl;
        return false;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error loading skill data: " << e.what() << std::endl;
        return false;
    }

    return true;
}
