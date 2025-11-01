#include <windows.h>

#include <commdlg.h>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")
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

#include "mumble/Mumble.h"

#include "ArcEvents.h"
#include "Defines.h"
#include "FileUtils.h"
#include "LogData.h"
#include "MumbleUtils.h"
#include "Render.h"
#include "Settings.h"
#include "Shared.h"
#include "Textures.h"
#include "Types.h"
#include "TypesUtils.h"

namespace
{
static auto last_time_aa_did_skip = std::chrono::steady_clock::time_point{};

void DrawRect(const RotationStep &rotation_step,
              const std::string &text,
              const ImU32 color,
              const float border_thickness = 2.0f)
{
    auto draw_list = ImGui::GetWindowDrawList();
    auto cursor_pos = ImGui::GetCursorScreenPos();
    auto border_color = color;

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
                           ? Globals::SkillIconSize +
                                 ImGui::GetStyle().ItemSpacing.x + text_size.x
                           : Globals::SkillIconSize;
    auto total_height = (Globals::SkillIconSize > text_size.y)
                            ? Globals::SkillIconSize
                            : text_size.y;

    // Draw thick border by drawing outer filled rect and inner transparent rect
    draw_list->AddRectFilled(
        ImVec2(cursor_pos.x - border_thickness,
               cursor_pos.y - border_thickness),
        ImVec2(cursor_pos.x + total_width + border_thickness,
               cursor_pos.y + total_height + border_thickness),
        border_color);

    // Cut out the inner area to create the border effect
    draw_list->AddRectFilled(
        ImVec2(cursor_pos.x, cursor_pos.y),
        ImVec2(cursor_pos.x + total_width, cursor_pos.y + total_height),
        IM_COL32(0, 0, 0, 0)); // Transparent to cut out inner area
}

std::string get_skill_text(const RotationStep &rotation_step)
{
    auto text = rotation_step.skill_data.name;

    if (Settings::ShowSkillTime)
    {
        text += " (";
        char time_buffer[32];
        snprintf(time_buffer,
                 sizeof(time_buffer),
                 "%.2f",
                 rotation_step.time_of_cast);
        text += time_buffer;
        text += ")";
    }

    return text;
}
} // namespace

RenderType::RenderType()
{
}

RenderType::~RenderType()
{
    ReleaseTextureMap(Globals::TextureMap);
}

void RenderType::set_data_path(const std::filesystem::path &path)
{
    data_path = path;
    img_path = data_path / "img";
    bench_path = data_path / "bench";

    benches_files = get_bench_files(bench_path);
}

void RenderType::append_to_played_rotation(
    const EvCombatDataPersistent &combat_data)
{
    if (played_rotation.size() > 300)
    {
        auto last_100 =
            std::vector<EvCombatDataPersistent>(played_rotation.end() - 100,
                                                played_rotation.end());
        played_rotation = std::move(last_100);
    }

    played_rotation.push_back(combat_data);
}

void RenderType::skill_activation_callback(
    const bool pressed,
    const EvCombatDataPersistent &combat_data)
{
    key_press_event_in_this_frame = pressed;

    if (pressed)
    {
        curr_combat_data = combat_data;

        append_to_played_rotation(combat_data);
    }
}

EvCombatDataPersistent RenderType::get_current_skill()
{
    if (!key_press_event_in_this_frame)
    {
        auto skill_ev = EvCombatDataPersistent{};
        skill_ev.SkillID = -10000;
        return skill_ev;
    }

    if (curr_combat_data.SkillID == 0)
    {
        auto skill_ev = EvCombatDataPersistent{};
        skill_ev.SkillID = -10000;
        return skill_ev;
    }

    return curr_combat_data;
}

void RenderType::set_show_window(const bool flag)
{
    show_window = flag;
}

void RenderType::CycleSkillsLogic(const EvCombatDataPersistent &skill_ev)
{
    static auto last_skill = EvCombatDataPersistent{};

    if (Globals::RotationRun.todo_rotation_steps.empty())
        return;

    if (skill_ev.SkillID == 0 || skill_ev.SkillID == -10000)
        return;

    SimpleSkillDetectionLogic(num_skills_wo_match,
                              time_since_last_match,
                              Globals::RotationRun,
                              skill_ev,
                              last_skill);
}

