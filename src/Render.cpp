#include <windows.h>

#include <commdlg.h>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")
#include <d3d11.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <map>
#include <numbers>
#include <set>
#include <sstream>
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
#include "Rotation.h"
#include "Settings.h"
#include "Shared.h"
#include "SkillData.h"
#include "Textures.h"
#include "Types.h"
#include "TypesUtils.h"
#include "Version.h"

namespace
{
auto DODGE_ICON_ID = 2;
auto UNK_SKILL_ICON_ID = 0;
static auto last_time_aa_did_skip = std::chrono::steady_clock::time_point{};

bool IsVersionIsRange(const std::string version,
                      const std::string &lower_version_bound,
                      const std::string &upper_version_bound)
{
    return version >= lower_version_bound && version <= upper_version_bound;
}

void DrawRect(const RotationStep &rotation_step,
              const std::string &text,
              const ImU32 color,
              const float border_thickness = 2.0f,
              const float icon_size = Globals::SkillIconSize)
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

    auto total_width = text_size.x > 0 ? icon_size + ImGui::GetStyle().ItemSpacing.x + text_size.x : icon_size;
    auto total_height = (icon_size > text_size.y) ? icon_size : text_size.y;

    // Draw thick border by drawing outer filled rect and inner transparent rect
    draw_list->AddRectFilled(
        ImVec2(cursor_pos.x - border_thickness, cursor_pos.y - border_thickness),
        ImVec2(cursor_pos.x + total_width + border_thickness, cursor_pos.y + total_height + border_thickness),
        border_color);

    // Cut out the inner area to create the border effect
    draw_list->AddRectFilled(ImVec2(cursor_pos.x, cursor_pos.y),
                             ImVec2(cursor_pos.x + total_width, cursor_pos.y + total_height),
                             IM_COL32(0, 0, 0, 0)); // Transparent to cut out inner area
}

std::string get_skill_text(const RotationStep &rotation_step)
{
    auto text = rotation_step.skill_data.name;

    if (rotation_step.time_of_cast != 0.0f)
    {
        text += " (";
        char time_buffer[32];
        snprintf(time_buffer, sizeof(time_buffer), "%.2f", rotation_step.time_of_cast);
        text += time_buffer;
        text += ")";
    }

    return text;
}

void SetTooltip(const std::string &text)
{
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text(text.c_str());
        ImGui::EndTooltip();
    }
}

void SetTooltip(const std::vector<std::string> &texts)
{
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        for (const auto &text : texts)
            ImGui::Text(text.c_str());
        ImGui::EndTooltip();
    }
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

    builds.initialize_build_categories();
    benches_files = get_bench_files(bench_path);
}

void RenderType::append_to_played_rotation(const EvCombatDataPersistent &combat_data)
{
    if (played_rotation.size() > 300)
    {
        auto last_100 = std::vector<EvCombatDataPersistent>(played_rotation.end() - 100, played_rotation.end());
        played_rotation = std::move(last_100);
    }

    played_rotation.push_back(combat_data);
}

void RenderType::skill_activation_callback(EvCombatDataPersistent combat_data)
{
    if (curr_combat_data.SkillID == combat_data.SkillID)
        combat_data.RepeatedSkill = true;

    skill_event_in_this_frame = true;
    curr_combat_data = combat_data;

    append_to_played_rotation(combat_data);
}

