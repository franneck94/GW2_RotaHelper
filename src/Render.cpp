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

#include "Defines.h"
#include "LogData.h"
#include "Render.h"
#include "Settings.h"
#include "Shared.h"
#include "Textures.h"
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
    std::ranges::transform(result, result.begin(), [&capitalize_next](char c) {
        if (c == ' ')
        {
            capitalize_next = true;
            return c;
        }

        if (capitalize_next)
        {
            capitalize_next = false;
            return static_cast<char>(std::toupper(c));
        }

        return static_cast<char>(std::tolower(c));
    });

    return "    " + result;
}

std::vector<BenchFileInfo> get_bench_files(
    const std::filesystem::path &bench_path)
{
    auto files = std::vector<BenchFileInfo>{};
    auto directory_files =
        std::map<std::string, std::vector<std::filesystem::path>>{};

    try
    {
        for (const auto &entry :
             std::filesystem::recursive_directory_iterator(bench_path))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".json")
            {
                auto relative_path =
                    std::filesystem::relative(entry.path(), bench_path);
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
                files.emplace_back(header_path,
                                   std::filesystem::path(dir_name),
                                   true);
            }

            for (const auto &file_path : dir_files)
            {
                auto relative_path =
                    std::filesystem::relative(file_path, bench_path);
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
} // namespace

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

void Render::skill_activation_callback(
    const bool pressed,
    const EvCombatDataPersistent &combat_data)
{
    key_press_event_in_this_frame = pressed;
    if (pressed)
    {
        std::lock_guard<std::mutex> lock(played_rotation_mutex);
        curr_combat_data = combat_data;

        try
        {
            if (played_rotation.size() > 300)
                played_rotation.clear();

            played_rotation.push_back(combat_data);
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error adding to played_rotation: " << e.what()
                      << '\n';
        }
    }
}

EvCombatDataPersistent Render::get_current_skill()
{
    if (!key_press_event_in_this_frame)
    {
        auto skill_ev = EvCombatDataPersistent{};
        skill_ev.SkillID = -10000;
        return skill_ev;
    }

    // Thread-safe access to curr_combat_data
    std::lock_guard<std::mutex> lock(played_rotation_mutex);
    if (curr_combat_data.SkillID == 0)
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

std::pair<std::vector<std::pair<int, const BenchFileInfo *>>,
          std::set<std::string>>
Render::get_file_data_pairs(std::string &filter_string)
{
    auto filtered_files = std::vector<std::pair<int, const BenchFileInfo *>>{};
    auto directories_with_matches = std::set<std::string>{};

    if (filter_string.empty())
    {
        for (int n = 0; n < benches_files.size(); n++)
            filtered_files.emplace_back(n, &benches_files[n]);

        return std::make_pair(filtered_files, directories_with_matches);
    }

    // First pass: find all files that match and collect their directories
    for (int n = 0; n < benches_files.size(); n++)
    {
        const auto &file_info = benches_files[n];

        if (!file_info.is_directory_header)
        {
            auto display_lower = file_info.display_name;
            std::transform(display_lower.begin(),
                           display_lower.end(),
                           display_lower.begin(),
                           ::tolower);

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
            auto display_lower = file_info.display_name;
            std::transform(display_lower.begin(),
                           display_lower.end(),
                           display_lower.begin(),
                           ::tolower);

            if (display_lower.find(filter_string) != std::string::npos)
            {
                filtered_files.emplace_back(n, &file_info);
            }
        }
    }

    return std::make_pair(filtered_files, directories_with_matches);
}

void Render::text_filter()
{
    ImGui::Text("Filter:");
    ImGui::SameLine();
    char *filter_buffer = (char *)Settings::FilterBuffer.c_str();

    ImGui::PushAllowKeyboardFocus(false);

    filter_input_pos = ImGui::GetCursorScreenPos();

    bool text_changed = ImGui::InputText("##filter",
                                         filter_buffer,
                                         sizeof(filter_buffer),
                                         ImGuiInputTextFlags_EnterReturnsTrue);

    Settings::FilterBuffer = std::string(filter_buffer);
    std::transform(Settings::FilterBuffer.begin(),
                   Settings::FilterBuffer.end(),
                   Settings::FilterBuffer.begin(),
                   ::tolower);

    if (text_changed)
        open_combo_next_frame = true;

    filter_input_width = ImGui::GetItemRectSize().x;

    if (ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_Tab))
        open_combo_next_frame = true;

    ImGui::PopAllowKeyboardFocus();

    const auto &[_filtered_files, directories_with_matches] =
        get_file_data_pairs(Settings::FilterBuffer);
    filtered_files = _filtered_files;

    auto combo_preview = std::string{};
    if (selected_bench_index >= 0 &&
        selected_bench_index < benches_files.size())
        combo_preview = benches_files[selected_bench_index]
                            .relative_path.filename()
                            .string();
    else
        combo_preview = "Select...";

    auto combo_preview_slice = std::string{"Select..."};
    if (combo_preview != "Select...")
    {
        combo_preview_slice = combo_preview.substr(0, combo_preview.size() - 8);
        formatted_name = format_build_name(combo_preview_slice);
        formatted_name = formatted_name.substr(4);
    }
}

void Render::selection()
{
    ImGui::Text("Select Bench File:");

    if (open_combo_next_frame)
    {
        ImGui::OpenPopup("benches_popup");
        open_combo_next_frame = false;
    }

    if (ImGui::Button(formatted_name.c_str(), ImVec2(-1, 0)))
        ImGui::OpenPopup("benches_popup");

    const auto window_pos = ImGui::GetWindowPos();
    const auto popup_pos = ImVec2(
        window_pos.x,
        filter_input_pos.y + ImGui::GetTextLineHeightWithSpacing() * 2.5f);


    ImGui::SetNextWindowPos(popup_pos, ImGuiCond_Appearing);
    ImGui::SetNextWindowSize(ImVec2(filter_input_width, 0),
                             ImGuiCond_Appearing);

    if (ImGui::BeginPopup("benches_popup"))
    {
        if (filtered_files.empty())
        {
            ImGui::TextDisabled("No results");
        }
        else
        {
            for (const auto &[original_index, file_info] : filtered_files)
            {
                if (file_info->is_directory_header)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text,
                                          ImVec4(0.7f, 0.7f, 0.9f, 1.0f));
                    ImGui::Selectable(file_info->display_name.c_str(),
                                      false,
                                      ImGuiSelectableFlags_Disabled);
                    ImGui::PopStyleColor();
                }
                else
                {
                    const auto is_selected =
                        (selected_bench_index == original_index);

                    auto formatted_name_item = std::string{};
                    if (file_info->is_directory_header)
                    {
                        formatted_name_item = file_info->display_name;
                    }
                    else
                    {
                        // Extract build type info from file path
                        auto path_str = file_info->relative_path.string();
                        auto build_type_postdic = std::string{};

                        if (path_str.find("dps") != std::string::npos)
                            build_type_postdic = "[DPS] ";
                        else if (path_str.find("quick") != std::string::npos)
                            build_type_postdic = "[Quick] ";
                        else if (path_str.find("alac") != std::string::npos)
                            build_type_postdic = "[Alac] ";

                        if (path_str.find("power") != std::string::npos)
                            build_type_postdic += "[Power] ";
                        else if (path_str.find("condition") !=
                                 std::string::npos)
                            build_type_postdic += "[Condi] ";

                        formatted_name_item =
                            format_build_name(file_info->display_name) + "##" +
                            build_type_postdic;
                    }

                    if (ImGui::Selectable(formatted_name_item.c_str(),
                                          is_selected))
                    {
                        selected_bench_index = original_index;
                        selected_file_path = file_info->full_path;

                        rotation_run.load_data(selected_file_path, img_path);
                        ReleaseTextureMap(texture_map);

                        // Close popup after selection
                        ImGui::CloseCurrentPopup();
                    }

                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
            }
        }
        ImGui::EndPopup();
    }
}

