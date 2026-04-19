#pragma once

#include <Windows.h>
#include <d3d11.h>

#include "DebugUi.h"
#include "DiscordManager.h"
#include "ImGuiBackend.h"
#include "app_camera.h"
#include "app_input.h"
#include "GameLoader.h"
#include "render/Dx11RenderPipeline.h"

#include <chrono>
#include <cstdint>

namespace app {

class Application {
public:
    int Run();

private:
    static constexpr int kWindowWidth = 1920;
    static constexpr int kWindowHeight = 1080;

    bool CreateMainWindow();
    bool CreateDx11Context();
    void RenderFrame();
    void ResizeBackBuffer(std::uint32_t client_w, std::uint32_t client_h);
    void UpdateFpsTitle();
    float ComputeFrameDeltaSeconds();
    void Shutdown();
    bool ProcessMessages();

    static bool IsWindowInteractive(HWND hwnd);
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM w_param, LPARAM l_param);

    HWND m_hwnd = nullptr;
    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_device_context = nullptr;
    IDXGISwapChain* m_swap_chain = nullptr;
    ID3D11RenderTargetView* m_render_target_view = nullptr;

    std::uint32_t m_client_w = kWindowWidth;
    std::uint32_t m_client_h = kWindowHeight;

    std::chrono::steady_clock::time_point m_lastFpsTitleUpdate {};
    std::uint32_t m_framesForFpsTitle = 0;

    std::chrono::steady_clock::time_point m_last_frame {};

    d11::app::AppInput m_app_input {};
    d11::app::AppCamera m_app_camera {};
    d11::render::Dx11RenderPipeline m_render_pipeline {};
    GameLoader m_game_loader {};
    DebugUi m_debug_ui {};
    ImGuiBackend m_imgui {};
    DiscordManager m_discord {};
};

} // namespace app
