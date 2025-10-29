#include <windows.h>

#include <commdlg.h>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")
#include <conio.h>
#include <d3d11.h>

#include <algorithm>
#include <d3d11.h>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <list>
#include <map>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

#include "LogData.h"
#include "Settings.h"
#include "Types.h"
#include "TypesUtils.h"

using json = nlohmann::json;

namespace
{
void collect_json(const json &jval,
                  IntNode &node,
                  const bool drop_first_char = false)
{
    if (jval.is_object())
    {
        for (auto it = jval.begin(); it != jval.end(); ++it)
        {
            auto key = std::string{it.key()};
            if (drop_first_char && !key.empty())
            {
                key = key.substr(1);
            }
            collect_json(it.value(), node.children[key]);
        }
    }
    else if (jval.is_array())
    {
        for (size_t i = 0; i < jval.size(); ++i)
        {
            collect_json(jval[i], node.children[std::to_string(i)]);
        }
    }
    else if (jval.is_number_integer())
    {
        node.value = jval.get<int>();
    }
    else if (jval.is_boolean())
    {
        node.value = jval.get<bool>();
    }
    else if (jval.is_number_float())
    {
        node.value = jval.get<float>();
    }
    else if (jval.is_string())
    {
        node.value = jval.get<std::string>();
    }
}

std::string get_cache_remainder(const std::string &cache_url,
                                const std::string &cache)
{
    const auto remainder = cache_url.substr(cache.length());

    return remainder;
}

std::string convert_cache_url(const std::string &cache_url)
{
    static const auto cache1 =
        std::string{"/cache/https_render.guildwars2.com_file_"};
    static const auto cache2 =
        std::string{"/cache/https_wiki.guildwars2.com_images_"};

    if (cache_url.find(cache1) != 0 && cache_url.find(cache2))
        return cache_url;

    auto remainder = std::string{};
    if (cache_url.find(cache1))
        remainder = get_cache_remainder(cache_url, cache1);
    else if (cache_url.find(cache2))
        remainder = get_cache_remainder(cache_url, cache2);
    else
        return cache_url;

    const auto last_underscore = remainder.find_last_of('_');
    if (last_underscore == std::string::npos)
        return cache_url;

    const auto hash = remainder.substr(0, last_underscore);
    const auto icon_id_with_ext = remainder.substr(last_underscore + 1);

    const auto dot_pos = icon_id_with_ext.find_last_of('.');
    const auto icon_id = (dot_pos != std::string::npos)
                             ? icon_id_with_ext.substr(0, dot_pos)
                             : icon_id_with_ext;

    return "https://render.guildwars2.com/file/" + hash + "/" + icon_id +
           ".png";
}


void get_skill_info(const IntNode &node, LogSkillInfoMap &log_skill_info_map)
{
    for (const auto &[icon_id, skill_node] : node.children)
    {
        auto name = std::string{};
        auto icon = std::string{};

        const auto name_it = skill_node.children.find("name");
        if (name_it != skill_node.children.end() &&
            name_it->second.value.has_value())
        {
            if (const auto pval =
                    std::get_if<std::string>(&name_it->second.value.value()))
                name = *pval;
        }

        const auto icon_it = skill_node.children.find("icon");
        if (icon_it != skill_node.children.end() &&
            icon_it->second.value.has_value())
        {
            if (const auto pval =
                    std::get_if<std::string>(&icon_it->second.value.value()))
                icon = convert_cache_url(*pval);
        }

        log_skill_info_map[std::stoi(icon_id)] = {
            name,
            icon,
        };
    }

    log_skill_info_map[-9999] = {
        "Unknown Skill",
        "local://-9999.png",
    };
}


bool remove_skill_if(const RotationStep &current, const RotationStep &previous)
{
    return (current.skill_data.skill_id == previous.skill_data.skill_id &&
            (current.time_of_cast - previous.time_of_cast) < 250);
}

bool is_skill_in_set(const int skill_id,
                     const std::string &skill_name,
                     const std::set<std::string> &set,
                     const bool exact_match = false)
{
    for (const auto &filter_string : set)
    {
        if (exact_match)
        {
            if (skill_name == filter_string)
                return true;
        }
        else
        {
            if (skill_name.find(filter_string) != std::string::npos)
                return true;
        }
    }

    return false;
}

bool get_is_skill_dropped(const SkillData &skill_data,
                          const SkillRules &skill_rules)
{
    const auto is_substr_drop_match =
        is_skill_in_set(skill_data.skill_id,
                        skill_data.name,
                        skill_rules.skills_substr_to_drop);
    const auto is_exact_drop_match =
        is_skill_in_set(skill_data.skill_id,
                        skill_data.name,
                        skill_rules.skills_match_to_drop,
                        true);

    auto drop_skill = is_substr_drop_match || is_exact_drop_match;
    if (!Settings::ShowWeaponSwap)
    {
        const auto drop_substr_swap =
            is_skill_in_set(skill_data.skill_id,
                            skill_data.name,
                            skill_rules.skills_substr_weapon_swap_like);
        const auto drop_match_swap =
            is_skill_in_set(skill_data.skill_id,
                            skill_data.name,
                            skill_rules.skills_match_weapon_swap_like,
                            true);

        drop_skill = is_substr_drop_match || is_exact_drop_match ||
                     drop_substr_swap || drop_match_swap;
    }

    return drop_skill;
}

bool get_is_special_skill(const SkillData &skill_data,
                          const SkillRules &skill_rules)
{
    const auto is_substr_gray_out =
        is_skill_in_set(skill_data.skill_id,
                        skill_data.name,
                        skill_rules.special_substr_to_gray_out);
    const auto is_match_gray_out =
        is_skill_in_set(skill_data.skill_id,
                        skill_data.name,
                        skill_rules.special_match_to_gray_out,
                        true);

    const auto is_special_skill =
        is_substr_gray_out || is_match_gray_out || skill_data.is_heal_skill;

    return is_special_skill;
}

void get_rotation_info(const IntNode &node,
                       const LogSkillInfoMap &log_skill_info_map,
                       RotationSteps &all_rotation_steps,
                       const SkillDataMap &skill_data_map,
                       const SkillRules &skill_rules)
{
    for (const auto &rotation_entry : node.children)
    {
        const auto &rotation_array = rotation_entry.second;

        for (const auto &skill_entry : rotation_array.children)
        {
            const auto &skill_array = skill_entry.second;

            auto icon_id = 0;
            auto duration_ms = 0.0f;
            auto time_of_cast = 0.0f;

            // Get time_of_cast (index 0)
            const auto time_of_cast_it = skill_array.children.find("0");
            if (time_of_cast_it != skill_array.children.end() &&
                time_of_cast_it->second.value.has_value())
            {
                if (const auto pval = std::get_if<float>(
                        &time_of_cast_it->second.value.value()))
                {
                    time_of_cast = *pval;
                }
            }

            // Get icon_id (index 1)
            const auto icon_id_it = skill_array.children.find("1");
            if (icon_id_it != skill_array.children.end() &&
                icon_id_it->second.value.has_value())
            {
                if (const auto pval =
                        std::get_if<int>(&icon_id_it->second.value.value()))
                {
                    icon_id = *pval;
                }
            }

            // Get duration_ms (index 2)
            const auto duration_it = skill_array.children.find("2");
            if (duration_it != skill_array.children.end() &&
                duration_it->second.value.has_value())
            {
                if (const auto pval =
                        std::get_if<int>(&duration_it->second.value.value()))
                {
                    duration_ms = static_cast<float>(*pval);
                }
            }

            // Search for skill name in skill_data_map using icon_id
            auto skill_data = SkillData{};
            for (const auto &[sid, _skill_data] : skill_data_map)
            {
                if (_skill_data.icon_id == icon_id)
                {
                    skill_data = _skill_data;
                    break;
                }
            }

            // fallback to search icon id in _skill_info_map
            if (skill_data.name == "" || skill_data.skill_id == 0)
            {
                auto skip_skill = false;
                for (const auto &[sid, _skill_data] : log_skill_info_map)
                {
                    if (sid == icon_id)
                    {
                        if (_skill_data.name.find("Relic") !=
                                std::string::npos ||
                            _skill_data.name.find("Sigil") != std::string::npos)
                            skip_skill = true;

                        skill_data.skill_id = sid;
                        skill_data.name = _skill_data.name;
                        skill_data.icon_id = icon_id;
                        break;
                    }
                }

                if (skip_skill)
                    continue;
            }

            // TODO
            if (skill_data.skill_id == 0)
            {
                skill_data.skill_id = icon_id;
                skill_data.name = "Unknown Skill";
                skill_data.icon_id = -9999;
                skill_data.recharge_time = -1;
                skill_data.recharge_time_with_alacrity = -1.0f;
                skill_data.cast_time = -1;
                skill_data.cast_time_with_quickness = -1.0f;
            }

            auto drop_skill = get_is_skill_dropped(skill_data, skill_rules);

            if (!drop_skill)
            {
                const auto is_special_skill =
                    get_is_special_skill(skill_data, skill_rules);

                const auto is_duplicate_skill = is_skill_in_set(
                    skill_data.skill_id,
                    skill_data.name,
                    skill_rules.special_substr_to_remove_duplicates);
                const auto was_there_previous =
                    !all_rotation_steps.empty()
                        ? all_rotation_steps.back().skill_data.name ==
                              skill_data.name
                        : false;

                if (is_duplicate_skill && was_there_previous)
                    continue;

                all_rotation_steps.push_back(
                    RotationStep{.time_of_cast = time_of_cast,
                                 .duration_ms = duration_ms,
                                 .skill_data = skill_data,
                                 .is_special_skill = is_special_skill});
            }
        }
    }

    std::sort(all_rotation_steps.begin(),
              all_rotation_steps.end(),
              [](const RotationStep &a, const RotationStep &b) {
                  return a.time_of_cast < b.time_of_cast;
              });
}

SkillDataMap get_skill_data_map(const nlohmann::json &j)
{
    auto skill_data_map = SkillDataMap{};

    for (const auto &[skill_id_str, skill_obj] : j.items())
    {
        auto skill_id = std::stoi(skill_id_str);
        auto skill_data = SkillData{};
        skill_data.skill_id = skill_id;

        if (skill_obj.contains("name") && skill_obj["name"].is_string())
            skill_data.name = skill_obj["name"].get<std::string>();

        if (skill_obj.contains("recharge") && skill_obj["recharge"].is_number())
        {
            skill_data.recharge_time = skill_obj["recharge"].get<int>();
            skill_data.recharge_time_with_alacrity =
                skill_data.recharge_time * 0.8f;
        }
        else
        {
            skill_data.recharge_time = -1;
            skill_data.recharge_time_with_alacrity = -1.0f;
        }

        if (skill_obj.contains("cast_time") &&
            skill_obj["cast_time"].is_number())
        {
            skill_data.cast_time = skill_obj["cast_time"].get<int>();
            skill_data.cast_time_with_quickness = skill_data.cast_time * 0.8f;
        }
        else
        {
            skill_data.cast_time = -1;
            skill_data.cast_time_with_quickness = -1.0f;
        }

        if (skill_obj.contains("is_auto_attack") &&
            skill_obj["is_auto_attack"].is_boolean())
            skill_data.is_auto_attack = skill_obj["is_auto_attack"].get<bool>();
        else
            skill_data.is_auto_attack = false; // Default to false

        if (skill_obj.contains("is_weapon_skill") &&
            skill_obj["is_weapon_skill"].is_boolean())
            skill_data.is_weapon_skill =
                skill_obj["is_weapon_skill"].get<bool>();
        else
            skill_data.is_weapon_skill = false; // Default to false

        if (skill_obj.contains("is_utility_skill") &&
            skill_obj["is_utility_skill"].is_boolean())
            skill_data.is_utility_skill =
                skill_obj["is_utility_skill"].get<bool>();
        else
            skill_data.is_utility_skill = false; // Default to false

        if (skill_obj.contains("is_elite_skill") &&
            skill_obj["is_elite_skill"].is_boolean())
            skill_data.is_elite_skill = skill_obj["is_elite_skill"].get<bool>();
        else
            skill_data.is_elite_skill = false; // Default to false

        if (skill_obj.contains("is_heal_skill") &&
            skill_obj["is_heal_skill"].is_boolean())
            skill_data.is_heal_skill = skill_obj["is_heal_skill"].get<bool>();
        else
            skill_data.is_heal_skill = false; // Default to false

        if (skill_obj.contains("is_profession_skill") &&
            skill_obj["is_profession_skill"].is_boolean())
            skill_data.is_profession_skill =
                skill_obj["is_profession_skill"].get<bool>();
        else
            skill_data.is_profession_skill = false; // Default to false

        skill_data.icon_id = 0; // Default value
        if (skill_obj.contains("icon") && skill_obj["icon"].is_string())
        {
            auto icon_url = skill_obj["icon"].get<std::string>();
            auto last_slash = icon_url.find_last_of('/');
            if (last_slash != std::string::npos)
            {
                auto filename = icon_url.substr(last_slash + 1);
                auto dot_pos = filename.find_last_of('.');
                if (dot_pos != std::string::npos)
                {
                    auto icon_id_str = filename.substr(0, dot_pos);
                    try
                    {
                        skill_data.icon_id = std::stoi(icon_id_str);
                    }
                    catch (const std::exception &)
                    {
                        skill_data.icon_id = 0;
                    }
                }
                else
                {
                    skill_data.icon_id = 9999;
                }
            }
            else
            {
                skill_data.icon_id = 9999;
            }
        }

        if (skill_data.name == "")
        {
            if (!skill_data.is_weapon_skill && !skill_data.is_auto_attack &&
                !skill_data.is_utility_skill && !skill_data.is_elite_skill &&
                !skill_data.is_heal_skill && !skill_data.is_profession_skill)
            {
                skill_data.name = "Weapon Swap";
                skill_data.icon_id = 9999;
            }
            else
            {
                skill_data.name = "Unknown";
                skill_data.icon_id = -99999;
            }
        }

        skill_data_map[skill_id] = skill_data;
    }

    return skill_data_map;
}

MetaData get_metadata(const nlohmann::json &j)
{
    auto metadata = MetaData{};

    const auto &build_meta = j["buildMetadata"];

    if (build_meta.contains("name") && build_meta["name"].is_string())
        metadata.name = build_meta["name"].get<std::string>();

    if (build_meta.contains("url") && build_meta["url"].is_string())
        metadata.url = build_meta["url"].get<std::string>();

    if (build_meta.contains("benchmark_type") &&
        build_meta["benchmark_type"].is_string())
        metadata.benchmark_type =
            build_meta["benchmark_type"].get<std::string>();

    if (build_meta.contains("profession") &&
        build_meta["profession"].is_string())
        metadata.profession = build_meta["profession"].get<std::string>();

    if (build_meta.contains("elite_spec") &&
        build_meta["elite_spec"].is_string())
        metadata.elite_spec = build_meta["elite_spec"].get<std::string>();

    if (build_meta.contains("build_type") &&
        build_meta["build_type"].is_string())
        metadata.build_type = build_meta["build_type"].get<std::string>();

    if (build_meta.contains("url_name") && build_meta["url_name"].is_string())
        metadata.url_name = build_meta["url_name"].get<std::string>();

    if (build_meta.contains("dps_report_url") &&
        build_meta["dps_report_url"].is_string())
        metadata.dps_report_url =
            build_meta["dps_report_url"].get<std::string>();

    if (build_meta.contains("html_file_path") &&
        build_meta["html_file_path"].is_string())
        metadata.html_file_path =
            build_meta["html_file_path"].get<std::string>();

    metadata.elite_spec_id = string_to_elite_spec(metadata.elite_spec);
    metadata.profession_id = string_to_profession(metadata.profession);

    return metadata;
}

std::tuple<LogSkillInfoMap, RotationSteps, MetaData> get_dpsreport_data(
    const nlohmann::json &j,
    const std::filesystem::path &json_path,
    const SkillDataMap &skill_data_map,
    const SkillRules &skill_rules)
{
    const auto rotation_data = j["rotation"];
    const auto skill_data = j["skillMap"];

    auto kv_rotation = IntNode{};
    collect_json(rotation_data, kv_rotation);
    auto kv_skill = IntNode{};
    collect_json(skill_data, kv_skill, true);

    auto log_skill_info_map = LogSkillInfoMap{};
    get_skill_info(kv_skill, log_skill_info_map);
    auto rotation_steps = RotationSteps{};
    get_rotation_info(kv_rotation,
                      log_skill_info_map,
                      rotation_steps,
                      skill_data_map,
                      skill_rules);

    auto metadata = get_metadata(j);

    return std::make_tuple(log_skill_info_map, rotation_steps, metadata);
}

bool DownloadFileFromURL(const std::string &url,
                         const std::filesystem::path &out_path)
{
    const auto hInternet = InternetOpenA("GW2RotaHelper",
                                         INTERNET_OPEN_TYPE_PRECONFIG,
                                         NULL,
                                         NULL,
                                         0);
    if (!hInternet)
    {
        std::cerr << "Failed to open internet connection" << std::endl;
        return false;
    }

    const auto hFile = InternetOpenUrlA(hInternet,
                                        url.c_str(),
                                        NULL,
                                        0,
                                        INTERNET_FLAG_RELOAD,
                                        0);
    if (!hFile)
    {
        std::cerr << "Failed to open URL: " << url << std::endl;
        InternetCloseHandle(hInternet);
        return false;
    }

    auto outFile = std::ofstream{out_path, std::ios::binary};
    if (!outFile.is_open())
    {
        std::cerr << "Failed to create output file: " << out_path << std::endl;
        InternetCloseHandle(hFile);
        InternetCloseHandle(hInternet);
        return false;
    }

    auto buffer = std::array<char, 4096>{};
    auto bytesRead = DWORD{0};
    auto success = true;

    do
    {
        if (InternetReadFile(hFile, buffer.data(), buffer.size(), &bytesRead))
        {
            if (bytesRead > 0)
            {
                outFile.write(buffer.data(), bytesRead);
                if (outFile.fail())
                {
                    std::cerr << "Failed to write to file: " << out_path
                              << std::endl;
                    success = false;
                    break;
                }
            }
        }
        else
        {
            std::cerr << "Failed to read from URL: " << url << std::endl;
            success = false;
            break;
        }
    } while (bytesRead > 0);

    // Cleanup
    outFile.close();
    InternetCloseHandle(hFile);
    InternetCloseHandle(hInternet);

    if (success)
    {
        std::cout << "Successfully downloaded: " << url << " -> " << out_path
                  << std::endl;
    }
    else
    {
        std::filesystem::remove(out_path);
    }

    return success;
}

std::list<std::future<void>> StartDownloadAllSkillIcons(
    const LogSkillInfoMap &log_skill_info_map,
    const std::filesystem::path &img_folder)
{
    std::filesystem::create_directories(img_folder);
    auto futures = std::list<std::future<void>>{};

    for (const auto &[icon_id, info] : log_skill_info_map)
    {
        if (info.icon_url.empty() || info.icon_url.find("local://") == 0)
            continue;

        auto ext{std::string{".png"}};
        const auto dot_pos{info.icon_url.find_last_of('.')};
        if (dot_pos != std::string::npos && dot_pos + 1 < info.icon_url.size())
        {
            ext = info.icon_url.substr(dot_pos);
        }
        const auto out_path{img_folder / (std::to_string(icon_id) + ext)};
        if (std::filesystem::exists(out_path))
            continue;

        std::cout << "Downloading " << info.icon_url << " to " << out_path
                  << std::endl;
        futures.emplace_back(
            std::async(std::launch::async, [url = info.icon_url, out_path]() {
                DownloadFileFromURL(url, out_path);
            }));
    }

    return futures;
}
} // namespace

