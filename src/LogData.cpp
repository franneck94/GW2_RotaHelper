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
#include "Types.h"

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

void get_skill_info(const IntNode &node, SkillInfoMap &skill_info_map)
{
    for (const auto &[icon_id, skill_node] : node.children)
    {
        auto name = std::string{};
        auto icon = std::string{};
        auto trait_proc = true;
        auto gear_proc = true;

        const auto name_it = skill_node.children.find("name");
        if (name_it != skill_node.children.end() &&
            name_it->second.value.has_value())
        {
            if (const auto pval =
                    std::get_if<std::string>(&name_it->second.value.value()))
                name = *pval;
        }

        if (name.find("Relic of") != std::string::npos ||
            name.find("Sigil of") != std::string::npos)
            continue;

        const auto icon_it = skill_node.children.find("icon");
        if (icon_it != skill_node.children.end() &&
            icon_it->second.value.has_value())
        {
            if (const auto pval =
                    std::get_if<std::string>(&icon_it->second.value.value()))
                icon = convert_cache_url(*pval);
        }

        const auto trait_v12_it = skill_node.children.find("traitProc");
        if (trait_v12_it != skill_node.children.end() &&
            trait_v12_it->second.value.has_value())
        {
            if (const auto pval =
                    std::get_if<bool>(&trait_v12_it->second.value.value()))
                trait_proc = *pval;
        }
        const auto trait_v3_it = skill_node.children.find("isTraitProc");
        if (trait_v3_it != skill_node.children.end() &&
            trait_v3_it->second.value.has_value())
        {
            if (const auto pval =
                    std::get_if<bool>(&trait_v3_it->second.value.value()))
                trait_proc = *pval;
        }

        const auto gear_v12_it = skill_node.children.find("gearProc");
        if (gear_v12_it != skill_node.children.end() &&
            gear_v12_it->second.value.has_value())
        {
            if (const auto pval =
                    std::get_if<bool>(&gear_v12_it->second.value.value()))
                gear_proc = *pval;
        }
        const auto gear_v3_it = skill_node.children.find("isGearProc");
        if (gear_v3_it != skill_node.children.end() &&
            gear_v3_it->second.value.has_value())
        {
            if (const auto pval =
                    std::get_if<bool>(&gear_v3_it->second.value.value()))
                gear_proc = *pval;
        }

        skill_info_map[std::stoi(icon_id)] = {name,
                                              icon,
                                              trait_proc,
                                              gear_proc};
    }
}

bool remove_skill_if(const RotationInfo &current, const RotationInfo &previous)
{
    return (current.skill_id == previous.skill_id &&
            (current.cast_time - previous.cast_time) < 250);
}

bool is_special_case(const int skill_id, const std::string &skill_name)
{
    if (skill_name.find("Bloodstone Fervor") != std::string::npos ||
        skill_name.find("Weapon Swap") != std::string::npos ||
        skill_name.find("Detonate ") != std::string::npos ||
        skill_name.find(" Kit") != std::string::npos)
        return true;

    return false;
}

void get_rotation_info(const IntNode &node,
                       const SkillInfoMap &skill_info_map,
                       RotationInfoVec &rotation_vector,
                       const SkillDataMap &skill_data_map)
{
    for (const auto &rotation_entry : node.children)
    {
        const auto &rotation_array = rotation_entry.second;

        for (const auto &skill_entry : rotation_array.children)
        {
            const auto &skill_array = skill_entry.second;

            auto icon_id = 0;
            auto duration_ms = 0.0f;
            auto cast_time = 0.0f;

            // Get cast_time (index 0)
            const auto cast_time_it = skill_array.children.find("0");
            if (cast_time_it != skill_array.children.end() &&
                cast_time_it->second.value.has_value())
            {
                if (const auto pval =
                        std::get_if<float>(&cast_time_it->second.value.value()))
                {
                    cast_time = *pval;
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

            auto skill_name = std::string{"Unknown Skill"};
            const auto skill_info_it = skill_info_map.find(icon_id);
            if (skill_info_it != skill_info_map.end())
            {
                skill_name = skill_info_it->second.name;
            }
            else
            {
                continue;
            }

            auto skill_id = 0;
            for (const auto &[sid, skill_data] : skill_data_map)
            {
                if (skill_data.icon_id == icon_id)
                {
                    skill_id = sid;
                    break;
                }
            }

            if (!skill_info_it->second.gear_proc &&
                !skill_info_it->second.trait_proc &&
                !is_special_case(skill_id, skill_name))
            {
                rotation_vector.push_back(RotationInfo{
                    .icon_id = icon_id,
                    .skill_id = skill_id,
                    .cast_time = cast_time,
                    .duration_ms = duration_ms,
                    .skill_name = skill_name,
                });
            }
        }
    }

    std::sort(rotation_vector.begin(),
              rotation_vector.end(),
              [](const RotationInfo &a, const RotationInfo &b) {
                  return a.cast_time < b.cast_time;
              });
}

SkillDataMap get_skill_data(const nlohmann::json &j)
{
    auto skill_data_map = SkillDataMap{};

    for (const auto &[skill_id_str, skill_obj] : j.items())
    {
        auto skill_id = std::stoi(skill_id_str);
        auto skill_data = SkillData{};

        if (skill_obj.contains("name") && skill_obj["name"].is_string())
            skill_data.name = skill_obj["name"].get<std::string>();

        if (skill_obj.contains("recharge") && skill_obj["recharge"].is_number())
            skill_data.recharge = skill_obj["recharge"].get<int>();

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
            }
        }

        skill_data_map[skill_id] = skill_data;
    }

    return skill_data_map;
}

