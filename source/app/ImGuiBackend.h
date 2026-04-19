#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

struct ID3D11Device;
struct ID3D11DeviceContext;

namespace app {

class ImGuiBackend {
public:
    static void EnableDpiAwareness();

    bool Initialize(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context);
    void Shutdown();

    /** После смены DPI монитора (WM_DPICHANGED): пересчёт размеров стиля и шрифтов. */
    void RefreshDpiFromWindow(HWND hwnd);

    void BeginFrame();
    void RenderDrawData();

    [[nodiscard]] bool IsInitialized() const { return m_initialized; }
    [[nodiscard]] bool WantsCaptureMouse() const;
    [[nodiscard]] bool WantsCaptureKeyboard() const;

    [[nodiscard]] LRESULT TryWndProc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param) const;

    ImGuiBackend() = default;
    ~ImGuiBackend() { Shutdown(); }

    ImGuiBackend(const ImGuiBackend&) = delete;
    ImGuiBackend& operator=(const ImGuiBackend&) = delete;

private:
    bool m_initialized = false;
    float m_lastDpiScale = 1.0f;
};

} // namespace app
