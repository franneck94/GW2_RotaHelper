#include <windows.h>

#include <commdlg.h>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")
#include <d3d11.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <map>
#include <numbers>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <utility>

#include "imgui.h"
#include "mumble/Mumble.h"

#include "ArcEvents.h"
#include "Defines.h"
#include "FileUtils.h"
#include "LogData.h"
#include "MumbleUtils.h"
#include "Render.h"
#include "RenderUtils.h"
#include "Rotation.h"
#include "Settings.h"
#include "Shared.h"
#include "SkillData.h"
#include "Textures.h"
#include "Types.h"
#include "TypesUtils.h"

#include "RotationRender.h"

namespace
{
auto DODGE_ICON_ID = 2;
auto UNK_SKILL_ICON_ID = 0;
} // namespace

void RotationRenderType::render()
{
    float window_width = 600.0f;
    float window_height = 100.0f;
    ImGuiIO &io = ImGui::GetIO();

    ImGui::SetNextWindowSize(ImVec2(window_width, window_height), ImGuiCond_FirstUseEver);
    float pos_x = (io.DisplaySize.x - window_width) * 0.5f;
    float pos_y = io.DisplaySize.y - window_height - 50.0f;
    ImGui::SetNextWindowPos(ImVec2(pos_x, pos_y), ImGuiCond_FirstUseEver);

    auto curr_flags_rota = flags_rota;
    if (Globals::RenderData.is_not_ui_adjust_active)
    {
        curr_flags_rota &= ~ImGuiWindowFlags_NoBackground;
        curr_flags_rota &= ~ImGuiWindowFlags_NoMove;
        curr_flags_rota &= ~ImGuiWindowFlags_NoResize;
    }

    render_rotation_keybinds(Globals::RenderData.show_rotation_keybinds);
    render_rotation_icons_overview(Globals::RenderData.show_rotation_icons_overview);

    if (ImGui::Begin("##GW2RotaHelper_Rota_Horizontal", &Settings::ShowWindow, curr_flags_rota))
    {
        const auto current_window_size = ImGui::GetWindowSize();
        Globals::SkillIconSize = min(current_window_size.y * 0.7f, 120.0f);
        Globals::SkillIconSize = max(Globals::SkillIconSize, 24.0f);

        render_rotation_horizontal();
    }

    ImGui::End();
}

void RotationRenderType::render_rotation_keybinds(bool &show_rotation_keybinds)
{
    if (!show_rotation_keybinds)
        return;

    ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_Once);
    if (ImGui::Begin("Rotation Keybinds", &show_rotation_keybinds))
    {
        for (const auto &text : Globals::RotationRun.rotation_text)
            ImGui::TextWrapped("%s", text.c_str());
    }

    ImGui::End();
}

void RotationRenderType::render_rotation_icons_overview(bool &show_rotation_icons_overview)
{
    constexpr static auto icon_size = 32;

    if (!show_rotation_icons_overview)
        return;

    auto num_icons = 0;
    auto curr_rota_index =
        Globals::RotationRun.all_rotation_steps.size() - Globals::RotationRun.missing_rotation_steps.size();

    auto curr_flags_rota = flags_rota_overview;
    if (Globals::RenderData.is_not_ui_adjust_active)
    {
        curr_flags_rota &= ~ImGuiWindowFlags_NoMove;
        curr_flags_rota &= ~ImGuiWindowFlags_NoResize;
    }

    ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_Once);
    if (ImGui::Begin("Rotation Icons Overview", &show_rotation_icons_overview, curr_flags_rota))
    {
        auto should_break = false;
        auto icons_in_line = 0;

        for (const auto &icon_lines : Globals::RotationRun.rotation_icon_lines)
        {
            bool first_in_line = true;
            for (const auto &line_data : icon_lines)
            {
                auto *texture = std::get<0>(line_data);
                auto name = std::get<1>(line_data);
                auto skill_id = std::get<2>(line_data);
                auto is_auto_attack = std::get<3>(line_data);

                if (!texture || !Globals::RenderData.pd3dDevice)
                    continue;

                if (!first_in_line)
                {
                    icons_in_line++;

                    if ((icons_in_line > 0 && (icons_in_line % 15) != 0))
                        ImGui::SameLine();
                    else
                    {
                        icons_in_line = 0;
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
                    }
                }
                else
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);

                first_in_line = false;

                auto rotation_step = RotationStep{};
                rotation_step.skill_data.name = name;
                rotation_step.skill_data.skill_id = skill_id;
                rotation_step.skill_data.is_auto_attack = is_auto_attack;
                const auto is_current = num_icons == curr_rota_index;

                auto alpha_offset = 0.0f;
                if (Globals::RenderData.do_highlight_skill)
                {
                    if (rotation_step.skill_data.skill_id == Globals::RenderData.highlight_skill_id)
                        alpha_offset = 0.5f;
                    else
                        alpha_offset = -0.65f;
                }

                if (!Globals::RenderData.do_highlight_skill && is_current)
                    DrawRect(rotation_step, "", IM_COL32(255, 255, 255, 255), 2.0F, icon_size);
                else if (rotation_step.skill_data.is_auto_attack) // orange
                    DrawRect(rotation_step, "", IM_COL32(255, 165, 0, 255), 2.0F);
                render_skill_texture(rotation_step, texture, 0, icon_size, false, alpha_offset);

                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) &&
                    ImGui::GetIO().KeyCtrl)
                    Globals::RenderData.is_not_ui_adjust_active = !Globals::RenderData.is_not_ui_adjust_active;
                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                {
                    Globals::RenderData.do_highlight_skill = !Globals::RenderData.do_highlight_skill;
                    if (Globals::RenderData.do_highlight_skill)
                        Globals::RenderData.highlight_skill_id = rotation_step.skill_data.skill_id;
                    else
                        Globals::RenderData.highlight_skill_id = SkillID::NONE;
                }

                ++num_icons;
            }

            if (num_icons > 0)
                ImGui::Dummy(ImVec2(0, 2));
        }
    }

    ImGui::End();
}

