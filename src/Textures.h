#pragma once

#include <d3d11.h>

#include <filesystem>
#include <string>
#include <unordered_map>

#include "nexus/Nexus.h"

#include "Types.h"

using TextureMap = std::unordered_map<int, ID3D11ShaderResourceView *>;

constexpr static auto SKILL_ICON_SIZE = 28.0F;

ID3D11ShaderResourceView *LoadTextureFromPNG_WIC(ID3D11Device *device,
                                                 const std::wstring &filename);

TextureMap LoadAllSkillTextures(ID3D11Device *device,
                                const SkillInfoMap &skill_info_map,
                                const std::filesystem::path &img_folder);

TextureMap LoadAllSkillTexturesWithAPI(AddonAPI *APIDefs,
                                       const SkillInfoMap &skill_info_map,
                                       const std::filesystem::path &img_folder);

void ReleaseTextureMap(TextureMap &texture_map);
