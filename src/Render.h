#pragma once

#include <atomic>
#include <d3d11.h>
#include <filesystem>
#include <future>
#include <string>
#include <thread>

#include "LogData.h"
#include "Types.h"

class Render
{
public:
    Render();

    void render(ID3D11Device *pd3dDevice);

    Render(bool &show_window) : show_window(show_window) {}

    void key_press_cb(const bool pressed);
    void toggle_vis(const bool flag);
    EvCombatDataPersistent get_current_skill();

    bool show_window;

    bool key_press_event_in_this_frame;

    std::filesystem::path data_path = std::filesystem::absolute(std::filesystem::path(__FILE__).parent_path().parent_path() / "data");
    std::filesystem::path img_path = data_path / "img";
    std::filesystem::path bench_path = data_path / "bench";
    std::vector<std::filesystem::path> benches_files;

    RotationRun rotation_run{};
    TextureMap texture_map{};

    int selected_bench_index = -1;
    std::filesystem::path selected_file_path;
};
