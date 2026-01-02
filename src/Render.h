#pragma once

#include <d3d11.h>

#include <filesystem>
#include <functional>
#include <mutex>
#include <set>
#include <string>
#include <string_view>
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
    enum class BuildCategory
    {
        RED_CROSSED,
        ORANGE_CROSSED,
        GREEN_TICKED,
        YELLOW_TICKED,
        UNTESTED
    };

public:
    RenderType();
    RenderType(bool &show_window) : show_window(show_window) {};
    ~RenderType();

    void render(ID3D11Device *pd3dDevice);

    /* RENDER OPTIONS WINDOW */
    void render_debug_data();
    void render_debug_window();
    void render_options_checkboxes();
    void render_options_window();
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
    void render_tick_and_text(bool &is_selected,
                              const int original_index,
                              const BenchFileInfo *const &file_info,
                              const std::string base_formatted_name,
                              const ImU32 Color,
                              const std::string &label);
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
    void restart_rotation(const bool not_ooc_triggered);
    void render_load_buttons();

    std::string get_keybind_str(const RotationStep &rotation_step);
    void get_rotation_icons();
    void get_rotation_text();
    void render_rotation_keybinds(bool &show_rotation_keybinds);
    void render_rotation_icons_overview(bool &show_rotation_icons_overview);

    /* RENDER ROTATION WINDOW */
    void render_rotation_window();
    void render_rotation_horizontal();
    void render_rotation_icons(const SkillState &skill_state,
                               const RotationStep &rotation_step,
                               const ID3D11ShaderResourceView *texture,
                               const std::string &text,
                               const int auto_attack_index = 0);
    void render_skill_texture(const RotationStep &rotation_step,
                              const ID3D11ShaderResourceView *texture,
                              const int auto_attack_index,
                              const float icon_size,
                              const bool show_keybind);
    void render_dodge_placeholder();
    void render_unknown_placeholder();
    void render_empty_placeholder();

    /* HELPER */
    float calculate_centered_position(const std::vector<std::string> &items) const;
    void append_to_played_rotation(const EvCombatDataPersistent &combat_data);
    void skill_activation_callback(EvCombatDataPersistent combat_data);
    void set_show_window(const bool flag);
    EvCombatDataPersistent get_current_skill();
    void CycleSkillsLogic(const EvCombatDataPersistent &skill_ev);
    void set_data_path(const std::filesystem::path &path);

    void initialize_build_categories();
    BuildCategory get_build_category(const std::string &display_name) const;

    bool show_window;

    bool skill_event_in_this_frame;
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
    bool show_rotation_keybinds = false;
    bool show_rotation_window = true;
    bool show_rotation_icons_overview = false;
    bool is_not_ui_adjust_active = false;

    ID3D11Device *pd3dDevice = nullptr;

    std::vector<std::string> rotation_text;
    std::vector<std::vector<std::pair<ID3D11ShaderResourceView *, std::string>>> rotation_icon_lines;

    ImGuiWindowFlags flags_rota = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground |
                                  ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus |
                                  ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoScrollbar |
                                  ImGuiWindowFlags_NoResize;

    // Build categorization cache
    std::map<std::string, BuildCategory> build_category_cache;
    bool build_categories_initialized = false;
};
