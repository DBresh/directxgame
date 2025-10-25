#include <DX3D/InputSystem/Camera.h>
#include <cmath>

namespace dx3d
{
    Camera::Camera()
    {
        updateViewMatrix();
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

        Vec3 forward{
            sin(m_yaw) * cos(m_pitch),
            sin(m_pitch),
            cos(m_yaw) * cos(m_pitch)
        };
        forward = forward.normalize();

        Vec3 right = cross(forward, Vec3(0, 1, 0)).normalize();

        m_position += forward * (m_moveForward * moveStep);
        m_position += right * (m_moveRight * moveStep);

        m_forward = forward;
        updateViewMatrix();
    }

    void Camera::updateViewMatrix()
    {
        Vec3 target = m_position + m_forward;
        m_viewMatrix.setLookAtLH(m_position, target, m_up);
    }

    void Camera::onKeyDown(int) {}
    void Camera::onKeyUp(int key)
    {
        if ((char)key == 'W' || (char)key == 'S') m_moveForward = 0;
        if ((char)key == 'A' || (char)key == 'D') m_moveRight = 0;
    }

    void Camera::onKeyPress(int key)
    {
        float dt = static_cast<float>(Time::Instance()->deltaTime());

        if ((char)key == 'W') m_moveForward = 1;
        if ((char)key == 'S') m_moveForward = -1;
        if ((char)key == 'D') m_moveRight = -1;
        if ((char)key == 'A') m_moveRight = 1;

        if ((char)key == 'I') m_pitch += m_sensitivity * 10 * dt;
        if ((char)key == 'K') m_pitch -= m_sensitivity * 10 * dt;
        if ((char)key == 'J') m_yaw -= m_sensitivity * 10 * dt;
        if ((char)key == 'L') m_yaw += m_sensitivity * 10 * dt;
    }

    void Camera::onMouseMove(Point deltaMouse)
    {
        float dt = static_cast<float>(Time::Instance()->deltaTime());
        m_yaw += deltaMouse.x * m_sensitivity * dt;
        m_pitch -= deltaMouse.y * m_sensitivity * dt;

        const float limit = 1.55f; // ~89 degrees
        if (m_pitch > limit)  m_pitch = limit;
        if (m_pitch < -limit) m_pitch = -limit;
    }

    void Camera::onMouseDown(int) {}
    void Camera::onMouseUp(int) {}
    void Camera::onMouseWheel(int) {}
}
