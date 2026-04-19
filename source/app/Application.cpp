#include "Application.h"

#include "assets/appicon/resource.h"

#include <windowsx.h>

#include <dxgi.h>

#include <algorithm>
#include <cstdio>

namespace {

constexpr float kMaxFrameDeltaSeconds = 0.1f;

UINT PickBackbufferMsaaCount(ID3D11Device* device)
{
    if (device == nullptr)
    {
        return 1u;
    }
    constexpr UINT kCandidates[] = {8u, 4u, 2u};
    for (const UINT n : kCandidates)
    {
        UINT levels = 0;
        if (SUCCEEDED(device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, n, &levels)) && levels > 0)
        {
            return n;
        }
    }
    return 1u;
}

} // namespace

namespace app {

bool Application::IsWindowInteractive(HWND hwnd)
{
    return GetForegroundWindow() == hwnd && IsIconic(hwnd) == FALSE;
}

int Application::Run() {
    ImGuiBackend::EnableDpiAwareness();

    if (!CreateMainWindow()) {
        return 1;
    }

    if (!CreateDx11Context()) {
        Shutdown();
        return 1;
    }

    m_last_frame = std::chrono::steady_clock::now();

    while (ProcessMessages()) {
        RenderFrame();
    }

    Shutdown();
    return 0;
}

bool Application::CreateMainWindow() {
    WNDCLASSEXW window_class{};
    window_class.cbSize = sizeof(window_class);
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = &Application::WindowProc;
    const HINSTANCE instance = GetModuleHandleW(nullptr);
    window_class.hInstance = instance;
    window_class.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_APP_ICON));
    window_class.hIconSm = (HICON)LoadImageW(
        instance,
        MAKEINTRESOURCEW(IDI_APP_ICON),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON),
        LR_DEFAULTCOLOR);
    window_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
    window_class.lpszClassName = L"D11_SA_DX11_WINDOW";

    if (!RegisterClassExW(&window_class)) {
        return false;
    }

    RECT rect{0, 0, kWindowWidth, kWindowHeight};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    const int window_w = rect.right - rect.left;
    const int window_h = rect.bottom - rect.top;

    RECT work_area{};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work_area, 0);
    const int pos_x = work_area.left + (work_area.right - work_area.left - window_w) / 2;
    const int pos_y = work_area.top + (work_area.bottom - work_area.top - window_h) / 2;

    m_hwnd = CreateWindowExW(
        0,
        window_class.lpszClassName,
        L"D11_SA",
        WS_OVERLAPPEDWINDOW,
        pos_x,
        pos_y,
        window_w,
        window_h,
        nullptr,
        nullptr,
        window_class.hInstance,
        this
    );

    if (!m_hwnd) {
        return false;
    }

    ShowWindow(m_hwnd, SW_SHOWDEFAULT);
    UpdateWindow(m_hwnd);
    return true;
}

bool Application::CreateDx11Context() {
    const D3D_FEATURE_LEVEL feature_levels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0
    };

    D3D_FEATURE_LEVEL created_feature_level = D3D_FEATURE_LEVEL_11_0;
    UINT device_flags = 0;
