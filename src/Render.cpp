#include <windows.h>

#include <commdlg.h>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")
#include <conio.h>
#include <d3d11.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <thread>
#include <utility>

#include "imgui.h"

#include "Constants.h"
#include "LogData.h"
#include "Render.h"
#include "Shared.h"

namespace
{
    std::vector<BenchFileInfo> get_bench_files(const std::filesystem::path &bench_path)
    {
        std::vector<BenchFileInfo> files;
        std::map<std::string, std::vector<std::filesystem::path>> directory_files;

        try
        {
            // First, collect all JSON files and organize by directory
            for (const auto &entry : std::filesystem::recursive_directory_iterator(bench_path))
            {
                if (entry.is_regular_file() && entry.path().extension() == ".json")
                {
                    auto relative_path = std::filesystem::relative(entry.path(), bench_path);
                    auto parent_dir = relative_path.parent_path().string();

                    if (parent_dir.empty())
                        parent_dir = "."; // Root directory

                    directory_files[parent_dir].push_back(entry.path());
                }
            }

            // Now organize the files with directory headers
            for (const auto &[dir_name, dir_files] : directory_files)
            {
                if (dir_name != ".")
                {
                    // Add directory header
                    auto header_path = bench_path / dir_name;
                    files.emplace_back(header_path, std::filesystem::path(dir_name), true);
                }

                // Add files in this directory
                for (const auto &file_path : dir_files)
                {
                    auto relative_path = std::filesystem::relative(file_path, bench_path);
                    files.emplace_back(file_path, relative_path, false);
                }
            }
        }
        catch (const std::filesystem::filesystem_error& ex)
        {
            // Handle filesystem errors gracefully
            std::cerr << "Error scanning bench files: " << ex.what() << std::endl;
        }

        return files;
    }

    ID3D11ShaderResourceView *LoadTextureFromPNG_WIC(ID3D11Device *device, const std::wstring &filename)
    {
        IWICImagingFactory *factory = nullptr;
        IWICBitmapDecoder *decoder = nullptr;
        IWICBitmapFrameDecode *frame = nullptr;
        IWICFormatConverter *converter = nullptr;

        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(hr))
            return nullptr;

        hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                              IID_PPV_ARGS(&factory));
        if (FAILED(hr))
            return nullptr;

        hr = factory->CreateDecoderFromFilename(filename.c_str(), nullptr, GENERIC_READ,
                                                WICDecodeMetadataCacheOnLoad, &decoder);
        if (FAILED(hr))
        {
            factory->Release();
            return nullptr;
        }

        hr = decoder->GetFrame(0, &frame);
        if (FAILED(hr))
        {
            decoder->Release();
            factory->Release();
            return nullptr;
        }

        hr = factory->CreateFormatConverter(&converter);
        if (FAILED(hr))
        {
            frame->Release();
            decoder->Release();
            factory->Release();
            return nullptr;
        }

        hr = converter->Initialize(frame, GUID_WICPixelFormat32bppRGBA,
                                   WICBitmapDitherTypeNone, nullptr, 0.f, WICBitmapPaletteTypeCustom);
        if (FAILED(hr))
        {
            converter->Release();
            frame->Release();
            decoder->Release();
            factory->Release();
            return nullptr;
        }

        UINT width, height;
        converter->GetSize(&width, &height);
        std::vector<BYTE> buffer(width * height * 4);
        converter->CopyPixels(nullptr, width * 4, static_cast<uint32_t>(buffer.size()), buffer.data());

        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA subResource = {};
        subResource.pSysMem = buffer.data();
        subResource.SysMemPitch = width * 4;

        ID3D11Texture2D *texture = nullptr;
        ID3D11ShaderResourceView *srv = nullptr;
        hr = device->CreateTexture2D(&desc, &subResource, &texture);
        if (SUCCEEDED(hr))
        {
            hr = device->CreateShaderResourceView(texture, nullptr, &srv);
        }

        if (texture)
            texture->Release();
        converter->Release();
        frame->Release();
        decoder->Release();
        factory->Release();
        CoUninitialize();

        return srv;
    }

    TextureMap LoadAllSkillTextures(
        ID3D11Device *device,
        const SkillInfoMap &skill_info_map,
        const std::filesystem::path &img_folder)
    {
        TextureMap texture_map;

        for (const auto &[skill_id, info] : skill_info_map)
        {
            if (info.name.empty())
                continue;
            std::string ext = ".png";
            size_t dot = info.icon_url.find_last_of('.');
            if (dot != std::string::npos && dot + 1 < info.icon_url.size())
            {
                ext = info.icon_url.substr(dot);
            }
            std::filesystem::path img_path = img_folder / (std::to_string(skill_id) + ext);
            if (!std::filesystem::exists(img_path))
                continue;
            auto *tex = LoadTextureFromPNG_WIC(device, img_path.wstring());
            if (tex)
                texture_map[skill_id] = tex;
        }
        return texture_map;
    }
}

Render::Render()
{
    benches_files = get_bench_files(bench_path);
}

void Render::key_press_cb(const bool pressed)
{
    key_press_event_in_this_frame = pressed;
}

EvCombatDataPersistent Render::get_current_skill()
{
    if (!key_press_event_in_this_frame)
        return EvCombatDataPersistent{};

    return combat_buffer[prev_combat_buffer_index];
}

void Render::toggle_vis(const bool flag)
{
    show_window = flag;
}

