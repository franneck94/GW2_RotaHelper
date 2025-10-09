#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <future>
#include <d3d11.h>

class Render
{
public:
    void render(ID3D11Device *pd3dDevice);

    Render(bool &show_window) : show_window(show_window), parsing_in_progress(false) {}

    bool show_window;

    const std::string& getSelectedFilePath() const { return selected_file_path; }

private:
    std::string selected_file_path;
    std::atomic<bool> parsing_in_progress;
    std::future<bool> parsing_future;
};
