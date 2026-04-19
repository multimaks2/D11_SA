#pragma once

#include "InputManager.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

// Состояние ввода и захват мыши (аналог слоя app_input + WinInput в gta-reversed: события → состояние).
namespace d11::app {

class AppInput {
public:
    void beginFrame(HWND hwnd);

    [[nodiscard]] d11::InputManager& inputManager() { return m_input; }
    [[nodiscard]] const d11::InputManager& inputManager() const { return m_input; }

    void onLButtonDown(HWND hwnd, int x, int y);
    void onLButtonUp(HWND hwnd);

    [[nodiscard]] bool tryMouseLookDelta(int x, int y, WPARAM wparam_mouse_buttons, int& out_dx, int& out_dy);

    AppInput() = default;
    ~AppInput() = default;

    AppInput(const AppInput&) = delete;
    AppInput& operator=(const AppInput&) = delete;
    AppInput(AppInput&&) = delete;
    AppInput& operator=(AppInput&&) = delete;

private:
    d11::InputManager m_input;
    bool m_lmb_down = false;
    int m_last_mouse_x = 0;
    int m_last_mouse_y = 0;
};

} // namespace d11::app
