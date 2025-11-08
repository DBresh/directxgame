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

    void Camera::update()
    {
        float dt = static_cast<float>(Time::Instance()->deltaTime());
        float moveStep = m_speed * dt;

        Vec3 forward = m_forward.normalize();
        Vec3 right = cross(Vec3(0, 1, 0), forward).normalize();

        m_position += forward * (m_moveForward * moveStep);
        m_position += right * (m_moveRight * moveStep);
        m_position += Vec3(0, 1, 0) * (m_moveUp * moveStep);

        updateViewMatrix();
    }

    void Camera::updateViewMatrix()
    {
        Vec3 forward{
            sin(m_yaw) * cos(m_pitch),
            sin(m_pitch),
            cos(m_yaw) * cos(m_pitch)
        };
        forward = forward.normalize();

        Vec3 right = cross(Vec3(0, 1, 0), forward).normalize();
        Vec3 up = cross(forward, right).normalize();

        m_forward = forward;

        Vec3 target = m_position + forward;
        m_viewMatrix.setLookAtLH(m_position, target, up);
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
        if ((char)key == 'J') m_yaw -= m_sensitivity * 10 * dt;
        if ((char)key == 'L') m_yaw += m_sensitivity * 10 * dt;
    }

    void Camera::onMouseMove(Point deltaMouse)
    {
        m_yaw += deltaMouse.x * m_sensitivity * 0.01f;
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