void Render::reload_btn()
{
    if (selected_bench_index >= 0 &&
        selected_bench_index < benches_files.size())
    {
        ImGui::SameLine();
        if (ImGui::Button("Reload"))
        {
            if (!selected_file_path.empty())
            {
                rotation_run.restart_rotation();
                ReleaseTextureMap(texture_map);

                std::lock_guard<std::mutex> lock(played_rotation_mutex);
                played_rotation.clear();
                curr_combat_data = EvCombatDataPersistent{};
            }
        }
    }
}

void Render::select_bench()
{
    text_filter();

    if (benches_files.empty())
        return;

    selection();

    reload_btn();
}

void Render::CycleSkillsLogic()
{
    static auto last_skill = EvCombatDataPersistent{};

    if (rotation_run.bench_rotation_list.empty())
        return;

    const auto skill_ev = get_current_skill();
    if (skill_ev.SkillID != 0 && last_skill.SkillID != skill_ev.SkillID)
    {
        auto curr_rota_skill = RotationInfo{};
        auto next_rota_skill = RotationInfo{};
        auto next_next_rota_skill = RotationInfo{};
        auto next_next_next_rota_skill = RotationInfo{};
        auto it = rotation_run.bench_rotation_list.begin();

        if (rotation_run.bench_rotation_list.size() > 1)
        {
            curr_rota_skill = *it;
            ++it;
        }
        if (rotation_run.bench_rotation_list.size() > 2)
        {
            next_rota_skill = *it;
            ++it;
        }
        if (rotation_run.bench_rotation_list.size() > 3)
        {
            next_next_rota_skill = *it;
            ++it;
        }
        if (rotation_run.bench_rotation_list.size() > 4)
        {
            next_next_next_rota_skill = *it;
            ++it;
        }

#ifdef USE_SKILL_ID_MATCH_LOGIC
        auto match_current = (curr_rota_skill.skill_id == skill_ev.SkillID);
        auto match_next = (next_rota_skill.skill_id == skill_ev.SkillID);
        auto match_next_next =
            (next_next_rota_skill.skill_id == skill_ev.SkillID);
        auto match_next_next_next =
            (next_next_rota_skill.skill_id == skill_ev.SkillID);
#else

        auto match_current = (curr_rota_skill.skill_name == skill_ev.SkillName);
        auto match_next = (next_rota_skill.skill_name == skill_ev.SkillName);
        auto match_next_next =
            (next_next_rota_skill.skill_name == skill_ev.SkillName);
        auto match_next_next_next =
            (next_next_rota_skill.skill_name == skill_ev.SkillName);

        if (curr_rota_skill.skill_name.find(" / ") != std::string::npos)
        {
            match_current =
                (curr_rota_skill.skill_name.find(skill_ev.SkillName)) !=
                std::string::npos;
            match_next =
                (next_rota_skill.skill_name.find(skill_ev.SkillName)) !=
                std::string::npos;
            match_next_next =
                (next_next_rota_skill.skill_name.find(skill_ev.SkillName)) !=
                std::string::npos;
            match_next_next_next =
                (next_next_next_rota_skill.skill_name.find(
                    skill_ev.SkillName)) != std::string::npos;
        }

#endif

#ifdef _DEBUG
        if (skill_ev.SkillID == static_cast<uint64_t>(-1))
            match_current = true;
#endif

        if (match_current)
        {
            rotation_run.bench_rotation_list.pop_front();
            last_skill = skill_ev;
        }
#ifdef USE_SKIP_NEXT_SKILL
        else if (match_next)
        {
            rotation_run.bench_rotation_list.pop_front();
            rotation_run.bench_rotation_list.pop_front();
            last_skill = skill_ev;
        }
#endif
#ifdef USE_SKIP_NEXT_NEXT_SKILL
        else if (match_next_next)
        {
            rotation_run.bench_rotation_list.pop_front();
            rotation_run.bench_rotation_list.pop_front();
            rotation_run.bench_rotation_list.pop_front();
            last_skill = skill_ev;
        }
#endif
#ifdef USE_SKIP_NEXT_NEXT_NEXT_SKILL
        else if (match_next_next_next)
        {
            rotation_run.bench_rotation_list.pop_front();
            rotation_run.bench_rotation_list.pop_front();
            rotation_run.bench_rotation_list.pop_front();
            rotation_run.bench_rotation_list.pop_front();
            last_skill = skill_ev;
        }
#endif
    }
}

