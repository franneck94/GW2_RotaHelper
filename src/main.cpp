#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include <d3d11.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <tchar.h>

#include "ArcEvents.h"
#include "Render.h"
#include "Settings.h"
#include "Shared.h"
#include "Types.h"

namespace
{
enum class Keys
{
    W,
    A,
    S,
    D,
    C,
    ANY,
    NONE,
};
};

// Data
static ID3D11Device *g_pd3dDevice = NULL;
static ID3D11DeviceContext *g_pd3dDeviceContext = NULL;
static IDXGISwapChain *g_pSwapChain = NULL;
static ID3D11RenderTargetView *g_mainRenderTargetView = NULL;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Main code
int main(int, char **)
{
    SettingsPath = std::filesystem::absolute(
        std::filesystem::path(__FILE__).parent_path() / "addon_settings.json");

    // Create application window
    // ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEX wc = {sizeof(WNDCLASSEX),
                     CS_CLASSDC,
                     WndProc,
                     0L,
                     0L,
                     GetModuleHandle(NULL),
                     NULL,
                     NULL,
                     NULL,
                     NULL,
                     _T("ImGui Example"),
                     NULL};
    ::RegisterClassEx(&wc);
    HWND hwnd = ::CreateWindow(wc.lpszClassName,
                               _T("Dear ImGui DirectX11 Example"),
                               WS_OVERLAPPEDWINDOW,
                               100,
                               100,
                               1280,
                               800,
                               NULL,
                               NULL,
                               wc.hInstance,
                               NULL);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    render.toggle_vis(true);

    auto data_path = std::filesystem::absolute(
        std::filesystem::path(__FILE__).parent_path().parent_path() / "data");
    render.set_data_path(data_path);

    Settings::Load(SettingsPath);

    // Main loop
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT)
    {
        if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGuiIO &io = ImGui::GetIO();

        auto key = Keys::NONE;
        std::map<int, Keys> key_map = {
            {0x57, Keys::W}, // VK_W
            {0x41, Keys::A}, // VK_A
            {0x53, Keys::S}, // VK_S
            {0x44, Keys::D}, // VK_D
            {0x43, Keys::C}  // VK_C
        };

        for (int vk = 0x08; vk <= 0xFE;
             vk++) // VK_BACK to 0xFE (most virtual keys)
        {
            if (GetAsyncKeyState(vk) & 0x8000)
            {
                auto it = key_map.find(vk);
                if (it != key_map.end())
                {
                    key = it->second;
                }
                break;
            }
        }

        if (key != Keys::NONE)
        {
            auto icon_id = std::int32_t{0};
            auto skill_id = std::int32_t{0};
            auto skill_name = std::string{""};
            switch (key)
            {
            case Keys::W:
                icon_id = 1058593;
                skill_id = 30713;
                skill_name = "Thunderclap";
                break;
            case Keys::A:
                icon_id = 1058590;
                skill_id = 30088;
                skill_name = "Electro-whirl";
                break;
            case Keys::S:
                icon_id = 103176;
                skill_id = 76530;
                skill_name = "Magnetic Bomb";
                break;
            case Keys::D:
                icon_id = 103404;
                skill_id = 5823;
                skill_name = "Fire Bomb";
                break;
            case Keys::ANY:
            default:
                icon_id = static_cast<uint64_t>(-1);
                skill_id = static_cast<uint64_t>(-1);
                skill_name = "Wildcard";
                break;
            }

            auto fake_ev = ArcDPS::CombatEvent{};
            auto fake_src = ArcDPS::AgentShort{};
            auto fake_dst = ArcDPS::AgentShort{};
            char fake_skillname[64] = {};
            strncpy(fake_skillname,
                    skill_name.c_str(),
                    sizeof(fake_skillname) - 1);
            auto fake_id = static_cast<uint64_t>(skill_id);
            auto fake_revision = static_cast<uint64_t>(1);

            fake_src.ID = 123;
            fake_src.Profession = 3;
            fake_src.Specialization = 43;
            fake_src.Name = (char *)"Source";
            fake_src.IsSelf = true;

            fake_dst.ID = 456;
            fake_dst.Profession = 5;
            fake_dst.Specialization = 6;
            fake_dst.Name = (char *)"Target";

            ArcEv::OnCombatLocal(&fake_ev,
                                 &fake_src,
                                 &fake_dst,
                                 fake_skillname,
                                 fake_id,
                                 fake_revision);
        }

        render.render(g_pd3dDevice);

        ImGui::Render();
        g_pd3dDeviceContext->OMSetRenderTargets(1,
                                                &g_mainRenderTargetView,
                                                NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView,
                                                   (float *)&clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0); // Present with vsync
    }

    Settings::Save(SettingsPath);

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}

bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0,
    };
    if (D3D11CreateDeviceAndSwapChain(NULL,
                                      D3D_DRIVER_TYPE_HARDWARE,
                                      NULL,
                                      createDeviceFlags,
                                      featureLevelArray,
                                      2,
                                      D3D11_SDK_VERSION,
                                      &sd,
                                      &g_pSwapChain,
                                      &g_pd3dDevice,
                                      &featureLevel,
                                      &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain)
    {
        g_pSwapChain->Release();
        g_pSwapChain = NULL;
    }
    if (g_pd3dDeviceContext)
    {
        g_pd3dDeviceContext->Release();
        g_pd3dDeviceContext = NULL;
    }
    if (g_pd3dDevice)
    {
        g_pd3dDevice->Release();
        g_pd3dDevice = NULL;
    }
}

void CreateRenderTarget()
{
    ID3D11Texture2D *pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer,
                                         NULL,
                                         &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView)
    {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = NULL;
    }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
                                                             UINT msg,
                                                             WPARAM wParam,
                                                             LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0,
                                        (UINT)LOWORD(lParam),
                                        (UINT)HIWORD(lParam),
                                        DXGI_FORMAT_UNKNOWN,
                                        0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
