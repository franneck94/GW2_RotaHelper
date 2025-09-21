#include <iostream>
#include <list>
#include <map>
#include <ranges>
#include <string>
#include <fstream>

#include "imgui.h"
#include "nlohmann/json.hpp"

// Windows includes for file dialog
#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#endif

#include "Constants.h"
#include "Render.h"
#include "Shared.h"

namespace
{
}

bool Render::OpenFileDialog()
{
#ifdef _WIN32
    OPENFILENAMEA ofn;
    char szFile[260] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "All Files\0*.*\0ARCDPS Logs\0*.evtc\0Compressed Logs\0*.zevtc\0\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn) == TRUE)
    {
        selected_file_path = std::string(szFile);
        return true;
    }
#endif
    return false;
}

bool Render::ParseEvtcFile(const std::string& filePath)
{
    // This function will call the GW2 Elite Insights Parser CLI
    // You'll need to download the CLI version from:
    // https://github.com/baaron4/GW2-Elite-Insights-Parser/releases/latest

    // Path to the Elite Insights CLI executable
    // You should place this in your project directory or specify the full path
    std::string cliPath = "GuildWars2EliteInsights-CLI.exe";
    std::string configPath = "ei_config.conf";

    // Build the command to execute
    // -c for config file
    // The config file specifies JSON output and other settings
    std::string command = "\"" + cliPath + "\" -c \"" + configPath + "\" \"" + filePath + "\"";

    // Execute the command
    int result = system(command.c_str());

    // Check if the command was successful (return code 0)
    if (result == 0)
    {
        // The JSON file will be created in the same directory as the log file
        // with the same name but .json extension
        std::string jsonPath = filePath;
        size_t lastDot = jsonPath.find_last_of('.');
        if (lastDot != std::string::npos)
        {
            jsonPath = jsonPath.substr(0, lastDot) + ".json";
        }
        else
        {
            jsonPath += ".json";
        }

        // TODO: Read and process the generated JSON file
        // You can use nlohmann/json library (already included in CMakeLists.txt)
        // to parse the JSON data and extract combat information

        // Example of reading the JSON output:
        try {
            std::ifstream jsonFile(jsonPath);
            if (jsonFile.is_open()) {
                nlohmann::json logData;
                jsonFile >> logData;

                // Example: Extract basic log information
                if (logData.contains("fightName")) {
                    std::string fightName = logData["fightName"];
                    // Process fight name...
                }

                if (logData.contains("players")) {
                    auto players = logData["players"];
                    // Process player data...
                }

                jsonFile.close();
            }
        } catch (const std::exception& e) {
            // Handle JSON parsing errors
            return false;
        }

        return true;
    }

    return false;
}

void Render::render()
{
    if (!show_window)
        return;

    if (ImGui::Begin("GW2RotaHelper", &show_window))
    {
        // File selection section
        if (ImGui::Button("Select File"))
        {
            OpenFileDialog();
        }

        if (!selected_file_path.empty())
        {
            ImGui::SameLine();
            ImGui::Text("Selected: %s", selected_file_path.c_str());

            // Add parse button
            if (ImGui::Button("Parse Log"))
            {
                if (ParseEvtcFile(selected_file_path))
                {
                    // Parsing successful
                    ImGui::OpenPopup("Parse Success");
                }
                else
                {
                    // Parsing failed
                    ImGui::OpenPopup("Parse Error");
                }
            }
        }

        // Popup modals
        if (ImGui::BeginPopupModal("Parse Success", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Log file parsed successfully!");
            if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("Parse Error", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Failed to parse log file.");
            if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }

        ImGui::Separator();

        ImGui::Text("Combat Events Buffer (last 10):");
        ImGui::BeginChild("CombatBufferChild", ImVec2(0, 200), true, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::Columns(1, "cb_columns", true);
        ImGui::Text("Skill Name");
        ImGui::NextColumn();
        ImGui::Separator();

        const auto start_index = (combat_buffer_index >= 10) ? (combat_buffer_index - 10) : (combat_buffer.size() + combat_buffer_index - 10);
        if (start_index == 0) return;

        for (int i = 0; i < 10; ++i)
        {
            const auto index = (start_index + i) % combat_buffer.size();
            const auto &entry = combat_buffer[index];

            ImGui::Text("%s", entry.SrcName.empty() ? "N/A" : entry.SrcName.c_str());
            ImGui::NextColumn();
            ImGui::Text("%s", entry.DstName.empty() ? "N/A" : entry.DstName.c_str());
            ImGui::NextColumn();
            ImGui::Text("%s", entry.SkillName.empty() ? "N/A" : entry.SkillName.c_str());
            ImGui::NextColumn();
        }

        ImGui::EndChild();
    }

    ImGui::End();
}