EvCombatDataPersistent RenderType::get_current_skill()
{
    if (!skill_event_in_this_frame)
    {
        auto skill_ev = EvCombatDataPersistent{};
        skill_ev.SkillID = SkillID::FALLBACK;
        return skill_ev;
    }

    if (curr_combat_data.SkillID == SkillID::NONE)
    {
        auto skill_ev = EvCombatDataPersistent{};
        skill_ev.SkillID = SkillID::FALLBACK;
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
    if (Globals::RotationRun.missing_rotation_steps.empty())
        return;

    if (skill_ev.SkillID == SkillID::NONE || skill_ev.SkillID == SkillID::FALLBACK)
        return;

    const auto debug_msg = std::string{"Player Casted Skill: "} + skill_ev.SkillName;
    (void)Globals::APIDefs->Log(LOGL_DEBUG, "GW2RotaHelper", debug_msg.c_str());

    SkillDetectionLogic(num_skills_wo_match, time_since_last_match, Globals::RotationRun, skill_ev);
}

float RenderType::calculate_centered_position(const std::vector<std::string> &items, const float add_width) const
{
    auto total_width = 0.0F;

    for (size_t i = 0; i < items.size(); ++i)
    {
        total_width += ImGui::CalcTextSize(items[i].c_str()).x;
        total_width += ImGui::GetFrameHeight(); // XXX: ui element size?

        if (i < items.size() - 1)
            total_width += ImGui::GetStyle().ItemSpacing.x;
    }

    total_width -= add_width;

    const auto window_width = ImGui::GetWindowSize().x;
    return ((window_width - total_width) * 0.5F);
}

void RenderType::render_debug_data()
{
    ImGui::Separator();

    ImGui::Text("Profession: %d (%s)",
                static_cast<int>(Globals::Identity.Profession),
                profession_to_string(static_cast<ProfessionID>(Globals::Identity.Profession)).c_str());
    ImGui::Text("Specialization: %u (%s)",
                Globals::Identity.Specialization,
                elite_spec_to_string(static_cast<EliteSpecID>(Globals::Identity.Specialization)).c_str());
    ImGui::Text("Map ID: %u", Globals::Identity.MapID);

    ImGui::Separator();

    ImGui::Text("Is in Combat: %d", Globals::MumbleData->Context.IsInCombat == true ? "true" : "false");

    ImGui::Text("Last Casted Skill ID: %u", curr_combat_data.SkillID);
    ImGui::Text("Last Casted Skill Name: %s",
                curr_combat_data.SkillName != "" ? curr_combat_data.SkillName.c_str() : "None");
    ImGui::Text("Last Arc Event Skill Name: %s",
                Globals::LastArcEventSkillName != "" ? Globals::LastArcEventSkillName.c_str() : "None");
    ImGui::Text("Last Event ID: %u", curr_combat_data.EventID);
    ImGui::Text("Repeated skill: %s", curr_combat_data.RepeatedSkill == true ? "true" : "false");
    ImGui::Text("Is Same Cast: %s", Globals::IsSameCast == true ? "true" : "false");
    const auto skill_data = SkillRuleData::GetDataByID(curr_combat_data.SkillID, Globals::RotationRun.skill_data_map);
    ImGui::Text("Weapon Type of Skill: %s", weapon_type_to_string(skill_data.weapon_type).c_str());

    ImGui::Separator();

    ImGui::Text("Download State: %s", download_state_to_string(Globals::BenchDataDownloadState).c_str());

    if (!keybinds.empty())
    {
        ImGui::Separator();

        ImGui::Text("Parsed Keybinds (sample):");

        int count = 0;
        for (const auto &[action_name, keybind_info] : keybinds)
        {
            if (count >= 5)
                break;

            auto display_text = action_name + ": ";
            if (keybind_info.button != Keys::NONE)
            {
                display_text += custom_keys_to_string(keybind_info.button);
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

    if (!Globals::CurrentlyPressedKeys.empty())
    {
        ImGui::Separator();

        ImGui::Text("Currently Pressed Keys:");

        for (const auto &key_code : Globals::CurrentlyPressedKeys)
            ImGui::Text("%s", windows_key_to_string(static_cast<WindowsKeys>(key_code)).c_str());
    }
}

void RenderType::render_rotation_keybinds(bool &show_rotation_keybinds)
{
    if (!show_rotation_keybinds)
        return;

    ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_Once);
    if (ImGui::Begin("Rotation Keybinds", &show_rotation_keybinds))
    {
        for (const auto &text : Globals::RotationRun.rotation_text)
            ImGui::TextWrapped("%s", text.c_str());
    }

    ImGui::End();
}

void RenderType::render_rotation_icons_overview(bool &show_rotation_icons_overview)
{
    constexpr static auto icon_size = 32;

    if (!show_rotation_icons_overview)
        return;

    auto num_icons = 0;
    auto curr_rota_index =
        Globals::RotationRun.all_rotation_steps.size() - Globals::RotationRun.missing_rotation_steps.size();

    auto curr_flags_rota = flags_rota_overview;
    if (is_not_ui_adjust_active)
    {
        curr_flags_rota &= ~ImGuiWindowFlags_NoMove;
        curr_flags_rota &= ~ImGuiWindowFlags_NoResize;
    }

    ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_Once);
    if (ImGui::Begin("Rotation Icons Overview", &show_rotation_icons_overview, curr_flags_rota))
    {
        auto should_break = false;
        auto icons_in_line = 0;

        for (const auto &icon_lines : Globals::RotationRun.rotation_icon_lines)
        {
            bool first_in_line = true;
            for (const auto &line_data : icon_lines)
            {
                auto *texture = std::get<0>(line_data);
                auto name = std::get<1>(line_data);
                auto skill_id = std::get<2>(line_data);
                auto is_auto_attack = std::get<3>(line_data);

                if (!texture || !pd3dDevice)
                    continue;

                if (!first_in_line)
                {
                    icons_in_line++;

                    if ((icons_in_line > 0 && (icons_in_line % 15) != 0))
                        ImGui::SameLine();
                    else
                    {
                        icons_in_line = 0;
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
                    }
                }
                else
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);

                first_in_line = false;

                auto rotation_step = RotationStep{};
                rotation_step.skill_data.name = name;
                rotation_step.skill_data.skill_id = skill_id;
                rotation_step.skill_data.is_auto_attack = is_auto_attack;
                const auto is_current = num_icons == curr_rota_index;

                auto alpha_offset = 0.0f;
                if (do_highlight_skill)
                {
                    if (rotation_step.skill_data.skill_id == highlight_skill_id)
                        alpha_offset = 0.5f;
                    else
                        alpha_offset = -0.65f;
                }

                if (!do_highlight_skill && is_current)
                    DrawRect(rotation_step, "", IM_COL32(255, 255, 255, 255), 2.0F, icon_size);
                else if (rotation_step.skill_data.is_auto_attack) // orange
                    DrawRect(rotation_step, "", IM_COL32(255, 165, 0, 255), 2.0F);
                render_skill_texture(rotation_step, texture, 0, icon_size, false, alpha_offset);

                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) &&
                    ImGui::GetIO().KeyCtrl)
                    is_not_ui_adjust_active = !is_not_ui_adjust_active;
                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                {
                    do_highlight_skill = !do_highlight_skill;
                    if (do_highlight_skill)
                        highlight_skill_id = rotation_step.skill_data.skill_id;
                    else
                        highlight_skill_id = SkillID::NONE;
                }

                ++num_icons;
            }

            if (num_icons > 0)
                ImGui::Dummy(ImVec2(0, 2));
        }
    }

    ImGui::End();
}

