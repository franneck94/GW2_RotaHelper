#include <cmath>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

#include <DirectXMath.h>
#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>

#include "arcdps/ArcDPS.h"
#include "imgui.h"
#include "mumble/Mumble.h"
#include "nexus/Nexus.h"
#include "rtapi/RTAPI.hpp"

#include "ArcEvents.h"
#include "Render.h"
#include "Settings.h"
#include "Shared.h"
#include "UeEvents.h"
#include "Version.h"
#include "resource.h"
#include "Constants.h"

namespace dx = DirectX;

void AddonLoad(AddonAPI *aApi);
void AddonUnload();
void AddonRender();
void AddonOptions();
ArcDPS::Exports *ArcdpsInit();
uintptr_t ArcdpsRelease();

HMODULE hSelf;
AddonDefinition AddonDef{};
std::filesystem::path AddonPath;
std::filesystem::path SettingsPath;
ID3D11Device *pd3dDevice = nullptr;

void ToggleShowWindowGW2_RotaHelper(const char *keybindIdentifier, bool)
{
    Settings::ToggleShowWindow(SettingsPath);
}

void RegisterQuickAccessShortcut()
{
    APIDefs->QuickAccess.Add("SHORTCUT_GW2_RotaHelper", "TEX_GW2_RotaHelper_NORMAL", "TEX_GW2_RotaHelper_HOVER", KB_TOGGLE_GW2_RotaHelper, "Toggle GW2_RotaHelper Window");
    APIDefs->InputBinds.RegisterWithString(KB_TOGGLE_GW2_RotaHelper, ToggleShowWindowGW2_RotaHelper, "CTRL+Q");
}

void DeregisterQuickAccessShortcut()
{
    APIDefs->QuickAccess.Remove("SHORTCUT_GW2_RotaHelper");
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        hSelf = hModule;
        DisableThreadLibraryCalls(hModule);
        break;
    case DLL_PROCESS_DETACH:
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}

extern "C" __declspec(dllexport) AddonDefinition *GetAddonDef()
{
    AddonDef.Signature = -2443566;
    AddonDef.APIVersion = NEXUS_API_VERSION;
    AddonDef.Name = "GW2RotaHelper";
    AddonDef.Version.Major = MAJOR;
    AddonDef.Version.Minor = MINOR;
    AddonDef.Version.Build = BUILD;
    AddonDef.Version.Revision = REVISION;
    AddonDef.Author = "Franneck.1274";
    AddonDef.Description = "GW2 Rota Helper";
    AddonDef.Load = AddonLoad;
    AddonDef.Unload = AddonUnload;
    AddonDef.Flags = EAddonFlags_None;
    AddonDef.Provider = EUpdateProvider_GitHub;
    AddonDef.UpdateLink = "https://github.com/franneck94/GW2RotaHelper";

    return &AddonDef;
}

void OnAddonLoaded(int *aSignature)
{
    if (!aSignature)
    {
        return;
    }
}
void OnAddonUnloaded(int *aSignature)
{
    if (!aSignature)
    {
        return;
    }
}

void AddonLoad(AddonAPI *aApi)
{
    if (!aApi)
        return;

    APIDefs = aApi;
    ImGui::SetCurrentContext((ImGuiContext *)APIDefs->ImguiContext);
    ImGui::SetAllocatorFunctions((void *(*)(size_t, void *))APIDefs->ImguiMalloc, (void (*)(void *, void *))APIDefs->ImguiFree);

    NexusLink = (NexusLinkData *)APIDefs->DataLink.Get("DL_NEXUS_LINK");
    RTAPIData = (RTAPI::RealTimeData *)APIDefs->DataLink.Get("DL_RTAPI");

    APIDefs->Renderer.Register(ERenderType_Render, AddonRender);
    APIDefs->Renderer.Register(ERenderType_OptionsRender, AddonOptions);

    AddonPath = APIDefs->Paths.GetAddonDirectory("GW2RotaHelper");
    SettingsPath = APIDefs->Paths.GetAddonDirectory("GW2RotaHelper/settings.json");

    std::filesystem::create_directories(AddonPath);

    auto data_path = AddonPath;
    std::filesystem::create_directories(data_path / "img");
    std::filesystem::create_directories(data_path / "bench");
    std::filesystem::create_directories(data_path / "bench/power");
    std::filesystem::create_directories(data_path / "bench/condition");
    render.set_data_path(data_path);

    Settings::Load(SettingsPath);

    APIDefs->Textures.LoadFromResource("TEX_GW2_RotaHelper_NORMAL", IDB_GW2_RotaHelper_NORMAL, hSelf, nullptr);
    APIDefs->Textures.LoadFromResource("TEX_GW2_RotaHelper_HOVER", IDB_GW2_RotaHelper_HOVER, hSelf, nullptr);
    RegisterQuickAccessShortcut();

    if (APIDefs && APIDefs->DataLink.Get)
    {
        IDXGISwapChain *pSwapChain = (IDXGISwapChain *)APIDefs->SwapChain;
        if (pSwapChain)
        {
            HRESULT hr = pSwapChain->GetDevice(__uuidof(ID3D11Device), (void **)&pd3dDevice);
            if (FAILED(hr))
                pd3dDevice = nullptr;
        }
    }
}

void AddonUnload()
{
    if (pd3dDevice)
        pd3dDevice->Release();

    APIDefs->Renderer.Deregister(AddonRender);
    APIDefs->Renderer.Deregister(AddonOptions);

    NexusLink = nullptr;
    RTAPIData = nullptr;

    Settings::Save(SettingsPath);

    DeregisterQuickAccessShortcut();
}

void AddonRender()
{
    if ((!NexusLink) || (!NexusLink->IsGameplay) || (!Settings::ShowWindow))
        return;

    render.toggle_vis(Settings::ShowWindow);
    render.render(pd3dDevice, APIDefs);
}

void AddonOptions()
{
}

extern "C" __declspec(dllexport) void *get_init_addr(char *arcversion, void *imguictx, void *id3dptr, HANDLE arcdll, void *mallocfn, void *freefn, uint32_t d3dversion)
{
    return ArcdpsInit;
}

extern "C" __declspec(dllexport) void *get_release_addr()
{
    return ArcdpsRelease;
}

ArcDPS::Exports *ArcdpsInit()
{
    ArcExports.Signature = -24255;
    ArcExports.ImGuiVersion = 18000;
    ArcExports.Size = sizeof(ArcDPS::Exports);
    ArcExports.Name = ADDON_NAME;
    ArcExports.Build = "0.1.1.0";
    ArcExports.CombatLocalCallback = ArcEv::OnCombatLocal;

    return &ArcExports;
}

uintptr_t ArcdpsRelease()
{
    ArcExports.CombatLocalCallback = nullptr;

    return 0;
}
