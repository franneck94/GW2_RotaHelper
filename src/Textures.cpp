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
#include <string>

#include "nexus/Nexus.h"

#include "Textures.h"
#include "Types.h"

ID3D11ShaderResourceView *LoadTextureFromPNG_WIC(ID3D11Device *device, const std::wstring &filename)
{
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

    hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                          IID_PPV_ARGS(&factory));
    if (FAILED(hr))
    {
        if (com_initialized)
            CoUninitialize();

        return nullptr;
    }

    hr = factory->CreateDecoderFromFilename(filename.c_str(), nullptr, GENERIC_READ,
                                            WICDecodeMetadataCacheOnLoad, &decoder);
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

    hr = converter->Initialize(frame, GUID_WICPixelFormat32bppRGBA,
                               WICBitmapDitherTypeNone, nullptr, 0.f, WICBitmapPaletteTypeCustom);
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

TextureMap LoadAllSkillTextures(
    ID3D11Device *device,
    const SkillInfoMap &skill_info_map,
    const std::filesystem::path &img_folder)
{
    if (!device)
        return {};

    TextureMap texture_map;

    for (const auto &[icon_id, info] : skill_info_map)
    {
        if (info.name.empty())
            continue;

        std::string ext = ".png";
        size_t dot = info.icon_url.find_last_of('.');
        if (dot != std::string::npos && dot + 1 < info.icon_url.size())
            ext = info.icon_url.substr(dot);

        std::filesystem::path img_path = img_folder / (std::to_string(icon_id) + ext);
        if (!std::filesystem::exists(img_path))
            continue;

        auto *tex = LoadTextureFromPNG_WIC(device, img_path.wstring());
        if (tex)
            texture_map[icon_id] = tex;
    }
    return texture_map;
}

TextureMap LoadAllSkillTexturesWithAPI(
    AddonAPI *APIDefs,
    const SkillInfoMap &skill_info_map,
    const std::filesystem::path &img_folder)
{
    if (!APIDefs)
        return {};

    TextureMap texture_map;

    for (const auto &[icon_id, info] : skill_info_map)
    {
        if (info.name.empty())
            continue;

        std::string ext = ".png";
        size_t dot = info.icon_url.find_last_of('.');
        if (dot != std::string::npos && dot + 1 < info.icon_url.size())
        {
            ext = info.icon_url.substr(dot);
        }

        std::filesystem::path img_path = img_folder / (std::to_string(icon_id) + ext);
        if (!std::filesystem::exists(img_path))
            continue;

        auto *nexus_texture = APIDefs->Textures.GetOrCreateFromFile(img_path.string().c_str(), img_path.string().c_str());
        if (nexus_texture && nexus_texture->Resource)
        {
            texture_map[icon_id] = (ID3D11ShaderResourceView*)nexus_texture->Resource;
        }
    }

    return texture_map;
}

void ReleaseTextureMap(TextureMap &texture_map)
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
