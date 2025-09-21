#pragma once

#include <string>

class Render
{
public:
    void render();

    Render(bool &show_window) : show_window(show_window) {}

    bool show_window;

    const std::string& getSelectedFilePath() const { return selected_file_path; }

private:
    std::string selected_file_path;
    bool OpenFileDialog();
    bool ParseEvtcFile(const std::string& filePath);
};
