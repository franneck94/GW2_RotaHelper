#pragma once

#include <d3d11.h>

#include "imgui.h"

class OptionsRenderType
{
public:
    OptionsRenderType(){};
    ~OptionsRenderType() = default;

    void render();

    void render_status();
    void render_precast_window(bool &show_precast_window);
    void render_horizontal_settings();
    void render_options_checkboxes();
    void render_debug_window(bool &show_debug_window);
    void render_debug_data();
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
    void render_orange_cross_and_text(bool &is_selected,
                                      const int original_index,
                                      const BenchFileInfo *const &file_info,
                                      const std::string base_formatted_name);
    void render_untested_and_text(bool &is_selected,
                                  const int original_index,
                                  const BenchFileInfo *const &file_info,
                                  const std::string base_formatted_name);
    void render_tick_and_text(bool &is_selected,
                              const int original_index,
                              const BenchFileInfo *const &file_info,
                              const std::string base_formatted_name,
                              const ImU32 Color,
                              const std::string &label);
    void render_selection();
    void render_load_buttons();
    void render_snowcrows_build_link();
    void render_select_bench();
    void render_xml_selection();

    int selected_bench_index = -1;
    ImVec2 filter_input_pos = ImVec2(0, 0);
    float filter_input_width = 0.0F;
    std::string formatted_name = std::string{"Select..."};
    std::vector<std::pair<int, const BenchFileInfo *>> filtered_files{};
    bool open_combo_next_frame = false;
};