void RenderType::render_precast_window(bool &show_precast_window)
{
    if (!show_precast_window)
        return;

    if (Globals::RotationRun.all_rotation_steps.size() > 0 && current_build_key.empty())
        current_build_key = Globals::RotationRun.meta_data.name;

    if (precast_skills_order.empty() && !current_build_key.empty() &&
        Globals::RotationRun.all_rotation_steps.size() > 0)
    {
        if (Settings::PrecastSkills.find(current_build_key) != Settings::PrecastSkills.end())
            precast_skills_order = Settings::PrecastSkills[current_build_key];
    }

    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_Once);
    if (ImGui::Begin("Precast Skills Configuration", &show_precast_window))
    {
        ImGui::TextWrapped("Drag and drop skills to arrange your precast order for this build.");
        ImGui::Separator();

        if (Globals::RotationRun.all_rotation_steps.empty())
        {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "No rotation loaded. Load a build first.");
            ImGui::End();
            return;
        }

        ImGui::Text("Build: %s", current_build_key.c_str());

        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Precast Skills Order:");
        ImGui::Separator();

        if (precast_skills_order.empty())
        {
            ImGui::TextDisabled("No precast skills configured yet.");
        }
        else
        {
            ImGui::BeginChild("precast_list", ImVec2(0, 150), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

            for (size_t i = 0; i < precast_skills_order.size(); ++i)
            {
                auto skill_id = precast_skills_order[i];
                const auto skill_it = Globals::RotationRun.rotation_skills.find(static_cast<SkillID>(skill_id));

                if (skill_it != Globals::RotationRun.rotation_skills.end())
                {
                    const auto &skill = skill_it->second;
                    auto label = std::to_string(i + 1) + ". " + skill.name + "###precast_" + std::to_string(i);

                    if (ImGui::Selectable(label.c_str(), false))
                        precast_drag_source = i;

                    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 5.0f))
                        precast_drag_source = i;

                    if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
                        precast_drag_source >= 0 && precast_drag_source != static_cast<int>(i))
                    {
                        // Swap items
                        std::swap(precast_skills_order[precast_drag_source], precast_skills_order[i]);
                        precast_drag_source = -1;
                    }
                }
            }

            ImGui::EndChild();
        }

        ImGui::Separator();

        // Available skills section
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Available Skills:");
        ImGui::Separator();

        ImGui::BeginChild("available_skills", ImVec2(0, 150), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        if (Globals::RotationRun.rotation_skills.empty())
            Globals::RotationRun.get_rotation_skills();

        for (const auto &[skill_id, rotation_skill] : Globals::RotationRun.rotation_skills)
        {
            // Check if skill is already in precast list
            auto is_in_precast =
                std::find(precast_skills_order.begin(), precast_skills_order.end(), static_cast<uint32_t>(skill_id)) !=
                precast_skills_order.end();

            auto label = rotation_skill.name + "###available_" + std::to_string(static_cast<uint32_t>(skill_id));

            ImGui::PushID(static_cast<int>(skill_id));

            if (is_in_precast)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f)); // Gray out
                ImGui::TextDisabled("%s", rotation_skill.name.c_str());
                ImGui::PopStyleColor();
            }
            else
            {
                if (ImGui::Selectable(label.c_str(), false))
                {
                    // Add skill to precast list
                    precast_skills_order.push_back(static_cast<uint32_t>(skill_id));
                }
            }

            ImGui::PopID();
        }

        ImGui::EndChild();

        ImGui::Separator();

        // Buttons
        const auto button_width = ImGui::GetWindowSize().x * 0.33f - ImGui::GetStyle().ItemSpacing.x;

        if (ImGui::Button("Save", ImVec2(button_width, 0)))
        {
            if (!current_build_key.empty())
            {
                Settings::PrecastSkills[current_build_key] = precast_skills_order;
                Settings::Save(Globals::SettingsPath);

                char log_msg[256];
                snprintf(log_msg,
                         sizeof(log_msg),
                         "Precast skills configuration saved for build: %s",
                         current_build_key.c_str());
                (void)Globals::APIDefs->Log(LOGL_DEBUG, "GW2RotaHelper", log_msg);
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Reset", ImVec2(button_width, 0)))
        {
            precast_skills_order.clear();
            if (!current_build_key.empty())
            {
                Settings::PrecastSkills.erase(current_build_key);
                Settings::Save(Globals::SettingsPath);

                char log_msg[256];
                snprintf(log_msg,
                         sizeof(log_msg),
                         "Precast skills configuration reset for build: %s",
                         current_build_key.c_str());
                (void)Globals::APIDefs->Log(LOGL_DEBUG, "GW2RotaHelper", log_msg);
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Remove Selected", ImVec2(button_width, 0)))
        {
            if (precast_drag_source >= 0 && precast_drag_source < static_cast<int>(precast_skills_order.size()))
            {
                precast_skills_order.erase(precast_skills_order.begin() + precast_drag_source);
                precast_drag_source = -1;
            }
        }
    }

    ImGui::End();
}

void RenderType::render_debug_window(bool &show_debug_window)
{
    if (ImGui::Begin("Debug Data###GW2RotaHelper_Debug", &show_debug_window))
        render_debug_data();

    ImGui::End();
}

bool RenderType::FileSelection()
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
        (void)Globals::APIDefs->Log(LOGL_DEBUG, "GW2RotaHelper", "Loaded XML InputBinds File.");

        // Immediately load keybinds when file is selected
        if (std::filesystem::exists(Settings::XmlSettingsPath))
        {
            keybinds = parse_xml_keybinds(Settings::XmlSettingsPath);
            keybinds_loaded = true;
            (void)Globals::APIDefs->Log(LOGL_DEBUG, "GW2RotaHelper", "parsed XML InputBinds File.");
        }

        return true;
    }

    return false;
}

void RenderType::render_xml_selection()
{
    if (!Settings::XmlSettingsPath.empty())
    {
        if (!keybinds_loaded && std::filesystem::exists(Settings::XmlSettingsPath))
        {
            keybinds = parse_xml_keybinds(Settings::XmlSettingsPath);

            keybinds_loaded = true;
        }
    }

    const auto button_width = ImGui::GetWindowSize().x * 0.5f - ImGui::GetStyle().ItemSpacing.x * 0.5f;

    if (ImGui::Button("Select Keybinds", ImVec2(button_width, 0)))
        FileSelection();

    ImGui::SameLine();

    if (ImGui::Button("Unselect Keybinds", ImVec2(button_width, 0)))
    {
        Settings::XmlSettingsPath.clear();
        Settings::Save(Globals::SettingsPath);

        keybinds_loaded = false;
        keybinds.clear();
    }
}

void RenderType::render_horizontal_settings()
{
    auto window_size_left = Settings::WindowSizeLeft;
    auto window_size_right = Settings::WindowSizeRight;
    auto window_size = window_size_right + window_size_left + 1;

    const auto items = std::vector<std::string>{
        "Window Size Left:",
        "-",
        std::to_string(Settings::WindowSizeLeft),
        "+",
        "Window Size Right:",
        "-",
        std::to_string(Settings::WindowSizeRight),
        "+",
    };
    const auto checkbox_size = ImGui::GetFrameHeight();
    const auto centered_pos = calculate_centered_position(items);
    ImGui::SetCursorPosX(centered_pos + 50.0F);

    ImGui::Text("Window Size Left:");
    ImGui::SameLine();

    if (ImGui::Button("-##left"))
    {
        if (Settings::WindowSizeLeft > 2)
        {
            Settings::WindowSizeLeft--;
            Settings::Save(Globals::SettingsPath);
        }
    }
    ImGui::SameLine();

    ImGui::Text("%d", Settings::WindowSizeLeft);
    ImGui::SameLine();

    if (ImGui::Button("+##left"))
    {
        if (Settings::WindowSizeLeft < 5)
        {
            Settings::WindowSizeLeft++;
            Settings::Save(Globals::SettingsPath);
        }
    }

    ImGui::SameLine();

    ImGui::Text("Window Size Right:");
    ImGui::SameLine();

    if (ImGui::Button("-##right"))
    {
        if (Settings::WindowSizeRight > 2)
        {
            Settings::WindowSizeRight--;
            Settings::Save(Globals::SettingsPath);
        }
    }
    ImGui::SameLine();

    ImGui::Text("%d", Settings::WindowSizeRight);
    ImGui::SameLine();

    if (ImGui::Button("+##right"))
    {
        if (Settings::WindowSizeRight < 10)
        {
            Settings::WindowSizeRight++;
            Settings::Save(Globals::SettingsPath);
        }
    }
}

