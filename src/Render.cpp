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
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <thread>
#include <utility>

#include "imgui.h"

#include "Constants.h"
#include "LogData.h"
#include "Render.h"
#include "Settings.h"
#include "Shared.h"
#include "Types.h"

namespace
{
    std::string format_build_name(const std::string &raw_name)
    {
        auto result = raw_name;

        const auto start = result.find_first_not_of(" \t");
        if (start != std::string::npos)
            result = result.substr(start);

        if (result.starts_with("condition_"))
            result = result.substr(10); // Remove "condition_"
        else if (result.starts_with("power_"))
            result = result.substr(6); // Remove "power_"

        std::replace(result.begin(), result.end(), '_', ' ');

        bool capitalize_next = true;
        std::ranges::transform(result, result.begin(), [&capitalize_next](char c)
                               {
            if (c == ' ') {
                capitalize_next = true;
                return c;
            }

            if (capitalize_next) {
                capitalize_next = false;
                return static_cast<char>(std::toupper(c));
            }

            return static_cast<char>(std::tolower(c)); });

        return "    " + result;
    }

    std::vector<BenchFileInfo> get_bench_files(const std::filesystem::path &bench_path)
    {
        std::vector<BenchFileInfo> files;
        std::map<std::string, std::vector<std::filesystem::path>> directory_files;

        try
        {
            for (const auto &entry : std::filesystem::recursive_directory_iterator(bench_path))
            {
                if (entry.is_regular_file() && entry.path().extension() == ".json")
                {
                    auto relative_path = std::filesystem::relative(entry.path(), bench_path);
                    auto parent_dir = relative_path.parent_path().string();

                    if (parent_dir.empty())
                        parent_dir = ".";

                    directory_files[parent_dir].push_back(entry.path());
                }
            }

            for (const auto &[dir_name, dir_files] : directory_files)
            {
                if (dir_name != ".")
                {
                    auto header_path = bench_path / dir_name;
                    files.emplace_back(header_path, std::filesystem::path(dir_name), true);
                }

                for (const auto &file_path : dir_files)
                {
                    auto relative_path = std::filesystem::relative(file_path, bench_path);
                    files.emplace_back(file_path, relative_path, false);
                }
            }
        }
        catch (const std::filesystem::filesystem_error &ex)
        {
            std::cerr << "Error scanning bench files: " << ex.what() << std::endl;
        }

        return files;
    }
}

Render::Render()
{
}

Render::~Render()
{
    ReleaseTextureMap(texture_map);
}

void Render::set_data_path(const std::filesystem::path &path)
{
    data_path = path;
    img_path = data_path / "img";
    bench_path = data_path / "bench";

    benches_files = get_bench_files(bench_path);
}

void Render::key_press_cb(const bool pressed, const EvCombatDataPersistent &combat_data)
{
    // NOTE: maybe add timer here if arc sends event over multiple frames
    key_press_event_in_this_frame = pressed;
    if (pressed)
        curr_combat_data = combat_data;
}

EvCombatDataPersistent Render::get_current_skill()
{
    if (!key_press_event_in_this_frame || curr_combat_data.SkillID == 0)
    {
        auto skill_ev = EvCombatDataPersistent{};
        skill_ev.SkillID = -10000;
        return skill_ev;
    }

    return curr_combat_data;
}

void Render::toggle_vis(const bool flag)
{
    show_window = flag;
}

std::pair<std::vector<std::pair<int, const BenchFileInfo *>>, std::set<std::string>> Render::get_file_data_pairs(std::string &filter_string)
{
    std::vector<std::pair<int, const BenchFileInfo *>> filtered_files;
    std::set<std::string> directories_with_matches;

    if (filter_string.empty())
    {
        for (int n = 0; n < benches_files.size(); n++)
            filtered_files.emplace_back(n, &benches_files[n]);

        return std::make_pair(std::move(filtered_files), std::move(directories_with_matches));
    }

    for (int n = 0; n < benches_files.size(); n++)
    {
        const auto &file_info = benches_files[n];

        if (!file_info.is_directory_header)
        {
            auto display_lower = file_info.display_name;
            std::transform(display_lower.begin(), display_lower.end(), display_lower.begin(), ::tolower);

            if (display_lower.starts_with(filter_string))
            {
                const auto parent_dir = file_info.relative_path.parent_path().string();
                if (!parent_dir.empty() && parent_dir != ".")
                {
                    directories_with_matches.insert(parent_dir);
                }
            }
        }
    }

    for (int n = 0; n < benches_files.size(); n++)
    {
        const auto &file_info = benches_files[n];

        if (file_info.is_directory_header)
        {
            if (directories_with_matches.count(file_info.relative_path.string()) > 0)
            {
                filtered_files.emplace_back(n, &file_info);
            }
        }
        else
        {
            auto display_lower = file_info.display_name;
            std::transform(display_lower.begin(), display_lower.end(), display_lower.begin(), ::tolower);

            if (display_lower.find(filter_string) != std::string::npos)
            {
                filtered_files.emplace_back(n, &file_info);
            }
        }
    }

    return std::make_pair(std::move(filtered_files), std::move(directories_with_matches));
}