void Render::DrawRect(const RotationInfo &skill_info,
                      const std::string &text,
                      ImU32 color)
{
    auto draw_list = ImGui::GetWindowDrawList();
    auto cursor_pos = ImGui::GetCursorScreenPos();
    auto border_color = color;
    auto border_thickness = 2.0f;

    auto text_size = ImVec2{};
    if (!text.empty())
    {
        char formatted_text[256];
        snprintf(formatted_text, sizeof(formatted_text), text.c_str());
        text_size = ImGui::CalcTextSize(formatted_text);
    }
    else
    {
        text_size = ImVec2(0, 0);
    }

    auto total_width = text_size.x > 0
                           ? 28 + ImGui::GetStyle().ItemSpacing.x + text_size.x
                           : SKILL_ICON_SIZE;
    auto total_height =
        (SKILL_ICON_SIZE > text_size.y) ? SKILL_ICON_SIZE : text_size.y;

    draw_list->AddRect(ImVec2(cursor_pos.x - border_thickness,
                              cursor_pos.y - border_thickness),
                       ImVec2(cursor_pos.x + total_width + border_thickness,
                              cursor_pos.y + total_height + border_thickness),
                       border_color,
                       0.0f,
                       0,
                       border_thickness);
}

void Render::rotation_render(ID3D11Device *pd3dDevice)
{
    ImGui::Spacing();
    ImGui::Indent(10.0f); // Shift content 10px to the right

    const auto [start, end, current_idx] =
        rotation_run.get_current_rotation_indices();

    for (int32_t i = start; i <= end; ++i)
    {
        if (i < 0 ||
            static_cast<size_t>(i) >= rotation_run.rotation_vector.size())
            continue;

        const auto &skill_info =
            rotation_run.get_rotation_skill(static_cast<size_t>(i));
        const auto *texture = texture_map[skill_info.icon_id];

        const auto is_current = (i == static_cast<int32_t>(current_idx));
        const auto is_last = (i == rotation_run.rotation_vector.size() - 1);

        auto text = std::string{};
        if (Settings::HorizontalSkillLayout || (!Settings::ShowSkillName && !Settings::ShowSkillTime))
            text = "";
        else
        {
            if (Settings::ShowSkillName)
            {
                text = skill_info.skill_name;
            }
            if (Settings::ShowSkillTime)
            {
                text += " (";
                char time_buffer[32];
                snprintf(time_buffer,
                         sizeof(time_buffer),
                         "%.2f",
                         skill_info.cast_time);
                text += time_buffer;
                text += ")";
            }
        }

        if (is_current && !is_last)
            DrawRect(skill_info, text);
        if (!is_current && is_last)
            DrawRect(skill_info, text, IM_COL32(255, 0, 0, 255));

        if (texture && pd3dDevice)
            ImGui::Image((ImTextureID)texture, ImVec2(28, 28));
        else
            ImGui::Dummy(ImVec2(28, 28));

        if (!text.empty() || Settings::HorizontalSkillLayout)
            ImGui::SameLine();

        if (!text.empty())
        {
            auto draw_list = ImGui::GetWindowDrawList();
            auto text_pos = ImGui::GetCursorScreenPos();

            draw_list->AddText(ImVec2(text_pos.x + 1, text_pos.y + 1),
                               IM_COL32(0, 0, 0, 200),
                               text.c_str());
            draw_list->AddText(text_pos,
                               ImGui::GetColorU32(ImGuiCol_Text),
                               text.c_str());
            ImGui::Dummy(ImGui::CalcTextSize(text.c_str()));
        }
    }

    CycleSkillsLogic();

    ImGui::Unindent(10.0f);
}

