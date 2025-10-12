#pragma once

#include <atomic>
#include <d3d11.h>
#include <filesystem>
#include <future>
#include <string>
#include <thread>

#include "LogData.h"
#include "Types.h"
#include "Textures.h"

struct BenchFileInfo
{
    std::filesystem::path full_path;
    std::filesystem::path relative_path;
    std::string display_name;
    bool is_directory_header;

    BenchFileInfo(const std::filesystem::path& full, const std::filesystem::path& relative, bool is_header = false)
        : full_path(full), relative_path(relative), is_directory_header(is_header)
    {
        if (is_header)
        {
            // Use folder icon for directory headers
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
    ~Render();

    void render(ID3D11Device *pd3dDevice);
    void select_bench(ID3D11Device *pd3dDevice);
    void rotation_render(ID3D11Device *pd3dDevice);

    Render(bool &show_window) : show_window(show_window) {}

    void key_press_cb(const bool pressed);
    void toggle_vis(const bool flag);
    EvCombatDataPersistent get_current_skill();

    bool show_window;

    bool key_press_event_in_this_frame;

    std::filesystem::path data_path = std::filesystem::absolute(std::filesystem::path(__FILE__).parent_path().parent_path() / "data");
    std::filesystem::path img_path = data_path / "img";
    std::filesystem::path bench_path = data_path / "bench";
    std::vector<BenchFileInfo> benches_files;

    RotationRun rotation_run{};
    TextureMap texture_map{};

    int selected_bench_index = -1;
    std::filesystem::path selected_file_path;
};
