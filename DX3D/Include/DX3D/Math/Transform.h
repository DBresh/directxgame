#pragma once
#include <DirectXMath.h>
#include <algorithm>
#include <math.h>
#include <DX3D/Math/Vec3d.h>

namespace dx3d
{
    using namespace DirectX;

    class Transform
    {
    public:
        Transform() = default;

        dx3d::Vec3d getPosition() const noexcept { return m_position; }

        void setPosition(const dx3d::Vec3d& p)
        {
            m_position = p;
            m_dirty = true;
        }

        void setPosition(const XMFLOAT3& p)
        {
            m_position.x = p.x;
            m_position.y = p.y;
            m_position.z = p.z;
            m_dirty = true;
        }

        void setPosition(double x, double y, double z)
        {
            m_position = dx3d::Vec3d(x, y, z);
            m_dirty = true;
        }

        void translate(const dx3d::Vec3d& d)
        {
            m_position += d;
            m_dirty = true;
        }

        XMFLOAT3 getScale()       const noexcept { return m_scale; }
        XMFLOAT4 getQuaternion()  const noexcept { return m_quat; }
        XMFLOAT3 getEuler()       const noexcept { return m_euler; }

        void setScale(const XMFLOAT3& s)
        {
            m_scale = s;
            m_dirty = true;
        }

        void setQuaternion(const XMFLOAT4& q)
        {
            m_quat = q;
            updateEulerFromQuat();
            m_dirty = true;
        }

        void setEuler(const XMFLOAT3& e)
        {
            m_euler = e;
            updateQuaternionFromEuler();
            m_dirty = true;
        }

        void rotateEuler(const XMFLOAT3& delta)
        {
            m_euler.x += delta.x;
            m_euler.y += delta.y;
            m_euler.z += delta.z;

            updateQuaternionFromEuler();
            m_dirty = true;
        }

        void rotateQuat(const XMFLOAT4& dq)
        {
            XMVECTOR q = XMLoadFloat4(&m_quat);
            XMVECTOR r = XMLoadFloat4(&dq);

            q = XMQuaternionMultiply(r, q);
            XMStoreFloat4(&m_quat, q);

            updateEulerFromQuat();
            m_dirty = true;
        }

        XMFLOAT4X4 getWorldMatrixRelative(const dx3d::Vec3d& cameraOrigin) const
        {
            dx3d::Vec3d relPos = m_position - cameraOrigin;

            XMMATRIX S = XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);
            XMMATRIX R = XMMatrixRotationQuaternion(XMLoadFloat4(&m_quat));
            XMMATRIX T = XMMatrixTranslation(
                static_cast<float>(relPos.x),
                static_cast<float>(relPos.y),
                static_cast<float>(relPos.z)
            );

            XMMATRIX world = S * R * T;

            XMFLOAT4X4 result;
            XMStoreFloat4x4(&result, world);
            return result;
        }

    private:
        void updateQuaternionFromEuler()
        {
            XMVECTOR q = XMQuaternionRotationRollPitchYaw(m_euler.x, m_euler.y, m_euler.z);
            XMStoreFloat4(&m_quat, q);
        }

        void updateEulerFromQuat()
        {
            XMVECTOR q = XMLoadFloat4(&m_quat);

            XMFLOAT4 qf;
            XMStoreFloat4(&qf, q);

            float ysqr = qf.y * qf.y;

            float t0 = +2.0f * (qf.w * qf.x + qf.y * qf.z);
            float t1 = +1.0f - 2.0f * (qf.x * qf.x + ysqr);
            float roll = atan2f(t0, t1);

            float t2 = +2.0f * (qf.w * qf.y - qf.z * qf.x);
            t2 = std::clamp(t2, -1.0f, 1.0f);
            float pitch = asinf(t2);

            float t3 = +2.0f * (qf.w * qf.z + qf.x * qf.y);
            float t4 = +1.0f - 2.0f * (ysqr + qf.z * qf.z);
            float yaw = atan2f(t3, t4);

            m_euler = XMFLOAT3(pitch, yaw, roll);
        }

        void recalcMatrix() const
        {
            XMVECTOR sca = XMLoadFloat3(&m_scale);
            XMVECTOR rot = XMLoadFloat4(&m_quat);

            XMMATRIX S = XMMatrixScalingFromVector(sca);
            XMMATRIX R = XMMatrixRotationQuaternion(rot);
            XMMATRIX T = XMMatrixTranslation(
                static_cast<float>(m_position.x),
                static_cast<float>(m_position.y),
                static_cast<float>(m_position.z)
            );

            XMMATRIX world = S * R * T;

            XMStoreFloat4x4(&m_world, world);

            m_dirty = false;
        }

    private:
        mutable bool m_dirty = true;
        mutable XMFLOAT4X4 m_world;

        dx3d::Vec3d m_position{ 0.0, 0.0, 0.0 };
        XMFLOAT3 m_scale{ 1.0f, 1.0f, 1.0f };

        XMFLOAT4 m_quat{ 0.0f, 0.0f, 0.0f, 1.0f };
        XMFLOAT3 m_euler{ 0.0f, 0.0f, 0.0f };
    };
}