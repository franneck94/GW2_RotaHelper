#pragma once

#include <d3d11.h>

#include <filesystem>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "imgui.h"

#include "LogData.h"
#include "Textures.h"
#include "Types.h"

struct BenchFileInfo
{
    std::filesystem::path full_path;
    std::filesystem::path relative_path;
    std::string display_name;
    bool is_directory_header;

    BenchFileInfo(const std::filesystem::path &full,
                  const std::filesystem::path &relative,
                  bool is_header = false)
        : full_path(full), relative_path(relative),
          is_directory_header(is_header)
    {
        if (is_header)
        {
            display_name = "[+] " + relative.string();
        }
        else
        {
            auto filename = relative.filename().string();
            if (filename.ends_with("_v4.json"))
            {
                filename = filename.substr(0, filename.length() - 8);
            }
            display_name = "    " + filename;
        }
    }
};

struct MatchCandidate
{
    size_t rotation_index;
    float confidence_score;
    size_t skills_to_advance;
};

struct MatchResult
{
    size_t position;
    float confidence;
    size_t advance_count;
};

struct SkillState
{
    bool is_history;
    bool is_current;
    bool is_last;
    bool is_auto_attack;
    bool is_completed_correct;
    bool is_completed_incorrect;
};

class RenderType
{
public:
    RenderType();
    RenderType(bool &show_window) : show_window(show_window)
    {
    }
    ~RenderType();

    void render(ID3D11Device *pd3dDevice);

    void render_options_window(bool &is_not_ui_adjust_active);
    void text_filter();
    void selection();
    void select_bench();
    void restart_rotation();
    void reload_btn();

    void render_rotation_window(const bool is_not_ui_adjust_active,
                                ID3D11Device *pd3dDevice);
    void rotation_render_details(ID3D11Device *pd3dDevice);
    void rotation_render_horizontal(ID3D11Device *pd3dDevice);
    void render_rotation_icons(const SkillState &skill_state,
                               const RotationInfo &skill_info,
                               const ID3D11ShaderResourceView *texture,
                               const std::string &text,

                               ID3D11Device *pd3dDevice);

    void skill_activation_callback(const bool pressed,
                                   const EvCombatDataPersistent &combat_data);
    void toggle_vis(const bool flag);
    EvCombatDataPersistent get_current_skill();
    void CycleSkillsLogic(const EvCombatDataPersistent &skill_ev);
    void set_data_path(const std::filesystem::path &path);

    bool show_window;

    bool key_press_event_in_this_frame;
    EvCombatDataPersistent curr_combat_data{};
    std::vector<EvCombatDataPersistent> played_rotation{};
    std::mutex played_rotation_mutex;

    uint32_t num_skills_wo_match = 0U;
    std::chrono::steady_clock::time_point time_since_last_match;

    bool open_combo_next_frame = false;
    ImVec2 filter_input_pos = ImVec2(0, 0);
    float filter_input_width = 0.0F;
    std::string formatted_name = std::string{"Select..."};
    std::vector<std::pair<int, const BenchFileInfo *>> filtered_files{};

    std::filesystem::path data_path;
    std::filesystem::path img_path;
    std::filesystem::path bench_path;
    std::vector<BenchFileInfo> benches_files;

    int selected_bench_index = -1;
    std::filesystem::path selected_file_path;

    ImGuiWindowFlags flags_rota =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNavInputs |
        ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize;
};
