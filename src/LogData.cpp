#include <windows.h>

#include <commdlg.h>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")
#include <conio.h>
#include <d3d11.h>

#include <d3d11.h>
#include <filesystem>
#include <future>
#include <iostream>
#include <map>
#include <queue>
#include <string>
#include <vector>
#include <fstream>

#include "nlohmann/json.hpp"

#include "LogData.h"

using json = nlohmann::json;

namespace
{
    void collect_json(const json &jval, IntNode &node, const bool drop_first_char = false)
    {
        if (jval.is_object())
        {
            for (auto it = jval.begin(); it != jval.end(); ++it)
            {
                std::string key = it.key();
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

    std::string ConvertCacheUrlToRenderUrl(const std::string &cache_url)
    {
        if (cache_url.find("/cache/https_render.guildwars2.com_file_") != 0)
            return cache_url;

        std::string remainder = cache_url.substr(std::string("/cache/https_render.guildwars2.com_file_").length());

        size_t last_underscore = remainder.find_last_of('_');
        if (last_underscore == std::string::npos)
        {
            return cache_url;
        }

        std::string hash = remainder.substr(0, last_underscore);
        std::string skill_id_with_ext = remainder.substr(last_underscore + 1);

        size_t dot_pos = skill_id_with_ext.find_last_of('.');
        std::string skill_id = (dot_pos != std::string::npos) ? skill_id_with_ext.substr(0, dot_pos) : skill_id_with_ext;

        return "https://render.guildwars2.com/file/" + hash + "/" + skill_id + ".png";
    }

    void get_skill_info(const IntNode &node, SkillInfoMap &skill_info_map)
    {
        for (const auto &[skill_id, skill_node] : node.children)
        {
            std::string name, icon;
            auto name_it = skill_node.children.find("name");
            if (name_it != skill_node.children.end() && name_it->second.value.has_value())
            {
                if (auto pval = std::get_if<std::string>(&name_it->second.value.value()))
                {
                    name = *pval;
                }
            }
            auto icon_it = skill_node.children.find("icon");
            if (icon_it != skill_node.children.end() && icon_it->second.value.has_value())
            {
                if (auto pval = std::get_if<std::string>(&icon_it->second.value.value()))
                {
                    icon = ConvertCacheUrlToRenderUrl(*pval);
                }
            }
            skill_info_map[std::stoi(skill_id)] = {name, icon};
        }
    }

    void get_rotation_info(const IntNode &node, const SkillInfoMap &skill_info_map, RotationInfoVec &rotation_vector)
    {
        // The rotation data is structured as an array of arrays
        // Each inner array contains: [cast_time, skill_id, duration, ?, ?]
        for (const auto &rotation_entry : node.children)
        {
            const auto &rotation_array = rotation_entry.second;

            for (const auto &skill_entry : rotation_array.children)
            {
                const auto &skill_array = skill_entry.second;

                // Extract values from the array: [cast_time, skill_id, duration, ?, ?]
                float cast_time = 0.0f;
                int skill_id = 0;
                int duration = 0;

                // Get cast_time (index 0)
                auto cast_time_it = skill_array.children.find("0");
                if (cast_time_it != skill_array.children.end() && cast_time_it->second.value.has_value())
                {
                    if (auto pval = std::get_if<float>(&cast_time_it->second.value.value()))
                    {
                        cast_time = *pval;
                    }
                }

                // Get skill_id (index 1)
                auto skill_id_it = skill_array.children.find("1");
                if (skill_id_it != skill_array.children.end() && skill_id_it->second.value.has_value())
                {
                    if (auto pval = std::get_if<int>(&skill_id_it->second.value.value()))
                    {
                        skill_id = *pval;
                    }
                }

                // Get duration (index 2)
                auto duration_it = skill_array.children.find("2");
                if (duration_it != skill_array.children.end() && duration_it->second.value.has_value())
                {
                    if (auto pval = std::get_if<int>(&duration_it->second.value.value()))
                    {
                        duration = *pval;
                    }
                }

                // Skip invalid entries
                if (skill_id <= 0)
                    continue;

                // Get skill name from skill map
                std::string skill_name = "Unknown Skill";
                auto skill_info_it = skill_info_map.find(skill_id);
                if (skill_info_it != skill_info_map.end())
                {
                    skill_name = skill_info_it->second.name;
                }

                // Convert cast_time from seconds to milliseconds
                int cast_time_ms = static_cast<int>(cast_time * 1000);

                rotation_vector.push_back(RotationInfo{
                    .skill_id = skill_id,
                    .cast_time = cast_time_ms,
                    .duration = duration,
                    .idle_time = 0,
                    .skill_name = skill_name});
            }
        }

        // Sort by cast time
        std::sort(rotation_vector.begin(), rotation_vector.end(), [](const RotationInfo &a, const RotationInfo &b)
                  { return a.cast_time < b.cast_time; });

        // Calculate idle times
        for (size_t i = 1; i < rotation_vector.size(); ++i)
        {
            rotation_vector[i].idle_time = rotation_vector[i].cast_time - (rotation_vector[i - 1].cast_time + rotation_vector[i - 1].duration);
        }
    }

    std::tuple<SkillInfoMap, RotationInfoVec> get_dpsreport_data(const nlohmann::json &j)
    {
        const auto rotation_data = j["players"][0]["details"]["rotation"]; // TODO: player index
        const auto skill_data = j["skillMap"];

        auto kv_rotation = IntNode{};
        collect_json(rotation_data, kv_rotation);
        auto kv_skill = IntNode{};
        collect_json(skill_data, kv_skill, true);

        SkillInfoMap skill_info_map;
        get_skill_info(kv_skill, skill_info_map);
        RotationInfoVec rotation_info_vec;
        get_rotation_info(kv_rotation, skill_info_map, rotation_info_vec);

        return std::make_tuple(skill_info_map, rotation_info_vec);
    }

    bool DownloadFileFromURL(const std::string &url, const std::filesystem::path &out_path)
    {
        HINTERNET hInternet = InternetOpenA("GW2RotaHelper", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
        if (!hInternet)
        {
            std::cerr << "Failed to open internet connection" << std::endl;
            return false;
        }

        HINTERNET hFile = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
        if (!hFile)
        {
            std::cerr << "Failed to open URL: " << url << std::endl;
            InternetCloseHandle(hInternet);
            return false;
        }

        std::ofstream outFile(out_path, std::ios::binary);
        if (!outFile.is_open())
        {
            std::cerr << "Failed to create output file: " << out_path << std::endl;
            InternetCloseHandle(hFile);
            InternetCloseHandle(hInternet);
            return false;
        }

        char buffer[4096];
        DWORD bytesRead = 0;
        bool success = true;

        do
        {
            if (InternetReadFile(hFile, buffer, sizeof(buffer), &bytesRead))
            {
                if (bytesRead > 0)
                {
                    outFile.write(buffer, bytesRead);
                    if (outFile.fail())
                    {
                        std::cerr << "Failed to write to file: " << out_path << std::endl;
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
            std::cout << "Successfully downloaded: " << url << " -> " << out_path << std::endl;
        }
        else
        {
            std::filesystem::remove(out_path);
        }

        return success;
    }

    std::queue<std::future<void>> StartDownloadAllSkillIcons(
        const SkillInfoMap &skill_info_map,
        const std::filesystem::path &img_folder)
    {
        std::filesystem::create_directories(img_folder);
        std::queue<std::future<void>> futures;

        for (const auto &[skill_id, info] : skill_info_map)
        {
            if (info.icon_url.empty())
                continue;

            std::string ext = ".png";
            const auto dot_pos = info.icon_url.find_last_of('.');
            if (dot_pos != std::string::npos && dot_pos + 1 < info.icon_url.size())
            {
                ext = info.icon_url.substr(dot_pos);
            }
            std::filesystem::path out_path = img_folder / (std::to_string(skill_id) + ext);
            if (std::filesystem::exists(out_path))
                continue;

            std::cout << "Downloading " << info.icon_url << " to " << out_path << std::endl;
            futures.push(std::async(std::launch::async, [url = info.icon_url, out_path]()
                                    { DownloadFileFromURL(url, out_path); }));
        }

        return futures;
    }
} // namespace

void RotationRun::load_data(const std::filesystem::path &json_path, const std::filesystem::path &img_path, ID3D11Device *pd3dDevice)
{
    std::ifstream file(json_path);
    nlohmann::json j;
    file >> j;

    auto [_skill_info_map, _bench_rotation_vector] = get_dpsreport_data(j);
    skill_info_map = std::move(_skill_info_map);
    rotation_vector = std::move(_bench_rotation_vector);

    restart_rotation();

    futures = StartDownloadAllSkillIcons(skill_info_map, img_path);
}

void RotationRun::print_rotation_info() const
{
    for (const auto &info : rotation_vector)
    {
        std::cout << "Skill ID: " << info.skill_id
                  << ", Name: " << info.skill_name
                  << ", CastTime: " << info.cast_time
                  << ", Duration: " << info.duration
                  << ", IdleTime: " << info.idle_time << std::endl;
    }
}

void RotationRun::print_skill_info() const
{
    for (const auto &[skill_id, info] : skill_info_map)
    {
        std::cout << "Skill ID: " << skill_id
                  << ", Name: " << info.name
                  << ", Icon: " << info.icon_url << std::endl;
    }
}

void RotationRun::pop_bench_rotation_queue()
{
    if (!bench_rotation_queue.empty())
    {
        bench_rotation_queue.pop();
    }
}

std::tuple<int, int, size_t> RotationRun::get_current_rotation_indices() const
{
    if (bench_rotation_queue.empty())
        return {-1, -1, -1};

    const auto queue_size = bench_rotation_queue.size();
    const auto total_size = rotation_vector.size();
    const auto current_idx = total_size - queue_size;
    const auto start = static_cast<int32_t>(current_idx - 2);
    const auto end = static_cast<int32_t>(current_idx + 2);

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
    bench_rotation_queue = RotationInfoQueue(std::deque<RotationInfo>(rotation_vector.begin(), rotation_vector.end()));
}

bool RotationRun::is_current_run_done() const
{
    return bench_rotation_queue.empty();
}