void RenderType::render_options_checkboxes()
{
    const auto sub_window_width = ImGui::GetWindowSize().x * 0.3f;

    if (Settings::BenchUpdateFailedBefore)
    {
        const auto items = std::vector<std::string>{
            "Skip Update",
        };
        const auto centered_pos = calculate_centered_position(items);
        ImGui::SetCursorPosX(centered_pos);

        if (ImGui::Checkbox("Skip Update", &Settings::SkipBenchFileUpdate))
            Settings::Save(Globals::SettingsPath);
        SetTooltip("When enabled, the addon will skip checking for benchmark file updates on startup.");
    }

    const auto second_row_items = std::vector<std::string>{
        "Move UI",
        "Show Weapon Swaps",
    };
    const auto centered_pos_row_2 = calculate_centered_position(second_row_items);
    ImGui::SetCursorPosX(centered_pos_row_2);

    if (ImGui::Checkbox("Move UI", &is_not_ui_adjust_active))
    {
    }
    SetTooltip("When enabled, you can move the rotation UI elements by dragging it.");

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
    SetTooltip("All weapon swap like skills will be shown in the rotation UI.");

    const auto third_row_items = std::vector<std::string>{"Show Keybind", "Strict Rotation", "Easy Skill Mode"};
    const auto centered_pos_row_3 = calculate_centered_position(third_row_items);
    ImGui::SetCursorPosX(centered_pos_row_3);

    if (ImGui::Checkbox("Show Keybind", &Settings::ShowKeybind))
        Settings::Save(Globals::SettingsPath);
    SetTooltip(std::vector{
        std::string{"You can load keybinds from your GW2 XML settings file."},
        std::string{"If not selected, default keybinds will be used."},
    });

    ImGui::SameLine();

    if (ImGui::Checkbox("Strict Rotation", &Settings::StrictModeForSkillDetection))
    {
        Settings::Save(Globals::SettingsPath);

        if (selected_file_path != "")
        {
            Globals::RotationRun.reset_rotation();
            Globals::RotationRun.load_data(selected_file_path, img_path);
        }
    }
    SetTooltip(std::vector{
        std::string{"When enabled, rotation progression requires exact skill matching (for not grayed out skills)."},
        std::string{"This will turn off the weapon swap icons."},
        std::string{"When disabled, allows more flexible skill detection with fallbacks."},
    });

    ImGui::SameLine();

    if (ImGui::Checkbox("Easy Skill Mode", &Settings::EasySkillMode))
    {
        Settings::Save(Globals::SettingsPath);

        if (selected_file_path != "")
        {
            Globals::RotationRun.reset_rotation();
            Globals::RotationRun.load_data(selected_file_path, img_path);
        }
    }
    SetTooltip(std::vector{
        std::string{"When enabled, some rotation skills are not shown or not mandatory to cast."},
        std::string{"For example on Mechanist the F skills are not shown to have a better overview as a beginner."},
        std::string{"For more info refer to the README.md."},
    });

    render_xml_selection();

    if (!Globals::RotationRun.all_rotation_steps.empty())
    {
        ImGui::Separator();

        if (ImGui::Checkbox("Overview (Keys)", &show_rotation_keybinds))
        {
            Settings::Save(Globals::SettingsPath);

            Globals::RotationRun.get_rotation_text(keybinds);
        }
        SetTooltip(std::vector{
            std::string{"Shows the full rotation in a text form of the actual keybinds."},
            std::string{"Newline indicates a weapon swap like action."},
        });

        ImGui::SameLine();

        if (ImGui::Checkbox("Overview (Icons)", &show_rotation_icons_overview))
        {
            Settings::Save(Globals::SettingsPath);

            Globals::RotationRun.get_rotation_icons();
        }
        SetTooltip(std::vector{
            std::string{"Shows the full rotation with skill icons, like in the simple rotation tab in dps.reports."},
            std::string{"Newline indicates a weapon swap like action."},
        });

        ImGui::SameLine();

        if (ImGui::Checkbox("Rotation Window", &show_rotation_window))
        {
            Settings::Save(Globals::SettingsPath);
            Globals::RotationRun.get_rotation_text(keybinds);
        }
        SetTooltip("Shows the rotation window of the last 2, the current and the next 7 skills.");

        render_rotation_keybinds(show_rotation_keybinds);
        render_rotation_icons_overview(show_rotation_icons_overview);

        static bool show_precast_window = false;
        const auto centered_pos_debug = calculate_centered_position({"Precast Window"});
        ImGui::SetCursorPosX(centered_pos_debug);

        if (ImGui::Button("Precast Window", ImVec2(sub_window_width, 0)))
            show_precast_window = !show_precast_window;

        if (show_precast_window)
            render_precast_window(show_precast_window);
    }

#ifdef _DEBUG
    const auto centered_pos_debug = calculate_centered_position({"Debug Window"});
    ImGui::SetCursorPosX(centered_pos_debug);

    static bool show_debug_window = false;
    if (ImGui::Button("Debug Window", ImVec2(sub_window_width, 0)))
        show_debug_window = !show_debug_window;

    if (show_debug_window)
        render_debug_window(show_debug_window);
#endif
}

void RenderType::render_options_window()
{
#ifdef _DEBUG
    const auto version_string = std::string("BETA v") + Globals::VersionString;
#else
    const auto version_string = std::string("v") + Globals::VersionString;
#endif

    const auto window_title =
        std::string("Rota Helper ") + version_string + " (Builds from " + BUILD_STR + ")###GW2RotaHelper_Options";

    if (ImGui::Begin(window_title.c_str(), &Settings::ShowWindow))
    {
        if (Globals::ExtractedBenchData)
        {
            Settings::VersionOfLastBenchFilesUpdate = Globals::VersionString;
            Settings::BenchUpdateFailedBefore = false;
            Settings::Save(Globals::SettingsPath);

            ImGui::Text("Successfully Downloaded and Extracted Bench Data.");
            ImGui::Text("Please disable and re-enable the addon within Nexus.");
            ImGui::End();
            return;
        }

        if (Globals::BenchDataDownloadState == DownloadState::FAILED)
        {
            Settings::BenchUpdateFailedBefore = true;
            Settings::Save(Globals::SettingsPath);

            ImGui::Text("Failed Downloading/Extracting Bench Data.");
            ImGui::Text("Please send me a screenshot of the log messages.");
            ImGui::Text("For now, you can download it manually from GitHub see: ");
            ImGui::Text("https://github.com/franneck94/GW2_RotaHelper");
            ImGui::End();
            return;
        }

        render_select_bench();
        render_snowcrows_build_link();

        // SETTINGS
        ImGui::Separator();
        render_horizontal_settings();
        render_options_checkboxes();

        if (benches_files.empty())
        {
            const auto missing_content_text = "IMPORTANT: Missing build files!";
            const auto centered_pos_missing = calculate_centered_position({missing_content_text});
            ImGui::SetCursorPosX(centered_pos_missing);
            ImGui::TextColored(ImVec4(1.0f, 0.1f, 0.1f, 1.0f), missing_content_text);
        }

        if (!Settings::SkipBenchFileUpdate && Globals::BenchFilesLowerVersionString != "" &&
            Globals::BenchFilesUpperVersionString != "" && Settings::VersionOfLastBenchFilesUpdate != "")
        {
            if (!IsVersionIsRange(Settings::VersionOfLastBenchFilesUpdate,
                                  Globals::BenchFilesLowerVersionString,
                                  Globals::BenchFilesUpperVersionString))
            {
                const auto missing_content_text3 = "NOTE: There is a newer version for the builds.";
                const auto centered_pos_missing3 = calculate_centered_position({missing_content_text3});
                ImGui::SetCursorPosX(centered_pos_missing3);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), missing_content_text3);

                static bool started_download = false;
                const auto btn_text = !started_download ? "Start Downloading" : "Downloading...";
                const auto centered_pos = calculate_centered_position({btn_text});
                ImGui::SetCursorPosX(centered_pos);

                if (!started_download)
                {
                    if (ImGui::Button(btn_text))
                    {
                        const auto AddonPath = Globals::APIDefs->Paths_GetAddonDirectory("GW2RotaHelper");

                        if (MAJOR == 0 && MINOR == 28 && BUILD == 0)
                            DropOldBuilds(AddonPath);

                        char buffer[128] = {'\0'};
                        sprintf(buffer,
                                "Started download since: %s (%s) is not in range [%s, %s]",
                                Globals::VersionString.c_str(),
                                Settings::VersionOfLastBenchFilesUpdate.c_str(),
                                Globals::BenchFilesLowerVersionString.c_str(),
                                Globals::BenchFilesUpperVersionString.c_str());
                        (void)Globals::APIDefs->Log(LOGL_DEBUG, "GW2RotaHelper", buffer);

                        started_download = true;

                        Globals::BenchDataDownloadState = DownloadState::STARTED;
                        DownloadAndExtractDataAsync(AddonPath);
                    }
                }
                else
                {
                    ImGui::Text(btn_text);
                }
            }
        }
    }

    if (!IsValidMap())
    {
        const auto warning_text = "NOTE: Rotation tool is in PvP/WvW deactivated!";
        const auto centered_pos = calculate_centered_position({warning_text});
        ImGui::SetCursorPosX(centered_pos);

        ImGui::SetCursorPosX(centered_pos);
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), warning_text);
    }

    ImGui::End();
}

