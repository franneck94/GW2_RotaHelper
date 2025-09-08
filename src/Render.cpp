#include <iostream>
#include <list>
#include <map>
#include <ranges>
#include <string>

#include "imgui.h"

#include "Constants.h"
#include "Render.h"
#include "Shared.h"

namespace
{
}

void Render::render()
{
    if (!show_window)
        return;

    if (ImGui::Begin("GW2RotaHelper", &show_window))
    {
        ImGui::Text("Combat Events Buffer (last 10):");
        ImGui::BeginChild("CombatBufferChild", ImVec2(0, 200), true, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::Columns(3, "cb_columns", true);
        ImGui::Text("Source");
        ImGui::NextColumn();
        ImGui::Text("Destination");
        ImGui::NextColumn();
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
