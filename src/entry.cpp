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
#include "FileUtils.h"
#include "MumbleUtils.h"
#include "Render.h"
#include "Settings.h"
#include "Shared.h"
#include "TEX_GW2RotaHelperHOVER_data.h"
#include "TEX_GW2RotaHelperNORMAL_data.h"
#include "Version.h"

namespace dx = DirectX;

void AddonLoad(AddonAPI_t *aApi);
void AddonUnload();
void AddonRender();
void AddonOptions();
ArcDPS::Exports *ArcdpsInit();
uintptr_t ArcdpsRelease();

HMODULE hSelf;
AddonDefinition_t AddonDef{};
std::filesystem::path AddonPath;
ID3D11Device *pd3dDevice = nullptr;

void ToggleShowWindowGW2_RotaHelper(const char *, bool isKeyDown)
{
    static bool wasKeyPressed = false;

    if (isKeyDown && !wasKeyPressed)
    {
        Settings::ToggleShowWindow(Globals::SettingsPath);
    }

    wasKeyPressed = isKeyDown;
}

void RegisterQuickAccessShortcut()
{
    Globals::APIDefs->QuickAccess_Add("SHORTCUT_GW2_RotaHelper",
                                      "TEX_GW2_RotaHelper_NORMAL",
                                      "TEX_GW2_RotaHelper_HOVER",
                                      KB_TOGGLE_GW2_RotaHelper,
                                      "Toggle GW2_RotaHelper Window");
    Globals::APIDefs->InputBinds_RegisterWithString(KB_TOGGLE_GW2_RotaHelper, ToggleShowWindowGW2_RotaHelper, "(null)");
}