static void open_url_in_browser(const std::string &url)
{
    ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void RenderType::render_snowcrows_build_link()
{
    if (Globals::RotationRun.meta_data.url.empty() ||
        Globals::RotationRun.meta_data.url.find("snowcrows.com") == std::string::npos)
        return;

    const auto button_width = ImGui::GetWindowSize().x * 0.5f - ImGui::GetStyle().ItemSpacing.x * 0.5f;

    const auto button_text = "Open SC Build Link";
    if (ImGui::Button(button_text, ImVec2(button_width, 0)))
        open_url_in_browser(Globals::RotationRun.meta_data.url);

    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Open the Snow Crows build guide in your default browser");

    ImGui::SameLine();

    const auto button_text2 = "Open DPS Report Link";
    if (ImGui::Button(button_text2, ImVec2(button_width, 0)))
        open_url_in_browser(Globals::RotationRun.meta_data.dps_report_url);

    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Open the Dps.Report in your default browser");
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

    auto *filter_buffer = (char *)Settings::FilterBuffer;

    filter_input_pos = ImGui::GetCursorScreenPos();

    ImGui::SetNextItemWidth(ImGui::GetWindowWidth() * 0.75f);
    auto text_changed = ImGui::InputText("##filter",
                                         filter_buffer,
                                         sizeof(Settings::FilterBuffer),
                                         ImGuiInputTextFlags_EnterReturnsTrue);

    to_lowercase(Settings::FilterBuffer);

    if (text_changed)
        open_combo_next_frame = true;

    filter_input_width = ImGui::GetWindowWidth() * 0.75f;

    if (ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_Tab))
        open_combo_next_frame = true;

    const auto &[_filtered_files, directories_with_matches] =
        get_file_data_pairs(benches_files, Settings::FilterBuffer);
    filtered_files = _filtered_files;

    auto combo_preview = std::string{};
    if (selected_bench_index >= 0 && selected_bench_index < benches_files.size())
        combo_preview = benches_files[selected_bench_index].relative_path.filename().string();
    else
        combo_preview = "Select...";

    auto combo_preview_slice = std::string{"Select..."};
    if (combo_preview != "Select...")
    {
        combo_preview_slice = combo_preview.substr(0, combo_preview.size() - 8);
        formatted_name = format_build_name(combo_preview_slice);
        formatted_name = formatted_name.substr(4);
        if (Globals::RotationRun.meta_data.overall_dps > 0.0)
        {
            char buf[64];
            sprintf(buf, " (%.0f DPS)", Globals::RotationRun.meta_data.overall_dps);
            formatted_name += buf;
        }
    }
}

void RenderType::render_symbol_and_text(bool &is_selected,
                                        const int original_index,
                                        const BenchFileInfo *const &file_info,
                                        const std::string &base_formatted_name,
                                        const std::string &selectable_id,
                                        std::function<void(ImDrawList *, ImVec2, float, float)> draw_symbol_func)
{
    auto symbol_size = ImGui::GetTextLineHeight() * 0.8f;
    auto item_height = ImGui::GetTextLineHeightWithSpacing();

    if (ImGui::Selectable((selectable_id + std::to_string(original_index)).c_str(),
                          is_selected,
                          0,
                          ImVec2(0, item_height)))
    {
        selected_bench_index = original_index;
        selected_file_path = file_info->full_path;

        show_rotation_keybinds = false;
        show_rotation_icons_overview = false;

        ReleaseTextureMap(Globals::TextureMap);
        Globals::RotationRun.load_data(selected_file_path, img_path);

        ImGui::CloseCurrentPopup();

        restart_rotation(true);
    }

    auto item_rect = ImGui::GetItemRectMin();
    auto draw_list = ImGui::GetWindowDrawList();

    float symbol_center_x = item_rect.x + symbol_size * 0.5f + 4;
    float symbol_center_y = item_rect.y + item_height * 0.5f;
    float symbol_radius = symbol_size * 0.4f;

    // Call the custom drawing function
    draw_symbol_func(draw_list, ImVec2(symbol_center_x, symbol_center_y), symbol_radius, symbol_size);

    // Check if mouse is hovering over the symbol area only
    auto mouse_pos = ImGui::GetMousePos();
    auto symbol_rect_min = ImVec2(item_rect.x, item_rect.y);
    auto symbol_rect_max = ImVec2(item_rect.x + symbol_size + 8, item_rect.y + item_height);

    if (mouse_pos.x >= symbol_rect_min.x && mouse_pos.x <= symbol_rect_max.x && mouse_pos.y >= symbol_rect_min.y &&
        mouse_pos.y <= symbol_rect_max.y)
    {
        ImGui::BeginTooltip();
        if (selectable_id.find("starred") != std::string::npos)
            ImGui::Text("Excellent working build");
        else if (selectable_id.find("red_crossed") != std::string::npos)
            ImGui::Text("Very bad working build");
        else if (selectable_id.find("orange_crossed") != std::string::npos)
            ImGui::Text("Poorly working build");
        else if (selectable_id.find("yellow_ticked") != std::string::npos)
            ImGui::Text("Okay-ish Working build");
        else if (selectable_id.find("green_ticked") != std::string::npos)
            ImGui::Text("Working build");
        else if (selectable_id.find("untested") != std::string::npos)
            ImGui::Text("Untested build");
        ImGui::EndTooltip();
    }

    auto text_pos =
        ImVec2(item_rect.x + symbol_size + 12, item_rect.y + (item_height - ImGui::GetTextLineHeight()) * 0.5f);
    draw_list->AddText(text_pos, ImGui::GetColorU32(ImGuiCol_Text), base_formatted_name.substr(4).c_str());
}