void RotationRunType::load_data(const std::filesystem::path &json_path,
                                const std::filesystem::path &img_path)
{
    auto file{std::ifstream{json_path}};
    auto j{nlohmann::json{}};
    file >> j;

    skill_data_map.clear();
    log_skill_info_map.clear();
    all_rotation_steps.clear();

    const auto skill_data_json =
        json_path.parent_path().parent_path().parent_path().parent_path() /
        "skills" / "gw2_skills_en.json";
    auto file2{std::ifstream{skill_data_json}};
    auto j2{nlohmann::json{}};
    file2 >> j2;

    skill_data_map = get_skill_data_map(j2);
    auto [_skill_info_map, _bench_all_rotation_steps, _meta_data] =
        get_dpsreport_data(j, json_path, skill_data_map, skill_rules);

    log_skill_info_map = _skill_info_map;
    all_rotation_steps = _bench_all_rotation_steps;
    meta_data = _meta_data;

    restart_rotation();

    futures = StartDownloadAllSkillIcons(log_skill_info_map, img_path);
}

void RotationRunType::pop_bench_rotation_queue()
{
    if (!todo_rotation_steps.empty())
        todo_rotation_steps.pop_front();
}

std::tuple<std::int32_t, std::int32_t, size_t> RotationRunType::
    get_current_rotation_indices() const
{
    constexpr static auto window_size = 10;
    constexpr static auto window_size_left = 2;
    constexpr static auto window_size_right =
        window_size - window_size_left - 1;

    if (todo_rotation_steps.empty())
        return {-1, -1, static_cast<size_t>(-1)};

    const auto num_skills_left =
        static_cast<int64_t>(todo_rotation_steps.size());
    const auto num_total_skills =
        static_cast<int64_t>(all_rotation_steps.size());

    auto current_idx = static_cast<size_t>(num_total_skills - num_skills_left);

    while (current_idx < num_total_skills - 1)
    {
        if (all_rotation_steps[current_idx].is_special_skill)
            ++current_idx;
        else
            break;
    }

    const auto start =
        current_idx - 2 >= 0
            ? static_cast<int32_t>(current_idx - window_size_left)
            : 0;
    const auto end =
        current_idx + window_size < num_total_skills
            ? start > 0 ? static_cast<int32_t>(current_idx + window_size_right)
                        : static_cast<int32_t>(window_size - 1)
            : static_cast<std::int32_t>(num_total_skills - 1);

    return {start, end, current_idx};
}

RotationStep RotationRunType::get_rotation_skill(const size_t idx) const
{
    if (idx < all_rotation_steps.size())
        return all_rotation_steps.at(idx);

    return RotationStep{};
}

void RotationRunType::restart_rotation()
{
    todo_rotation_steps = std::list<RotationStep>(all_rotation_steps.begin(),
                                                  all_rotation_steps.end());
}

bool RotationRunType::is_current_run_done() const
{
    return todo_rotation_steps.empty();
}

void RotationRunType::reset_rotation()
{
    log_skill_info_map.clear();
    all_rotation_steps.clear();
    todo_rotation_steps.clear();
    skill_data_map.clear();
    meta_data = MetaData{};
}
