#include "InputManager.h"

namespace d11 {

void InputManager::beginFrame(HWND hwnd)
{
    m_prev = m_curr;
    m_curr.reset();

    if (hwnd == nullptr || !IsWindow(hwnd))
    {
        m_focusMode = InputFocusMode::Invalid;
        return;
    }

    if (IsIconic(hwnd) != FALSE)
    {
        m_focusMode = InputFocusMode::Minimized;
    }
    else if (GetForegroundWindow() != hwnd)
    {
        m_focusMode = InputFocusMode::WindowVisibleNoFocus;
    }
    else
    {
        m_focusMode = InputFocusMode::ForegroundGame;
    }

    for (int vk = 1; vk < 256; ++vk)
    {
        if ((GetAsyncKeyState(vk) & 0x8000) != 0)
        {
            m_curr.set(static_cast<std::size_t>(vk));
        }
    }
}

bool InputManager::vkInRange(int virtualKey)
{
    return virtualKey > 0 && virtualKey < 256;
}

bool InputManager::isKeyDown(int virtualKey) const
{
    if (!vkInRange(virtualKey))
    {
        return false;
    }
    return m_curr.test(static_cast<std::size_t>(virtualKey));
}

bool InputManager::isKeyPressed(int virtualKey) const
{
    if (!vkInRange(virtualKey))
    {
        return false;
    }
    const std::size_t i = static_cast<std::size_t>(virtualKey);
    return m_curr.test(i) && !m_prev.test(i);
}

bool InputManager::isKeyReleased(int virtualKey) const
{
    if (!vkInRange(virtualKey))
    {
        return false;
    }
    const std::size_t i = static_cast<std::size_t>(virtualKey);
    return !m_curr.test(i) && m_prev.test(i);
}

bool InputManager::isKeyDownForGame(int virtualKey) const
{
    return isForegroundGame() && isKeyDown(virtualKey);
}

bool InputManager::isKeyPressedForGame(int virtualKey) const
{
    return isForegroundGame() && isKeyPressed(virtualKey);
}

bool InputManager::isKeyReleasedForGame(int virtualKey) const
{
    return isForegroundGame() && isKeyReleased(virtualKey);
}

bool InputManager::isKeyPressedWhenUnfocused(int virtualKey) const
{
    return m_focusMode == InputFocusMode::WindowVisibleNoFocus && isKeyPressed(virtualKey);
}

bool InputManager::isWindowRenderable() const
{
    return m_focusMode != InputFocusMode::Invalid && m_focusMode != InputFocusMode::Minimized;
}

} // namespace d11
