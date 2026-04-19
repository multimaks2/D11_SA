#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include <bitset>
#include <cstdint>

namespace d11 {

enum class InputFocusMode : std::uint8_t
{
    ForegroundGame = 0,
    WindowVisibleNoFocus,
    Minimized,
    Invalid
};

class InputManager {
public:
    void beginFrame(HWND hwnd);

    [[nodiscard]] InputFocusMode getFocusMode() const { return m_focusMode; }
    [[nodiscard]] bool isForegroundGame() const { return m_focusMode == InputFocusMode::ForegroundGame; }

    [[nodiscard]] bool isKeyDown(int virtualKey) const;
    [[nodiscard]] bool isKeyPressed(int virtualKey) const;
    [[nodiscard]] bool isKeyReleased(int virtualKey) const;

    [[nodiscard]] bool isKeyDownForGame(int virtualKey) const;
    [[nodiscard]] bool isKeyPressedForGame(int virtualKey) const;
    [[nodiscard]] bool isKeyReleasedForGame(int virtualKey) const;

    [[nodiscard]] bool isKeyPressedWhenUnfocused(int virtualKey) const;
    [[nodiscard]] bool isWindowRenderable() const;

private:
    static bool vkInRange(int virtualKey);

    InputFocusMode m_focusMode = InputFocusMode::Invalid;
    std::bitset<256> m_curr {};
    std::bitset<256> m_prev {};
};

} // namespace d11
