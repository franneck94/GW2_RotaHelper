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

namespace
{
static const inline std::set<std::string_view> red_crossed_builds = {
    // POWER BUILDS
    "power_bladesworn",
    "power_alacrity_bladesworn",
    "power_alacrity_bladesworn_overcharged",
    "power_amalgam",
    "power_holosmith",
    "power_evoker",
    "power_evoker_scepter_dagger_pf",
    "power_deadeye_staff_and_dagger",
    // CONDITION BUILDS
    "condition_daredevil",
    "condition_firebrand",
    "condition_quickness_firebrand",
    "condition_evoker",
    "inferno_evoker_specialized_elements",
    "condition_evoker_specialized_elements",
    "condition_amalgam_steamshrieker",
    "condition_alacrity_amalgam_two_kits",
    "condition_holosmith_spear",
    "condition_boon_chronomancer",
    "condition_chronomancer",
    "condition_mirage_dagger",
    "condition_mirage_dune_cloak",
    "condition_mirage_ih_ether",
    "condition_mirage_ih_oasis",
    "condition_alacrity_mirage_staff",
    "condition_alacrity_mirage_axe_torch_staff",
};

static const inline std::set<std::string_view> orange_crossed_builds = {
    // POWER BUILDS
    "power_vindicator",
    "power_quickness_untamed",
    "power_quickness_untamed_maces",
    "power_quickness_untamed_offhand_mace",
    "power_catalyst_scepter_dagger_pf",
    "power_catalyst_sword_dagger_pf",
    "power_catalyst_scepter_dagger_inferno",
    // CONDI BUILDS
    "condition_catalyst",
    "condition_virtuoso",
    "condition_quickness_scrapper",
    "condition_quickness_untamed",
    "condition_alacrity_scourge",
};

static const inline std::set<std::string_view> yellow_tick_builds = {
    // POWER BUILDS
    "power_untamed",
    "power_untamed_sword_axe",
    "power_tempest_sword",
    "power_tempest_hammer",
    "power_herald",
    "power_luminary",
    "power_ritualist",
    "power_conduit",
    "power_virtuoso_spear_greatsword",
    "power_virtuoso_dagger_sword_greatsword",
    "power_virtuoso",
    "power_tempest",
    "power_tempest_inferno_scepter_dagger",
    "inferno_tempest_scepter_dagger",
    "power_chronomancer",
    "power_troubadour",
    // POWER BOON BUILDS
    "power_alacrity_tempest_inferno_scepter_focus",
    "power_quickness_herald",
    "power_boon_chronomancer",
    "power_alacrity_tempest_hammer",
    "power_alacrity_tempest",
    "power_alacrity_tempest_inferno_scepter_focus",
    "celestial_alacrity_scourge",
    "power_alacrity_renegade",
    "power_quickness_ritualist",
    // CONDITION BUILDS
    "condition_druid",
    "condition_thief",
    "condition_thief_spear",
    "condition_willbender",
    "condition_willbender_scepter",
    "condition_weaver_scepter",
    "condition_weaver_pistol",
    "condition_tempest",
    "condition_reaper",
    "condition_scourge",
    "condition_conduit",
    "condition_renegade",
    "condition_mechanist_kitless",
    "condition_soulbeast",
    "condition_soulbeast_shortbow",
    "condition_soulbeast_quickdraw",
    // CONDI BOON BUILDS
    "condition_alacrity_tempest_scepter",
    "condition_alacrity_renegade",
    "condition_alacrity_tempest",
    "condition_quickness_herald_shortbow",
    "condition_quickness_herald_spear",
    "condition_quickness_herald",
};

static const inline std::set<std::string_view> green_tick_builds = {
    // POWER BUILDS
    "power_berserker",
    "power_warrior",
    "power_spellbreaker",
    "power_berserker_axe_axe_axe_mace",
    "power_scrapper",
    "power_berserker_greatsword",
    "power_reaper",
    "power_reaper_greatsword_sword_sword",
    "power_harbinger_greatsword_dagger_sword",
    "power_harbinger_greatsword_sword_sword",
    "power_weaver",
    "power_mechanist",
    "power_mechanist_sword",
    "power_harbinger",
    "power_reaper_spear",
    "power_paragon",
    "power_galeshot",
    "power_soulbeast_hammer",
    "power_spellbreaker_hammer",
    "power_berserker_hammer_axe_mace",
    // POWER BOON BUILDS
    "power_quickness_herald_sword",
    "power_quickness_berserker",
    "power_alacrity_mechanist_sword",
    "power_quickness_berserker_greatsword",
    "power_quickness_scrapper",
    "power_alacrity_mechanist",
    "power_quickness_harbinger",
    "power_quickness_galeshot",
    // CONDITION BUILDS
    "condition_tempest_scepter",
    "condition_harbinger",
    "condition_berserker",
    "condition_mechanist",
    "condition_mechanist_two_kits",
    // CONDI BOON BUILDS
    "condition_quickness_harbinger",
    "condition_alacrity_mechanist_1_kit",
    "condition_alacrity_mechanist",
};

bool IsVersionIsRange(const std::string version,
                      const std::string &lower_version_bound,
                      const std::string &upper_version_bound)
{
    return version >= lower_version_bound && version <= upper_version_bound;
}

bool IsInBuildCategory(std::string_view display_name, const std::set<std::string_view> &category_builds)
{
    return (category_builds.find(display_name.substr(4)) != category_builds.end());
}

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