#if defined(_DEBUG)
    device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    HRESULT create_device_hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        device_flags,
        feature_levels,
        2,
        D3D11_SDK_VERSION,
        &m_device,
        &created_feature_level,
        &m_device_context);

    if (FAILED(create_device_hr) || m_device == nullptr || m_device_context == nullptr) {
        return false;
    }

    UINT msaa = PickBackbufferMsaaCount(m_device);

    IDXGIDevice* dxgi_device = nullptr;
    if (FAILED(m_device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgi_device))) || dxgi_device == nullptr)
    {
        return false;
    }

    IDXGIAdapter* adapter = nullptr;
    const HRESULT adapter_hr = dxgi_device->GetAdapter(&adapter);
    dxgi_device->Release();
    if (FAILED(adapter_hr) || adapter == nullptr)
    {
        return false;
    }

    IDXGIFactory* factory = nullptr;
    const HRESULT factory_hr = adapter->GetParent(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&factory));
    adapter->Release();
    if (FAILED(factory_hr) || factory == nullptr)
    {
        return false;
    }

    DXGI_SWAP_CHAIN_DESC swap_chain_desc{};
    swap_chain_desc.BufferDesc.Width = kWindowWidth;
    swap_chain_desc.BufferDesc.Height = kWindowHeight;
    swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.BufferCount = 2;
    swap_chain_desc.OutputWindow = m_hwnd;
    swap_chain_desc.Windowed = TRUE;
    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    HRESULT swap_hr = E_FAIL;
    while (true)
    {
        swap_chain_desc.SampleDesc.Count = msaa;
        swap_chain_desc.SampleDesc.Quality = 0;
        swap_hr = factory->CreateSwapChain(m_device, &swap_chain_desc, &m_swap_chain);
        if (SUCCEEDED(swap_hr))
        {
            break;
        }
        if (msaa <= 1u)
        {
            factory->Release();
            return false;
        }
        msaa = (msaa == 8u) ? 4u : (msaa == 4u) ? 2u : 1u;
    }

    factory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER);
    factory->Release();

    ID3D11Texture2D* back_buffer = nullptr;
    const HRESULT back_buffer_result = m_swap_chain->GetBuffer(
        0,
        __uuidof(ID3D11Texture2D),
        reinterpret_cast<void**>(&back_buffer)
    );
    if (FAILED(back_buffer_result) || !back_buffer) {
        return false;
    }

    const HRESULT render_target_result = m_device->CreateRenderTargetView(
        back_buffer,
        nullptr,
        &m_render_target_view
    );
    back_buffer->Release();

    if (FAILED(render_target_result)) {
        return false;
    }

    m_device_context->OMSetRenderTargets(1, &m_render_target_view, nullptr);

    D3D11_VIEWPORT viewport{};
    viewport.Width = static_cast<FLOAT>(kWindowWidth);
    viewport.Height = static_cast<FLOAT>(kWindowHeight);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    m_device_context->RSSetViewports(1, &viewport);

    if (!m_render_pipeline.Initialize(m_device, kWindowWidth, kWindowHeight, msaa))
    {
        return false;
    }

    if (!m_imgui.Initialize(m_hwnd, m_device, m_device_context))
    {
        m_render_pipeline.Shutdown();
        return false;
    }

    RECT client_rect{};
    GetClientRect(m_hwnd, &client_rect);
    m_client_w = static_cast<std::uint32_t>((std::max)(0L, client_rect.right - client_rect.left));
    m_client_h = static_cast<std::uint32_t>((std::max)(0L, client_rect.bottom - client_rect.top));
    if (m_client_w == 0u || m_client_h == 0u)
    {
        m_client_w = kWindowWidth;
        m_client_h = kWindowHeight;
    }

    m_debug_ui.ApplyStyle();
    m_debug_ui.SetGameLoader(&m_game_loader);
    m_render_pipeline.SetGridVisible(m_debug_ui.IsSceneGridVisible());
    m_render_pipeline.SetAxesVisible(m_debug_ui.IsSceneAxesVisible());
    m_game_loader.StartLoadAsync();

    m_discord.InitializeDefault();

    return true;
}

void Application::UpdateFpsTitle() {
    ++m_framesForFpsTitle;
    using clock = std::chrono::steady_clock;
    const auto now = clock::now();
    if (m_lastFpsTitleUpdate.time_since_epoch().count() == 0) {
        m_lastFpsTitleUpdate = now;
    }
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastFpsTitleUpdate);
    if (elapsed.count() < 1000) {
        return;
    }
    const float elapsed_ms = static_cast<float>(elapsed.count());
    const float fps = (elapsed_ms > 0.0f) ? (m_framesForFpsTitle * 1000.0f / elapsed_ms) : 0.0f;
    m_framesForFpsTitle = 0;
    m_lastFpsTitleUpdate = now;
    wchar_t title_buf[128];
    std::swprintf(title_buf, 128, L"D11_SA | %.0f FPS", static_cast<double>(fps));
    SetWindowTextW(m_hwnd, title_buf);
}