void RotationRenderType::render_rotation_horizontal()
{
    ImGui::Spacing();
    ImGui::Indent(10.0f);

    const auto [start, end, current_idx] = Globals::RotationRun.get_current_rotation_indices();

    if (start < 1 && Settings::ShowPreCasts)
    {
        if (Globals::RotationRun.all_rotation_steps.size() > 0 && Globals::RenderData.current_build_key.empty())
            Globals::RenderData.current_build_key = Globals::RotationRun.meta_data.name;

        if (Globals::RenderData.precast_skills_order.empty() && !Globals::RenderData.current_build_key.empty() &&
            Globals::RotationRun.all_rotation_steps.size() > 0)
        {
            if (Settings::PrecastSkills.find(Globals::RenderData.current_build_key) != Settings::PrecastSkills.end())
                Globals::RenderData.precast_skills_order = Settings::PrecastSkills[Globals::RenderData.current_build_key];
        }

        for (const auto &skill_id : Globals::RenderData.precast_skills_order)
        {
            const auto skill_it = Globals::RotationRun.rotation_skills.find(static_cast<SkillID>(skill_id));
            if (skill_it != Globals::RotationRun.rotation_skills.end())
            {
                if (skill_it->second.texture)
                {
                    RotationStep precast_step;
                    precast_step.skill_data.name = skill_it->second.name;
                    precast_step.skill_data.skill_id = static_cast<SkillID>(skill_id);
                    precast_step.duration_ms = -1;
                    precast_step.time_of_cast = -1;

                    SkillState precast_skill_state;
                    precast_skill_state.is_current = false;
                    precast_skill_state.is_last = false;

                    render_rotation_icons(precast_skill_state,
                                          precast_step,
                                          skill_it->second.texture,
                                          "PreCast",
                                          0,
                                          true);
                    ImGui::SameLine();
                }
            }
        }
    }

    for (int32_t window_idx = start; window_idx <= end; ++window_idx)
    {
        if (window_idx < 0 || static_cast<size_t>(window_idx) >= Globals::RotationRun.all_rotation_steps.size())
            continue;

        const auto &rotation_step = Globals::RotationRun.get_rotation_skill(static_cast<size_t>(window_idx));
        const auto *texture = Globals::TextureMap[rotation_step.skill_data.icon_id];

        const auto skill_state = get_skill_state(Globals::RotationRun,
                                                 Globals::RenderData.played_rotation,
                                                 window_idx,
                                                 current_idx,
                                                 rotation_step.skill_data.is_auto_attack);
        const auto text = std::string{""};
        const int aa_index = (static_cast<size_t>(window_idx) < Globals::RotationRun.auto_attack_indices.size())
                                 ? Globals::RotationRun.auto_attack_indices[window_idx]
                                 : 0;
        render_rotation_icons(skill_state, rotation_step, texture, text, aa_index);

        ImGui::SameLine();
    }

    ImGui::Unindent(10.0f);
}