float RenderType::calculate_centered_position(
    const std::vector<std::string> &items) const
{
    auto total_width = 0.0f;

    for (size_t i = 0; i < items.size(); ++i)
    {
        total_width += ImGui::CalcTextSize(items[i].c_str()).x;
        total_width += ImGui::GetFrameHeight(); // Checkbox size

        if (i < items.size() - 1)
            total_width += ImGui::GetStyle().ItemSpacing.x;
    }

    const auto window_width = ImGui::GetWindowSize().x;
    return (window_width - total_width) * 0.5f;
}

void RenderType::render_debug_data()
{
    ImGui::Separator();
    ImGui::Text("Profession: %d (%s)",
                static_cast<int>(Globals::Identity.Profession),
                profession_to_string(
                    static_cast<ProfessionID>(Globals::Identity.Profession))
                    .c_str());
    ImGui::Text("Specialization: %u (%s)",
                Globals::Identity.Specialization,
                elite_spec_to_string(
                    static_cast<EliteSpecID>(Globals::Identity.Specialization))
                    .c_str());
    ImGui::Text("Map ID: %u", Globals::Identity.MapID);
    ImGui::Text("Is in Combat: %d", Globals::MumbleData->Context.IsInCombat);

    ImGui::Text("Last Casted Skill ID: %u", curr_combat_data.SkillID);
    ImGui::Text("Last Casted Skill Name: %s",
                curr_combat_data.SkillName.c_str());
    ImGui::Text("Last Event ID: %u", curr_combat_data.EventID);

    if (!keybinds.empty())
    {
        ImGui::Separator();
        ImGui::Text("Parsed Keybinds (sample):");

        int count = 0;
        for (const auto &[action_name, keybind_info] : keybinds)
        {
            if (count >= 5)
                break; // Show only first 5 for testing

            auto display_text = action_name + ": ";
            if (keybind_info.button != Keys::NONE)
            {
                display_text += keys_to_string(keybind_info.button);
                if (keybind_info.modifier != Modifiers::NONE)
                {
                    display_text += " + " + modifiers_to_string(keybind_info.modifier);
                }
            }
            else
            {
                display_text += "No binding";
            }

            ImGui::Text("%s", display_text.c_str());
            count++;
        }
    }
}

void RenderType::render_xml_selection()
{
    if (!Settings::XmlSettingsPath.empty())
    {
        if (!keybinds_loaded &&
            std::filesystem::exists(Settings::XmlSettingsPath))
        {
            keybinds = parse_xml_keybinds(Settings::XmlSettingsPath);

            keybinds_loaded = true;
        }
    }

    const auto button_width = ImGui::GetWindowSize().x * 0.5f -
                              ImGui::GetStyle().ItemSpacing.x * 0.5f;

    if (ImGui::Button("Select Keybinds", ImVec2(button_width, 0)))
    {
        OPENFILENAME ofn;
        CHAR szFile[260] = {0};

        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = "XML Files\0*.xml\0All Files\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = "C:/";
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        if (GetOpenFileName(&ofn) == TRUE)
        {
            Settings::XmlSettingsPath = std::filesystem::path(szFile);
            Settings::Save(Globals::SettingsPath);

            keybinds_loaded = false;
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("Unselect Keybinds", ImVec2(button_width, 0)))
    {
        Settings::XmlSettingsPath.clear();
        Settings::Save(Globals::SettingsPath);

        keybinds_loaded = false;
        keybinds.clear();
    }
}

void RenderType::render_options_checkboxes(bool &is_not_ui_adjust_active)
{
    const auto first_row_items =
        std::vector<std::string>{"Names", "Times", "Horizontal"};
    const auto centered_pos_row_1 =
        calculate_centered_position(first_row_items);
    ImGui::SetCursorPosX(centered_pos_row_1);

    if (ImGui::Checkbox("Names", &Settings::ShowSkillName))
    {
        Settings::Save(Globals::SettingsPath);
    }

    ImGui::SameLine();

    if (ImGui::Checkbox("Times", &Settings::ShowSkillTime))
    {
        Settings::Save(Globals::SettingsPath);
    }

    ImGui::SameLine();

    if (ImGui::Checkbox("Horizontal", &Settings::HorizontalSkillLayout))
    {
        Settings::Save(Globals::SettingsPath);
    }

    const auto second_row_items = std::vector<std::string>{
        "Move Skill UI",
        "Show Weapon Swaps",
    };
    const auto centered_pos_row_2 =
        calculate_centered_position(second_row_items);
    ImGui::SetCursorPosX(centered_pos_row_2);

    if (ImGui::Checkbox("Move Skill UI", &is_not_ui_adjust_active))
    {
    }

    ImGui::SameLine();

    if (ImGui::Checkbox("Show Weapon Swaps", &Settings::ShowWeaponSwap))
    {
        Settings::Save(Globals::SettingsPath);

        if (selected_file_path != "")
        {
            Globals::RotationRun.reset_rotation();
            Globals::RotationRun.load_data(selected_file_path, img_path);
        }
    }

    const auto third_row_items = std::vector<std::string>{"Show Keybind"};
    const auto centered_pos_row_3 =
        calculate_centered_position(third_row_items);
    ImGui::SetCursorPosX(centered_pos_row_3);

    if (ImGui::Checkbox("Show Keybind", &Settings::ShowKeybind))
    {
        Settings::Save(Globals::SettingsPath);
    }

#ifdef _DEBUG
    render_xml_selection();
#endif
}

void RenderType::render_options_window(bool &is_not_ui_adjust_active)
{
    if (ImGui::Begin("Rota Helper ###GW2RotaHelper_Options",
                     &Settings::ShowWindow))
    {
#ifdef _DEBUG
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f),
                           "RotaHelper BETA v%s",
                           Globals::VersionString.c_str());
#else
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f),
                           "RotaHelper v%s",
                           Globals::VersionString.c_str());
