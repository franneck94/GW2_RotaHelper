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
            ImGui::Text("For now, you can download it manually from GitHub see: ");
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

void OptionsRenderType::render_precast_window(bool &show_precast_window)
{
    if (!show_precast_window)
        return;

    if (Globals::RotationRun.all_rotation_steps.size() > 0 && Globals::RenderData.current_build_key.empty())
        Globals::RenderData.current_build_key = Globals::RotationRun.meta_data.name;

    if (Globals::RenderData.precast_skills_order.empty() && !Globals::RenderData.current_build_key.empty() &&
        Globals::RotationRun.all_rotation_steps.size() > 0)
    {
        if (Settings::PrecastSkills.find(Globals::RenderData.current_build_key) != Settings::PrecastSkills.end())
            Globals::RenderData.precast_skills_order = Settings::PrecastSkills[Globals::RenderData.current_build_key];
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

        ImGui::Text("Build: %s", Globals::RenderData.current_build_key.c_str());

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

                        ImGui::PushID(static_cast<int>(i));

                        ImGui::Image((ImTextureID)skill.texture, ImVec2(icon_size, icon_size));

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
                            ImGui::PopID();
                            break;
                        }

                        ImGui::PopID();
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
        int icon_count = 0;

        for (const auto &[skill_id, rotation_skill] : Globals::RotationRun.rotation_skills)
        {
            if (rotation_skill.texture)
            {
                if (current_x + icon_size > ImGui::GetWindowWidth() - 20.0f && icon_count > 0)
                {
                    current_x = 0.0f;
                    ImGui::Dummy(ImVec2(0, spacing));
                }
                else if (icon_count > 0)
                {
                    ImGui::SameLine();
                }

                ImGui::PushID(static_cast<int>(skill_id));

                ImGui::Image((ImTextureID)rotation_skill.texture, ImVec2(icon_size, icon_size));

                if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                    Globals::RenderData.precast_skills_order.push_back(static_cast<uint32_t>(skill_id));

                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("%s", rotation_skill.name.c_str());
                    ImGui::Text("Click to add to precast list");
                    ImGui::EndTooltip();
                }

                ImGui::PopID();
                current_x += icon_size + spacing;
                icon_count++;
            }
        }

        ImGui::EndChild();

        ImGui::Separator();

        const auto button_width = ImGui::GetWindowSize().x * 0.33f - ImGui::GetStyle().ItemSpacing.x;

        if (ImGui::Button("Save", ImVec2(button_width, 0)))
        {
            if (!Globals::RenderData.current_build_key.empty())
            {
                Settings::PrecastSkills[Globals::RenderData.current_build_key] =
                    Globals::RenderData.precast_skills_order;
                Settings::Save(Globals::SettingsPath);

                char log_msg[256];
                snprintf(log_msg,
                         sizeof(log_msg),
                         "Precast skills configuration saved for build: %s",
                         Globals::RenderData.current_build_key.c_str());
                (void)Globals::APIDefs->Log(LOGL_DEBUG, "GW2RotaHelper", log_msg);
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Reset", ImVec2(button_width, 0)))
        {
            Globals::RenderData.precast_skills_order.clear();
            if (!Globals::RenderData.current_build_key.empty())
            {
                Settings::PrecastSkills.erase(Globals::RenderData.current_build_key);
                Settings::Save(Globals::SettingsPath);

                char log_msg[256];
                snprintf(log_msg,
                         sizeof(log_msg),
                         "Precast skills configuration reset for build: %s",
                         Globals::RenderData.current_build_key.c_str());
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
        std::string{"IMPORTANT: You have to load in a keybinds xml file."},
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

        static bool show_precast_window = false;
        const auto centered_pos_debug = calculate_centered_position({"Precast Window"});
        ImGui::SetCursorPosX(centered_pos_debug);

        if (ImGui::Button("Precast Window", ImVec2(sub_window_width, 0)))
            show_precast_window = !show_precast_window;

        if (Globals::RotationRun.rotation_skills.empty())
            Globals::RotationRun.get_rotation_skills();

        if (show_precast_window)
            render_precast_window(show_precast_window);
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
        Globals::RenderData.selected_file_path = file_info->full_path;

        Globals::RenderData.show_rotation_keybinds = false;
        Globals::RenderData.show_rotation_icons_overview = false;

        ReleaseTextureMap(Globals::TextureMap);
        Globals::RotationRun.load_data(Globals::RenderData.selected_file_path, Globals::RenderData.img_path);
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
                            Globals::RenderData.selected_file_path = file_info->full_path;

                            ReleaseTextureMap(Globals::TextureMap);
                            Globals::RotationRun.load_data(Globals::RenderData.selected_file_path,
                                                           Globals::RenderData.img_path);

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
