#include <windows.h>

#include <string>
#include <vector>

#include "imgui.h"

#include "RenderUtils.h"
#include "Shared.h"

void SetTooltip(const std::string &text)
{
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text(text.c_str());
        ImGui::EndTooltip();
    }
}

void SetTooltip(const std::vector<std::string> &texts)
{
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        for (const auto &text : texts)
            ImGui::Text(text.c_str());
        ImGui::EndTooltip();
    }
}

void open_url_in_browser(const std::string &url)
{
    ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

float calculate_centered_position(const std::vector<std::string> &items, const float add_width)
{
    auto total_width = 0.0F;

    for (size_t i = 0; i < items.size(); ++i)
    {
        total_width += ImGui::CalcTextSize(items[i].c_str()).x;
        total_width += ImGui::GetFrameHeight(); // XXX: ui element size?

        if (i < items.size() - 1)
            total_width += ImGui::GetStyle().ItemSpacing.x;
    }

    total_width -= add_width;

    const auto window_width = ImGui::GetWindowSize().x;
    return ((window_width - total_width) * 0.5F);
}

void DrawRect(const RotationStep &rotation_step,
              const std::string &text,
              const ImU32 color,
              const float border_thickness,
              const float icon_size)
{
    auto draw_list = ImGui::GetWindowDrawList();
    auto cursor_pos = ImGui::GetCursorScreenPos();
    auto border_color = color;

    auto text_size = ImVec2{};
    if (!text.empty())
    {
        char formatted_text[256];
        snprintf(formatted_text, sizeof(formatted_text), text.c_str());
        text_size = ImGui::CalcTextSize(formatted_text);
    }
    else
    {
        text_size = ImVec2(0, 0);
    }

    auto total_width = text_size.x > 0 ? icon_size + ImGui::GetStyle().ItemSpacing.x + text_size.x : icon_size;
    auto total_height = (icon_size > text_size.y) ? icon_size : text_size.y;

    // Draw thick border by drawing outer filled rect and inner transparent rect
    draw_list->AddRectFilled(
        ImVec2(cursor_pos.x - border_thickness, cursor_pos.y - border_thickness),
        ImVec2(cursor_pos.x + total_width + border_thickness, cursor_pos.y + total_height + border_thickness),
        border_color);

    // Cut out the inner area to create the border effect
    draw_list->AddRectFilled(ImVec2(cursor_pos.x, cursor_pos.y),
                             ImVec2(cursor_pos.x + total_width, cursor_pos.y + total_height),
                             IM_COL32(0, 0, 0, 0)); // Transparent to cut out inner area
}

std::string get_skill_text(const RotationStep &rotation_step)
{
    auto text = rotation_step.skill_data.name;

    if (rotation_step.time_of_cast != 0.0f)
    {
        text += " (";
        char time_buffer[32];
        snprintf(time_buffer, sizeof(time_buffer), "%.2f", rotation_step.time_of_cast);
        text += time_buffer;
        text += ")";
    }

    return text;
}

bool IsVersionIsRange(const std::string version,
                      const std::string &lower_version_bound,
                      const std::string &upper_version_bound)
{
    return version >= lower_version_bound && version <= upper_version_bound;
}