float Application::ComputeFrameDeltaSeconds()
{
    const auto now = std::chrono::steady_clock::now();
    float dt = static_cast<float>(
                   std::chrono::duration_cast<std::chrono::microseconds>(now - m_last_frame).count()) *
               1e-6f;
    m_last_frame = now;
    if (dt > kMaxFrameDeltaSeconds)
    {
        dt = kMaxFrameDeltaSeconds;
    }
    return dt;
}

void Application::ResizeBackBuffer(std::uint32_t client_w, std::uint32_t client_h)
{
    if (m_swap_chain == nullptr || m_device == nullptr || m_device_context == nullptr)
    {
        return;
    }
    if (client_w == 0u || client_h == 0u)
    {
        return;
    }
    if (client_w == m_client_w && client_h == m_client_h && m_render_target_view != nullptr)
    {
        return;
    }

    m_client_w = client_w;
    m_client_h = client_h;

    m_device_context->OMSetRenderTargets(0, nullptr, nullptr);

    if (m_render_target_view != nullptr)
    {
        m_render_target_view->Release();
        m_render_target_view = nullptr;
    }

    HRESULT hr = m_swap_chain->ResizeBuffers(0, client_w, client_h, DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr))
    {
        return;
    }

    ID3D11Texture2D* back_buffer = nullptr;
    hr = m_swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&back_buffer));
    if (FAILED(hr) || back_buffer == nullptr)
    {
        return;
    }

    hr = m_device->CreateRenderTargetView(back_buffer, nullptr, &m_render_target_view);
    back_buffer->Release();
    if (FAILED(hr))
    {
        m_render_target_view = nullptr;
        return;
    }

    m_device_context->OMSetRenderTargets(1, &m_render_target_view, nullptr);

    D3D11_VIEWPORT viewport{};
    viewport.Width = static_cast<FLOAT>(client_w);
    viewport.Height = static_cast<FLOAT>(client_h);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    m_device_context->RSSetViewports(1, &viewport);

    m_render_pipeline.Resize(m_device, client_w, client_h);
}

void Application::RenderFrame() {
    m_app_input.beginFrame(m_hwnd);

    if (m_app_input.inputManager().isKeyPressedForGame(VK_ESCAPE) && !m_imgui.WantsCaptureKeyboard())
    {
        PostQuitMessage(0);
        return;
    }

    UpdateFpsTitle();
    const float dt = ComputeFrameDeltaSeconds();

    m_imgui.BeginFrame();
    m_debug_ui.PushFrameTiming(dt);
    m_debug_ui.UpdateFovInertia(m_app_camera.gtaCamera());
    float previewYaw = 0.0f;
    float previewPitch = 0.0f;
    float previewWheel = 0.0f;
    float previewPanDx = 0.0f;
    float previewPanDy = 0.0f;
    m_debug_ui.GetAndResetModelPreviewNav(previewYaw, previewPitch, previewWheel, previewPanDx, previewPanDy);
    m_render_pipeline.UpdateModelPreview(
        m_device_context,
        m_debug_ui,
        m_game_loader,
        previewYaw,
        previewPitch,
        previewWheel,
        previewPanDx,
        previewPanDy);
    m_render_pipeline.UpdateTxdInspectThumbnails(m_debug_ui);
    m_debug_ui.DrawUi(m_app_camera.gtaCamera());

    if (m_app_input.inputManager().isWindowRenderable() && !m_imgui.WantsCaptureKeyboard())
    {
        m_app_camera.updateMovement(dt, m_app_input.inputManager(), &m_debug_ui);
    }

    m_render_pipeline.SetGridVisible(m_debug_ui.IsSceneGridVisible());
    m_render_pipeline.SetAxesVisible(m_debug_ui.IsSceneAxesVisible());
    m_render_pipeline.SetImportedSceneCubesVisible(m_debug_ui.IsFallbackCubesVisible());
    m_render_pipeline.SetImportedSceneDffVisible(m_debug_ui.IsDffMeshesVisible());
    m_render_pipeline.SetSceneDffIdeDistanceCull(m_debug_ui.IsSceneDffIdeDistanceCull());
    m_render_pipeline.PumpImportedSceneDffUploads(m_debug_ui, m_game_loader);

    if (m_debug_ui.ConsumeImportSceneRequest())
    {
        const auto st = m_game_loader.GetLoadState();
        std::printf(
            "[SceneImport] Application: processing import request, GameLoader state=%u (0=Idle 1=Loading 2=Loaded 3=Failed)\n",
            static_cast<unsigned>(st));
        std::fflush(stdout);
        m_render_pipeline.ImportSceneFromLoader(m_game_loader, m_debug_ui, m_debug_ui.IsImportQuaternionMode());
    }

    m_render_pipeline.DrawFrame(
        m_device_context,
        m_render_target_view,
        m_app_camera.gtaCamera(),
        m_client_w,
        m_client_h
    );

    m_imgui.RenderDrawData();

    m_swap_chain->Present(1, 0);

    m_discord.Tick();
}

