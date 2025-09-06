#include <cmath>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

#include <DirectXMath.h>
#include <Windows.h>

#include "imgui.h"
#include "mumble/Mumble.h"
#include "nexus/Nexus.h"
#include "rtapi/RTAPI.hpp"
#include "arcdps/ArcDPS.h"

#include "Render.h"
#include "Settings.h"
#include "Shared.h"
#include "Version.h"
#include "resource.h"
#include "ArcEvents.h"
#include "UeEvents.h"

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

constexpr const char *KB_ARCDPS_OPTIONS = "KB_ARCDPS_OPTIONS";
constexpr const char *ICON_ARCDPS = "ICON_ARCDPS";
constexpr const char *ICON_ARCDPS_HOVER = "ICON_ARCDPS_HOVER";
constexpr const char *QA_ARCDPS = "QA_ARCDPS";

void ToggleShowWindowLogProofs(const char* aIdentifier, KEYBINDS_PROCESS aKeybindHandler, const char* aKeybind)
{
    Settings::ShowWindow = !Settings::ShowWindow;
    Settings::Settings[SHOW_WINDOW] = Settings::ShowWindow;
    Settings::Save(SettingsPath);
}

void RegisterQuickAccessShortcut()
{
    APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, "Registering GW2RotaHelper quick access shortcut");
    APIDefs->QuickAccess.Add("SHORTCUT_LOG_PROOFS", "TEX_GW2RotaHelper_NORMAL", "TEX_LOG_HOVER", KB_TOGGLE_GW2RotaHelper, "Toggle GW2RotaHelper Window");
}

void DeregisterQuickAccessShortcut()
{
    APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, "Deregistering GW2RotaHelper quick access shortcut");
    APIDefs->QuickAccess.Remove("SHORTCUT_LOG_PROOFS");
}

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
    AddonDef.Signature = -244213566;
    AddonDef.APIVersion = NEXUS_API_VERSION;
    AddonDef.Name = "GW2RotaHelper";
    AddonDef.Version.Major = MAJOR;
    AddonDef.Version.Minor = MINOR;
    AddonDef.Version.Build = BUILD;
    AddonDef.Version.Revision = REVISION;
    AddonDef.Author = "Franneck.1274";
    AddonDef.Description = "API Fetch from https://GW2RotaHelper-production.up.railway.app/";
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
    ImGui::SetAllocatorFunctions((void *(*)(size_t, void *))APIDefs->ImguiMalloc, (void (*)(void *, void *))APIDefs->ImguiFree); // on imgui 1.80+

    APIDefs->Renderer.Register(ERenderType_Render, AddonRender);
    APIDefs->Renderer.Register(ERenderType_OptionsRender, AddonOptions);
    AddonPath = APIDefs->Paths.GetAddonDirectory("GW2RotaHelper");
    SettingsPath = APIDefs->Paths.GetAddonDirectory("GW2RotaHelper/settings.json");
    std::filesystem::create_directory(AddonPath);
    Settings::Load(SettingsPath);

    RegisterQuickAccessShortcut();
}

void AddonUnload()
{
    APIDefs->Renderer.Deregister(AddonOptions);
    APIDefs->Renderer.Deregister(AddonRender);

    NexusLink = nullptr;
    RTAPIData = nullptr;

    Settings::Save(SettingsPath);

    DeregisterQuickAccessShortcut();
    APIDefs->InputBinds.Deregister(KB_TOGGLE_GW2RotaHelper);
}

void AddonRender()
{
    if ((!NexusLink) || (!NexusLink->IsGameplay) || (!Settings::ShowWindow))
    {
        return;
    }

    static Render render{Settings::ShowWindow};
    render.render();
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
    ArcExports.Signature = -24255225;
    ArcExports.ImGuiVersion = 18000;
    ArcExports.Size = sizeof(ArcDPS::Exports);
    ArcExports.Name = ADDON_NAME;
    ArcExports.Build = "0.1.0.0";
    ArcExports.CombatSquadCallback = ArcEv::OnCombatSquad;
    ArcExports.CombatLocalCallback = ArcEv::OnCombatLocal;

    return &ArcExports;
}

uintptr_t ArcdpsRelease()
{
    return 0;
}
