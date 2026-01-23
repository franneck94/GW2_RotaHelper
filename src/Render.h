#pragma once

#include <d3d11.h>

#include <filesystem>
#include <string>
#include <vector>

#include "imgui.h"

#include "Builds.h"
#include "LogData.h"
#include "OptionsRender.h"
#include "RotationRender.h"
#include "Textures.h"
#include "Types.h"

struct RenderDataType
{
    ID3D11Device *pd3dDevice = nullptr;

    BuildsType builds;

    std::filesystem::path data_path;
    std::filesystem::path img_path;
    std::filesystem::path bench_path;
    std::vector<BenchFileInfo> benches_files;

    std::map<std::string, KeybindInfo> keybinds{};
    bool keybinds_loaded = false;

    std::string current_build_key;
    std::vector<uint32_t> precast_skills_order;
    int precast_drag_source = -1;
    int precast_drag_dest = -1;

    bool is_not_ui_adjust_active = false;

    bool skill_event_in_this_frame;
    EvCombatDataPersistent curr_combat_data{};
    std::vector<EvCombatDataPersistent> played_rotation{};

    bool show_rotation_keybinds = false;
    bool show_rotation_window = true;
    bool show_rotation_icons_overview = false;

    bool do_highlight_skill = false;
    SkillID highlight_skill_id = SkillID::NONE;

    std::filesystem::path selected_file_path;
};

class RenderType
{
public:
    RenderType() {};
    RenderType(bool &show_window) : show_window(show_window) {};
    ~RenderType();

    void render(ID3D11Device *pd3dDevice);
    void restart_rotation(const bool not_ooc_triggered);

    void append_to_played_rotation(const EvCombatDataPersistent &combat_data);
    void skill_activation_callback(EvCombatDataPersistent combat_data);
    void set_show_window(const bool flag);
    EvCombatDataPersistent get_current_skill();
    void CycleSkillsLogic(const EvCombatDataPersistent &skill_ev);
    void set_data_path(const std::filesystem::path &path);

public:
    bool show_window;

    uint32_t num_skills_wo_match = 0U;
    std::chrono::steady_clock::time_point time_since_last_match;
};