    auto total_width = text_size.x > 0 ? Globals::SkillIconSize + ImGui::GetStyle().ItemSpacing.x + text_size.x
                                       : Globals::SkillIconSize;
    auto total_height = (Globals::SkillIconSize > text_size.y) ? Globals::SkillIconSize : text_size.y;

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

    if (Settings::ShowSkillTime)
    {
        text += " (";
        char time_buffer[32];
        snprintf(time_buffer, sizeof(time_buffer), "%.2f", rotation_step.time_of_cast);
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
    (void)Globals::APIDefs->Log(ELogLevel_DEBUG, "GW2RotaHelper", debug_msg.c_str());

    SkillDetectionLogic(num_skills_wo_match, time_since_last_match, Globals::RotationRun, skill_ev);
}

float RenderType::calculate_centered_position(const std::vector<std::string> &items) const
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
            (void)Globals::APIDefs->Log(ELogLevel_DEBUG, "GW2RotaHelper", "Loaded XML InputBinds File.");
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
    const auto first_row_items = std::vector<std::string>{"Names", "Times", "Horizontal"};
    const auto centered_pos_row_1 = calculate_centered_position(first_row_items);
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
    const auto centered_pos_row_2 = calculate_centered_position(second_row_items);
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
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("All weapon swap like skills will be shown in the rotation UI.");
        ImGui::EndTooltip();
    }

    const auto third_row_items = std::vector<std::string>{"Show Keybind", "Strict Rotation", "Easy Skill Mode"};
    const auto centered_pos_row_3 = calculate_centered_position(third_row_items);
    ImGui::SetCursorPosX(centered_pos_row_3);

    if (ImGui::Checkbox("Show Keybind", &Settings::ShowKeybind))
    {
        Settings::Save(Globals::SettingsPath);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("You can load keybinds from your GW2 XML settings file.");
        ImGui::Text("If not selected, default keybinds will be used.");
        ImGui::EndTooltip();
    }

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
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("When enabled, rotation progression requires exact skill "
                    "matching (for not grayed out skills).");
        ImGui::Text("This will turn off the weapon swap icons.");
        ImGui::Text("When disabled, allows more flexible skill detection with "
                    "fallbacks.");
        ImGui::EndTooltip();
    }

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
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("When enabled, some rotation skills are not shown.");
        ImGui::Text("For example on Mechanist the F skills are not shown to have a better overview as a beginner.");
        ImGui::EndTooltip();
    }

#ifdef _DEBUG
    render_xml_selection();
#endif
}