auto draw_cross_factory(ImU32 color)
{
    return [color](ImDrawList *draw_list, ImVec2 center, float radius, float size) {
        float line_thickness = 2.0f;

        auto cross_top_left = ImVec2(center.x - radius * 0.7f, center.y - radius * 0.7f);
        auto cross_bottom_right = ImVec2(center.x + radius * 0.7f, center.y + radius * 0.7f);
        auto cross_top_right = ImVec2(center.x + radius * 0.7f, center.y - radius * 0.7f);
        auto cross_bottom_left = ImVec2(center.x - radius * 0.7f, center.y + radius * 0.7f);

        draw_list->AddLine(cross_top_left, cross_bottom_right, color, line_thickness);
        draw_list->AddLine(cross_top_right, cross_bottom_left, color, line_thickness);
    };
}

void RenderType::render_red_cross_and_text(bool &is_selected,
                                           const int original_index,
                                           const BenchFileInfo *const &file_info,
                                           const std::string base_formatted_name)
{
    auto draw_cross = draw_cross_factory(IM_COL32(220, 20, 60, 255));
    render_symbol_and_text(is_selected, original_index, file_info, base_formatted_name, "##red_cross_", draw_cross);
}

void RenderType::render_orange_cross_and_text(bool &is_selected,
                                              const int original_index,
                                              const BenchFileInfo *const &file_info,
                                              const std::string base_formatted_name)
{
    auto draw_cross = draw_cross_factory(IM_COL32(255, 140, 0, 255));
    render_symbol_and_text(is_selected, original_index, file_info, base_formatted_name, "##ora_cross_", draw_cross);
}

void RenderType::render_untested_and_text(bool &is_selected,
                                          const int original_index,
                                          const BenchFileInfo *const &file_info,
                                          const std::string base_formatted_name)
{
    auto draw_question_mark = [](ImDrawList *draw_list, ImVec2 center, float radius, float size) {
        float line_thickness = 2.5f;

        // Question mark curve (top part)
        auto curve_center = ImVec2(center.x, center.y - radius * 0.3f);
        auto curve_radius = radius * 0.4f;
        draw_list->AddCircle(curve_center, curve_radius, IM_COL32(128, 128, 128, 255), 16, line_thickness);

        // Remove bottom part of circle to make it look like a question mark
        auto mask_rect_min = ImVec2(center.x - curve_radius * 1.2f, center.y - radius * 0.1f);
        auto mask_rect_max = ImVec2(center.x + curve_radius * 1.2f, center.y + radius * 0.8f);
        draw_list->AddRectFilled(mask_rect_min, mask_rect_max, IM_COL32(0, 0, 0, 0));

        // Vertical line (middle part)
        auto line_start = ImVec2(center.x, center.y + radius * 0.1f);
        auto line_end = ImVec2(center.x, center.y + radius * 0.4f);
        draw_list->AddLine(line_start, line_end, IM_COL32(128, 128, 128, 255), line_thickness);

        // Dot (bottom part)
        auto dot_center = ImVec2(center.x, center.y + radius * 0.6f);
        draw_list->AddCircleFilled(dot_center, line_thickness * 0.6f, IM_COL32(128, 128, 128, 255));
    };

    render_symbol_and_text(is_selected,
                           original_index,
                           file_info,
                           base_formatted_name,
                           "##untested_",
                           draw_question_mark);
}

void RenderType::render_tick_and_text(bool &is_selected,
                                      const int original_index,
                                      const BenchFileInfo *const &file_info,
                                      const std::string base_formatted_name,
                                      const ImU32 Color,
                                      const std::string &label)
{
    auto draw_tick = [&Color](ImDrawList *draw_list, ImVec2 center, float radius, float size) {
        float line_thickness = 2.5f;

        auto tick_start = ImVec2(center.x - radius * 0.5f, center.y);
        auto tick_middle = ImVec2(center.x - radius * 0.1f, center.y + radius * 0.4f);
        auto tick_end = ImVec2(center.x + radius * 0.6f, center.y - radius * 0.5f);
        draw_list->AddLine(tick_start, tick_middle, Color, line_thickness);
        draw_list->AddLine(tick_middle, tick_end, Color, line_thickness);
    };

    render_symbol_and_text(is_selected, original_index, file_info, base_formatted_name, label, draw_tick);
}

void RenderType::render_selection()
{
    ImGui::Text("Select Bench File:");

    const auto window_pos = ImGui::GetWindowPos();
    const auto popup_pos = ImVec2(window_pos.x, filter_input_pos.y + ImGui::GetTextLineHeightWithSpacing() * 2.5f);
    ImGui::SetNextWindowPos(popup_pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(filter_input_width, 0), ImGuiCond_Always);

    if (open_combo_next_frame)
    {
        ImGui::OpenPopup("benches_popup");
        open_combo_next_frame = false;
    }

    if (ImGui::Button(formatted_name.c_str(), ImVec2(-1, 0)))
        ImGui::OpenPopup("benches_popup");

    if (ImGui::BeginPopup("benches_popup"))
    {
        if (filtered_files.empty())
        {
            ImGui::TextDisabled("No builds found w.r.t. filter text.");
        }
        else
        {
            ImGui::BeginChild("scrollable_bench_list",
                              ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 15),
                              false,
                              ImGuiWindowFlags_AlwaysVerticalScrollbar);

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
                    auto is_selected = (selected_bench_index == original_index);

                    auto base_formatted_name = std::string{};
                    auto formatted_name_item = std::string{};
                    auto is_red_crossed = false;
                    auto is_green_ticked = false;
                    auto is_orange_crossed = false;
                    auto is_yellow_ticked = false;
                    auto is_untested = false;

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
                        else if (path_str.find("condition") != std::string::npos)
                            build_type_postdic += "[Condi] ";

                        base_formatted_name = format_build_name(file_info->display_name);

                        auto category = builds.get_build_category(file_info->display_name);
                        is_red_crossed = (category == BuildCategory::RED_CROSSED);
                        is_orange_crossed = (category == BuildCategory::ORANGE_CROSSED);
                        is_green_ticked = (category == BuildCategory::GREEN_TICKED);
                        is_yellow_ticked = (category == BuildCategory::YELLOW_TICKED);
                        is_untested = (category == BuildCategory::UNTESTED);

                        formatted_name_item = base_formatted_name + "##" + build_type_postdic;
                    }

                    if (is_red_crossed)
                    {
                        continue;
                        // render_red_cross_and_text(is_selected, original_index, file_info, base_formatted_name);
                    }
                    else if (is_green_ticked)
                    {
                        const auto green = IM_COL32(34, 139, 34, 255);
                        render_tick_and_text(is_selected,
                                             original_index,
                                             file_info,
                                             base_formatted_name,
                                             green,
                                             "##green");
                    }
                    else if (is_yellow_ticked)
                    {
                        render_tick_and_text(is_selected,
                                             original_index,
                                             file_info,
                                             base_formatted_name,
                                             IM_COL32(218, 165, 32, 255),
                                             "##yellow");
                    }
                    else if (is_orange_crossed)
                    {
                        render_orange_cross_and_text(is_selected, original_index, file_info, base_formatted_name);
                    }
                    else if (is_untested)
                    {
                        render_untested_and_text(is_selected, original_index, file_info, base_formatted_name);
                    }
                    else
                    {
                        if (ImGui::Selectable(formatted_name_item.c_str(), is_selected))
                        {
                            selected_bench_index = original_index;
                            selected_file_path = file_info->full_path;

                            ReleaseTextureMap(Globals::TextureMap);
                            Globals::RotationRun.load_data(selected_file_path, img_path);

                            ImGui::CloseCurrentPopup();
                        }
                    }

                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndChild(); // End scrollable child window
        }
        ImGui::EndPopup();
    }
}