#endif

        render_select_bench();

        render_options_checkboxes(is_not_ui_adjust_active);

#ifdef _DEBUG
#ifdef GW2_NEXUS_ADDON
        if (ImGui::CollapsingHeader("Debug Data",
                                    ImGuiTreeNodeFlags_DefaultOpen))
        {
            render_debug_data();
        }
#else
        ImGuiIO &io = ImGui::GetIO();
        (void)io;
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                    1000.0f / io.Framerate,
                    io.Framerate);
#endif
#endif
    }

#ifndef _DEBUG
    if (!IsValidMap())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.1f, 0.1f, 1.0f),
                           "Rotation only shown in Training Area!");
    }
#endif

    ImGui::End();
}

void RenderType::render_select_bench()
{
    render_text_filter();

    if (benches_files.empty())
        return;

    render_selection();

    render_load_buttons();
}


void RenderType::render_text_filter()
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

    Settings::FilterBuffer = to_lowercase(std::string(filter_buffer));

    if (text_changed)
        open_combo_next_frame = true;

    filter_input_width = ImGui::GetItemRectSize().x;

    if (ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_Tab))
        open_combo_next_frame = true;

    ImGui::PopAllowKeyboardFocus();

    const auto &[_filtered_files, directories_with_matches] =
        get_file_data_pairs(benches_files, Settings::FilterBuffer);
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

void RenderType::render_selection()
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

                        ReleaseTextureMap(Globals::TextureMap);
                        Globals::RotationRun.load_data(selected_file_path,
                                                       img_path);

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

void RenderType::render_load_buttons()
{
    if (selected_bench_index >= 0 &&
        selected_bench_index < benches_files.size())
    {
        const auto button_width = ImGui::GetWindowSize().x * 0.5f -
                                  ImGui::GetStyle().ItemSpacing.x * 0.5f;

        if (ImGui::Button("Reload", ImVec2(button_width, 0)))
        {
            if (!selected_file_path.empty())
                restart_rotation();
        }

        ImGui::SameLine();

        if (ImGui::Button("Unload", ImVec2(button_width, 0)))
        {
            restart_rotation();
            Globals::RotationRun.reset_rotation();
        }
    }
}

void RenderType::restart_rotation()
{
    Globals::RotationRun.restart_rotation();
    ReleaseTextureMap(Globals::TextureMap);

    played_rotation.clear();
    curr_combat_data = EvCombatDataPersistent{};

    time_since_last_match = std::chrono::steady_clock::now();
    num_skills_wo_match = 0U;

    last_time_aa_did_skip = std::chrono::steady_clock::now();
    ArcEv::ResetSkillCastTracking();
}