void RenderType::render_options_window(bool &is_not_ui_adjust_active)
{
#ifdef _DEBUG
    const auto version_string = std::string("BETA v") + Globals::VersionString;
#else
    const auto version_string = std::string("v") + Globals::VersionString;
#endif

    const auto window_title = std::string("Rota Helper ") + version_string + "###GW2RotaHelper_Options";

    if (ImGui::Begin(window_title.c_str(), &Settings::ShowWindow))
    {
        if (Globals::ExtractedBenchData)
        {
            ImGui::Text("Successfully Downloaded and Extracted Bench Data.");
            ImGui::Text("Please restart the game or the addon.");
            ImGui::End();
            return;
        }

        if (Globals::BenchDataDownloadState == DownloadState::FAILED)
        {
            ImGui::Text("Failed Downloading/Extracting Bench Data. Please reach out to me.");
            ImGui::End();
            return;
        }

        render_select_bench();
        render_snowcrows_build_link();
        render_options_checkboxes(is_not_ui_adjust_active);

        if (benches_files.empty())
        {
            const auto missing_content_text = "IMPORTANT: Missing build files!";
            const auto centered_pos_missing = calculate_centered_position({missing_content_text});
            ImGui::SetCursorPosX(centered_pos_missing);
            ImGui::TextColored(ImVec4(1.0f, 0.1f, 0.1f, 1.0f), missing_content_text);

            const auto missing_content_text_2 = "Please download and extract the ZIP from GitHub.";
            const auto centered_pos_missing_2 = calculate_centered_position({missing_content_text_2});
            ImGui::SetCursorPosX(centered_pos_missing_2);
            ImGui::TextColored(ImVec4(1.0f, 0.1f, 0.1f, 1.0f), missing_content_text_2);
        }

        if (Globals::BenchFilesLowerVersionString != "" && Globals::BenchFilesUpperVersionString != "" &&
            Settings::VersionOfLastBenchFilesUpdate != "")
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
                        started_download = true;

                        const auto AddonPath = Globals::APIDefs->Paths.GetAddonDirectory("GW2RotaHelper");
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
        else if (Settings::VersionOfLastBenchFilesUpdate == "")
        {
            Settings::VersionOfLastBenchFilesUpdate =
                "0.1.0.0"; // init with oldest version to always update in next cycle
            Settings::Save(Globals::SettingsPath);
        }

#ifdef _DEBUG
#ifdef GW2_NEXUS_ADDON
        if (ImGui::CollapsingHeader("Debug Data", ImGuiTreeNodeFlags_DefaultOpen))
            render_debug_data();
#endif
#endif
    }

#ifndef _DEBUG
    if (!IsValidMap())
    {
        const auto warning_text = "NOTE: Rotation only shown in Training Area!";
        const auto centered_pos = calculate_centered_position({warning_text});
        ImGui::SetCursorPosX(centered_pos);

        ImGui::SetCursorPosX(centered_pos);
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), warning_text);
    }
#endif

    ImGui::End();
}

static void copy_to_clipboard(const std::string &url)
{
    if (OpenClipboard(nullptr))
    {
        EmptyClipboard();
        const auto size = (url.length() + 1) * sizeof(char);
        HGLOBAL h_mem = GlobalAlloc(GMEM_MOVEABLE, size);
        if (h_mem)
        {
            auto *buffer = static_cast<char *>(GlobalLock(h_mem));
            if (buffer)
            {
                strcpy_s(buffer, size, url.c_str());
                GlobalUnlock(h_mem);
                SetClipboardData(CF_TEXT, h_mem);
            }
        }
        CloseClipboard();
    }
}