void RenderType::render_load_buttons()
{
    if (selected_bench_index >= 0 && selected_bench_index < benches_files.size())
    {
        const auto button_width = ImGui::GetWindowSize().x * 0.5f - ImGui::GetStyle().ItemSpacing.x * 0.5f;

        if (ImGui::Button("Reload", ImVec2(button_width, 0)))
        {
            if (!selected_file_path.empty())
                restart_rotation(true);
        }

        ImGui::SameLine();

        if (ImGui::Button("Unload", ImVec2(button_width, 0)))
        {
            restart_rotation(true);
            Globals::RotationRun.reset_rotation();
        }
    }
}

void RenderType::restart_rotation(const bool not_ooc_triggered)
{
    Globals::RotationRun.restart_rotation();

    played_rotation.clear();
    curr_combat_data = EvCombatDataPersistent{};

    time_since_last_match = std::chrono::steady_clock::now();
    num_skills_wo_match = 0U;

    last_time_aa_did_skip = std::chrono::steady_clock::now();
    ArcEv::ResetSkillCastTracking();
    Globals::SkillLastTimeCast.clear();

    if (not_ooc_triggered)
    {
        show_rotation_keybinds = false;
        show_rotation_icons_overview = false;
        show_rotation_window = true;
        do_highlight_skill = false;
        highlight_skill_id = SkillID::NONE;
    }
}

void RenderType::render_rotation_window()
{
    float window_width = 600.0f;
    float window_height = 100.0f;
    ImGuiIO &io = ImGui::GetIO();

    ImGui::SetNextWindowSize(ImVec2(window_width, window_height), ImGuiCond_FirstUseEver);
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

    if (ImGui::Begin("##GW2RotaHelper_Rota_Horizontal", &Settings::ShowWindow, curr_flags_rota))
    {
        const auto current_window_size = ImGui::GetWindowSize();
        Globals::SkillIconSize = min(current_window_size.y * 0.7f, 120.0f);
        Globals::SkillIconSize = max(Globals::SkillIconSize, 24.0f);

        render_rotation_horizontal();
    }

    ImGui::End();
}

void RenderType::render_rotation_horizontal()
{
    ImGui::Spacing();
    ImGui::Indent(10.0f);

    const auto [start, end, current_idx] = Globals::RotationRun.get_current_rotation_indices();

    for (int32_t window_idx = start; window_idx <= end; ++window_idx)
    {
        if (window_idx < 0 || static_cast<size_t>(window_idx) >= Globals::RotationRun.all_rotation_steps.size())
            continue;

        const auto &rotation_step = Globals::RotationRun.get_rotation_skill(static_cast<size_t>(window_idx));
        const auto *texture = Globals::TextureMap[rotation_step.skill_data.icon_id];

        const auto skill_state = get_skill_state(Globals::RotationRun,
                                                 played_rotation,
                                                 window_idx,
                                                 current_idx,
                                                 rotation_step.skill_data.is_auto_attack);
        const auto text = std::string{""};
        const int aa_index = (static_cast<size_t>(window_idx) < Globals::RotationRun.auto_attack_indices.size())
                                 ? Globals::RotationRun.auto_attack_indices[window_idx]
                                 : 0;
        render_rotation_icons(skill_state, rotation_step, texture, text, aa_index);

        ImGui::SameLine();
    }

    ImGui::Unindent(10.0f);
}

void RenderType::render_keybind(const RotationStep &rotation_step)
{
    auto *draw_list = ImGui::GetWindowDrawList();
    const auto icon_pos = ImGui::GetItemRectMin();
    const auto icon_size = ImGui::GetItemRectSize();

    const auto &skill_key_mapping = Globals::RotationRun.skill_key_mapping;
    const auto skill_type = rotation_step.skill_data.skill_type;
    const auto icon_id = rotation_step.skill_data.icon_id;

    auto keybind_str = std::string{};
    if (Settings::XmlSettingsPath.empty())
    {
        if (skill_key_mapping.skill_7 == icon_id)
            keybind_str = "7";
        else if (skill_key_mapping.skill_8 == icon_id)
            keybind_str = "8";
        else if (skill_key_mapping.skill_9 == icon_id)
            keybind_str = "9";
        else
            keybind_str = default_skillslot_to_string(skill_type);
    }
    else
    {
        const auto &[keybind, modifier] = get_keybind_for_skill_type(skill_type, keybinds);
        if (keybind == Keys::NONE)
        {
            if (skill_key_mapping.skill_7 == icon_id)
                keybind_str = "7";
            else if (skill_key_mapping.skill_8 == icon_id)
                keybind_str = "8";
            else if (skill_key_mapping.skill_9 == icon_id)
                keybind_str = "9";
            else
                keybind_str = default_skillslot_to_string(skill_type);
        }
        else
            keybind_str = custom_keys_to_string(keybind);

        if (modifier != Modifiers::NONE)
            keybind_str = modifiers_to_string(modifier) + " + " + keybind_str;
    }
    if (keybind_str != "")
    {
        auto text_size = ImGui::CalcTextSize(keybind_str.c_str());
        auto padding = 2.0f;
        auto text_pos = ImVec2{};

        if (keybind_str.length() <= 4)
        {
            text_pos = ImVec2(icon_pos.x + icon_size.x - text_size.x - padding,
                              icon_pos.y + icon_size.y - text_size.y - padding);
        }
        else
        {
            text_pos = ImVec2(icon_pos.x + (icon_size.x - text_size.x) * 0.5f,
                              icon_pos.y + icon_size.y - text_size.y - padding);
        }

        draw_list->AddRectFilled(ImVec2(text_pos.x - 2, text_pos.y - 1),
                                 ImVec2(text_pos.x + text_size.x + 2, text_pos.y + text_size.y + 1),
                                 IM_COL32(0, 0, 0, 180),
                                 3.0f);
        draw_list->AddText(text_pos, IM_COL32(255, 255, 255, 255), keybind_str.c_str());
    }
}

