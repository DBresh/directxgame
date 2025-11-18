#pragma once
#include <DirectXMath.h>
#include <algorithm>
#include <math.h>

namespace dx3d
{
    using namespace DirectX;

    class Transform
    {
    public:
        Transform() = default;

        // ================= GETTERS =================
        XMFLOAT3 getPosition()    const noexcept { return m_position; }
        XMFLOAT3 getScale()       const noexcept { return m_scale; }
        XMFLOAT4 getQuaternion()  const noexcept { return m_quat; }

        // Euler only for debugging / editor
        XMFLOAT3 getEuler() const noexcept { return m_euler; }

        const XMFLOAT4X4& getWorldMatrix() const
        {
            if (m_dirty)
                recalcMatrix();
            return m_world;
        }

        // ================= SETTERS =================
        void setPosition(const XMFLOAT3& p)
        {
            m_position = p;
            m_dirty = true;
        }

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

        // ================= MODIFY =================
        void translate(const XMFLOAT3& d)
        {
            m_position.x += d.x;
            m_position.y += d.y;
            m_position.z += d.z;
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

    private:
        // ===================================================
        // INTERNAL FUNCTIONS
        // ===================================================
        void updateQuaternionFromEuler()
        {
            XMVECTOR q =
                XMQuaternionRotationRollPitchYaw(m_euler.x, m_euler.y, m_euler.z);
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
            float roll = atan2(t0, t1);

            float t2 = +2.0f * (qf.w * qf.y - qf.z * qf.x);
            t2 = std::clamp(t2, -1.0f, 1.0f);
            float pitch = asin(t2);

            float t3 = +2.0f * (qf.w * qf.z + qf.x * qf.y);
            float t4 = +1.0f - 2.0f * (ysqr + qf.z * qf.z);
            float yaw = atan2(t3, t4);

            m_euler = XMFLOAT3(pitch, yaw, roll);
        }


        void recalcMatrix() const
        {
            XMVECTOR pos = XMLoadFloat3(&m_position);
            XMVECTOR sca = XMLoadFloat3(&m_scale);
            XMVECTOR rot = XMLoadFloat4(&m_quat);

            XMMATRIX S = XMMatrixScalingFromVector(sca);
            XMMATRIX R = XMMatrixRotationQuaternion(rot);
            XMMATRIX T = XMMatrixTranslationFromVector(pos);

            XMMATRIX world = S * R * T;

            XMStoreFloat4x4(&m_world, world);

            m_dirty = false;
        }


    private:
        mutable bool m_dirty = true;
        mutable XMFLOAT4X4 m_world;

        XMFLOAT3 m_position{ 0,0,0 };
        XMFLOAT3 m_scale{ 1,1,1 };

        XMFLOAT4 m_quat{ 0,0,0,1 }; // rotation base
        XMFLOAT3 m_euler{ 0,0,0 };  // for UI
    };
}
