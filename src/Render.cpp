#include <windows.h>

#include <commdlg.h>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")
#include <conio.h>
#include <d3d11.h>

#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <map>
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
    std::vector<std::filesystem::path> get_bench_files(const std::filesystem::path &bench_path)
    {
        std::vector<std::filesystem::path> files;

        for (const auto &entry : std::filesystem::directory_iterator(bench_path))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".json")
            {
                files.push_back(entry.path());
            }
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
            if (ImGui::BeginCombo("##benches_combo", selected_bench_index >= 0 ? benches_files[selected_bench_index].filename().string().c_str() : "Select..."))
            {
                for (int n = 0; n < benches_files.size(); n++)
                {
                    bool is_selected = (selected_bench_index == n);
                    if (ImGui::Selectable(benches_files[n].filename().string().c_str(), is_selected))
                    {
                        selected_bench_index = n;
                        selected_file_path = bench_path / std::filesystem::path(benches_files[n]);

                        rotation_run.load_data(selected_file_path, img_path, pd3dDevice);
                        texture_map.clear();
                    }
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
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
            if (is_current)
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.7f, 0.3f, 0.3f));

            if (texture)
                ImGui::Image((ImTextureID)texture, ImVec2(28, 28));
            else
                ImGui::Dummy(ImVec2(28, 28));

            ImGui::SameLine();

            if (is_current)
                ImGui::Text("-> %s", skill_info.skill_name.empty() ? "N/A" : skill_info.skill_name.c_str());
            else
                ImGui::Text("   %s", skill_info.skill_name.empty() ? "N/A" : skill_info.skill_name.c_str());

            if (is_current)
                ImGui::PopStyleColor();
        }

        const auto skill_ev = get_current_skill();
        if (skill_ev.SkillID != 0)
            rotation_run.pop_bench_rotation_queue();

        ImGui::EndChild();
    }

    ImGui::End();
}