void Render::select_bench()
{
    if (!benches_files.empty())
    {
        ImGui::Text("Select Bench File:");

        ImGui::Text("Filter:");
        ImGui::SameLine();
        char *filter_buffer = (char *)Settings::FilterBuffer.c_str();
        if (ImGui::InputText("##filter", filter_buffer, sizeof(filter_buffer)))
        {
            Settings::FilterBuffer = std::string(filter_buffer);
            std::transform(Settings::FilterBuffer.begin(), Settings::FilterBuffer.end(), Settings::FilterBuffer.begin(), ::tolower);
        }

        const auto &[filtered_files, directories_with_matches] = get_file_data_pairs(Settings::FilterBuffer);

        auto combo_preview = std::string{};
        if (selected_bench_index >= 0 && selected_bench_index < benches_files.size())
            combo_preview = benches_files[selected_bench_index].relative_path.filename().string();
        else
            combo_preview = "Select...";

        auto combo_preview_slice = std::string{"Select..."};
        auto formatted_name = std::string{"Select..."};
        if (combo_preview != "Select...")
        {
            combo_preview_slice = combo_preview.substr(0, combo_preview.size() - 8);
            formatted_name = format_build_name(combo_preview_slice);
            formatted_name = formatted_name.substr(4);
        }

        if (ImGui::BeginCombo("##benches_combo", formatted_name.c_str()))
        {
            for (const auto &[original_index, file_info] : filtered_files)
            {
                if (file_info->is_directory_header)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.9f, 1.0f));
                    ImGui::Selectable(file_info->display_name.c_str(), false, ImGuiSelectableFlags_Disabled);
                    ImGui::PopStyleColor();
                }
                else
                {
                    const auto is_selected = (selected_bench_index == original_index);
                    auto formatted_name = file_info->is_directory_header ? file_info->display_name : format_build_name(file_info->display_name);

                    if (ImGui::Selectable(formatted_name.c_str(), is_selected))
                    {
                        selected_bench_index = original_index;
                        selected_file_path = file_info->full_path;

                        rotation_run.load_data(selected_file_path, img_path);
                        ReleaseTextureMap(texture_map);
                    }

                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        if (selected_bench_index >= 0 && selected_bench_index < benches_files.size())
        {
            ImGui::SameLine();
            if (ImGui::Button("Reload Rotation"))
            {
                if (!selected_file_path.empty())
                {
                    rotation_run.load_data(selected_file_path, img_path);
                    ReleaseTextureMap(texture_map);
                }
            }
        }
    }
}

void Render::rotation_render(ID3D11Device *pd3dDevice)
{
    static auto last_skill = EvCombatDataPersistent{};

    ImGui::BeginChild("CombatBufferChild", ImVec2(0, 200), true, ImGuiWindowFlags_HorizontalScrollbar);

    const auto [start, end, current_idx] = rotation_run.get_current_rotation_indices();

    for (int32_t i = start; i <= end; ++i)
    {
        if (i < 0 || static_cast<size_t>(i) >= rotation_run.rotation_vector.size())
            continue;

        const auto &skill_info = rotation_run.get_rotation_skill(static_cast<size_t>(i));
        const auto *texture = texture_map[skill_info.skill_id];

        const auto is_current = (i == static_cast<int32_t>(current_idx));

        if (texture && pd3dDevice)
            ImGui::Image((ImTextureID)texture, ImVec2(28, 28));
        else
            ImGui::Dummy(ImVec2(28, 28));

        ImGui::SameLine();

        if (is_current)
            ImGui::Text("-> %s (%.2f) <-", skill_info.skill_name.empty() ? "N/A" : skill_info.skill_name.c_str(), skill_info.cast_time);
        else
            ImGui::Text("   %s (%.2f)", skill_info.skill_name.empty() ? "N/A" : skill_info.skill_name.c_str(), skill_info.cast_time);
    }

    const auto skill_ev = get_current_skill();
    if (skill_ev.SkillID != 0 && last_skill.SkillID != skill_ev.SkillID)
    {
        if (rotation_run.bench_rotation_list.size() > 2)
        {
            // With a list, we can directly access elements by iterator
            auto it = rotation_run.bench_rotation_list.begin();
            const auto curr_rota_skill = *it;
            ++it;
            const auto next_rota_skill = *it;
            ++it;
            const auto next_next_rota_skill = *it;

            const auto match_current = (curr_rota_skill.skill_id == skill_ev.SkillID);
            const auto match_next = (next_rota_skill.skill_id == skill_ev.SkillID);
            const auto match_next_next = (next_next_rota_skill.skill_id == skill_ev.SkillID);

            if (match_current)
            {
                rotation_run.bench_rotation_list.pop_front();
                last_skill = skill_ev;
            }
            else if (match_next)
            {
                rotation_run.bench_rotation_list.pop_front();
                rotation_run.bench_rotation_list.pop_front();
                last_skill = skill_ev;
            }
            else if (match_next_next)
            {
                rotation_run.bench_rotation_list.pop_front();
                rotation_run.bench_rotation_list.pop_front();
                rotation_run.bench_rotation_list.pop_front();
                last_skill = skill_ev;
            }
        }
    }

    ImGui::EndChild();
}

void Render::render(ID3D11Device *pd3dDevice, AddonAPI *APIDefs)
{
    if (!Settings::ShowWindow)
        return;

    if (benches_files.size() == 0)
        benches_files = get_bench_files(bench_path);

    if (ImGui::Begin("GW2RotaHelper", &Settings::ShowWindow))
    {
        select_bench();

        if (rotation_run.futures.size() != 0)
        {
            if (rotation_run.futures.front().valid())
            {
                rotation_run.futures.front().get();
                rotation_run.futures.pop_front();
            }

            ImGui::End();

            return;
        }

        if (texture_map.size() == 0)
        {
            if (APIDefs && !pd3dDevice)
            {
                texture_map = LoadAllSkillTexturesWithAPI(APIDefs, rotation_run.skill_info_map, img_path);
            }
            else if (pd3dDevice)
            {
                texture_map = LoadAllSkillTextures(pd3dDevice, rotation_run.skill_info_map, img_path);
            }
        }

        ImGui::Separator();

        rotation_render(pd3dDevice);
    }

    ImGui::End();
}