void RenderType::render_skill_texture(const RotationStep &rotation_step,
                                      const ID3D11ShaderResourceView *texture,
                                      const int auto_attack_index,
                                      const float icon_size,
                                      const bool show_keybind,
                                      const float alpha_offset)
{
    const auto is_special_skill = rotation_step.is_special_skill;
    auto tint_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    if (is_special_skill)
        tint_color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    if (alpha_offset != 0.0f)
        tint_color.w = max(0.1f, min(1.0f, tint_color.w + alpha_offset));

    ImGui::Image((ImTextureID)texture, ImVec2(icon_size, icon_size), ImVec2(0, 0), ImVec2(1, 1), tint_color);

    if (show_keybind)
        render_keybind(rotation_step);

    if (show_keybind && rotation_step.skill_data.is_auto_attack && auto_attack_index > 0)
    {
        auto *draw_list = ImGui::GetWindowDrawList();
        auto icon_pos = ImGui::GetItemRectMin();
        auto icon_size = ImGui::GetItemRectSize();

        auto index_str = std::to_string(auto_attack_index);
        auto text_size = ImGui::CalcTextSize(index_str.c_str());

        auto index_pos = ImVec2(icon_pos.x + 2, icon_pos.y + 2);

        auto circle_center = ImVec2(index_pos.x + text_size.x * 0.5f + 2, index_pos.y + text_size.y * 0.5f + 1);
        auto circle_radius = (text_size.x > text_size.y ? text_size.x : text_size.y) * 0.6f;
        draw_list->AddCircleFilled(circle_center, circle_radius, IM_COL32(255, 165, 0, 200));
        draw_list->AddCircle(circle_center, circle_radius, IM_COL32(255, 255, 255, 255), 0, 1.5f);

        draw_list->AddText(ImVec2(index_pos.x + 2, index_pos.y + 1), IM_COL32(255, 255, 255, 255), index_str.c_str());
    }

    auto tooltip_text = get_skill_text(rotation_step);
    SetTooltip(tooltip_text);
}

void RenderType::render_dodge_placeholder()
{
    auto draw_list = ImGui::GetWindowDrawList();
    auto cursor_pos = ImGui::GetCursorScreenPos();

    draw_list->AddRectFilled(cursor_pos,
                             ImVec2(cursor_pos.x + Globals::SkillIconSize, cursor_pos.y + Globals::SkillIconSize),
                             IM_COL32(200, 200, 200, 255)); // Light gray background

    auto text_size = ImGui::CalcTextSize("D");
    auto text_pos = ImVec2(cursor_pos.x + (Globals::SkillIconSize - text_size.x) * 0.5f,
                           cursor_pos.y + (Globals::SkillIconSize - text_size.y) * 0.5f);

    draw_list->AddText(text_pos, IM_COL32(0, 0, 0, 255), "D");
    ImGui::Dummy(ImVec2(Globals::SkillIconSize, Globals::SkillIconSize));
}

void RenderType::render_unknown_placeholder()
{
    auto draw_list = ImGui::GetWindowDrawList();
    auto cursor_pos = ImGui::GetCursorScreenPos();

    draw_list->AddRectFilled(cursor_pos,
                             ImVec2(cursor_pos.x + Globals::SkillIconSize, cursor_pos.y + Globals::SkillIconSize),
                             IM_COL32(200, 200, 200, 255)); // Light gray background

    auto text_size = ImGui::CalcTextSize("D");
    auto text_pos = ImVec2(cursor_pos.x + (Globals::SkillIconSize - text_size.x) * 0.1f,
                           cursor_pos.y + (Globals::SkillIconSize - text_size.y) * 0.5f);

    draw_list->AddText(text_pos, IM_COL32(0, 0, 0, 255), "Unknown");
    ImGui::Dummy(ImVec2(Globals::SkillIconSize, Globals::SkillIconSize));
}

void RenderType::render_empty_placeholder()
{
    ImGui::Dummy(ImVec2(Globals::SkillIconSize, Globals::SkillIconSize));
}

void RenderType::render_rotation_icons(const SkillState &skill_state,
                                       const RotationStep &rotation_step,
                                       const ID3D11ShaderResourceView *texture,
                                       const std::string &text,
                                       const int auto_attack_index)
{
    const auto is_special_skill = rotation_step.is_special_skill;

    if (skill_state.is_current && !skill_state.is_last) // white
        DrawRect(rotation_step, text, IM_COL32(255, 255, 255, 255), 7.0F);
    else if (skill_state.is_last) // pruple
        DrawRect(rotation_step, text, IM_COL32(128, 0, 128, 255), 2.0F);
    else if (rotation_step.skill_data.is_auto_attack) // orange
        DrawRect(rotation_step, text, IM_COL32(255, 165, 0, 255), 2.0F);

    if (texture && pd3dDevice)
        render_skill_texture(rotation_step, texture, auto_attack_index, Globals::SkillIconSize, Settings::ShowKeybind);
    else if (rotation_step.skill_data.icon_id == DODGE_ICON_ID)
        render_dodge_placeholder();
    else if (rotation_step.skill_data.icon_id == UNK_SKILL_ICON_ID)
        render_unknown_placeholder();
    else
        render_empty_placeholder();

    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        is_not_ui_adjust_active = !is_not_ui_adjust_active;
}

void RenderType::render(ID3D11Device *pd3dDevice)
{
    static auto time_went_ooc = std::chrono::steady_clock::now();

    this->pd3dDevice = pd3dDevice;

    if (!Settings::ShowWindow)
        return;

    if (skill_event_in_this_frame)
    {
        const auto skill_ev = get_current_skill();
        skill_event_in_this_frame = false;
        CycleSkillsLogic(skill_ev);
    }

    const auto curr_is_infight = IsInfight();
    if (!curr_is_infight)
    {
        auto now = std::chrono::steady_clock::now();
        auto time_since_went_ooc_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - time_went_ooc).count();

        if (time_since_went_ooc_ms > 1000)
        {
            restart_rotation(false);
            time_went_ooc = std::chrono::steady_clock::now();
        }
    }
    else
    {
        time_went_ooc = std::chrono::steady_clock::now();
    }

    if (benches_files.size() == 0)
        benches_files = get_bench_files(bench_path);

    render_options_window();

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
        Globals::TextureMap = LoadAllSkillTextures(pd3dDevice, Globals::RotationRun.log_skill_info_map, img_path);
    }

    if (!IsValidMap())
        return;

    if (show_rotation_window)
        render_rotation_window();
}
