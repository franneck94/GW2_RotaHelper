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

std::string to_lowercase(const std::string &str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::string get_current_profession_name()
{
    if (!Globals::MumbleData)
        return "";

    try
    {
        auto identity = ParseMumbleIdentity(Globals::MumbleData->Identity);
        return profession_to_string(
            static_cast<ProfessionID>(identity.Profession));
    }
    catch (...)
    {
        return "";
    }
}

std::pair<std::vector<std::pair<int, const BenchFileInfo *>>,
          std::set<std::string>>
get_file_data_pairs(std::vector<BenchFileInfo> &benches_files,
                    std::string &filter_string)
{
    auto filtered_files = std::vector<std::pair<int, const BenchFileInfo *>>{};
    auto directories_with_matches = std::set<std::string>{};

    if (filter_string.empty())
    {
        // When filter is empty, filter by current character's profession
        const auto profession = get_current_profession_name();
        auto current_profession = to_lowercase(profession);

        if (current_profession.empty())
        {
            // If no profession available, show all files
            for (int n = 0; n < benches_files.size(); n++)
                filtered_files.emplace_back(n, &benches_files[n]);
        }
        else
        {
            // Get elite specs for this profession
            auto profession_id = string_to_profession(current_profession);
            auto elite_specs = get_elite_specs_for_profession(profession_id);

            // First pass: find files that match the profession or elite specs and collect their directories
            for (int n = 0; n < benches_files.size(); n++)
            {
                const auto &file_info = benches_files[n];

                if (!file_info.is_directory_header)
                {
                    auto display_lower = to_lowercase(file_info.display_name);
                    auto path_lower =
                        to_lowercase(file_info.relative_path.string());

                    bool matches = false;

                    // Check if file matches current profession
                    if ((display_lower.find(current_profession) !=
                         std::string::npos) ||
                        (path_lower.find(current_profession) !=
                         std::string::npos))
                    {
                        matches = true;
                    }

                    // Check if file matches any elite spec for this profession
                    if (!matches)
                    {
                        for (const auto &elite_spec : elite_specs)
                        {
                            if (display_lower.find(elite_spec) !=
                                    std::string::npos ||
                                path_lower.find(elite_spec) !=
                                    std::string::npos)
                            {
                                matches = true;
                                break;
                            }
                        }
                    }

                    if (matches)
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
                    auto display_lower = to_lowercase(file_info.display_name);
                    auto path_lower =
                        to_lowercase(file_info.relative_path.string());

                    bool matches = false;

                    // Check if file matches current profession
                    if (display_lower.find(current_profession) !=
                            std::string::npos ||
                        path_lower.find(current_profession) !=
                            std::string::npos)
                    {
                        matches = true;
                    }

                    // Check if file matches any elite spec for this profession
                    if (!matches)
                    {
                        for (const auto &elite_spec : elite_specs)
                        {
                            if (display_lower.find(elite_spec) !=
                                    std::string::npos ||
                                path_lower.find(elite_spec) !=
                                    std::string::npos)
                            {
                                matches = true;
                                break;
                            }
                        }
                    }

                    if (matches)
                    {
                        filtered_files.emplace_back(n, &file_info);
                    }
                }
            }
        }

        return std::make_pair(filtered_files, directories_with_matches);
    }

    // First pass: find all files that match and collect their directories
    for (int n = 0; n < benches_files.size(); n++)
    {
        const auto &file_info = benches_files[n];

        if (!file_info.is_directory_header)
        {
            auto display_lower = to_lowercase(file_info.display_name);

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
            auto display_lower = to_lowercase(file_info.display_name);

            if (display_lower.find(filter_string) != std::string::npos)
            {
                filtered_files.emplace_back(n, &file_info);
            }
        }
    }

    return std::make_pair(filtered_files, directories_with_matches);
}

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


bool IsSkillAutoAttack(const uint64_t skill_id,
                       const std::string &skill_name,
                       const SkillDataMap &skill_data_map)
{
    auto it = skill_data_map.find(static_cast<int>(skill_id));

    if (it != skill_data_map.end())
        return it->second.is_auto_attack;

    for (const auto &[skill_id, skill_data] : skill_data_map)
    {
        if (skill_data.name == skill_name)
        {
            return skill_data.is_auto_attack;
        }
    }

    return false; // Default to false if skill not found
}

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

bool CheckTheNextNskills(const EvCombatDataPersistent &skill_ev,
                         const RotationStep &future_rota_skill,
                         const uint32_t n,
                         const bool is_okay,
                         RotationRunType &rotation_run,
                         EvCombatDataPersistent &last_skill)
{
    auto is_match =
        ((future_rota_skill.skill_data.name == skill_ev.SkillName) && is_okay);

    if (is_match)
    {
        for (uint32_t i = 0; i < n; ++i)
            rotation_run.todo_rotation_steps.pop_front();

        last_skill = skill_ev;
    }

    return is_match;
}

void SimpleSkillDetectionLogic(
    uint32_t &num_skills_wo_match,
    std::chrono::steady_clock::time_point &time_since_last_match,
    RotationRunType &rotation_run,
    const EvCombatDataPersistent &skill_ev,
    EvCombatDataPersistent &last_skill)
{
    static auto wait_time_point = std::chrono::steady_clock::now();

    auto curr_rota_skill = RotationStep{};
    auto next_rota_skill = RotationStep{};
    auto next_next_rota_skill = RotationStep{};
    auto next_next_next_rota_skill = RotationStep{};
    auto it = rotation_run.todo_rotation_steps.begin();

    auto now = std::chrono::steady_clock::now();
    auto duration_since_wait =
        std::chrono::duration_cast<std::chrono::milliseconds>(now -
                                                              wait_time_point)
            .count();

    if (duration_since_wait < 50) // wait 50ms to detect next skill
        return;
    wait_time_point = std::chrono::steady_clock::now();

    if (num_skills_wo_match == 0)
        time_since_last_match = std::chrono::steady_clock::now();

    if (rotation_run.todo_rotation_steps.size() > 1)
    {
        curr_rota_skill = *it;

        while (curr_rota_skill.is_special_skill &&
               rotation_run.todo_rotation_steps.size() > 2)
        {
            rotation_run.todo_rotation_steps.pop_front();
            it = rotation_run.todo_rotation_steps.begin();
            curr_rota_skill = *it;
        }

        ++it;
    }
    if (rotation_run.todo_rotation_steps.size() > 2)
    {
        next_rota_skill = *it;
        ++it;
    }
    if (rotation_run.todo_rotation_steps.size() > 3)
    {
        next_next_rota_skill = *it;
        ++it;
    }
    if (rotation_run.todo_rotation_steps.size() > 4)
    {
        next_next_next_rota_skill = *it;
        ++it;
    }

    if (CheckTheNextNskills(skill_ev,
                            curr_rota_skill,
                            1,
                            true,
                            rotation_run,
                            last_skill))
    {
        num_skills_wo_match = 0U;
        return;
    }

#ifdef USE_SKIP_NEXT_SKILL
    const auto time_since_last_aa_skip =
        std::chrono::duration_cast<std::chrono::seconds>(now -
                                                         last_time_aa_did_skip)
            .count();

    const auto include_aa_skip = (time_since_last_aa_skip > 3 ||
                                  !next_rota_skill.skill_data.is_auto_attack);
    if (include_aa_skip && CheckTheNextNskills(skill_ev,
                                               next_rota_skill,
                                               2,
                                               true,
                                               rotation_run,
                                               last_skill))
    {
        num_skills_wo_match = 0U;

        if (next_rota_skill.skill_data.is_auto_attack &&
            next_next_rota_skill.skill_data.is_auto_attack)
        {
            last_time_aa_did_skip = std::chrono::steady_clock::now();
        }

        return;
    }
#endif

    const auto curr_is_auto_attack =
        IsSkillAutoAttack(skill_ev.SkillID,
                          skill_ev.SkillName,
                          Globals::RotationRun.skill_data);

#ifdef USE_SKIP_NEXT_NEXT_SKILL
    const auto next_next_is_okay =
        (next_next_rota_skill.is_special_skill ||
         !next_next_rota_skill.skill_data.is_auto_attack);

    if (!curr_is_auto_attack && CheckTheNextNskills(skill_ev,
                                                    next_next_rota_skill,
                                                    3,
                                                    next_next_is_okay,
                                                    rotation_run,
                                                    last_skill))
    {
        num_skills_wo_match = 0U;
        return;
    }
#endif

#ifdef USE_SKIP_NEXT_NEXT_NEXT_SKILL
    const auto next_next_next_is_okay =
        (next_next_rota_skill.is_special_skill ||
         !next_next_next_rota_skill.skill_data.is_auto_attack);

    if (!curr_is_auto_attack && CheckTheNextNskills(skill_ev,
                                                    next_next_next_rota_skill,
                                                    4,
                                                    next_next_next_is_okay,
                                                    rotation_run,
                                                    last_skill))
    {
        num_skills_wo_match = 0U;
        return;
    }
#endif

    if (!curr_is_auto_attack)
        ++num_skills_wo_match;

    if (num_skills_wo_match > 5)
    {
        if (curr_rota_skill.skill_data.is_auto_attack || curr_is_auto_attack)
            return;

        const auto now = std::chrono::steady_clock::now();
        const auto duration_since_last_match =
            std::chrono::duration_cast<std::chrono::seconds>(
                now - time_since_last_match)
                .count();

        if (duration_since_last_match < 10)
            return;

        for (auto it = rotation_run.todo_rotation_steps.begin();
             it != rotation_run.todo_rotation_steps.end();
             ++it)
        {
            const auto diff =
                std::distance(it, rotation_run.todo_rotation_steps.begin());
            if (diff > 6)
                return;

            const auto rota_skill = *it;
            if (rota_skill.skill_data.name == skill_ev.SkillName)
            {
                while (rotation_run.todo_rotation_steps.begin() != it)
                    rotation_run.todo_rotation_steps.pop_front();

                rotation_run.todo_rotation_steps.pop_front();

                last_skill = skill_ev;
                num_skills_wo_match = 0U;
                time_since_last_match = now;
                return;
            }
        }
    }
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

SkillState get_skill_state(
    const RotationRunType &rotation_run,
    const std::vector<EvCombatDataPersistent> &played_rotation,
    const size_t window_idx,
    const size_t current_idx,
    const bool is_auto_attack)
{
    const auto is_current = (window_idx == static_cast<int32_t>(current_idx));
    const auto is_last =
        (window_idx == Globals::RotationRun.all_rotation_steps.size() - 1);
    const auto is_completed = (window_idx < static_cast<int32_t>(current_idx));

    auto is_completed_correct = false;
    auto is_completed_incorrect = false;
    if (played_rotation.size() > window_idx && window_idx < current_idx)
    {
        const auto casted_skill = played_rotation[window_idx];
        const auto bench_skill =
            Globals::RotationRun.all_rotation_steps[window_idx];

        is_completed_correct =
            (casted_skill.SkillName == bench_skill.skill_data.name) ? true
                                                                    : false;
        is_completed_incorrect = !is_completed_correct;
    }

    return SkillState{
        .is_history = window_idx < current_idx,
        .is_current = is_current,
        .is_last = is_last,
        .is_auto_attack = is_auto_attack,
        .is_completed_correct = is_completed_correct,
        .is_completed_incorrect = is_completed_incorrect,
    };
}

bool SKillCastIsTooEarlyWrtRechargeTime(
    const std::chrono::steady_clock::time_point &now,
    const EvCombatDataPersistent &combat_data,
    std::map<uint64_t, std::chrono::steady_clock::time_point>
        &skill_last_cast_times)
{
    auto last_cast_time = skill_last_cast_times[combat_data.SkillID];
    skill_last_cast_times[combat_data.SkillID] = now;

    const auto skill_data_it = Globals::RotationRun.skill_data.find(
        static_cast<int>(combat_data.SkillID));

    if (skill_data_it != Globals::RotationRun.skill_data.end())
    {
        auto current_profession = get_current_profession_name();
        auto profession_lower = to_lowercase(current_profession);

        auto is_mesmer_weapon_4 = false;
        auto is_berserker_f1 = false;

        if (profession_lower == "mesmer")
        {
            // TODO: For Chrono - CS reset
            is_mesmer_weapon_4 = RotationRunType::mesmer_weapon_4_skills.count(
                                     combat_data.SkillID) > 0;
        }
        else if (profession_lower == "warrior")
        {
            is_berserker_f1 = RotationRunType::berserker_f1_skills.count(
                                  combat_data.SkillID) > 0;
        }

        if (!is_mesmer_weapon_4 && !is_berserker_f1)
        {
            const auto &skill_data = skill_data_it->second;
            const auto recharge_time_s = skill_data.recharge_time;
            const auto recharge_time_w_alac_s =
                static_cast<int>(recharge_time_s * 0.8f);

            const auto cast_time_diff = now - last_cast_time;
            const auto cast_time_diff_s =
                std::chrono::duration_cast<std::chrono::seconds>(cast_time_diff)
                    .count();
            const auto recharge_duration_s =
                std::chrono::seconds(recharge_time_w_alac_s);

            if (cast_time_diff_s < recharge_time_w_alac_s * 0.7 &&
                recharge_time_w_alac_s > 0 &&
                !skill_data.is_auto_attack) // XXX: Hacky
                return true;
        }
    }

    return false;
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

void RenderType::skill_activation_callback(
    const bool pressed,
    const EvCombatDataPersistent &combat_data)
{
    static auto skill_last_cast_times =
        std::map<uint64_t, std::chrono::steady_clock::time_point>{};

    key_press_event_in_this_frame = pressed;

    if (pressed)
    {
        std::lock_guard<std::mutex> lock(played_rotation_mutex);

        const auto now = std::chrono::steady_clock::now();

        if (skill_last_cast_times.find(combat_data.SkillID) !=
            skill_last_cast_times.end())
        {
            if (SKillCastIsTooEarlyWrtRechargeTime(now,
                                                   combat_data,
                                                   skill_last_cast_times))
                return;
        }
        else
        {
            skill_last_cast_times[combat_data.SkillID] = now;
        }

        curr_combat_data = combat_data;

        if (played_rotation.size() > 300)
        {
            auto last_100 =
                std::vector<EvCombatDataPersistent>(played_rotation.end() - 100,
                                                    played_rotation.end());
            played_rotation = std::move(last_100);
        }

        played_rotation.push_back(combat_data);
    }
}

EvCombatDataPersistent RenderType::get_current_skill()
{
    std::lock_guard<std::mutex> lock(played_rotation_mutex);

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

void RenderType::toggle_vis(const bool flag)
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

void RenderType::render_debug_data()
{
    static auto identity = Mumble::Identity{};
    static auto last_parse_time = std::chrono::steady_clock::now();

    if (!Globals::MumbleData)
        return;

    const auto now = std::chrono::steady_clock::now();
    const auto time_since_last_parse =
        std::chrono::duration_cast<std::chrono::seconds>(now - last_parse_time)
            .count();

    if (time_since_last_parse >= 2)
    {
        identity = ParseMumbleIdentity(Globals::MumbleData->Identity);
        last_parse_time = now;
    }

    ImGui::Separator();
    ImGui::Text(
        "Profession: %d (%s)",
        static_cast<int>(identity.Profession),
        profession_to_string(static_cast<ProfessionID>(identity.Profession))
            .c_str());
    ImGui::Text(
        "Specialization: %u (%s)",
        identity.Specialization,
        elite_spec_to_string(static_cast<EliteSpecID>(identity.Specialization))
            .c_str());
    ImGui::Text("Map ID: %u", identity.MapID);
    ImGui::Text("Is in Combat: %d", Globals::MumbleData->Context.IsInCombat);

    ImGui::Text("Last Casted Skill ID: %u", curr_combat_data.SkillID);
    ImGui::Text("Last Casted Skill Name: %s",
                curr_combat_data.SkillName.c_str());
    ImGui::Text("Last Event ID: %u", curr_combat_data.EventID);
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

        select_bench();

        const auto checkbox_width =
            ImGui::CalcTextSize("Names").x + ImGui::CalcTextSize("Times").x +
            ImGui::GetStyle().ItemSpacing.x * 3 + ImGui::GetFrameHeight() * 2;
        const auto window_width = ImGui::GetWindowSize().x;
        ImGui::SetCursorPosX((window_width - checkbox_width) * 0.5f);

        if (ImGui::Checkbox("Names", &Settings::ShowSkillName))
        {
            Settings::Save(Globals::SettingsPath);
        }

        ImGui::SameLine();

        if (ImGui::Checkbox("Times", &Settings::ShowSkillTime))
        {
            Settings::Save(Globals::SettingsPath);
        }

        const auto checkbox_width2 = ImGui::CalcTextSize("Horizontal").x +
                                     ImGui::CalcTextSize("Adjust UI").x +
                                     ImGui::GetStyle().ItemSpacing.x * 3 +
                                     ImGui::GetFrameHeight() * 2;
        ImGui::SetCursorPosX((window_width - checkbox_width2) * 0.5f);

        if (ImGui::Checkbox("Horizontal", &Settings::HorizontalSkillLayout))
        {
            if (Settings::HorizontalSkillLayout)
                Globals::SkillIconSize = 64.0F;
            else
                Globals::SkillIconSize = 28.0F;

            Settings::Save(Globals::SettingsPath);
        }

        ImGui::SameLine();

        if (ImGui::Checkbox("Adjust UI", &is_not_ui_adjust_active))
        {
        }

#ifdef _DEBUG
        if (ImGui::CollapsingHeader("Debug Data",
                                    ImGuiTreeNodeFlags_DefaultOpen))
        {
            render_debug_data();
        }
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

void RenderType::select_bench()
{
    text_filter();

    if (benches_files.empty())
        return;

    selection();

    reload_btn();
}


void RenderType::text_filter()
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

void RenderType::selection()
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

                        Globals::RotationRun.load_data(selected_file_path,
                                                       img_path);
                        ReleaseTextureMap(Globals::TextureMap);

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

void RenderType::reload_btn()
{
    if (selected_bench_index >= 0 &&
        selected_bench_index < benches_files.size())
    {
        if (ImGui::Button("Reload", ImVec2(-1, 0)))
        {
            if (!selected_file_path.empty())
                restart_rotation();
        }
    }
}

void RenderType::restart_rotation()
{
    Globals::RotationRun.restart_rotation();
    ReleaseTextureMap(Globals::TextureMap);

    std::lock_guard<std::mutex> lock(played_rotation_mutex);
    played_rotation.clear();
    curr_combat_data = EvCombatDataPersistent{};

    time_since_last_match = std::chrono::steady_clock::now();
    num_skills_wo_match = 0U;

    last_time_aa_did_skip = std::chrono::steady_clock::now();
}

void RenderType::render_rotation_window(const bool is_not_ui_adjust_active,
                                        ID3D11Device *pd3dDevice)
{
    float window_width = 0.0f;
    float window_height = Globals::SkillIconSize * 0.0F;
    ImGuiIO &io = ImGui::GetIO();
    if (!Settings::HorizontalSkillLayout)
    {
        window_width = 400.0f;
        window_height = Globals::SkillIconSize * 10.0F;
    }
    else
    {
        window_width = Globals::SkillIconSize * 10.0F;
        window_height = 50.0F;
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
            rotation_render_details(pd3dDevice);
        }
    }
    else
    {
        if (ImGui::Begin("##GW2RotaHelper_Rota_Horizontal",
                         &Settings::ShowWindow,
                         curr_flags_rota))
        {
            rotation_render_horizontal(pd3dDevice);
        }
    }

    ImGui::End();
}

void RenderType::rotation_render_details(ID3D11Device *pd3dDevice)
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

void RenderType::rotation_render_horizontal(ID3D11Device *pd3dDevice)
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
            ArcEv::ResetSkillCastTracking();
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
