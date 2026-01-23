#pragma once

#include <string>
#include <vector>

#include "imgui.h"

#include "Shared.h"

void SetTooltip(const std::string &text);

void SetTooltip(const std::vector<std::string> &texts);

void open_url_in_browser(const std::string &url);

float calculate_centered_position(const std::vector<std::string> &items, const float add_width = 0);

void DrawRect(const RotationStep &rotation_step,
              const std::string &text,
              const ImU32 color,
              const float border_thickness = 2.0f,
              const float icon_size = Globals::SkillIconSize);

std::string get_skill_text(const RotationStep &rotation_step);


auto draw_cross_factory(ImU32 color)
{
    return [color](ImDrawList *draw_list, ImVec2 center, float radius, float size) {
        float line_thickness = 2.0f;

        auto cross_top_left = ImVec2(center.x - radius * 0.7f, center.y - radius * 0.7f);
        auto cross_bottom_right = ImVec2(center.x + radius * 0.7f, center.y + radius * 0.7f);
        auto cross_top_right = ImVec2(center.x + radius * 0.7f, center.y - radius * 0.7f);
        auto cross_bottom_left = ImVec2(center.x - radius * 0.7f, center.y + radius * 0.7f);

        draw_list->AddLine(cross_top_left, cross_bottom_right, color, line_thickness);
        draw_list->AddLine(cross_top_right, cross_bottom_left, color, line_thickness);
    };
}

bool IsVersionIsRange(const std::string version,
                      const std::string &lower_version_bound,
                      const std::string &upper_version_bound);
