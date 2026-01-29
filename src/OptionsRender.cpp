#include <windows.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <numbers>
#include <set>
#include <sstream>
#include <string>

#include "imgui.h"
#include "mumble/Mumble.h"

#include "ArcEvents.h"
#include "Defines.h"
#include "FileUtils.h"
#include "LogData.h"
#include "MumbleUtils.h"
#include "OptionsRender.h"
#include "Render.h"
#include "RenderUtils.h"
#include "Rotation.h"
#include "Settings.h"
#include "Shared.h"
#include "SkillData.h"
#include "Textures.h"
#include "Types.h"
#include "TypesUtils.h"
#include "Version.h"

void OptionsRenderType::render()
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
            ImGui::Text("For now, you can download manually from GitHub see: ");
            ImGui::Text("https://github.com/franneck94/GW2_RotaHelper");
            ImGui::End();
            return;
        }

        render_select_bench();
        render_snowcrows_build_link();

        ImGui::Separator();
        render_horizontal_settings();
        render_options_checkboxes();

        render_status();
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

void OptionsRenderType::render_status()
{
    if (Globals::RenderData.benches_files.empty())
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

void OptionsRenderType::render_precast_window()
{
    if (!show_precast_window)
        return;

    // Early exit if rotation data is being updated/cleared
    if (Globals::RotationRun.rotation_skills.empty())
    {
        show_precast_window = false;
        return;
    }

    auto curr_build_key = Globals::RenderData.current_build_key;
    if (curr_build_key.empty() || Globals::RotationRun.all_rotation_steps.empty())
        return;

    if (Globals::RenderData.precast_skills_order.empty())
    {
        if (Settings::PrecastSkills.find(curr_build_key) != Settings::PrecastSkills.end())
            Globals::RenderData.precast_skills_order = Settings::PrecastSkills[curr_build_key];
    }

    ImGui::SetNextWindowSize(ImVec2(800, 500), ImGuiCond_Once);
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

        ImGui::Text("Build: %s", curr_build_key.c_str());

        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Precast Skills Order:");
        ImGui::Separator();

        if (Globals::RenderData.precast_skills_order.empty())
        {
            ImGui::TextDisabled("No precast skills configured yet.");
        }
        else
        {
            ImGui::BeginChild("precast_list", ImVec2(0, 100), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

            const float icon_size = 48.0f;
            const float spacing = ImGui::GetStyle().ItemSpacing.x;
            float current_x = 0.0f;
            int counter1 = 0;

            for (size_t i = 0; i < Globals::RenderData.precast_skills_order.size(); ++i)
            {
                auto skill_id = Globals::RenderData.precast_skills_order[i];
                const auto skill_it = Globals::RotationRun.rotation_skills.find(static_cast<SkillID>(skill_id));

                if (skill_it != Globals::RotationRun.rotation_skills.end())
                {
                    const auto &skill = skill_it->second;

                    if (skill.texture)
                    {
                        if (current_x + icon_size > ImGui::GetWindowWidth() - 20.0f && i > 0)
                        {
                            current_x = 0.0f;
                            ImGui::Dummy(ImVec2(0, spacing));
                        }
                        else if (i > 0)
                        {
                            ImGui::SameLine();
                        }

                        // ImGui::PushID(counter1 + 1'000'000); // Use safe incremental ID based on count
                        // ++counter1;

                        // ImGui::Image((ImTextureID)skill.texture, ImVec2(icon_size, icon_size));

                        if (ImGui::IsItemHovered())
                        {
                            ImGui::BeginTooltip();
                            ImGui::Text("%zu. %s", i + 1, skill.name.c_str());
                            ImGui::EndTooltip();
                        }

                        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 5.0f))
                            Globals::RenderData.precast_drag_source = i;

                        if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
                            Globals::RenderData.precast_drag_source >= 0 &&
                            Globals::RenderData.precast_drag_source != static_cast<int>(i))
                        {
                            // Swap items
                            std::swap(Globals::RenderData.precast_skills_order[Globals::RenderData.precast_drag_source],
                                      Globals::RenderData.precast_skills_order[i]);
                            Globals::RenderData.precast_drag_source = -1;
                        }

                        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
                        {
                            Globals::RenderData.precast_skills_order.erase(
                                Globals::RenderData.precast_skills_order.begin() + i);
                            // ImGui::PopID();
                            break;
                        }

                        // ImGui::PopID();
                        current_x += icon_size + spacing;
                    }
                }
            }

            ImGui::EndChild();
        }

        ImGui::Separator();

        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Available Skills:");
        ImGui::Separator();

        ImGui::BeginChild("available_skills", ImVec2(0, 150), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

        const float icon_size = 48.0f;
        const float spacing = ImGui::GetStyle().ItemSpacing.x;
        float current_x = 0.0f;
        int counter2 = 0;
        int icon_count = 0;

        for (const auto &[skill_id, rotation_skill] : Globals::RotationRun.rotation_skills)
        {
            // Comprehensive texture validation to prevent graphics driver crashes
            if (rotation_skill.texture && rotation_skill.texture != nullptr)
            {
                // Additional safety: verify texture is still valid by checking if the skill still exists
                auto current_skill_check = Globals::RotationRun.rotation_skills.find(skill_id);
                if (current_skill_check == Globals::RotationRun.rotation_skills.end() ||
                    current_skill_check->second.texture != rotation_skill.texture)
                {
                    continue; // Skip if texture has been invalidated
                }

                if (current_x + icon_size > ImGui::GetWindowWidth() - 20.0f && icon_count > 0)
                {
                    current_x = 0.0f;
                    ImGui::Dummy(ImVec2(0, spacing));
                }
                else if (icon_count > 0)
                {
                    ImGui::SameLine();
                }

                // ImGui::PushID(counter2 + 100'000); // Use safe incremental ID based on count
                // ++counter2;

                if (rotation_skill.texture != nullptr && (uintptr_t)rotation_skill.texture > 0x1000)
                {
                    // ImGui::Image((ImTextureID)rotation_skill.texture, ImVec2(icon_size, icon_size));
                }
                else
                {
                    // Render placeholder if texture is invalid
                    ImGui::Dummy(ImVec2(icon_size, icon_size));
                }

                if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                    Globals::RenderData.precast_skills_order.push_back(static_cast<uint32_t>(skill_id));

                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("%s", rotation_skill.name.c_str());
                    ImGui::Text("Click to add to precast list");
                    ImGui::EndTooltip();
                }

                // ImGui::PopID();
                current_x += icon_size + spacing;
                icon_count++;
            }
        }

        ImGui::EndChild();

        ImGui::Separator();

        const auto button_width = ImGui::GetWindowSize().x * 0.33f - ImGui::GetStyle().ItemSpacing.x;

        if (ImGui::Button("Save", ImVec2(button_width, 0)))
        {
            if (!curr_build_key.empty())
            {
                Settings::PrecastSkills[curr_build_key] = Globals::RenderData.precast_skills_order;
                Settings::Save(Globals::SettingsPath);

                char log_msg[256];
                snprintf(log_msg,
                         sizeof(log_msg),
                         "Precast skills configuration saved for build: %s",
                         curr_build_key.c_str());
                (void)Globals::APIDefs->Log(LOGL_DEBUG, "GW2RotaHelper", log_msg);
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Reset", ImVec2(button_width, 0)))
        {
            Globals::RenderData.precast_skills_order.clear();
            if (!curr_build_key.empty())
            {
                Settings::PrecastSkills.erase(curr_build_key);
                Settings::Save(Globals::SettingsPath);

                char log_msg[256];
                snprintf(log_msg,
                         sizeof(log_msg),
                         "Precast skills configuration reset for build: %s",
                         curr_build_key.c_str());
                (void)Globals::APIDefs->Log(LOGL_DEBUG, "GW2RotaHelper", log_msg);
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Remove Last", ImVec2(button_width, 0)))
        {
            if (!Globals::RenderData.precast_skills_order.empty())
                Globals::RenderData.precast_skills_order.pop_back();

            if (Globals::RenderData.precast_skills_order.empty())
                Settings::Save(Globals::SettingsPath);
        }
    }

    ImGui::End();
}

void SetCurrentSkillMappings()
{
    auto &current_mappings = Settings::UtilitySkillSlots[Globals::RenderData.current_build_key];

    if (current_mappings.empty() && Globals::RotationRun.skill_key_mapping.skill_7 != -1)
    {
        const auto &skill_key_mapping = Globals::RotationRun.skill_key_mapping;
        const auto &log_skill_info_map = Globals::RotationRun.log_skill_info_map;

        if (skill_key_mapping.skill_7 != -1)
        {
            auto heal_skill_it = log_skill_info_map.find(skill_key_mapping.skill_7);
            if (heal_skill_it != log_skill_info_map.end())
            {
                for (const auto &[skill_id, rotation_skill] : Globals::RotationRun.rotation_skills)
                {
                    if (rotation_skill.name == heal_skill_it->second.name)
                    {
                        current_mappings["HEAL"] = static_cast<uint32_t>(skill_id);
                        break;
                    }
                }
            }
        }

        if (skill_key_mapping.skill_8 != -1)
        {
            auto util1_skill_it = log_skill_info_map.find(skill_key_mapping.skill_8);
            if (util1_skill_it != log_skill_info_map.end())
            {
                for (const auto &[skill_id, rotation_skill] : Globals::RotationRun.rotation_skills)
                {
                    if (rotation_skill.name == util1_skill_it->second.name)
                    {
                        current_mappings["UTILITY_1"] = static_cast<uint32_t>(skill_id);
                        break;
                    }
                }
            }
        }

        if (skill_key_mapping.skill_9 != -1)
        {
            auto util2_skill_it = log_skill_info_map.find(skill_key_mapping.skill_9);
            if (util2_skill_it != log_skill_info_map.end())
            {
                for (const auto &[skill_id, rotation_skill] : Globals::RotationRun.rotation_skills)
                {
                    if (rotation_skill.name == util2_skill_it->second.name)
                    {
                        current_mappings["UTILITY_2"] = static_cast<uint32_t>(skill_id);
                        break;
                    }
                }
            }
        }
    }
}

void OptionsRenderType::render_skill_slots_window()
{
    if (!show_skill_slots_window)
        return;

    // Early exit if rotation data is being updated/cleared
    if (Globals::RotationRun.rotation_skills.empty())
    {
        show_skill_slots_window = false;
        return;
    }

    auto curr_build_key = Globals::RenderData.current_build_key;
    if (curr_build_key.empty() || Globals::RotationRun.all_rotation_steps.empty())
        return;

    SetCurrentSkillMappings();
    if (Settings::UtilitySkillSlots.find(curr_build_key) == Settings::UtilitySkillSlots.end())
        return;

    auto &current_mappings = Settings::UtilitySkillSlots[curr_build_key];
    static uint32_t selected_skill_for_assignment = 0;

    ImGui::SetNextWindowSize(ImVec2(700, 400), ImGuiCond_Once);
    if (ImGui::Begin("Skill Slot Mapping Configuration", &show_skill_slots_window))
    {
        ImGui::TextWrapped("Map your utility skills to their respective slot assignments for this build.");
        ImGui::Separator();

        if (Globals::RotationRun.all_rotation_steps.empty())
        {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "No rotation loaded. Load a build first.");
            ImGui::End();
            return;
        }

        ImGui::Text("Build: %s", curr_build_key.c_str());
        ImGui::Separator();

        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Utility Skill Slots:");
        ImGui::Separator();

        auto utility_slots = std::vector<std::pair<std::string, std::string>>{};
        auto used_skill_types = std::set<SkillSlot>{};

        for (const auto &[skill_id, rotation_skill] : Globals::RotationRun.rotation_skills)
            used_skill_types.insert(rotation_skill.skill_type);

        utility_slots.push_back({"HEAL", "Heal"});
        utility_slots.push_back({"UTILITY_1", "Utility 1"});
        utility_slots.push_back({"UTILITY_2", "Utility 2"});
        utility_slots.push_back({"UTILITY_3", "Utility 3"});
        utility_slots.push_back({"ELITE", "Elite"});

        for (const auto &[slot_key, slot_display] : utility_slots)
        {
            ImGui::Text("%s:", slot_display.c_str());
            ImGui::SameLine();
            ImGui::SetCursorPosX(120.0f);

            ImGui::BeginGroup();

            auto mapped_skill_id = current_mappings.find(slot_key);
            if (mapped_skill_id != current_mappings.end())
            {
                const auto skill_it =
                    Globals::RotationRun.rotation_skills.find(static_cast<SkillID>(mapped_skill_id->second));
                if (skill_it != Globals::RotationRun.rotation_skills.end())
                {
                    const auto &skill = skill_it->second;
                    ImGui::Text("%s", skill.name.c_str());

                    ImGui::SameLine();
                    if (ImGui::Button("Clear"))
                    {
                        auto key_to_erase = slot_key; // Make a copy to avoid reference issues
                        current_mappings.erase(key_to_erase);
                    }
                }
                else
                {
                    ImGui::TextDisabled("Unknown skill (ID: %u) - clearing invalid mapping", mapped_skill_id->second);
                    // Remove invalid mapping to prevent future issues
                    current_mappings.erase(mapped_skill_id);
                }
            }
            else
            {
                ImGui::TextDisabled("No skill assigned - select a skill below and click here");
            }

            ImGui::EndGroup();

            // Click-based assignment - check if this slot was clicked and we have a selected skill
            if (ImGui::IsItemClicked() && selected_skill_for_assignment != 0)
            {
                // Validate that the skill ID exists in the current rotation before mapping
                const auto skill_it =
                    Globals::RotationRun.rotation_skills.find(static_cast<SkillID>(selected_skill_for_assignment));
                if (skill_it != Globals::RotationRun.rotation_skills.end())
                {
                    current_mappings[slot_key] = selected_skill_for_assignment;
                    selected_skill_for_assignment = 0; // Clear selection after assignment
                }
            }
        }

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Available Skills:");
        ImGui::TextDisabled("Only skills used in this rotation are shown below.");
        ImGui::TextDisabled("Click a skill to select it, then click the slot above to assign it.");
        ImGui::Separator();

        ImGui::BeginChild("skill_selection", ImVec2(0, 150), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

        for (const auto &[skill_id, rotation_skill] : Globals::RotationRun.rotation_skills)
        {
            if (rotation_skill.skill_type != SkillSlot::HEAL && rotation_skill.skill_type != SkillSlot::UTILITY_1 &&
                rotation_skill.skill_type != SkillSlot::UTILITY_2 &&
                rotation_skill.skill_type != SkillSlot::UTILITY_3 && rotation_skill.skill_type != SkillSlot::ELITE)
            {
                continue;
            }

            // Use text-based selection instead of icons
            bool is_selected = (selected_skill_for_assignment == static_cast<uint32_t>(skill_id));

            if (is_selected)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f)); // Green text for selected
            }

            if (ImGui::Selectable(rotation_skill.name.c_str(), is_selected))
            {
                if (is_selected)
                {
                    selected_skill_for_assignment = 0; // Deselect if already selected
                }
                else
                {
                    selected_skill_for_assignment = static_cast<uint32_t>(skill_id);
                }
            }

            if (is_selected)
            {
                ImGui::PopStyleColor();
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Click to select/deselect, then click a slot above to assign");
                ImGui::EndTooltip();
            }
        }

        ImGui::EndChild();

        ImGui::Separator();

        const auto button_width = ImGui::GetWindowSize().x * 0.33f - ImGui::GetStyle().ItemSpacing.x;

        if (ImGui::Button("Save", ImVec2(button_width, 0)))
        {
            if (!curr_build_key.empty())
            {
                Settings::Save(Globals::SettingsPath);

                char log_msg[256];
                snprintf(log_msg, sizeof(log_msg), "Skill slot mapping saved for build: %s", curr_build_key.c_str());
                (void)Globals::APIDefs->Log(LOGL_DEBUG, "GW2RotaHelper", log_msg);
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Reset", ImVec2(button_width, 0)))
        {
            if (!curr_build_key.empty())
            {
                Settings::UtilitySkillSlots.erase(curr_build_key);
                Settings::Save(Globals::SettingsPath);

                char log_msg[256];
                snprintf(log_msg, sizeof(log_msg), "Skill slot mapping reset for build: %s", curr_build_key.c_str());
                (void)Globals::APIDefs->Log(LOGL_DEBUG, "GW2RotaHelper", log_msg);
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Clear All", ImVec2(button_width, 0)))
            current_mappings.clear();
    }

    ImGui::End();
}

void OptionsRenderType::render_horizontal_settings()
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

void OptionsRenderType::render_options_checkboxes()
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

    const auto second_row_items = std::vector<std::string>{"Move UI", "Show Weapon Swaps", "Show PreCasts"};
    const auto centered_pos_row_2 = calculate_centered_position(second_row_items);
    ImGui::SetCursorPosX(centered_pos_row_2);

    if (ImGui::Checkbox("Move UI", &Globals::RenderData.is_not_ui_adjust_active))
    {
    }
    SetTooltip("When enabled, you can move the rotation UI elements by dragging it.");

    ImGui::SameLine();

    if (ImGui::Checkbox("Show Weapon Swaps", &Settings::ShowWeaponSwap))
    {
        Settings::Save(Globals::SettingsPath);

        if (Globals::RenderData.selected_file_path != "")
        {
            Globals::RotationRun.reset_rotation();
            Globals::RotationRun.load_data(Globals::RenderData.selected_file_path, Globals::RenderData.img_path);
        }
    }
    SetTooltip("All weapon swap like skills will be shown in the rotation UI.");

    ImGui::SameLine();

    if (ImGui::Checkbox("Show PreCasts", &Settings::ShowPreCasts))
    {
        Settings::Save(Globals::SettingsPath);

        if (Globals::RenderData.selected_file_path != "")
        {
            Globals::RotationRun.reset_rotation();
            Globals::RotationRun.load_data(Globals::RenderData.selected_file_path, Globals::RenderData.img_path);
        }
    }
    SetTooltip("All weapon swap like skills will be shown in the rotation UI.");

    const auto third_row_items = std::vector<std::string>{
        "Show Keybind",
        "Easy Skill Mode",
        "Enable Keypress Logic",
    };
    const auto centered_pos_row_3 = calculate_centered_position(third_row_items);
    ImGui::SetCursorPosX(centered_pos_row_3);

    if (ImGui::Checkbox("Show Keybind", &Settings::ShowKeybind))
        Settings::Save(Globals::SettingsPath);
    SetTooltip(std::vector{
        std::string{"You can load keybinds from your GW2 XML settings file."},
        std::string{"If not selected, default keybinds will be used."},
    });

    ImGui::SameLine();

    if (ImGui::Checkbox("Easy Skill Mode", &Settings::EasySkillMode))
    {
        Settings::Save(Globals::SettingsPath);

        if (Globals::RenderData.selected_file_path != "")
        {
            Globals::RotationRun.reset_rotation();
            Globals::RotationRun.load_data(Globals::RenderData.selected_file_path, Globals::RenderData.img_path);
        }
    }
    SetTooltip(std::vector{
        std::string{"When enabled, some rotation skills are not shown or not mandatory to cast."},
        std::string{"For example on Mechanist the F skills are not shown to have a better overview as a beginner."},
        std::string{"For more info refer to the README.md."},
    });

    ImGui::SameLine();

    if (ImGui::Checkbox("Enable Keypress Logic", &Settings::UseSkillEvents))
    {
        Settings::Save(Globals::SettingsPath);
    }
    SetTooltip(std::vector{
        std::string{"EXPERIMENTAL: In addition to the arcdps events, the addon will try to detect skill activations by "
                    "keypresses."},
        std::string{"IMPORTANT: In this beta state only the default skill keybinds are used."},
    });

    render_xml_selection();

    if (!Globals::RotationRun.all_rotation_steps.empty())
    {
        ImGui::Separator();

        if (ImGui::Checkbox("Overview (Keys)", &Globals::RenderData.show_rotation_keybinds))
        {
            Settings::Save(Globals::SettingsPath);

            Globals::RotationRun.get_rotation_text(Globals::RenderData.keybinds);
        }
        SetTooltip(std::vector{
            std::string{"Shows the full rotation in a text form of the actual keybinds."},
            std::string{"Newline indicates a weapon swap like action."},
        });

        ImGui::SameLine();

        if (ImGui::Checkbox("Overview (Icons)", &Globals::RenderData.show_rotation_icons_overview))
        {
            Settings::Save(Globals::SettingsPath);

            Globals::RotationRun.get_rotation_icons();
        }
        SetTooltip(std::vector{
            std::string{"Shows the full rotation with skill icons, like in the simple rotation tab in dps.reports."},
            std::string{"Newline indicates a weapon swap like action."},
        });

        ImGui::SameLine();

        if (ImGui::Checkbox("Rotation Window", &Globals::RenderData.show_rotation_window))
        {
            Settings::Save(Globals::SettingsPath);
            Globals::RotationRun.get_rotation_text(Globals::RenderData.keybinds);
        }
        SetTooltip("Shows the rotation window of the last 2, the current and the next 7 skills.");

        const auto button_width = ImGui::GetWindowSize().x * 0.5f;

        if (ImGui::Button("Precast Window", ImVec2(button_width, 0)))
            show_precast_window = !show_precast_window;

        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowSize().x * 0.5f);

        if (ImGui::Button("Skill Slot Mapping", ImVec2(button_width, 0)))
            show_skill_slots_window = !show_skill_slots_window;

        if (Globals::RotationRun.rotation_skills.empty())
            Globals::RotationRun.get_rotation_skills();

        if (show_precast_window)
            render_precast_window();

        if (show_skill_slots_window)
            render_skill_slots_window();
    }

#ifdef _DEBUG
    const auto debug_button_width = ImGui::GetWindowSize().x * 0.5f - ImGui::GetStyle().ItemSpacing.x * 0.5f;

    static bool show_debug_window = false;
    if (ImGui::Button("Debug Window", ImVec2(debug_button_width, 0)))
        show_debug_window = !show_debug_window;

    ImGui::SameLine();

    if (ImGui::Button("Open settings.json", ImVec2(debug_button_width, 0)))
    {
        const auto settings_path = Globals::SettingsPath.string();
        ShellExecuteA(nullptr, "open", settings_path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }

    if (show_debug_window)
        render_debug_window(show_debug_window);
#endif
}

void OptionsRenderType::render_debug_window(bool &show_debug_window)
{
    if (ImGui::Begin("Debug Data###GW2RotaHelper_Debug", &show_debug_window))
        render_debug_data();

    ImGui::End();
}

void OptionsRenderType::render_debug_data()
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

    ImGui::Text("Last Casted Skill ID: %u", Globals::RenderData.curr_combat_data.SkillID);
    ImGui::Text("Last Casted Skill Name: %s",
                Globals::RenderData.curr_combat_data.SkillName != ""
                    ? Globals::RenderData.curr_combat_data.SkillName.c_str()
                    : "None");
    ImGui::Text("Last Arc Event Skill Name: %s",
                Globals::LastArcEventSkillName != "" ? Globals::LastArcEventSkillName.c_str() : "None");
    ImGui::Text("Last Event ID: %u", Globals::RenderData.curr_combat_data.EventID);
    ImGui::Text("Repeated skill: %s", Globals::RenderData.curr_combat_data.RepeatedSkill == true ? "true" : "false");
    ImGui::Text("Is Same Cast: %s", Globals::IsSameCast == true ? "true" : "false");
    const auto skill_data =
        SkillRuleData::GetDataByID(Globals::RenderData.curr_combat_data.SkillID, Globals::RotationRun.skill_data_map);
    ImGui::Text("Weapon Type of Skill: %s", weapon_type_to_string(skill_data.weapon_type).c_str());

    ImGui::Separator();

    ImGui::Text("Download State: %s", download_state_to_string(Globals::BenchDataDownloadState).c_str());

    if (!Globals::RenderData.keybinds.empty())
    {
        ImGui::Separator();

        ImGui::Text("Parsed Keybinds (sample):");

        for (const auto &[action_name, keybind_info] : Globals::RenderData.keybinds)
        {
            if (action_name.find("Skill") == std::string::npos)
                continue;

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
        }
    }

    ImGui::Separator();
    ImGui::Text("Currently Pressed Keys:");
    if (!Globals::CurrentlyPressedKeys.empty())
    {
        for (const auto &key_code : Globals::CurrentlyPressedKeys)
            ImGui::Text("%s", windows_key_to_string(static_cast<WindowsKeys>(key_code)).c_str());
    }
}

void OptionsRenderType::render_text_filter()
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
        get_file_data_pairs(Globals::RenderData.benches_files, Settings::FilterBuffer);
    filtered_files = _filtered_files;

    auto combo_preview = std::string{};
    if (selected_bench_index >= 0 && selected_bench_index < Globals::RenderData.benches_files.size())
        combo_preview = Globals::RenderData.benches_files[selected_bench_index].relative_path.filename().string();
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

void OptionsRenderType::set_data_on_build_load(const BenchFileInfo *const &file_info)
{
    Globals::RenderData.selected_file_path = file_info->full_path;
    Globals::RenderData.show_rotation_keybinds = false;
    Globals::RenderData.show_rotation_icons_overview = false;
    show_precast_window = false;
    show_skill_slots_window = false;
    // ReleaseTextureMap(Globals::TextureMap);
    Globals::RotationRun.load_data(Globals::RenderData.selected_file_path, Globals::RenderData.img_path);
    Globals::RenderData.current_build_key = Globals::RotationRun.meta_data.name;
    Globals::TextureMap = LoadAllSkillTextures(Globals::RenderData.pd3dDevice,
                                               Globals::RotationRun.log_skill_info_map,
                                               Globals::RenderData.img_path);
}

void OptionsRenderType::render_symbol_and_text(bool &is_selected,
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
        set_data_on_build_load(file_info);
        ImGui::CloseCurrentPopup();

        Globals::Render.restart_rotation(true);
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


void OptionsRenderType::render_red_cross_and_text(bool &is_selected,
                                                  const int original_index,
                                                  const BenchFileInfo *const &file_info,
                                                  const std::string base_formatted_name)
{
    auto draw_cross = draw_cross_factory(IM_COL32(220, 20, 60, 255));
    render_symbol_and_text(is_selected, original_index, file_info, base_formatted_name, "##red_cross_", draw_cross);
}

void OptionsRenderType::render_orange_cross_and_text(bool &is_selected,
                                                     const int original_index,
                                                     const BenchFileInfo *const &file_info,
                                                     const std::string base_formatted_name)
{
    auto draw_cross = draw_cross_factory(IM_COL32(255, 140, 0, 255));
    render_symbol_and_text(is_selected, original_index, file_info, base_formatted_name, "##ora_cross_", draw_cross);
}

void OptionsRenderType::render_untested_and_text(bool &is_selected,
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

void OptionsRenderType::render_tick_and_text(bool &is_selected,
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

void OptionsRenderType::render_selection()
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

                        auto category = Globals::RenderData.builds.get_build_category(file_info->display_name);
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
                            set_data_on_build_load(file_info);

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

void OptionsRenderType::render_load_buttons()
{
    if (selected_bench_index >= 0 && selected_bench_index < Globals::RenderData.benches_files.size())
    {
        const auto button_width = ImGui::GetWindowSize().x * 0.5f - ImGui::GetStyle().ItemSpacing.x * 0.5f;

        if (ImGui::Button("Reload", ImVec2(button_width, 0)))
        {
            if (!Globals::RenderData.selected_file_path.empty())
                Globals::Render.restart_rotation(true);
        }

        ImGui::SameLine();

        if (ImGui::Button("Unload", ImVec2(button_width, 0)))
        {
            Globals::Render.restart_rotation(true);
            Globals::RotationRun.reset_rotation();
        }
    }
}

void OptionsRenderType::render_snowcrows_build_link()
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

void OptionsRenderType::render_select_bench()
{
    render_text_filter();

    if (Globals::RenderData.benches_files.empty())
        return;

    render_selection();

    render_load_buttons();
}

void OptionsRenderType::render_xml_selection()
{
    if (!Settings::XmlSettingsPath.empty())
    {
        if (!Globals::RenderData.keybinds_loaded && std::filesystem::exists(Settings::XmlSettingsPath))
        {
            Globals::RenderData.keybinds = parse_xml_keybinds(Settings::XmlSettingsPath);

            Globals::RenderData.keybinds_loaded = true;
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

        Globals::RenderData.keybinds_loaded = false;
        Globals::RenderData.keybinds.clear();
    }
}