void RenderType::render_rotation_window(const bool is_not_ui_adjust_active,
                                        ID3D11Device *pd3dDevice)
{
    float window_width = 0.0f;
    float window_height = 0.0f;
    ImGuiIO &io = ImGui::GetIO();

    if (!Settings::HorizontalSkillLayout)
    {
        window_width = 400.0f;
        window_height = 400.0f;
    }
    else
    {
        window_width = 600.0f;
        window_height = 100.0f;
    }

    ImGui::SetNextWindowSize(ImVec2(window_width, window_height),
                             ImGuiCond_FirstUseEver);
    float pos_x = (io.DisplaySize.x - window_width) * 0.5f;
    float pos_y = io.DisplaySize.y - window_height - 50.0f;
    ImGui::SetNextWindowPos(ImVec2(pos_x, pos_y), ImGuiCond_FirstUseEver);

    auto curr_flags_rota = flags_rota;
    if (is_not_ui_adjust_active)
    {
        curr_flags_rota &= ~ImGuiWindowFlags_NoBackground;
        curr_flags_rota &= ~ImGuiWindowFlags_NoMove;
        curr_flags_rota &= ~ImGuiWindowFlags_NoResize;
    }

    if (!Settings::HorizontalSkillLayout)
    {
        if (ImGui::Begin("##GW2RotaHelper_Rota_Details",
                         &Settings::ShowWindow,
                         curr_flags_rota))
        {
            const auto current_window_size = ImGui::GetWindowSize();
            Globals::SkillIconSize = min(current_window_size.x * 0.15f, 80.0f);
            Globals::SkillIconSize = max(Globals::SkillIconSize, 24.0f);

            render_rotation_details(pd3dDevice);
        }
    }
    else
    {
        if (ImGui::Begin("##GW2RotaHelper_Rota_Horizontal",
                         &Settings::ShowWindow,
                         curr_flags_rota))
        {
            const auto current_window_size = ImGui::GetWindowSize();
            Globals::SkillIconSize = min(current_window_size.y * 0.7f, 80.0f);
            Globals::SkillIconSize = max(Globals::SkillIconSize, 24.0f);

            render_rotation_horizontal(pd3dDevice);
        }
    }

    ImGui::End();
}

