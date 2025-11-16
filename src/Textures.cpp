#include <windows.h>

#include <commdlg.h>
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")
#include <conio.h>
#include <d3d11.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>

#include "nexus/Nexus.h"

#include "LogData.h"
#include "Textures.h"
#include "Types.h"

namespace
{
static const std::map<int, int> fix_skill_img_ids = {
    {3332122, 3379164}, // Isolate
    {3332077, 3379162}, // Perforate
    {3332117, 3379165}, // Distress
    {3332087, 3379166}, // Extirpate
    {3332102, 3379163}, // Addle
};
} // namespace


ID3D11ShaderResourceView *LoadTextureFromPNG_WIC(ID3D11Device *device, const std::wstring &filename)
{
    if (!device)
        return nullptr;

    IWICImagingFactory *factory = nullptr;
    IWICBitmapDecoder *decoder = nullptr;
    IWICBitmapFrameDecode *frame = nullptr;
    IWICFormatConverter *converter = nullptr;
    ID3D11Texture2D *texture = nullptr;
    ID3D11ShaderResourceView *srv = nullptr;

    auto hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    auto com_initialized = SUCCEEDED(hr);

    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
        return nullptr;

    try
    {
        hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
        if (FAILED(hr))
        {
            if (com_initialized)
                CoUninitialize();
            return nullptr;
        }

        hr = factory->CreateDecoderFromFilename(filename.c_str(),
                                                nullptr,
                                                GENERIC_READ,
                                                WICDecodeMetadataCacheOnLoad,
                                                &decoder);
        if (FAILED(hr))
        {
            factory->Release();
            if (com_initialized)
                CoUninitialize();
            return nullptr;
        }

        hr = decoder->GetFrame(0, &frame);
        if (FAILED(hr))
        {
            decoder->Release();
            factory->Release();
            if (com_initialized)
                CoUninitialize();
            return nullptr;
        }

        hr = factory->CreateFormatConverter(&converter);
        if (FAILED(hr))
        {
            frame->Release();
            decoder->Release();
            factory->Release();
            if (com_initialized)
                CoUninitialize();
            return nullptr;
        }

        hr = converter->Initialize(frame,
                                   GUID_WICPixelFormat32bppRGBA,
                                   WICBitmapDitherTypeNone,
                                   nullptr,
                                   0.f,
                                   WICBitmapPaletteTypeCustom);
        if (FAILED(hr))
        {
            converter->Release();
            frame->Release();
            decoder->Release();
            factory->Release();
            if (com_initialized)
                CoUninitialize();
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

        hr = device->CreateTexture2D(&desc, &subResource, &texture);
        if (SUCCEEDED(hr))
        {
            hr = device->CreateShaderResourceView(texture, nullptr, &srv);
        }

        // Cleanup
        if (texture)
            texture->Release();
        converter->Release();
        frame->Release();
        decoder->Release();
        factory->Release();

        if (com_initialized)
            CoUninitialize();

        return srv;
    }
    catch (const std::bad_alloc &e)
    {
        // Handle memory allocation failures
        if (texture)
            texture->Release();
        if (converter)
            converter->Release();
        if (frame)
            frame->Release();
        if (decoder)
            decoder->Release();
        if (factory)
            factory->Release();
        if (com_initialized)
            CoUninitialize();
        return nullptr;
    }
    catch (const std::exception &e)
    {
        // Handle other standard exceptions
        if (texture)
            texture->Release();
        if (converter)
            converter->Release();
        if (frame)
            frame->Release();
        if (decoder)
            decoder->Release();
        if (factory)
            factory->Release();
        if (com_initialized)
            CoUninitialize();
        return nullptr;
    }
    catch (...)
    {
        // Handle any other exceptions
        if (texture)
            texture->Release();
        if (converter)
            converter->Release();
        if (frame)
            frame->Release();
        if (decoder)
            decoder->Release();
        if (factory)
            factory->Release();
        if (com_initialized)
            CoUninitialize();
        return nullptr;
    }
}

TextureMapType LoadAllSkillTextures(ID3D11Device *device,
                                    const LogSkillInfoMap &log_skill_info_map,
                                    const std::filesystem::path &img_folder)
{
    if (!device)
        return {};

    TextureMapType texture_map;

    try
    {
        for (const auto &[icon_id, info] : log_skill_info_map)
        {
            try
            {
                if (info.name.empty())
                    continue;

                auto actual_icon_id = icon_id;
                if (fix_skill_img_ids.find(icon_id) != fix_skill_img_ids.end())
                    actual_icon_id = fix_skill_img_ids.at(icon_id);

                std::string ext = ".png";
                size_t dot = info.icon_url.find_last_of('.');
                if (dot != std::string::npos && dot + 1 < info.icon_url.size())
                    ext = info.icon_url.substr(dot);

                const auto img_path = img_folder / (std::to_string(actual_icon_id) + ext);
                if (!std::filesystem::exists(img_path))
                    continue;

                auto *tex = LoadTextureFromPNG_WIC(device, img_path.wstring());
                if (tex)
                    texture_map[icon_id] = tex;
            }
            catch (const std::exception &e)
            {
                // Skip this texture if there's an error loading it
                continue;
            }
        }
    }
    catch (const std::exception &e)
    {
        // If there's a general error, clean up what we have and return empty map
        ReleaseTextureMap(texture_map);
        return {};
    }

    return texture_map;
}

void ReleaseTextureMap(TextureMapType &texture_map)
{
    for (auto &[icon_id, texture] : texture_map)
    {
        if (texture)
        {
            texture->Release();
            texture = nullptr;
        }
    }

    texture_map.clear();
}
