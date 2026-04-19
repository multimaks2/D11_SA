#include "ImGuiBackend.h"

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace app {

void ImGuiBackend::EnableDpiAwareness()
{
    ImGui_ImplWin32_EnableDpiAwareness();
}

bool ImGuiBackend::Initialize(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context)
{
    if (m_initialized)
    {
        return true;
    }
    if (hwnd == nullptr || device == nullptr || context == nullptr)
    {
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    io.IniFilename = "imgui.ini";

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    const float dpi = ImGui_ImplWin32_GetDpiScaleForHwnd(hwnd);
    style.ScaleAllSizes(dpi);
    // ImGui 1.92+: DPI/user font scale lives on ImGuiStyle, not ImGuiIO.
    style.FontScaleMain = 1.0f;
    style.FontScaleDpi = dpi;
    m_lastDpiScale = dpi;

    if (!ImGui_ImplWin32_Init(hwnd))
    {
        ImGui::DestroyContext();
        return false;
    }
    if (!ImGui_ImplDX11_Init(device, context))
    {
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        return false;
    }

    m_initialized = true;
    return true;
}

void ImGuiBackend::RefreshDpiFromWindow(HWND hwnd)
{
    if (!m_initialized || hwnd == nullptr || ImGui::GetCurrentContext() == nullptr)
    {
        return;
    }
    const float scale = ImGui_ImplWin32_GetDpiScaleForHwnd(hwnd);
    if (scale <= 0.0f)
    {
        return;
    }
    ImGuiStyle& style = ImGui::GetStyle();
    if (m_lastDpiScale > 0.0f)
    {
        style.ScaleAllSizes(scale / m_lastDpiScale);
    }
    m_lastDpiScale = scale;
    style.FontScaleDpi = scale;
    ImGui_ImplDX11_CreateDeviceObjects();
}

void ImGuiBackend::Shutdown()
{
    if (!m_initialized)
    {
        return;
    }
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    m_initialized = false;
    m_lastDpiScale = 1.0f;
}

void ImGuiBackend::BeginFrame()
{
    if (!m_initialized)
    {
        return;
    }
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImGuiBackend::RenderDrawData()
{
    if (!m_initialized)
    {
        return;
    }
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

bool ImGuiBackend::WantsCaptureMouse() const
{
    return m_initialized && ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureMouse;
}

bool ImGuiBackend::WantsCaptureKeyboard() const
{
    return m_initialized && ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureKeyboard;
}

LRESULT ImGuiBackend::TryWndProc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param) const
{
    if (!m_initialized)
    {
        return 0;
    }
    return ImGui_ImplWin32_WndProcHandler(hwnd, msg, w_param, l_param);
}

} // namespace app
