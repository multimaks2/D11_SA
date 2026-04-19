#include "app_camera.h"

#include "DebugUi.h"

#include <DirectXMath.h>

#include <cmath>

namespace {

constexpr float kMovementFixedTimeStep = 1.0f / 60.0f;
constexpr float kFallbackMouseSensitivity = 0.12f;
constexpr float kFallbackMoveSpeed = 600.0f;
constexpr float kFallbackShiftMultiplier = 1.25f;
constexpr float kFallbackAltMultiplier = 0.1f;
constexpr float kFallbackSpring = 20.0f;
constexpr float kFallbackDamping = 0.85f;

} // namespace

namespace d11::app {

void AppCamera::updateMovement(float dt, const d11::InputManager& input, const DebugUi* debug_ui)
{
    (void)dt;
    const DebugUi::CameraControlSettings* cs = debug_ui ? &debug_ui->GetCameraControlSettings() : nullptr;
    const float moveSpeed = cs ? cs->moveSpeed : kFallbackMoveSpeed;
    const float shiftMul = cs ? cs->shiftSpeedMultiplier : kFallbackShiftMultiplier;
    const float altMul = cs ? cs->altSpeedMultiplier : kFallbackAltMultiplier;
    const float spring = cs ? cs->movementSpringStrength : kFallbackSpring;
    const float damping = cs ? cs->movementDamping : kFallbackDamping;

    if (!input.isForegroundGame())
    {
        m_movement_inertia_initialized = false;
        return;
    }
    if (!m_movement_inertia_initialized)
    {
        m_target_move_x = m_camera.GetX();
        m_target_move_y = m_camera.GetY();
        m_target_move_z = m_camera.GetZ();
        m_move_velocity_x = 0.0f;
        m_move_velocity_y = 0.0f;
        m_move_velocity_z = 0.0f;
        m_movement_inertia_initialized = true;
    }

    float maxSpeed = moveSpeed;
    if (input.isKeyDown(VK_SHIFT))
    {
        maxSpeed *= shiftMul;
    }
    if (input.isKeyDown(VK_MENU))
    {
        maxSpeed *= altMul;
    }

    const float yawRad = DirectX::XMConvertToRadians(m_camera.GetYawDegrees());
    const float forwardX = std::sin(yawRad);
    const float forwardY = std::cos(yawRad);
    const float rightX = std::cos(yawRad);
    const float rightY = -std::sin(yawRad);
    const float moveStep = maxSpeed * kMovementFixedTimeStep;

    if (input.isKeyDown('W'))
    {
        m_target_move_x += forwardX * moveStep;
        m_target_move_y += forwardY * moveStep;
    }
    if (input.isKeyDown('S'))
    {
        m_target_move_x -= forwardX * moveStep;
        m_target_move_y -= forwardY * moveStep;
    }
    if (input.isKeyDown('D'))
    {
        m_target_move_x += rightX * moveStep;
        m_target_move_y += rightY * moveStep;
    }
    if (input.isKeyDown('A'))
    {
        m_target_move_x -= rightX * moveStep;
        m_target_move_y -= rightY * moveStep;
    }
    if (input.isKeyDown(VK_SPACE))
    {
        m_target_move_z += moveStep;
    }
    if (input.isKeyDown(VK_CONTROL))
    {
        m_target_move_z -= moveStep;
    }

    float posX = m_camera.GetX();
    float posY = m_camera.GetY();
    float posZ = m_camera.GetZ();
    const float springStep = spring * kMovementFixedTimeStep;

    m_move_velocity_x += (m_target_move_x - posX) * springStep;
    m_move_velocity_y += (m_target_move_y - posY) * springStep;
    m_move_velocity_z += (m_target_move_z - posZ) * springStep;
    m_move_velocity_x *= damping;
    m_move_velocity_y *= damping;
    m_move_velocity_z *= damping;

    posX += m_move_velocity_x * kMovementFixedTimeStep;
    posY += m_move_velocity_y * kMovementFixedTimeStep;
    posZ += m_move_velocity_z * kMovementFixedTimeStep;
    m_camera.SetPosition(posX, posY, posZ);
}

void AppCamera::applyMouseLook(int dx, int dy, const DebugUi* debug_ui)
{
    const float baseSens = debug_ui ? debug_ui->GetCameraControlSettings().mouseLookSensitivity : kFallbackMouseSensitivity;
    const float sensitivity = ::EffectiveMouseLookSensitivity(baseSens, m_camera.GetFovDegrees());
    m_camera.AddYawDegrees(static_cast<float>(dx) * sensitivity);
    m_camera.AddPitchDegrees(static_cast<float>(-dy) * sensitivity);
}

} // namespace d11::app
