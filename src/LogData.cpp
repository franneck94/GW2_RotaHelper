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
#include <map>
#include <queue>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

#include "LogData.h"
#include "Types.h"

using json = nlohmann::json;

namespace
{
    void collect_json(const json &jval, IntNode &node, const bool drop_first_char = false)
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

    std::string get_cache_remainder(const std::string &cache_url, const std::string &cache)
    {
        const auto remainder = cache_url.substr(cache.length());

        return remainder;
    }

    std::string convert_cache_url(const std::string &cache_url)
    {
        static const auto cache1 = std::string{"/cache/https_render.guildwars2.com_file_"};
        static const auto cache2 = std::string{"/cache/https_wiki.guildwars2.com_images_"};

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
        const auto skill_id_with_ext = remainder.substr(last_underscore + 1);

        const auto dot_pos = skill_id_with_ext.find_last_of('.');
        const auto skill_id = (dot_pos != std::string::npos) ? skill_id_with_ext.substr(0, dot_pos) : skill_id_with_ext;

        return "https://render.guildwars2.com/file/" + hash + "/" + skill_id + ".png";
    }

    void get_skill_info(const IntNode &node, SkillInfoMap &skill_info_map)
    {
        for (const auto &[skill_id, skill_node] : node.children)
        {
            auto name = std::string{};
            auto icon = std::string{};
            auto trait_proc = false;
            auto gear_proc = false;

            const auto name_it = skill_node.children.find("name");
            if (name_it != skill_node.children.end() && name_it->second.value.has_value())
            {
                if (const auto pval = std::get_if<std::string>(&name_it->second.value.value()))
                    name = *pval;
            }

            const auto icon_it = skill_node.children.find("icon");
            if (icon_it != skill_node.children.end() && icon_it->second.value.has_value())
            {
                if (const auto pval = std::get_if<std::string>(&icon_it->second.value.value()))
                    icon = convert_cache_url(*pval);
            }

            const auto trait_v12_it = skill_node.children.find("traitProc");
            if (trait_v12_it != skill_node.children.end() && trait_v12_it->second.value.has_value())
            {
                if (const auto pval = std::get_if<bool>(&trait_v12_it->second.value.value()))
                    trait_proc = *pval;
            }
            const auto trait_v3_it = skill_node.children.find("isTraitProc");
            if (trait_v3_it != skill_node.children.end() && trait_v3_it->second.value.has_value())
            {
                if (const auto pval = std::get_if<bool>(&trait_v3_it->second.value.value()))
                    trait_proc = *pval;
            }

            const auto gear_v12_it = skill_node.children.find("gearProc");
            if (gear_v12_it != skill_node.children.end() && gear_v12_it->second.value.has_value())
            {
                if (const auto pval = std::get_if<bool>(&gear_v12_it->second.value.value()))
                    gear_proc = *pval;
            }
            const auto gear_v3_it = skill_node.children.find("isGearProc");
            if (gear_v3_it != skill_node.children.end() && gear_v3_it->second.value.has_value())
            {
                if (const auto pval = std::get_if<bool>(&gear_v3_it->second.value.value()))
                    gear_proc = *pval;
            }

            skill_info_map[std::stoi(skill_id)] = {name, icon, trait_proc, gear_proc};
        }
    }

    bool remove_skill_if(const RotationInfo &current, const RotationInfo &previous)
    {
        return (current.skill_id == previous.skill_id &&
                (current.cast_time - previous.cast_time) < 250);
    }

