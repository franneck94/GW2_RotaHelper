#include <cmath>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

#include <DirectXMath.h>
#include <Windows.h>

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

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        hSelf = hModule;
        break;
    case DLL_PROCESS_DETACH:
        break;
    case DLL_THREAD_ATTACH:
        break;
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
    APIDefs = aApi;
    ImGui::SetCurrentContext((ImGuiContext *)APIDefs->ImguiContext);
    ImGui::SetAllocatorFunctions((void *(*)(size_t, void *))APIDefs->ImguiMalloc, (void (*)(void *, void *))APIDefs->ImguiFree);

    NexusLink = (NexusLinkData *)APIDefs->DataLink.Get("DL_NEXUS_LINK");
    RTAPIData = (RTAPI::RealTimeData *)APIDefs->DataLink.Get("DL_RTAPI");
    APIDefs->Renderer.Register(ERenderType_Render, AddonRender);
    APIDefs->Renderer.Register(ERenderType_OptionsRender, AddonOptions);
    AddonPath = APIDefs->Paths.GetAddonDirectory("GW2RotaHelper");
    SettingsPath = APIDefs->Paths.GetAddonDirectory("GW2RotaHelper/settings.json");
    std::filesystem::create_directory(AddonPath);
    Settings::Load(SettingsPath);
}

void AddonUnload()
{
    APIDefs->Renderer.Deregister(AddonOptions);
    APIDefs->Renderer.Deregister(AddonRender);

    NexusLink = nullptr;
    RTAPIData = nullptr;

    Settings::Save(SettingsPath);
}

void AddonRender()
{
    if ((!NexusLink) || (!NexusLink->IsGameplay) || (!Settings::ShowWindow))
    {
        return;
    }

    render.toggle_vis(Settings::ShowWindow);

    ImGuiIO &io = ImGui::GetIO();
    ID3D11Device *pd3dDevice = nullptr;

    if (io.BackendRendererUserData)
    {
        struct ImGui_ImplDX11_Data
        {
            ID3D11Device *pd3dDevice;
            ID3D11DeviceContext *pd3dDeviceContext;
        };

        ImGui_ImplDX11_Data *bd = (ImGui_ImplDX11_Data *)io.BackendRendererUserData;
        pd3dDevice = bd->pd3dDevice;
    }

    if (pd3dDevice)
    {
        render.render(pd3dDevice);
    }
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
    return 0;
}