void RotationRenderType::render_rotation_icons(const SkillState &skill_state,
                                               const RotationStep &rotation_step,
                                               const ID3D11ShaderResourceView *texture,
                                               const std::string &text,
                                               const int auto_attack_index,
                                               const bool is_precast)
{
    const auto is_special_skill = rotation_step.is_special_skill;

    if (is_precast)
    {
        DrawRect(rotation_step, text, IM_COL32(0, 0, 0, 255), 2.0F);
        render_skill_texture(rotation_step, texture, auto_attack_index, Globals::SkillIconSize, false);

        auto *draw_list = ImGui::GetWindowDrawList();
        auto icon_pos = ImGui::GetItemRectMin();
        auto icon_size = ImGui::GetItemRectSize();

        auto text_size = ImGui::CalcTextSize(text.c_str());
        auto text_pos =
            ImVec2(icon_pos.x + (icon_size.x - text_size.x) * 0.5f, icon_pos.y + icon_size.y - text_size.y - 2.0f);

        draw_list->AddRectFilled(ImVec2(text_pos.x - 2, text_pos.y - 1),
                                 ImVec2(text_pos.x + text_size.x + 2, text_pos.y + text_size.y + 1),
                                 IM_COL32(0, 0, 0, 180),
                                 3.0f);
        draw_list->AddText(text_pos, IM_COL32(255, 255, 255, 255), text.c_str());

        return;
    }

    if (skill_state.is_current && !skill_state.is_last) // white
        DrawRect(rotation_step, text, IM_COL32(255, 255, 255, 255), 7.0F);
    else if (skill_state.is_last) // pruple
        DrawRect(rotation_step, text, IM_COL32(128, 0, 128, 255), 2.0F);
    else if (rotation_step.skill_data.is_auto_attack) // orange
        DrawRect(rotation_step, text, IM_COL32(255, 165, 0, 255), 2.0F);

    if (texture && Globals::RenderData.pd3dDevice)
        render_skill_texture(rotation_step, texture, auto_attack_index, Globals::SkillIconSize, Settings::ShowKeybind);
    else if (rotation_step.skill_data.icon_id == DODGE_ICON_ID)
        render_dodge_placeholder();
    else if (rotation_step.skill_data.icon_id == UNK_SKILL_ICON_ID)
        render_unknown_placeholder();
    else
        render_empty_placeholder();

    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        Globals::RenderData.is_not_ui_adjust_active = !Globals::RenderData.is_not_ui_adjust_active;
}

void RotationRenderType::render_skill_texture(const RotationStep &rotation_step,
                                              const ID3D11ShaderResourceView *texture,
                                              const int auto_attack_index,
                                              const float icon_size,
                                              const bool show_keybind,
                                              const float alpha_offset)
{
    const auto is_special_skill = rotation_step.is_special_skill;
    auto tint_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    if (is_special_skill)
        tint_color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    if (alpha_offset != 0.0f)
        tint_color.w = max(0.1f, min(1.0f, tint_color.w + alpha_offset));

    ImGui::Image((ImTextureID)texture, ImVec2(icon_size, icon_size), ImVec2(0, 0), ImVec2(1, 1), tint_color);

    if (show_keybind)
        render_keybind(rotation_step);

    if (show_keybind && rotation_step.skill_data.is_auto_attack && auto_attack_index > 0)
    {
        auto *draw_list = ImGui::GetWindowDrawList();
        auto icon_pos = ImGui::GetItemRectMin();
        auto icon_size = ImGui::GetItemRectSize();

        auto index_str = std::to_string(auto_attack_index);
        auto text_size = ImGui::CalcTextSize(index_str.c_str());

        auto index_pos = ImVec2(icon_pos.x + 2, icon_pos.y + 2);

        auto circle_center = ImVec2(index_pos.x + text_size.x * 0.5f + 2, index_pos.y + text_size.y * 0.5f + 1);
        auto circle_radius = (text_size.x > text_size.y ? text_size.x : text_size.y) * 0.6f;
        draw_list->AddCircleFilled(circle_center, circle_radius, IM_COL32(255, 165, 0, 200));
        draw_list->AddCircle(circle_center, circle_radius, IM_COL32(255, 255, 255, 255), 0, 1.5f);

        draw_list->AddText(ImVec2(index_pos.x + 2, index_pos.y + 1), IM_COL32(255, 255, 255, 255), index_str.c_str());
    }

    auto tooltip_text = get_skill_text(rotation_step);
    SetTooltip(tooltip_text);
}

