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

class Render
{
public:
    Render();
    Render(bool &show_window) : show_window(show_window)
    {
    }
    ~Render();

    void set_data_path(const std::filesystem::path &path);

    void render(ID3D11Device *pd3dDevice, AddonAPI *APIDefs = nullptr);
    void text_filter();
    void selection();
    void reload_btn();
    void select_bench();
    void rotation_render(ID3D11Device *pd3dDevice);
    std::pair<std::vector<std::pair<int, const BenchFileInfo *>>,
              std::set<std::string>>
    get_file_data_pairs(std::string &filter_string);
    void skill_activation_callback(const bool pressed,
                                   const EvCombatDataPersistent &combat_data);
    void toggle_vis(const bool flag);
    EvCombatDataPersistent get_current_skill();
    void CycleSkillsLogic();
    void DrawRect(const RotationInfo &skill_info,
                  ImU32 color = IM_COL32(255, 255, 255, 255));

    bool show_window;

    bool key_press_event_in_this_frame;
    EvCombatDataPersistent curr_combat_data{};
    std::vector<EvCombatDataPersistent> played_rotation{};
    std::mutex
        played_rotation_mutex; // Protects both curr_combat_data and played_rotation

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
};
