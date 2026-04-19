#pragma once

// Камера и оси в стиле GTA / RenderWare (как в gta-reversed: CVector — x, y, z; «высота» — z).
// Плоскость карты — XY, вверх — +Z. Совпадает с d11_sa_old_code/Engine/Camera.h (Camera3D).

#include <DirectXMath.h>

#include <cmath>

namespace d11::render {

class GtaStyleCamera {
public:
    GtaStyleCamera()
        : m_x(0.0f)
        , m_y(-20.0f)
        , m_z(15.0f)
        , m_yawDeg(0.0f)
        , m_pitchDeg(DirectX::XMConvertToDegrees(std::asinf(-0.6f)))
        , m_fovDeg(90.0f)
    {
    }

    void SetPosition(float x, float y, float z)
    {
        m_x = x;
        m_y = y;
        m_z = z;
    }

    void SetAnglesDegrees(float yawZ, float pitchX)
    {
        m_yawDeg = yawZ;
        m_pitchDeg = pitchX;
        NormalizeYaw();
        ClampPitch();
    }

    void SetFovDegrees(float fov)
    {
        m_fovDeg = fov;
        if (m_fovDeg < 1.0f)
        {
            m_fovDeg = 1.0f;
        }
        if (m_fovDeg > 160.0f)
        {
            m_fovDeg = 160.0f;
        }
    }

    void AddYawDegrees(float delta)
    {
        m_yawDeg += delta;
        NormalizeYaw();
    }

    void AddPitchDegrees(float delta)
    {
        m_pitchDeg += delta;
        ClampPitch();
    }

    [[nodiscard]] float GetX() const { return m_x; }
    [[nodiscard]] float GetY() const { return m_y; }
    [[nodiscard]] float GetZ() const { return m_z; }
    [[nodiscard]] float GetYawDegrees() const { return m_yawDeg; }
    [[nodiscard]] float GetPitchDegrees() const { return m_pitchDeg; }
    [[nodiscard]] float GetFovDegrees() const { return m_fovDeg; }

    [[nodiscard]] DirectX::XMMATRIX ViewMatrix() const
    {
        using namespace DirectX;
        const float yr = XMConvertToRadians(m_yawDeg);
        const float pr = XMConvertToRadians(m_pitchDeg);
        const float cp = std::cos(pr);
        const float sp = std::sin(pr);
        const XMVECTOR eye = XMVectorSet(m_x, m_y, m_z, 0.0f);
        const XMVECTOR forward = XMVectorSet(std::sin(yr) * cp, std::cos(yr) * cp, sp, 0.0f);
        const XMVECTOR at = XMVectorAdd(eye, forward);
        const XMVECTOR up = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
        return XMMatrixLookAtRH(eye, at, up);
    }

    [[nodiscard]] DirectX::XMMATRIX ProjectionMatrix(float aspect) const
    {
        using namespace DirectX;
        const float fov = XMConvertToRadians(m_fovDeg);
        // Ближняя плоскость: слишком большое значение обрезает геометрию в упор (оси/сетка).
        constexpr float kNearClip = 0.01f;
        constexpr float kFarClip = 900000.0f;
        return XMMatrixPerspectiveFovRH(fov, aspect, kNearClip, kFarClip);
    }

    [[nodiscard]] DirectX::XMMATRIX WorldViewProjection(float aspect) const
    {
        using namespace DirectX;
        const XMMATRIX world = XMMatrixIdentity();
        const XMMATRIX view = ViewMatrix();
        const XMMATRIX proj = ProjectionMatrix(aspect);
        return XMMatrixMultiply(XMMatrixMultiply(world, view), proj);
    }

private:
    void ClampPitch()
    {
        constexpr float kLimit = 89.0f;
        if (m_pitchDeg > kLimit)
        {
            m_pitchDeg = kLimit;
        }
        if (m_pitchDeg < -kLimit)
        {
            m_pitchDeg = -kLimit;
        }
    }

    void NormalizeYaw()
    {
        m_yawDeg = std::fmod(m_yawDeg, 360.0f);
        if (m_yawDeg > 180.0f)
        {
            m_yawDeg -= 360.0f;
        }
        else if (m_yawDeg < -180.0f)
        {
            m_yawDeg += 360.0f;
        }
    }

    float m_x;
    float m_y;
    float m_z;
    float m_yawDeg;
    float m_pitchDeg;
    float m_fovDeg;
};

} // namespace d11::render