void RenderType::render_snowcrows_build_link()
{
    if (Globals::RotationRun.meta_data.url.empty() ||
        Globals::RotationRun.meta_data.url.find("snowcrows.com") == std::string::npos)
        return;

    const auto button_width = ImGui::GetWindowSize().x * 0.5f - ImGui::GetStyle().ItemSpacing.x * 0.5f;

    const auto button_text = "Copy SC Build Link";
    if (ImGui::Button(button_text, ImVec2(button_width, 0)))
    {
        copy_to_clipboard(Globals::RotationRun.meta_data.url);
    }

    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Copy the Snow Crows build guide link to clipboard");

    ImGui::SameLine();

    const auto button_text2 = "Copy DPS Report Link";
    if (ImGui::Button(button_text2, ImVec2(button_width, 0)))
    {
        copy_to_clipboard(Globals::RotationRun.meta_data.dps_report_url);
    }

    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Copy the Dps.Report link to clipboard");
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

    ImGui::PushAllowKeyboardFocus(false);

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

    ImGui::PopAllowKeyboardFocus();

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

    // Make the selectable area
    if (ImGui::Selectable((selectable_id + std::to_string(original_index)).c_str(),
                          is_selected,
                          0,
                          ImVec2(0, item_height)))
    {
        selected_bench_index = original_index;
        selected_file_path = file_info->full_path;

        ReleaseTextureMap(Globals::TextureMap);
        Globals::RotationRun.load_data(selected_file_path, img_path);

        ImGui::CloseCurrentPopup();
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
        if (selectable_id.find("starred") != std::string::npos)
            ImGui::SetTooltip("Excellent working build");
        else if (selectable_id.find("red_crossed") != std::string::npos)
            ImGui::SetTooltip("Very bad working build");
        else if (selectable_id.find("orange_crossed") != std::string::npos)
            ImGui::SetTooltip("Poorly working build");
        else if (selectable_id.find("yellow_ticked") != std::string::npos)
            ImGui::SetTooltip("Okay-ish Working build");
        else if (selectable_id.find("green_ticked") != std::string::npos)
            ImGui::SetTooltip("Working build");
        else if (selectable_id.find("untested") != std::string::npos)
            ImGui::SetTooltip("Untested build");
    }

    auto text_pos =
        ImVec2(item_rect.x + symbol_size + 12, item_rect.y + (item_height - ImGui::GetTextLineHeight()) * 0.5f);
    draw_list->AddText(text_pos, ImGui::GetColorU32(ImGuiCol_Text), base_formatted_name.substr(4).c_str());
}

void RenderType::render_red_cross_and_text(bool &is_selected,
                                           const int original_index,
                                           const BenchFileInfo *const &file_info,
                                           const std::string base_formatted_name)
{
    auto draw_cross = [](ImDrawList *draw_list, ImVec2 center, float radius, float size) {
        float line_thickness = 2.0f;

        // Draw red cross (X shape)
        auto cross_top_left = ImVec2(center.x - radius * 0.7f, center.y - radius * 0.7f);
        auto cross_bottom_right = ImVec2(center.x + radius * 0.7f, center.y + radius * 0.7f);
        auto cross_top_right = ImVec2(center.x + radius * 0.7f, center.y - radius * 0.7f);
        auto cross_bottom_left = ImVec2(center.x - radius * 0.7f, center.y + radius * 0.7f);

        // Draw the two lines of the X
        draw_list->AddLine(cross_top_left, cross_bottom_right, IM_COL32(220, 20, 60, 255), line_thickness);
        draw_list->AddLine(cross_top_right, cross_bottom_left, IM_COL32(220, 20, 60, 255), line_thickness);
    };

    render_symbol_and_text(is_selected, original_index, file_info, base_formatted_name, "##red_crossed_", draw_cross);
}

