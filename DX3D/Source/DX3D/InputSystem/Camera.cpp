#include <DX3D/InputSystem/Camera.h>
#include <Windows.h>

using namespace DirectX;

namespace dx3d
{
    Camera::Camera()
    {
        updateViewMatrix();
        setPerspective(degToRad(90.0f), 16.0f / 9.0f, 0.1f, 100000.0f);
    }

    void Camera::setPerspective(float fov, float aspect, float zNear, float zFar)
    {
        XMMATRIX P = XMMatrixPerspectiveFovLH(fov, aspect, zNear, zFar);
        XMStoreFloat4x4(&m_proj, P);
    }

    void Camera::setScreenSize(float width, float height)
    {
        if (height <= 0.0f) return;

        float aspect = width / height;

        setPerspective(degToRad(90.0f), aspect, 0.1f, 100000.0f);
    }

    void Camera::setPosition(float x, float y, float z)
    {
        m_position = XMFLOAT3(x, y, z);
        updateViewMatrix();
    }

    void Camera::updateViewMatrix()
    {
        float cosPitch = cosf(m_pitch);
        float sinPitch = sinf(m_pitch);
        float cosYaw = cosf(m_yaw);
        float sinYaw = sinf(m_yaw);

        XMVECTOR forward = XMVectorSet(
            cosPitch * sinYaw,
            sinPitch,
            -cosPitch * cosYaw,
            0.0f
        );
        forward = XMVector3Normalize(forward);
        XMStoreFloat3(&m_forward, forward);

        XMVECTOR eye = XMLoadFloat3(&m_position);
        XMVECTOR up = XMVectorSet(0, 1, 0, 0);

        XMMATRIX V = XMMatrixLookAtLH(eye, eye + forward, up);
        XMStoreFloat4x4(&m_view, V);
    }

    void Camera::update()
    {
        float dt = (float)Time::Instance()->deltaTime();
        float move = m_speed * dt;

        XMVECTOR forward = XMLoadFloat3(&m_forward);
        XMVECTOR up = XMVectorSet(0, 1, 0, 0);
        XMVECTOR right = XMVector3Normalize(XMVector3Cross(up, forward));

        XMVECTOR pos = XMLoadFloat3(&m_position);

        pos += forward * (m_moveForward * move);
        pos += right * (m_moveRight * move);
        pos += up * (m_moveUp * move);

        XMStoreFloat3(&m_position, pos);

        updateViewMatrix();
    }

    void Camera::onKeyDown(int key)
    {
        if (key == VK_SHIFT)
            m_speed *= 5;
    }

    void Camera::onKeyUp(int key)
    {
        if ((char)key == 'W' || (char)key == 'S') m_moveForward = 0;
        if ((char)key == 'A' || (char)key == 'D') m_moveRight = 0;
        if (key == VK_SPACE || key == VK_CONTROL) m_moveUp = 0;

        if (key == VK_SHIFT)
            m_speed /= 5;
    }

    void Camera::onKeyPress(int key)
    {
        float dt = (float)Time::Instance()->deltaTime();

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

        const float limit = 1.55f;
        if (m_pitch > limit) m_pitch = limit;
        if (m_pitch < -limit) m_pitch = -limit;

        updateViewMatrix();
    }

    void Camera::onMouseDown(int) {}
    void Camera::onMouseUp(int) {}
    void Camera::onMouseWheel(int) {}
}