void RotationRenderType::render_keybind(const RotationStep &rotation_step)
{
    auto *draw_list = ImGui::GetWindowDrawList();
    const auto icon_pos = ImGui::GetItemRectMin();
    const auto icon_size = ImGui::GetItemRectSize();

    const auto &skill_key_mapping = Globals::RotationRun.skill_key_mapping;
    const auto skill_type = rotation_step.skill_data.skill_type;
    const auto icon_id = rotation_step.skill_data.icon_id;

    auto keybind_str = std::string{};
    if (Settings::XmlSettingsPath.empty())
    {
        if (skill_key_mapping.skill_7 == icon_id)
            keybind_str = "7";
        else if (skill_key_mapping.skill_8 == icon_id)
            keybind_str = "8";
        else if (skill_key_mapping.skill_9 == icon_id)
            keybind_str = "9";
        else
            keybind_str = default_skillslot_to_string(skill_type);
    }
    else
    {
        const auto &[keybind, modifier] = get_keybind_for_skill_type(skill_type, Globals::RenderData.keybinds);
        if (keybind == Keys::NONE)
        {
            if (skill_key_mapping.skill_7 == icon_id)
                keybind_str = "7";
            else if (skill_key_mapping.skill_8 == icon_id)
                keybind_str = "8";
            else if (skill_key_mapping.skill_9 == icon_id)
                keybind_str = "9";
            else
                keybind_str = default_skillslot_to_string(skill_type);
        }
        else
            keybind_str = custom_keys_to_string(keybind);

        if (modifier != Modifiers::NONE)
            keybind_str = modifiers_to_string(modifier) + " + " + keybind_str;
    }
    if (keybind_str != "")
    {
        auto text_size = ImGui::CalcTextSize(keybind_str.c_str());
        auto padding = 2.0f;
        auto text_pos = ImVec2{};

        if (keybind_str.length() <= 4)
        {
            text_pos = ImVec2(icon_pos.x + icon_size.x - text_size.x - padding,
                              icon_pos.y + icon_size.y - text_size.y - padding);
        }
        else
        {
            text_pos = ImVec2(icon_pos.x + (icon_size.x - text_size.x) * 0.5f,
                              icon_pos.y + icon_size.y - text_size.y - padding);
        }

        draw_list->AddRectFilled(ImVec2(text_pos.x - 2, text_pos.y - 1),
                                 ImVec2(text_pos.x + text_size.x + 2, text_pos.y + text_size.y + 1),
                                 IM_COL32(0, 0, 0, 180),
                                 3.0f);
        draw_list->AddText(text_pos, IM_COL32(255, 255, 255, 255), keybind_str.c_str());
    }
}

void RotationRenderType::render_dodge_placeholder()
{
    auto draw_list = ImGui::GetWindowDrawList();
    auto cursor_pos = ImGui::GetCursorScreenPos();

    draw_list->AddRectFilled(cursor_pos,
                             ImVec2(cursor_pos.x + Globals::SkillIconSize, cursor_pos.y + Globals::SkillIconSize),
                             IM_COL32(200, 200, 200, 255)); // Light gray background

    auto text_size = ImGui::CalcTextSize("D");
    auto text_pos = ImVec2(cursor_pos.x + (Globals::SkillIconSize - text_size.x) * 0.5f,
                           cursor_pos.y + (Globals::SkillIconSize - text_size.y) * 0.5f);

    draw_list->AddText(text_pos, IM_COL32(0, 0, 0, 255), "D");
    ImGui::Dummy(ImVec2(Globals::SkillIconSize, Globals::SkillIconSize));
}

void RotationRenderType::render_unknown_placeholder()
{
    auto draw_list = ImGui::GetWindowDrawList();
    auto cursor_pos = ImGui::GetCursorScreenPos();

    draw_list->AddRectFilled(cursor_pos,
                             ImVec2(cursor_pos.x + Globals::SkillIconSize, cursor_pos.y + Globals::SkillIconSize),
                             IM_COL32(200, 200, 200, 255)); // Light gray background

    auto text_size = ImGui::CalcTextSize("D");
    auto text_pos = ImVec2(cursor_pos.x + (Globals::SkillIconSize - text_size.x) * 0.1f,
                           cursor_pos.y + (Globals::SkillIconSize - text_size.y) * 0.5f);

    draw_list->AddText(text_pos, IM_COL32(0, 0, 0, 255), "Unknown");
    ImGui::Dummy(ImVec2(Globals::SkillIconSize, Globals::SkillIconSize));
}

void RotationRenderType::render_empty_placeholder()
{
    ImGui::Dummy(ImVec2(Globals::SkillIconSize, Globals::SkillIconSize));
}
