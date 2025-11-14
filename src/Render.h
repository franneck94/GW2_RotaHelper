#pragma once

#include <d3d11.h>

#include <filesystem>
#include <functional>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "imgui.h"

#include "FileUtils.h"
#include "LogData.h"
#include "Textures.h"
#include "Types.h"

class RenderType
{
public:
    RenderType();
    RenderType(bool &show_window) : show_window(show_window) {};
    ~RenderType();

    void render(ID3D11Device *pd3dDevice);

    /* RENDER OPTIONS WINDOW */
    void render_debug_data();
    void render_options_checkboxes(bool &is_not_ui_adjust_active);
    void render_options_window(bool &is_not_ui_adjust_active);
    void render_snowcrows_build_link();
    void render_text_filter();
    void render_symbol_and_text(bool &is_selected,
                                const int original_index,
                                const BenchFileInfo *const &file_info,
                                const std::string &base_formatted_name,
                                const std::string &selectable_id,
                                std::function<void(ImDrawList *, ImVec2, float, float)> draw_symbol_func);
    void render_red_cross_and_text(bool &is_selected,
                                   const int original_index,
                                   const BenchFileInfo *const &file_info,
                                   const std::string base_formatted_name);
    void render_star_and_text(bool &is_selected,
                              const int original_index,
                              const BenchFileInfo *const &file_info,
                              const std::string base_formatted_name);
    void render_tick_and_text(bool &is_selected,
                              const int original_index,
                              const BenchFileInfo *const &file_info,
                              const std::string base_formatted_name);
    void render_orange_cross_and_text(bool &is_selected,
                                      const int original_index,
                                      const BenchFileInfo *const &file_info,
                                      const std::string base_formatted_name);
    void render_untested_and_text(bool &is_selected,
                                  const int original_index,
                                  const BenchFileInfo *const &file_info,
                                  const std::string base_formatted_name);
    void render_selection();
    void render_xml_selection();
    void render_select_bench();
    void render_keybind(const RotationStep &rotation_step);
    void restart_rotation();
    void render_load_buttons();

    /* RENDER ROTATION WINDOW */
    void render_rotation_window(const bool is_not_ui_adjust_active, ID3D11Device *pd3dDevice);
    void render_rotation_details(ID3D11Device *pd3dDevice);
    void render_rotation_horizontal(ID3D11Device *pd3dDevice);
    void render_rotation_icons(const SkillState &skill_state,
                               const RotationStep &rotation_step,
                               const ID3D11ShaderResourceView *texture,
                               const std::string &text,
                               ID3D11Device *pd3dDevice);

    /* HELPER */
    float calculate_centered_position(const std::vector<std::string> &items) const;
    void append_to_played_rotation(const EvCombatDataPersistent &combat_data);
    void skill_activation_callback(EvCombatDataPersistent combat_data);
    void set_show_window(const bool flag);
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

    std::map<std::string, KeybindInfo> keybinds{};
    bool keybinds_loaded = false;

    ImGuiWindowFlags flags_rota = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground |
                                  ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus |
                                  ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoScrollbar |
                                  ImGuiWindowFlags_NoResize;
};