void RenderType::render_orange_cross_and_text(bool &is_selected,
                                              const int original_index,
                                              const BenchFileInfo *const &file_info,
                                              const std::string base_formatted_name)
{
    auto draw_cross = [](ImDrawList *draw_list, ImVec2 center, float radius, float size) {
        float line_thickness = 2.0f;

        // Draw orange cross (X shape)
        auto cross_top_left = ImVec2(center.x - radius * 0.7f, center.y - radius * 0.7f);
        auto cross_bottom_right = ImVec2(center.x + radius * 0.7f, center.y + radius * 0.7f);
        auto cross_top_right = ImVec2(center.x + radius * 0.7f, center.y - radius * 0.7f);
        auto cross_bottom_left = ImVec2(center.x - radius * 0.7f, center.y + radius * 0.7f);

        // Draw the two lines of the X
        draw_list->AddLine(cross_top_left, cross_bottom_right, IM_COL32(255, 140, 0, 255), line_thickness);
        draw_list->AddLine(cross_top_right, cross_bottom_left, IM_COL32(255, 140, 0, 255), line_thickness);
    };

    render_symbol_and_text(is_selected,
                           original_index,
                           file_info,
                           base_formatted_name,
                           "##orange_crossed_",
                           draw_cross);
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

void RenderType::render_star_and_text(bool &is_selected,
                                      const int original_index,
                                      const BenchFileInfo *const &file_info,
                                      const std::string base_formatted_name)
{
    auto draw_star = [](ImDrawList *draw_list, ImVec2 center, float radius, float size) {
        ImVec2 star_points[10];
        for (int i = 0; i < 10; i++)
        {
            const auto angle = i * std::numbers::pi / 5.0f - std::numbers::pi / 2.0f;
            const auto star_radius = (i % 2 == 0) ? radius : radius * 0.4f;
            star_points[i] = ImVec2(center.x + cos(angle) * star_radius, center.y + sin(angle) * star_radius);
        }

        draw_list->AddConvexPolyFilled(star_points, 10, IM_COL32(255, 215, 0, 255));
        draw_list->AddPolyline(star_points, 10, IM_COL32(200, 150, 0, 255), true, 1.0f);
    };

    render_symbol_and_text(is_selected, original_index, file_info, base_formatted_name, "##starred_", draw_star);
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
            ImGui::TextDisabled(
                "No builds found. Please make sure to also download the ZIP file from Github w.r.t. the README.");
        }
        else
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

                        is_red_crossed = (IsInBuildCategory(file_info->display_name, red_crossed_builds) ||
                                          file_info->display_name.find("antiquary") != std::string::npos);
                        is_orange_crossed =
                            !is_red_crossed && (IsInBuildCategory(file_info->display_name, orange_crossed_builds) ||
                                                file_info->display_name.find("daredevil") != std::string::npos ||
                                                file_info->display_name.find("deadeye") != std::string::npos);
                        is_green_ticked =
                            !is_orange_crossed && IsInBuildCategory(file_info->display_name, green_tick_builds);
                        is_yellow_ticked =
                            !is_green_ticked && IsInBuildCategory(file_info->display_name, yellow_tick_builds);

                        is_untested = !is_green_ticked && !is_green_ticked && !is_red_crossed && !is_orange_crossed &&
                                      !is_yellow_ticked;

                        formatted_name_item = base_formatted_name + "##" + build_type_postdic;
                    }

                    if (is_red_crossed)
                    {
                        render_red_cross_and_text(is_selected, original_index, file_info, base_formatted_name);
                    }
                    else if (is_green_ticked)
                    {
                        render_tick_and_text(is_selected,
                                             original_index,
                                             file_info,
                                             base_formatted_name,
                                             IM_COL32(34, 139, 34, 255),
                                             "##green_ticked_");
                    }
                    else if (is_yellow_ticked)
                    {
                        render_tick_and_text(is_selected,
                                             original_index,
                                             file_info,
                                             base_formatted_name,
                                             IM_COL32(218, 165, 32, 255),
                                             "##yellow_ticked_");
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
    Globals::SkillLastTimeCast.clear();
}

