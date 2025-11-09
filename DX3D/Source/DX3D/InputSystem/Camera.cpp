#include <DX3D/InputSystem/Camera.h>
#include <cmath>
#include <Windows.h>

namespace dx3d
{
    Camera::Camera()
    {
        updateViewMatrix();
        m_forward = Vec3(0, 0, 1);
        m_yaw = 0.0f;
        m_pitch = 0.0f;

        setPerspective(degToRad(90), 16.0f / 9.0f, 0.1f, 100.0f);
    }

    void Camera::setPerspective(float fov, float aspect, float zNear, float zFar)
    {
        m_projMatrix.setPerspectiveFovLH(fov, aspect, zNear, zFar);
    }

    void Camera::setPosition(const Vec3& pos)
    {
        m_position = pos;
        updateViewMatrix();
    }

    void Camera::updateViewMatrix()
    {
        float cosPitch = cosf(m_pitch);
        float sinPitch = sinf(m_pitch);
        float cosYaw = cosf(m_yaw);
        float sinYaw = sinf(m_yaw);

        // row-major версія: дивимось по -Z при yaw=0,pitch=0
        Vec3 forward{
            cosPitch * sinYaw,
            sinPitch,
            -cosPitch * cosYaw
        };
        forward = forward.normalize();

        Vec3 upWorld(0, 1, 0);
        Vec3 right = cross(upWorld, forward).normalize();
        Vec3 up = cross(forward, right).normalize();

        m_forward = forward;

        Vec3 target = m_position + forward;
        m_viewMatrix.setLookAtLH(m_position, target, up);
    }

    void Camera::update()
    {
        float dt = static_cast<float>(Time::Instance()->deltaTime());
        float moveStep = m_speed * dt;

        Vec3 forward = m_forward.normalize();

        // Узгоджено з updateViewMatrix()
        Vec3 right = Vec3(forward.z, 0.0f, -forward.x).normalize();
        Vec3 up(0, 1, 0);

        m_position += forward * (m_moveForward * moveStep);
        m_position += right * (m_moveRight * moveStep);
        m_position += up * (m_moveUp * moveStep);

        updateViewMatrix();
    }


    void Camera::onKeyDown(int key) {
        if (key == VK_SHIFT) m_speed *= 5;
    }

    void Camera::onKeyUp(int key)
    {
        if ((char)key == 'W' || (char)key == 'S') m_moveForward = 0;
        if ((char)key == 'A' || (char)key == 'D') m_moveRight = 0;
        if (key == VK_SPACE || key == VK_CONTROL) m_moveUp = 0;
        if (key == VK_SHIFT) m_speed /= 5;
    }

    void Camera::onKeyPress(int key)
    {
        float dt = static_cast<float>(Time::Instance()->deltaTime());

        if ((char)key == 'W') m_moveForward = 1;
        if ((char)key == 'S') m_moveForward = -1;

        if ((char)key == 'D') m_moveRight = 1;
        if ((char)key == 'A') m_moveRight = -1;

        if (key == VK_SPACE)   m_moveUp = 1;
        if (key == VK_CONTROL) m_moveUp = -1;

        if ((char)key == 'I') m_pitch += m_sensitivity * 10 * dt;
        if ((char)key == 'K') m_pitch -= m_sensitivity * 10 * dt;
        if ((char)key == 'J') m_yaw += m_sensitivity * 10 * dt;
        if ((char)key == 'L') m_yaw -= m_sensitivity * 10 * dt;
    }

    void Camera::onMouseMove(Point deltaMouse)
    {
        m_yaw -= deltaMouse.x * m_sensitivity * 0.01f;
        m_pitch -= deltaMouse.y * m_sensitivity * 0.01f;

        const float limit = 1.55f; // ~89°
        if (m_pitch > limit)  m_pitch = limit;
        if (m_pitch < -limit) m_pitch = -limit;

        updateViewMatrix();
    }

    void Camera::onMouseDown(int) {}
    void Camera::onMouseUp(int) {}
    void Camera::onMouseWheel(int) {}
}