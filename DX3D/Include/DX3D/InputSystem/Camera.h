#pragma once
#include <DX3D/Math/Vec3.h>
#include <DX3D/Math/Matrix4x4.h>
#include <DX3D/Core/Time.h>
#include <DX3D/InputSystem/InputListener.h>

namespace dx3d
{
    class Camera final : public InputListener
    {
    public:
        Camera();

        void update();

        const Matrix4x4& getViewMatrix() const noexcept { return m_viewMatrix; }
        const Matrix4x4& getProjectionMatrix() const noexcept { return m_projMatrix; }

        void setPerspective(float fov, float aspect, float zNear, float zFar);
        void setPosition(const Vec3& pos);
        const Vec3& getPosition() const noexcept { return m_position; }

        void onKeyDown(int key) override;
        void onKeyUp(int key) override;
        void onKeyPress(int key) override;
        void onMouseMove(Point deltaMouse) override;
        void onMouseDown(int button) override;
        void onMouseUp(int button) override;
        void onMouseWheel(int delta) override;

        constexpr float degToRad(float deg) { return deg * 3.1415926535f / 180.0f; }

    private:
        void updateViewMatrix();

    private:
        Vec3 m_position{ 0.3f, 0.0f, -2.0f };
        Vec3 m_forward{ 0.0f, 0.0f, 1.0f };
        Vec3 m_up{ 0.0f, 1.0f, 0.0f };

        float m_yaw{ 0.0f };
        float m_pitch{ 0.0f };
        float m_speed{ 2.0f };
        float m_sensitivity{ 0.2f };

        int m_moveForward{ 0 };
        int m_moveRight{ 0 };
        int m_moveUp{ 0 };

        Matrix4x4 m_viewMatrix;
        Matrix4x4 m_projMatrix;
    };
}