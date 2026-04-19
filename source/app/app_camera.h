#pragma once

#include "InputManager.h"
#include "render/GtaStyleCamera.h"

class DebugUi;

// Позиция/обзор для рендера (в gta-reversed низкоуровневый app_camera — RwCamera; здесь — «сцена» + WASD).
namespace d11::app {

class AppCamera {
public:
    [[nodiscard]] d11::render::GtaStyleCamera& gtaCamera() { return m_camera; }
    [[nodiscard]] const d11::render::GtaStyleCamera& gtaCamera() const { return m_camera; }

    void updateMovement(float dt, const d11::InputManager& input, const DebugUi* debug_ui);
    void applyMouseLook(int dx, int dy, const DebugUi* debug_ui);

    AppCamera() = default;
    ~AppCamera() = default;

    AppCamera(const AppCamera&) = delete;
    AppCamera& operator=(const AppCamera&) = delete;
    AppCamera(AppCamera&&) = delete;
    AppCamera& operator=(AppCamera&&) = delete;

private:
    d11::render::GtaStyleCamera m_camera;
    bool m_movement_inertia_initialized = false;
    float m_target_move_x = 0.0f;
    float m_target_move_y = 0.0f;
    float m_target_move_z = 0.0f;
    float m_move_velocity_x = 0.0f;
    float m_move_velocity_y = 0.0f;
    float m_move_velocity_z = 0.0f;
};

} // namespace d11::app
