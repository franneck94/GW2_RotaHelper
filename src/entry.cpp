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
#include "Constants.h"
#include "MumbleUtils.h"
#include "Render.h"
#include "Settings.h"
#include "Shared.h"
#include "Version.h"
#include "resource.h"

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
ID3D11Device *pd3dDevice = nullptr;

void ToggleShowWindowGW2_RotaHelper(const char *keybindIdentifier, bool)
{
    Settings::ToggleShowWindow(Globals::SettingsPath);
}

void RegisterQuickAccessShortcut()
{
    Globals::APIDefs->QuickAccess.Add("SHORTCUT_GW2_RotaHelper",
                                      "TEX_GW2_RotaHelper_NORMAL",
                                      "TEX_GW2_RotaHelper_HOVER",
                                      KB_TOGGLE_GW2_RotaHelper,
                                      "Toggle GW2_RotaHelper Window");
    Globals::APIDefs->InputBinds.RegisterWithString(
        KB_TOGGLE_GW2_RotaHelper,
        ToggleShowWindowGW2_RotaHelper,
        "CTRL+Q");
}

void DeregisterQuickAccessShortcut()
{
    Globals::APIDefs->QuickAccess.Remove("SHORTCUT_GW2_RotaHelper");
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID)
{
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

    Globals::APIDefs = aApi;
    ImGui::SetCurrentContext((ImGuiContext *)Globals::APIDefs->ImguiContext);
    ImGui::SetAllocatorFunctions(
        (void *(*)(size_t, void *))Globals::APIDefs->ImguiMalloc,
        (void (*)(void *, void *))Globals::APIDefs->ImguiFree);

    Globals::NexusLink =
        (NexusLinkData *)Globals::APIDefs->DataLink.Get("DL_NEXUS_LINK");
    Globals::MumbleData =
        (Mumble::Data *)Globals::APIDefs->DataLink.Get("DL_MUMBLE_LINK");
    Globals::RTAPIData =
        (RTAPI::RealTimeData *)Globals::APIDefs->DataLink.Get("DL_RTAPI");

    Globals::APIDefs->Renderer.Register(ERenderType_Render, AddonRender);
    Globals::APIDefs->Renderer.Register(ERenderType_OptionsRender,
                                        AddonOptions);

    AddonPath = Globals::APIDefs->Paths.GetAddonDirectory("GW2RotaHelper");
    Globals::SettingsPath = Globals::APIDefs->Paths.GetAddonDirectory(
        "GW2RotaHelper/settings.json");

    std::filesystem::create_directories(AddonPath);

    auto data_path = AddonPath;
    std::filesystem::create_directories(data_path / "img");
    std::filesystem::create_directories(data_path / "bench");
    std::filesystem::create_directories(data_path / "bench/power");
    std::filesystem::create_directories(data_path / "bench/condition");
    Globals::Render.set_data_path(data_path);

    Settings::Load(Globals::SettingsPath);

    if (Settings::HorizontalSkillLayout)
        Globals::SkillIconSize = 64.0F;
    else
        Globals::SkillIconSize = 28.0F;

    Globals::APIDefs->Textures.LoadFromResource("TEX_GW2_RotaHelper_NORMAL",
                                                IDB_GW2_RotaHelper_NORMAL,
                                                hSelf,
                                                nullptr);
    Globals::APIDefs->Textures.LoadFromResource("TEX_GW2_RotaHelper_HOVER",
                                                IDB_GW2_RotaHelper_HOVER,
                                                hSelf,
                                                nullptr);
    RegisterQuickAccessShortcut();

#ifdef _DEBUG
    // Globals::MemoryReader.Initialize();
#endif

    Globals::APIDefs->Events.Subscribe("EV_ARCDPS_COMBATEVENT_LOCAL_RAW",
                                       ArcEv::OnCombatLocal);

    if (Globals::APIDefs && Globals::APIDefs->DataLink.Get)
    {
        IDXGISwapChain *pSwapChain =
            (IDXGISwapChain *)Globals::APIDefs->SwapChain;
        if (pSwapChain)
        {
            HRESULT hr = pSwapChain->GetDevice(__uuidof(ID3D11Device),
                                               (void **)&pd3dDevice);
            if (FAILED(hr))
                pd3dDevice = nullptr;
        }
    }
}

void AddonUnload()
{
#ifdef _DEBUG
    // Globals::MemoryReader.Cleanup();
#endif

    if (pd3dDevice)
        pd3dDevice->Release();

    Globals::APIDefs->Renderer.Deregister(AddonRender);
    Globals::APIDefs->Renderer.Deregister(AddonOptions);

    Globals::NexusLink = nullptr;
    Globals::RTAPIData = nullptr;

    Settings::Save(Globals::SettingsPath);

    DeregisterQuickAccessShortcut();

    Globals::APIDefs->Events.Unsubscribe("EV_ARCDPS_COMBATEVENT_LOCAL_RAW",
                                         ArcEv::OnCombatLocal);
}

void AddonRender()
{
    if ((!Globals::NexusLink) || (!Globals::NexusLink->IsGameplay) ||
        (!Settings::ShowWindow))
        return;

    Globals::Render.toggle_vis(Settings::ShowWindow);
    Globals::Render.render(pd3dDevice);
}

void AddonOptions()
{
}
