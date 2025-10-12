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
#include "Shared.h"
#include "Types.h"
#include "Settings.h"

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

void Render::key_press_cb(const bool pressed)
{
    key_press_event_in_this_frame = pressed;
}

EvCombatDataPersistent Render::get_current_skill()
{
    if (!key_press_event_in_this_frame)
        return EvCombatDataPersistent{};

    return combat_buffer[prev_combat_buffer_index];
}

void Render::toggle_vis(const bool flag)
{
    show_window = flag;
}

void Render::select_bench()
{
    static char filter_buffer[256] = "";
    static std::string filter_string;

    if (!benches_files.empty())
    {
        ImGui::Text("Select Bench File:");

        ImGui::Text("Filter:");
        ImGui::SameLine();
        if (ImGui::InputText("##filter", filter_buffer, sizeof(filter_buffer)))
        {
            filter_string = std::string(filter_buffer);
            std::transform(filter_string.begin(), filter_string.end(), filter_string.begin(), ::tolower);
        }

        std::vector<std::pair<int, const BenchFileInfo *>> filtered_files;
        std::set<std::string> directories_with_matches;

        if (filter_string.empty())
        {
            for (int n = 0; n < benches_files.size(); n++)
            {
                filtered_files.emplace_back(n, &benches_files[n]);
            }
        }
        else
        {
            for (int n = 0; n < benches_files.size(); n++)
            {
                const auto &file_info = benches_files[n];

                if (!file_info.is_directory_header)
                {
                    auto display_lower = file_info.display_name;
                    std::transform(display_lower.begin(), display_lower.end(), display_lower.begin(), ::tolower);

                    if (display_lower.find(filter_string) != std::string::npos)
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
        }

        std::string combo_preview;
        if (selected_bench_index >= 0 && selected_bench_index < benches_files.size())
        {
            combo_preview = benches_files[selected_bench_index].relative_path.filename().string();
        }
        else
        {
            combo_preview = "Select...";
        }

        if (ImGui::BeginCombo("##benches_combo", combo_preview.c_str()))
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
    if (skill_ev.SkillID != 0)
        rotation_run.pop_bench_rotation_queue();

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
                rotation_run.futures.pop();
            }

            ImGui::End();

            return;
        }

        // Prefer Nexus API for texture loading when available, fallback to direct D3D
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