void Render::render(ID3D11Device *pd3dDevice)
{
    if (!show_window)
        return;

    if (benches_files.size() == 0)
        benches_files = get_bench_files(bench_path);

    if (ImGui::Begin("GW2RotaHelper", &show_window))
    {
        if (!benches_files.empty())
        {
            ImGui::Text("Select Bench File:");

            // Add filter input
            ImGui::Text("Filter:");
            ImGui::SameLine();
            if (ImGui::InputText("##filter", filter_buffer, sizeof(filter_buffer)))
            {
                filter_string = std::string(filter_buffer);
                std::transform(filter_string.begin(), filter_string.end(), filter_string.begin(), ::tolower);
            }

            // Create filtered list
            std::vector<std::pair<int, const BenchFileInfo*>> filtered_files;
            std::set<std::string> directories_with_matches;

            if (filter_string.empty())
            {
                // No filter, show all
                for (int n = 0; n < benches_files.size(); n++)
                {
                    filtered_files.emplace_back(n, &benches_files[n]);
                }
            }
            else
            {
                // First pass: find all matching files and their directories
                for (int n = 0; n < benches_files.size(); n++)
                {
                    const auto& file_info = benches_files[n];

                    if (!file_info.is_directory_header)
                    {
                        std::string display_lower = file_info.display_name;
                        std::transform(display_lower.begin(), display_lower.end(), display_lower.begin(), ::tolower);

                        if (display_lower.find(filter_string) != std::string::npos)
                        {
                            // File matches filter
                            auto parent_dir = file_info.relative_path.parent_path().string();
                            if (!parent_dir.empty() && parent_dir != ".")
                            {
                                directories_with_matches.insert(parent_dir);
                            }
                        }
                    }
                }

                // Second pass: add matching files and their directory headers
                for (int n = 0; n < benches_files.size(); n++)
                {
                    const auto& file_info = benches_files[n];

                    if (file_info.is_directory_header)
                    {
                        // Only show directory header if it contains matching files
                        if (directories_with_matches.count(file_info.relative_path.string()) > 0)
                        {
                            filtered_files.emplace_back(n, &file_info);
                        }
                    }
                    else
                    {
                        std::string display_lower = file_info.display_name;
                        std::transform(display_lower.begin(), display_lower.end(), display_lower.begin(), ::tolower);

                        if (display_lower.find(filter_string) != std::string::npos)
                        {
                            filtered_files.emplace_back(n, &file_info);
                        }
                    }
                }
            }

            std::string combo_preview;
            if (selected_bench_index >= 0 && selected_bench_index < benches_files.size())
            {
                combo_preview = benches_files[selected_bench_index].relative_path.filename().string();
            }
            else
            {
                combo_preview = "Select...";
            }

            if (ImGui::BeginCombo("##benches_combo", combo_preview.c_str()))
            {
                for (const auto& [original_index, file_info] : filtered_files)
                {
                    if (file_info->is_directory_header)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.9f, 1.0f));
                        ImGui::Selectable(file_info->display_name.c_str(), false, ImGuiSelectableFlags_Disabled);
                        ImGui::PopStyleColor();
                    }
                    else
                    {
                        bool is_selected = (selected_bench_index == original_index);
                        if (ImGui::Selectable(file_info->display_name.c_str(), is_selected))
                        {
                            selected_bench_index = original_index;
                            selected_file_path = file_info->full_path;

                            rotation_run.load_data(selected_file_path, img_path, pd3dDevice);
                            texture_map.clear();
                        }
                        if (is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
        }

        if (!selected_file_path.empty())
        {
            ImGui::SameLine();
            ImGui::Text("Selected: %s", selected_file_path.stem().string().c_str());
        }

        if (rotation_run.futures.size() != 0)
        {
            if (rotation_run.futures.front().valid())
            {
                rotation_run.futures.front().get();
                rotation_run.futures.pop();
            }

            ImGui::End();

            return;
        }
        else if (texture_map.size() == 0)
        {
            texture_map = LoadAllSkillTextures(pd3dDevice, rotation_run.skill_info_map, img_path);
        }

        ImGui::Separator();

        ImGui::BeginChild("CombatBufferChild", ImVec2(0, 200), true, ImGuiWindowFlags_HorizontalScrollbar);

        const auto [start, end, current_idx] = rotation_run.get_current_rotation_indices();

        for (int32_t i = start; i <= end; ++i)
        {
            if (i < 0 || static_cast<size_t>(i) >= rotation_run.rotation_vector.size())
                continue;

            const auto &skill_info = rotation_run.get_rotation_skill(static_cast<size_t>(i));
            auto texture = texture_map[skill_info.skill_id];

            bool is_current = (i == static_cast<int32_t>(current_idx));

            if (texture)
                ImGui::Image((ImTextureID)texture, ImVec2(28, 28));
            else
                ImGui::Dummy(ImVec2(28, 28));

            ImGui::SameLine();

            if (is_current)
                ImGui::Text("-> %s (%.2f) <-", skill_info.skill_name.empty() ? "N/A" : skill_info.skill_name.c_str(), skill_info.cast_time);
            else
                ImGui::Text("   %s (%.2f)", skill_info.skill_name.empty() ? "N/A" : skill_info.skill_name.c_str(), skill_info.cast_time);
        }

        const auto skill_ev = get_current_skill();
        if (skill_ev.SkillID != 0)
            rotation_run.pop_bench_rotation_queue();

        ImGui::EndChild();
    }

    ImGui::End();
}