std::tuple<SkillInfoMap, RotationInfoVec> get_dpsreport_data(
    const nlohmann::json &j,
    const std::filesystem::path &json_path,
    const SkillDataMap &skill_data_map)
{
    const auto rotation_data = j["rotation"];
    const auto skill_data = j["skillMap"];

    auto kv_rotation = IntNode{};
    collect_json(rotation_data, kv_rotation);
    auto kv_skill = IntNode{};
    collect_json(skill_data, kv_skill, true);

    auto skill_info_map = SkillInfoMap{};
    get_skill_info(kv_skill, skill_info_map);
    auto rotation_info_vec = RotationInfoVec{};
    get_rotation_info(kv_rotation,
                      skill_info_map,
                      rotation_info_vec,
                      skill_data_map);

    return std::make_tuple(skill_info_map, rotation_info_vec);
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
    const SkillInfoMap &skill_info_map,
    const std::filesystem::path &img_folder)
{
    std::filesystem::create_directories(img_folder);
    auto futures = std::list<std::future<void>>{};

    for (const auto &[icon_id, info] : skill_info_map)
    {
        if (info.icon_url.empty())
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

void RotationRun::load_data(const std::filesystem::path &json_path,
                            const std::filesystem::path &img_path)
{
    auto file{std::ifstream{json_path}};
    auto j{nlohmann::json{}};
    file >> j;

    const auto skill_data_json =
        json_path.parent_path().parent_path().parent_path().parent_path() /
        "skills" / "gw2_skills_en.json";
    auto file2{std::ifstream{skill_data_json}};
    auto j2{nlohmann::json{}};
    file2 >> j2;

    skill_data = get_skill_data(j2);
    auto [_skill_info_map, _bench_rotation_vector] =
        get_dpsreport_data(j, json_path, skill_data);

    skill_info_map = _skill_info_map;
    rotation_vector = _bench_rotation_vector;

    restart_rotation();

    futures = StartDownloadAllSkillIcons(skill_info_map, img_path);
}

void RotationRun::pop_bench_rotation_queue()
{
    if (!bench_rotation_list.empty())
    {
        bench_rotation_list.pop_front();
    }
}

std::tuple<int, int, size_t> RotationRun::get_current_rotation_indices() const
{
    constexpr static auto window_size = 10;
    constexpr static auto window_size_left = 2;
    constexpr static auto window_size_right =
        window_size - window_size_left - 1;

    if (bench_rotation_list.empty())
        return {-1, -1, -1};

    const auto num_skills_left =
        static_cast<int64_t>(bench_rotation_list.size());
    const auto num_total_skills = static_cast<int64_t>(rotation_vector.size());
    const auto current_idx =
        static_cast<int64_t>(num_total_skills - num_skills_left);
    const auto start =
        current_idx - 2 >= 0
            ? static_cast<int32_t>(current_idx - window_size_left)
            : 0;
    const auto end =
        current_idx + window_size < num_total_skills
            ? start > 0 ? static_cast<int32_t>(current_idx + window_size_right)
                        : static_cast<int32_t>(window_size - 1)
            : num_total_skills - 1;

    return {start, end, current_idx};
}

RotationInfo RotationRun::get_rotation_skill(const size_t idx) const
{
    if (idx < rotation_vector.size())
        return rotation_vector.at(idx);

    return RotationInfo{};
}

void RotationRun::restart_rotation()
{
    bench_rotation_list =
        std::list<RotationInfo>(rotation_vector.begin(), rotation_vector.end());
}

bool RotationRun::is_current_run_done() const
{
    return bench_rotation_list.empty();
}
