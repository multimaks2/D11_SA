#include "app_input.h"

#include <windowsx.h>

namespace d11::app {

void AppInput::beginFrame(HWND hwnd)
{
    m_input.beginFrame(hwnd);
}

void AppInput::onLButtonDown(HWND hwnd, int x, int y)
{
    m_lmb_down = true;
    m_last_mouse_x = x;
    m_last_mouse_y = y;
    SetCapture(hwnd);
}

void AppInput::onLButtonUp(HWND hwnd)
{
    m_lmb_down = false;
    if (GetCapture() == hwnd)
    {
        ReleaseCapture();
    }
}

bool AppInput::tryMouseLookDelta(int x, int y, WPARAM wparam_mouse_buttons, int& out_dx, int& out_dy)
{
    if (!m_lmb_down || (wparam_mouse_buttons & MK_LBUTTON) == 0)
    {
        return false;
    }
    out_dx = x - m_last_mouse_x;
    out_dy = y - m_last_mouse_y;
    m_last_mouse_x = x;
    m_last_mouse_y = y;
    return true;
}

} // namespace d11::app
