#pragma once
#include <DX3D/Math/Vec3.h>
#include <DX3D/Math/Quaternion.h>
#include <DX3D/Math/Matrix4x4.h>

namespace dx3d {

    class Transform {
    public:
        Transform() = default;

        Vec3 getPosition() const { return m_position; }
        Vec3 getRotation() const { return m_rotation.toEuler(); }
        Vec3 getScale() const { return m_scale; }

        void setPosition(const Vec3& position) {
            m_position = position;
            m_dirty = true;
        }
        void setRotation(const Vec3& euler) {
            m_rotation = Quaternion::fromEuler(euler);
            m_dirty = true;
        }
        void setScale(const Vec3& scale) {
            m_scale = scale;
            m_dirty = true;
        }

        void translate(const Vec3& translation) {
            m_position = m_position + translation;
            m_dirty = true;
        }

        void rotate(const Vec3& euler) {
            m_rotation = m_rotation * Quaternion::fromEuler(euler);
            m_dirty = true;
        }

        Matrix4x4 getWorldMatrix() const {
            if (m_dirty) {
                recalculateMatrix();
                m_dirty = false;
            }
            return m_worldMatrix;
        }

    private:
        void recalculateMatrix() const {
            m_worldMatrix.setIdentity();
            Matrix4x4 scaleMat;
            scaleMat.setScale(m_scale);
            m_worldMatrix = m_worldMatrix * scaleMat;

            Matrix4x4 rotationMat = m_rotation.toMatrix();
            m_worldMatrix = m_worldMatrix * rotationMat;

            Matrix4x4 translationMat;
            translationMat.setTranslate(m_position);
            m_worldMatrix = m_worldMatrix * translationMat;
        }

    private:
        mutable bool m_dirty = true;
        mutable Matrix4x4 m_worldMatrix;
        Vec3 m_position{ 0, 0, 0 };
        Quaternion m_rotation;
        Vec3 m_scale{ 1, 1, 1 };
    };

}