void RenderType::render_rotation_details(ID3D11Device *pd3dDevice)
{
    ImGui::Spacing();
    ImGui::Indent(10.0f);

    const auto [start, end, current_idx] =
        Globals::RotationRun.get_current_rotation_indices();

    for (int32_t window_idx = start; window_idx <= end; ++window_idx)
    {
        if (window_idx < 0 ||
            static_cast<size_t>(window_idx) >=
                Globals::RotationRun.all_rotation_steps.size())
            continue;

        const auto &rotation_step = Globals::RotationRun.get_rotation_skill(
            static_cast<size_t>(window_idx));
        const auto *texture =
            Globals::TextureMap[rotation_step.skill_data.icon_id];

        const auto skill_state =
            get_skill_state(Globals::RotationRun,
                            played_rotation,
                            window_idx,
                            current_idx,
                            rotation_step.skill_data.is_auto_attack);

        auto text = std::string{};
        if ((!Settings::ShowSkillName && !Settings::ShowSkillTime))
            text = "";
        else
            text = get_skill_text(rotation_step);

        render_rotation_icons(skill_state,
                              rotation_step,
                              texture,
                              text,
                              pd3dDevice);

        if (!text.empty())
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

    ImGui::Unindent(10.0f);
}

void RenderType::render_rotation_horizontal(ID3D11Device *pd3dDevice)
{
    ImGui::Spacing();
    ImGui::Indent(10.0f);

    const auto [start, end, current_idx] =
        Globals::RotationRun.get_current_rotation_indices();

    for (int32_t window_idx = start; window_idx <= end; ++window_idx)
    {
        if (window_idx < 0 ||
            static_cast<size_t>(window_idx) >=
                Globals::RotationRun.all_rotation_steps.size())
            continue;

        const auto &rotation_step = Globals::RotationRun.get_rotation_skill(
            static_cast<size_t>(window_idx));
        const auto *texture =
            Globals::TextureMap[rotation_step.skill_data.icon_id];

        const auto skill_state =
            get_skill_state(Globals::RotationRun,
                            played_rotation,
                            window_idx,
                            current_idx,
                            rotation_step.skill_data.is_auto_attack);
        const auto text = std::string{""};

        render_rotation_icons(skill_state,
                              rotation_step,
                              texture,
                              text,
                              pd3dDevice);

        ImGui::SameLine();
    }

    ImGui::Unindent(10.0f);
}


void RenderType::render_rotation_icons(const SkillState &skill_state,
                                       const RotationStep &rotation_step,
                                       const ID3D11ShaderResourceView *texture,
                                       const std::string &text,
                                       ID3D11Device *pd3dDevice)
{
    const auto is_special_skill = rotation_step.is_special_skill;

    if (skill_state.is_current && !skill_state.is_last) // white
        DrawRect(rotation_step, text, IM_COL32(255, 255, 255, 255), 7.0F);
    else if (skill_state.is_last) // pruple
        DrawRect(rotation_step, text, IM_COL32(128, 0, 128, 255));
    else if (rotation_step.skill_data.is_auto_attack) // orange
        DrawRect(rotation_step, text, IM_COL32(255, 165, 0, 255));

    if (texture && pd3dDevice)
    {
        auto tint_color = is_special_skill ? ImVec4(0.5f, 0.5f, 0.5f, 1.0f)
                                           : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        ImGui::Image((ImTextureID)texture,
                     ImVec2(Globals::SkillIconSize, Globals::SkillIconSize),
                     ImVec2(0, 0),
                     ImVec2(1, 1),
                     tint_color);

        if (Settings::ShowKeybind)
        {
            auto *draw_list = ImGui::GetWindowDrawList();
            auto icon_pos = ImGui::GetItemRectMin();
            auto icon_size = ImGui::GetItemRectSize();
            const auto skill_type = rotation_step.skill_data.skill_type;
            const auto keybind = get_keybind_str(skill_type);
            if (keybind != "")
            {
                auto text_size = ImGui::CalcTextSize(keybind.c_str());
                auto padding = 2.0f;
                auto text_pos =
                    ImVec2(icon_pos.x + icon_size.x - text_size.x - padding,
                           icon_pos.y + icon_size.y - text_size.y - padding);
                // Draw background for readability
                draw_list->AddRectFilled(ImVec2(text_pos.x - 2, text_pos.y - 1),
                                         ImVec2(text_pos.x + text_size.x + 2,
                                                text_pos.y + text_size.y + 1),
                                         IM_COL32(0, 0, 0, 180),
                                         3.0f);
                // Draw keybind text
                draw_list->AddText(text_pos,
                                   IM_COL32(255, 255, 255, 255),
                                   keybind.c_str());
            }
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();

            auto tooltip_text = get_skill_text(rotation_step);
            ImGui::Text("%s", tooltip_text.c_str());

            ImGui::EndTooltip();
        }
    }
    else
        ImGui::Dummy(ImVec2(Globals::SkillIconSize, Globals::SkillIconSize));
}

void RenderType::render(ID3D11Device *pd3dDevice)
{
    static auto is_not_ui_adjust_active = false;
    static auto time_went_ooc = std::chrono::steady_clock::now();

    if (!Settings::ShowWindow)
        return;

    if (key_press_event_in_this_frame)
    {
        const auto skill_ev = get_current_skill();
        key_press_event_in_this_frame = false;
        CycleSkillsLogic(skill_ev);
    }

    const auto curr_is_infight = IsInfight();
    if (!curr_is_infight)
    {
        auto now = std::chrono::steady_clock::now();
        auto time_since_went_ooc_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(now -
                                                                  time_went_ooc)
                .count();

        if (time_since_went_ooc_ms > 1000)
        {
            restart_rotation();
            time_went_ooc = std::chrono::steady_clock::now();
        }
    }
    else
    {
        time_went_ooc = std::chrono::steady_clock::now();
    }

    if (benches_files.size() == 0)
        benches_files = get_bench_files(bench_path);

    render_options_window(is_not_ui_adjust_active);

    if (Globals::RotationRun.futures.size() != 0)
    {
        if (Globals::RotationRun.futures.front().valid())
        {
            Globals::RotationRun.futures.front().get();
            Globals::RotationRun.futures.pop_front();
        }

        return;
    }

    if (Globals::RotationRun.all_rotation_steps.size() == 0)
        return;

    if (Globals::TextureMap.size() == 0)
    {
        Globals::TextureMap =
            LoadAllSkillTextures(pd3dDevice,
                                 Globals::RotationRun.log_skill_info_map,
                                 img_path);
    }

#ifndef _DEBUG
    if (!IsValidMap())
        return;
#endif

    render_rotation_window(is_not_ui_adjust_active, pd3dDevice);
}