void DeregisterQuickAccessShortcut()
{
    Globals::APIDefs->QuickAccess_Remove("SHORTCUT_GW2_RotaHelper");
    Globals::APIDefs->InputBinds_Deregister(KB_TOGGLE_GW2_RotaHelper);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        hSelf = hModule;
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

extern "C" __declspec(dllexport) AddonDefinition_t *GetAddonDef()
{
    AddonDef.Signature = -2443566;
    AddonDef.APIVersion = NEXUS_API_VERSION;
    AddonDef.Name = "GW2RotaHelper";
    AddonDef.Version.Major = MAJOR;
    AddonDef.Version.Minor = MINOR;
    AddonDef.Version.Build = BUILD;
    AddonDef.Version.Revision = REVISION;
    AddonDef.Author = "Franneck.1274";
    AddonDef.Description = "Displays the player's skill rotation and helps "
                           "improving it by providing visual feedback.";
    AddonDef.Load = AddonLoad;
    AddonDef.Unload = AddonUnload;
    AddonDef.Flags = AF_None;
    AddonDef.Provider = UP_GitHub;
    AddonDef.UpdateLink = "https://github.com/franneck94/GW2_RotaHelper";

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

void AddonLoad(AddonAPI_t *aApi)
{
    if (!aApi)
        return;

    Globals::APIDefs = aApi;
    ImGui::SetCurrentContext((ImGuiContext *)Globals::APIDefs->ImguiContext);
    ImGui::SetAllocatorFunctions((void *(*)(size_t, void *))Globals::APIDefs->ImguiMalloc,
                                 (void (*)(void *, void *))Globals::APIDefs->ImguiFree);

    Globals::NexusLink = (NexusLinkData_t *)Globals::APIDefs->DataLink_Get("DL_NEXUS_LINK");
    Globals::MumbleData = (Mumble::Data *)Globals::APIDefs->DataLink_Get("DL_MUMBLE_LINK");
    Globals::RTAPIData = (RTAPI::RealTimeData *)Globals::APIDefs->DataLink_Get("DL_RTAPI");

    Globals::APIDefs->GUI_Register(RT_Render, AddonRender);
    Globals::APIDefs->GUI_Register(RT_OptionsRender, AddonOptions);

    AddonPath = Globals::APIDefs->Paths_GetAddonDirectory("GW2RotaHelper");
    Globals::SettingsPath = AddonPath / "settings.json";

    const auto data_path = AddonPath;
    std::filesystem::create_directories(AddonPath);

    const auto bench_dir = data_path / "bench";
    const auto img_dir = data_path / "img";
    const auto skills_dir = data_path / "skills";

    bool has_data_files = false;
    if (std::filesystem::exists(bench_dir) && std::filesystem::exists(img_dir) && std::filesystem::exists(skills_dir))
    {
        try
        {
            has_data_files = !std::filesystem::is_empty(bench_dir) || !std::filesystem::is_empty(img_dir) ||
                             !std::filesystem::is_empty(skills_dir);
        }
        catch (...)
        {
            has_data_files = false;
        }
    }

    if (!has_data_files)
    {
        Globals::BenchDataDownloadState = DownloadState::STARTED;
        DownloadAndExtractDataAsync(AddonPath);
    }

    Globals::Render.set_data_path(data_path);

    Settings::Load(Globals::SettingsPath);

    Globals::SkillIconSize = 64.0F;
    Globals::APIDefs->Textures_LoadFromMemory("TEX_GW2_RotaHelper_NORMAL",
                                              (void *)ARR_GW2RotaHelperNORMAL,
                                              ARR_GW2RotaHelperNORMAL_size,
                                              nullptr);
    Globals::APIDefs->Textures_LoadFromMemory("TEX_GW2_RotaHelper_HOVER",
                                              (void *)ARR_GW2RotaHelperHOVER,
                                              ARR_GW2RotaHelperHOVER_size,
                                              nullptr);
    RegisterQuickAccessShortcut();

    Globals::APIDefs->Events_Subscribe("EV_ARCDPS_COMBATEVENT_LOCAL_RAW", ArcEv::OnCombatLocal);

    if (Globals::APIDefs && Globals::APIDefs->DataLink_Get)
    {
        auto *pSwapChain = (IDXGISwapChain *)Globals::APIDefs->SwapChain;
        if (pSwapChain)
        {
            auto hr = pSwapChain->GetDevice(__uuidof(ID3D11Device), (void **)&pd3dDevice);
            if (FAILED(hr))
                pd3dDevice = nullptr;
        }
    }

    (void)Globals::APIDefs->Log(LOGL_DEBUG, "GW2RotaHelper", "Loaded Addon");
}

void AddonUnload()
{
    if (pd3dDevice)
        pd3dDevice->Release();

    Globals::APIDefs->GUI_Deregister(AddonRender);
    Globals::APIDefs->GUI_Deregister(AddonOptions);

    Globals::NexusLink = nullptr;
    Globals::RTAPIData = nullptr;

    Settings::Save(Globals::SettingsPath);

    DeregisterQuickAccessShortcut();

    Globals::APIDefs->Events_Unsubscribe("EV_ARCDPS_COMBATEVENT_LOCAL_RAW", ArcEv::OnCombatLocal);
    (void)Globals::APIDefs->Log(LOGL_DEBUG, "GW2RotaHelper", "Unloaded Addon");
}

void TriggerParseMumble()
{
    static auto last_parse_time = std::chrono::steady_clock::now();
    static auto pending_identity = Mumble::Identity{};
    static auto pending_count = 0;

    const auto now = std::chrono::steady_clock::now();
    const auto time_since_last_parse = std::chrono::duration_cast<std::chrono::seconds>(now - last_parse_time).count();

    if (time_since_last_parse >= 1 || time_since_last_parse == 0)
    {
        auto current_identity = ParseMumbleIdentity(Globals::MumbleData->Identity);

        if (current_identity.Profession != Globals::Identity.Profession)
        {
            if (current_identity.Profession != pending_identity.Profession)
            {
                pending_identity = current_identity;
                pending_count = 1;
            }
            else
            {
                pending_count++;

                if (pending_count >= 3)
                {
                    Globals::Identity = current_identity;
                    pending_count = 0;
                    pending_identity = Mumble::Identity{};
                    (void)Globals::APIDefs->Log(LOGL_INFO, "GW2RotaHelper", "Detected other profession.");
                }
            }
        }
        else
        {
            Globals::Identity = current_identity;
            pending_count = 0;
            pending_identity = Mumble::Identity{};
        }

        last_parse_time = now;
    }
}

void AddonRender()
{
    static auto profession = ProfessionID::UNKNOWN;

    if ((!Globals::NexusLink) || (!Globals::NexusLink->IsGameplay) || (!Settings::ShowWindow))
        return;

    TriggerParseMumble();
    const auto curr_profession = static_cast<ProfessionID>(Globals::Identity.Profession);
    if (profession != curr_profession)
    {
        Globals::RotationRun.reset_rotation();
        profession = curr_profession;
    }

    Globals::Render.set_show_window(Settings::ShowWindow);
    Globals::Render.render(pd3dDevice);
}

void AddonOptions()
{
}
