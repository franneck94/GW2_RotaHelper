#pragma once

#include <d3d11.h>

#include "imgui.h"

class RotationRenderType
{
public:
    RotationRenderType() {};
    ~RotationRenderType() = default;

    void render();
    void render_rotation_icons_overview(bool &show_rotation_icons_overview);
    void render_rotation_keybinds(bool &show_rotation_keybinds);
    void render_rotation_horizontal();
    void render_rotation_icons(const SkillState &skill_state,
                               const RotationStep &rotation_step,
                               const ID3D11ShaderResourceView *texture,
                               const std::string &text,
                               const int auto_attack_index = 0,
                               const bool is_precast = false);
    void render_skill_texture(const RotationStep &rotation_step,
                              const ID3D11ShaderResourceView *texture,
                              const int auto_attack_index,
                              const float icon_size,
                              const bool show_keybind,
                              const float alpha_offset = 0.0F);
    void render_keybind(const RotationStep &rotation_step);
    void render_dodge_placeholder();
    void render_unknown_placeholder();
    void render_empty_placeholder();

public:
    ImGuiWindowFlags flags_rota_overview = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
                                           ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus |
                                           ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoScrollbar |
                                           ImGuiWindowFlags_NoResize;
    ImGuiWindowFlags flags_rota = flags_rota_overview | ImGuiWindowFlags_NoBackground;
};