    void get_rotation_info(const IntNode &node, const SkillInfoMap &skill_info_map, RotationInfoVec &rotation_vector)
    {
        for (const auto &rotation_entry : node.children)
        {
            const auto &rotation_array = rotation_entry.second;

            for (const auto &skill_entry : rotation_array.children)
            {
                const auto &skill_array = skill_entry.second;

                auto skill_id = 0;
                auto duration_ms = 0.0f;
                auto cast_time = 0.0f;
                auto unk = 0.0f;
                auto status = RotationStatus::UNKNOWN;

                // Get cast_time (index 0)
                const auto cast_time_it = skill_array.children.find("0");
                if (cast_time_it != skill_array.children.end() && cast_time_it->second.value.has_value())
                {
                    if (const auto pval = std::get_if<float>(&cast_time_it->second.value.value()))
                    {
                        cast_time = *pval;
                    }
                }

                // Get skill_id (index 1)
                const auto skill_id_it = skill_array.children.find("1");
                if (skill_id_it != skill_array.children.end() && skill_id_it->second.value.has_value())
                {
                    if (const auto pval = std::get_if<int>(&skill_id_it->second.value.value()))
                    {
                        skill_id = *pval;
                    }
                }

                // Get duration_ms (index 2)
                const auto duration_it = skill_array.children.find("2");
                if (duration_it != skill_array.children.end() && duration_it->second.value.has_value())
                {
                    if (const auto pval = std::get_if<int>(&duration_it->second.value.value()))
                    {
                        duration_ms = static_cast<float>(*pval);
                    }
                }

                // Get RotationStatus (index 3)
                const auto status_it = skill_array.children.find("3");
                if (status_it != skill_array.children.end() && status_it->second.value.has_value())
                {
                    if (const auto pval = std::get_if<int>(&status_it->second.value.value()))
                    {
                        status = static_cast<RotationStatus>(*pval);
                    }
                }

                // Get unk (index 4)
                const auto unk_it = skill_array.children.find("4");
                if (unk_it != skill_array.children.end() && unk_it->second.value.has_value())
                {
                    if (const auto pval = std::get_if<float>(&unk_it->second.value.value()))
                    {
                        unk = static_cast<float>(*pval);
                    }
                }

                if (skill_id <= 0)
                    continue;

                auto skill_name = std::string{"Unknown Skill"};
                const auto skill_info_it = skill_info_map.find(skill_id);
                if (skill_info_it != skill_info_map.end())
                {
                    skill_name = skill_info_it->second.name;
                }

                if (!skill_info_it->second.gear_proc && !skill_info_it->second.trait_proc)
                {
                    rotation_vector.push_back(RotationInfo{
                        .skill_id = skill_id,
                        .cast_time = cast_time,
                        .duration_ms = duration_ms,
                        .unk = unk,
                        .skill_name = skill_name,
                        .status = status,
                    });
                }
            }
        }

        std::sort(rotation_vector.begin(), rotation_vector.end(), [](const RotationInfo &a, const RotationInfo &b)
                  { return a.cast_time < b.cast_time; });
    }

    std::tuple<SkillInfoMap, RotationInfoVec> get_dpsreport_data(const nlohmann::json &j, const std::filesystem::path &json_path)
    {
        const auto filename = json_path.filename().string();
        const auto rotation_data = j["rotation"];
        const auto skill_data = j["skillMap"];

        auto kv_rotation = IntNode{};
        collect_json(rotation_data, kv_rotation);
        auto kv_skill = IntNode{};
        collect_json(skill_data, kv_skill, true);

        auto skill_info_map = SkillInfoMap{};
        get_skill_info(kv_skill, skill_info_map);
        auto rotation_info_vec = RotationInfoVec{};
        get_rotation_info(kv_rotation, skill_info_map, rotation_info_vec);

        return std::make_tuple(skill_info_map, rotation_info_vec);
    }

    bool DownloadFileFromURL(const std::string &url, const std::filesystem::path &out_path)
    {
        const auto hInternet = InternetOpenA("GW2RotaHelper", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
        if (!hInternet)
        {
            std::cerr << "Failed to open internet connection" << std::endl;
            return false;
        }

        const auto hFile = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
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
        auto futures = std::queue<std::future<void>>{};

        for (const auto &[skill_id, info] : skill_info_map)
        {
            if (info.icon_url.empty())
                continue;

            auto ext{std::string{".png"}};
            const auto dot_pos{info.icon_url.find_last_of('.')};
            if (dot_pos != std::string::npos && dot_pos + 1 < info.icon_url.size())
            {
                ext = info.icon_url.substr(dot_pos);
            }
            const auto out_path{img_folder / (std::to_string(skill_id) + ext)};
            if (std::filesystem::exists(out_path))
                continue;

            std::cout << "Downloading " << info.icon_url << " to " << out_path << std::endl;
            futures.push(std::async(std::launch::async, [url = info.icon_url, out_path]()
                                    { DownloadFileFromURL(url, out_path); }));
        }

        return futures;
    }
} // namespace

void RotationRun::load_data(const std::filesystem::path &json_path, const std::filesystem::path &img_path)
{
    auto file{std::ifstream{json_path}};
    auto j{nlohmann::json{}};
    file >> j;

    auto [_skill_info_map, _bench_rotation_vector]{get_dpsreport_data(j, json_path)};
    skill_info_map = std::move(_skill_info_map);
    rotation_vector = std::move(_bench_rotation_vector);

    restart_rotation();

    futures = StartDownloadAllSkillIcons(skill_info_map, img_path);
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
