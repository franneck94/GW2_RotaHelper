#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#endif

#include <chrono>
#include <fstream>
#include <future>
#include <iostream>
#include <list>
#include <map>
#include <ranges>
#include <string>

#include "imgui.h"
#include "nlohmann/json.hpp"

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
    char szFile[260] = {0};

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

bool Render::ParseEvtcFile(const std::string &filePath)
{
    std::string cliPath = "GuildWars2EliteInsights-CLI.exe";
    std::string configPath = "ei_config.conf";
    std::string command = cliPath + " -c  " + configPath + " " + filePath;

    int result = system(command.c_str());

    if (result == 0)
    {
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

        try
        {
            std::ifstream jsonFile(jsonPath);
            if (jsonFile.is_open())
            {
                nlohmann::json logData;
                jsonFile >> logData;

                if (logData.contains("fightName"))
                {
                    std::string fightName = logData["fightName"];
                }

                if (logData.contains("players"))
                {
                    auto players = logData["players"];
                }

                jsonFile.close();
            }
        }
        catch (const std::exception &e)
        {
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
        if (ImGui::Button("Select File"))
        {
            OpenFileDialog();
        }

        if (!selected_file_path.empty())
        {
            ImGui::SameLine();
            ImGui::Text("Selected: %s", selected_file_path.c_str());

            // Add parse button
            if (!parsing_in_progress)
            {
                if (ImGui::Button("Parse Log"))
                {
                    // Start parsing in a separate thread
                    parsing_in_progress = true;
                    parsing_future = std::async(std::launch::async, &Render::ParseEvtcFile, this, selected_file_path);
                }
            }
            else
            {
                // Show parsing progress
                ImGui::Button("Parsing...");
                ImGui::SameLine();

                // Simple spinning indicator
                static float spinner_time = 0.0f;
                spinner_time += ImGui::GetIO().DeltaTime;
                const char *spinner_chars = "|/-\\";
                ImGui::Text("%c", spinner_chars[(int)(spinner_time / 0.1f) % 4]);

                // Check if parsing is complete
                if (parsing_future.valid() && parsing_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
                {
                    bool success = parsing_future.get();
                    parsing_in_progress = false;

                    if (success)
                    {
                        ImGui::OpenPopup("Parse Success");
                    }
                    else
                    {
                        ImGui::OpenPopup("Parse Error");
                    }
                }
            }
        }

        if (ImGui::BeginPopupModal("Parse Success", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Log file parsed successfully!");
            if (ImGui::Button("OK", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("Parse Error", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Failed to parse log file.");
            if (ImGui::Button("OK", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
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
        if (start_index == 0)
            return;

        for (int i = 0; i < 10; ++i)
        {
            const auto index = (start_index + i) % combat_buffer.size();
            const auto &entry = combat_buffer[index];

            ImGui::Text("%s", entry.SkillName.empty() ? "N/A" : entry.SkillName.c_str());
            ImGui::NextColumn();
        }

        ImGui::EndChild();
    }

    ImGui::End();
}