void RenderType::render_rotation_window(const bool is_not_ui_adjust_active, ID3D11Device *pd3dDevice)
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

    if (!Settings::HorizontalSkillLayout)
    {
        if (ImGui::Begin("##GW2RotaHelper_Rota_Details", &Settings::ShowWindow, curr_flags_rota))
        {
            const auto current_window_size = ImGui::GetWindowSize();
            Globals::SkillIconSize = min(current_window_size.x * 0.15f, 80.0f);
            Globals::SkillIconSize = max(Globals::SkillIconSize, 24.0f);

            render_rotation_details(pd3dDevice);
        }
    }
    else
    {
        if (ImGui::Begin("##GW2RotaHelper_Rota_Horizontal", &Settings::ShowWindow, curr_flags_rota))
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

        auto text = std::string{};
        if ((!Settings::ShowSkillName && !Settings::ShowSkillTime))
            text = "";
        else
            text = get_skill_text(rotation_step);

        render_rotation_icons(skill_state, rotation_step, texture, text, pd3dDevice);

        if (!text.empty())
            ImGui::SameLine();

        if (!text.empty())
        {
            auto draw_list = ImGui::GetWindowDrawList();
            auto text_pos = ImGui::GetCursorScreenPos();

            draw_list->AddText(ImVec2(text_pos.x + 1, text_pos.y + 1), IM_COL32(0, 0, 0, 200), text.c_str());
            draw_list->AddText(text_pos, ImGui::GetColorU32(ImGuiCol_Text), text.c_str());
            ImGui::Dummy(ImGui::CalcTextSize(text.c_str()));
        }
    }

    ImGui::Unindent(10.0f);
}

void RenderType::render_rotation_horizontal(ID3D11Device *pd3dDevice)
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

        render_rotation_icons(skill_state, rotation_step, texture, text, pd3dDevice);

        ImGui::SameLine();
    }

    ImGui::Unindent(10.0f);
}

void RenderType::render_keybind(const RotationStep &rotation_step)
{
    auto *draw_list = ImGui::GetWindowDrawList();
    auto icon_pos = ImGui::GetItemRectMin();
    auto icon_size = ImGui::GetItemRectSize();
    const auto skill_type = rotation_step.skill_data.skill_type;

    auto keybind_str = std::string{};
    if (Settings::XmlSettingsPath.empty())
        keybind_str = skillslot_to_string(skill_type);
    else
    {
        const auto &[keybind, modifier] = get_keybind_for_skill_type(skill_type, keybinds);
        if (keybind == Keys::NONE)
            keybind_str = skillslot_to_string(skill_type);
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
            // Short text: bottom right corner
            text_pos = ImVec2(icon_pos.x + icon_size.x - text_size.x - padding,
                              icon_pos.y + icon_size.y - text_size.y - padding);
        }
        else
        {
            // Long text: bottom center
            text_pos = ImVec2(icon_pos.x + (icon_size.x - text_size.x) * 0.5f,
                              icon_pos.y + icon_size.y - text_size.y - padding);
        }

        // Draw background for readability
        draw_list->AddRectFilled(ImVec2(text_pos.x - 2, text_pos.y - 1),
                                 ImVec2(text_pos.x + text_size.x + 2, text_pos.y + text_size.y + 1),
                                 IM_COL32(0, 0, 0, 180),
                                 3.0f);
        // Draw keybind_str text
        draw_list->AddText(text_pos, IM_COL32(255, 255, 255, 255), keybind_str.c_str());
    }
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
        auto tint_color = is_special_skill ? ImVec4(0.5f, 0.5f, 0.5f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        ImGui::Image((ImTextureID)texture,
                     ImVec2(Globals::SkillIconSize, Globals::SkillIconSize),
                     ImVec2(0, 0),
                     ImVec2(1, 1),
                     tint_color);

        if (Settings::ShowKeybind)
        {
            render_keybind(rotation_step);
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();

            auto tooltip_text = get_skill_text(rotation_step);
            ImGui::Text("%s", tooltip_text.c_str());

            ImGui::EndTooltip();
        }
    }
    else if (rotation_step.skill_data.icon_id == 2)
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
    else if (rotation_step.skill_data.icon_id == 9)
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
    else
        ImGui::Dummy(ImVec2(Globals::SkillIconSize, Globals::SkillIconSize));
}

void RenderType::render(ID3D11Device *pd3dDevice)
{
    static auto is_not_ui_adjust_active = false;
    static auto time_went_ooc = std::chrono::steady_clock::now();

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
        Globals::TextureMap = LoadAllSkillTextures(pd3dDevice, Globals::RotationRun.log_skill_info_map, img_path);
    }

#ifndef _DEBUG
    if (!IsValidMap())
        return;
#endif

    render_rotation_window(is_not_ui_adjust_active, pd3dDevice);
}