void Application::Shutdown() {
    m_discord.Shutdown();
    m_imgui.Shutdown();
    m_render_pipeline.Shutdown();

    if (m_render_target_view) {
        m_render_target_view->Release();
        m_render_target_view = nullptr;
    }
    if (m_swap_chain) {
        m_swap_chain->Release();
        m_swap_chain = nullptr;
    }
    if (m_device_context) {
        m_device_context->Release();
        m_device_context = nullptr;
    }
    if (m_device) {
        m_device->Release();
        m_device = nullptr;
    }
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

bool Application::ProcessMessages() {
    MSG message{};
    while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
        if (message.message == WM_QUIT) {
            return false;
        }
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return true;
}

LRESULT CALLBACK Application::WindowProc(HWND hwnd, UINT message, WPARAM w_param, LPARAM l_param) {
    if (message == WM_NCCREATE)
    {
        const CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(l_param);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        return DefWindowProcW(hwnd, message, w_param, l_param);
    }

    auto* app = reinterpret_cast<Application*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!app)
    {
        return DefWindowProcW(hwnd, message, w_param, l_param);
    }

    const LRESULT imgui_handled = app->m_imgui.TryWndProc(hwnd, message, w_param, l_param);
    if (imgui_handled != 0)
    {
        return imgui_handled;
    }

    switch (message) {
    case WM_SYSCOMMAND:
        if ((w_param & 0xFFF0u) == SC_KEYMENU)
        {
            return 0;
        }
        break;

    case WM_SYSKEYDOWN:
        if (w_param == VK_F4)
        {
            break;
        }
        return 0;

    case WM_SYSKEYUP:
    case WM_SYSCHAR:
    case WM_MENUCHAR:
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_SIZE:
        if (w_param != SIZE_MINIMIZED)
        {
            const std::uint32_t nw = static_cast<std::uint32_t>(LOWORD(l_param));
            const std::uint32_t nh = static_cast<std::uint32_t>(HIWORD(l_param));
            app->ResizeBackBuffer(nw, nh);
        }
        return 0;

    case WM_DPICHANGED:
    {
        const LRESULT dlr = DefWindowProcW(hwnd, message, w_param, l_param);
        app->m_imgui.RefreshDpiFromWindow(hwnd);
        return dlr;
    }

    case WM_LBUTTONDOWN:
        app->m_app_input.onLButtonDown(hwnd, GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param));
        return 0;

    case WM_LBUTTONUP:
        app->m_app_input.onLButtonUp(hwnd);
        return 0;

    case WM_MOUSEMOVE:
        if (!IsWindowInteractive(hwnd) || app->m_imgui.WantsCaptureMouse())
        {
            return 0;
        }
        {
            int dx = 0;
            int dy = 0;
            if (app->m_app_input.tryMouseLookDelta(
                    GET_X_LPARAM(l_param),
                    GET_Y_LPARAM(l_param),
                    w_param,
                    dx,
                    dy))
            {
                app->m_app_camera.applyMouseLook(dx, dy, &app->m_debug_ui);
            }
        }
        return 0;

    case WM_MOUSEWHEEL:
        if (!IsWindowInteractive(hwnd) || app->m_imgui.WantsCaptureMouse())
        {
            return 0;
        }
        app->m_debug_ui.QueueFovScrollDelta(GET_WHEEL_DELTA_WPARAM(w_param), app->m_app_camera.gtaCamera());
        return 0;

    default:
        break;
    }

    return DefWindowProcW(hwnd, message, w_param, l_param);
}

} // namespace app