void Render::render(ID3D11Device *pd3dDevice)
{
    if (!Settings::ShowWindow)
        return;

    if (benches_files.size() == 0)
        benches_files = get_bench_files(bench_path);

    constexpr auto flags_options = ImGuiWindowFlags_NoCollapse;
    if (ImGui::Begin("###GW2RotaHelper_Options",
                     &Settings::ShowWindow,
                     flags_options))
    {
        select_bench();

        ImGui::Checkbox("Names", &Settings::ShowSkillName);
        ImGui::SameLine();
        ImGui::Checkbox("Times", &Settings::ShowSkillTime);
        ImGui::SameLine();
        ImGui::Checkbox("Horizontal", &Settings::HorizontalSkillLayout);
    }
    ImGui::End();

    if (rotation_run.futures.size() != 0)
    {
        if (rotation_run.futures.front().valid())
        {
            rotation_run.futures.front().get();
            rotation_run.futures.pop_front();
        }

        return;
    }

    if (rotation_run.rotation_vector.size() == 0)
        return;

    // Position window at bottom center of screen
    ImGuiIO &io = ImGui::GetIO();
    float window_width = 400.0f;
    float window_height = SKILL_ICON_SIZE * 10.0F; // we draw 10 images
    float pos_x = (io.DisplaySize.x - window_width) * 0.5f;
    float pos_y =
        io.DisplaySize.y - window_height - 50.0f; // 50px margin from bottom

    ImGui::SetNextWindowPos(ImVec2(pos_x, pos_y), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(window_width, window_height),
                             ImGuiCond_FirstUseEver);
    constexpr auto flags_rota =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoScrollbar;
    if (ImGui::Begin("##GW2RotaHelper_Rota", &Settings::ShowWindow, flags_rota))
    {
        if (texture_map.size() == 0)
        {
            texture_map = LoadAllSkillTextures(pd3dDevice,
                                               rotation_run.skill_info_map,
                                               img_path);
        }

        rotation_render(pd3dDevice);
    }
    ImGui::End();
}
