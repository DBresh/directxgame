#pragma once
#include <DX3D/Core/Core.h>
#include <DX3D/Math/Vec3.h>
#include <DX3D/Math/Vec4.h>
#include <memory>
#include <cmath>

namespace dx3d
{
    class Matrix4x4
    {
    public:
        f32 mat[4][4];

        Matrix4x4()
        {
            setIdentity();
        }

        void setIdentity()
        {
            ::memset(mat, 0, sizeof(float) * 16);
            for (int i = 0; i < 4; ++i)
                mat[i][i] = 1.0f;
        }

        void setTranslate(const Vec3& t)
        {
            setIdentity();
            mat[3][0] = t.x;
            mat[3][1] = t.y;
            mat[3][2] = t.z;
        }

        void setScale(const Vec3& s)
        {
            setIdentity();
            mat[0][0] = s.x;
            mat[1][1] = s.y;
            mat[2][2] = s.z;
        }

        void setRotationX(float angle)
        {
            setIdentity();
            float cosA = cos(angle);
            float sinA = sin(angle);
            mat[1][1] = cosA;
            mat[1][2] = sinA;
            mat[2][1] = -sinA;
            mat[2][2] = cosA;
        }

        void setRotationY(float angle)
        {
            setIdentity();
            float cosA = cos(angle);
            float sinA = sin(angle);
            mat[0][0] = cosA;
            mat[0][2] = -sinA;
            mat[2][0] = sinA;
            mat[2][2] = cosA;
        }

        void setRotationZ(float angle)
        {
            setIdentity();
            float cosA = cos(angle);
            float sinA = sin(angle);
            mat[0][0] = cosA;
            mat[0][1] = sinA;
            mat[1][0] = -sinA;
            mat[1][1] = cosA;
        }

        Matrix4x4& operator*=(const Matrix4x4& rhs)
        {
            Matrix4x4 result;
            for (int i = 0; i < 4; ++i)
            {
                for (int j = 0; j < 4; ++j)
                {
                    result.mat[i][j] = 0.0f;
                    for (int k = 0; k < 4; ++k)
                    {
                        result.mat[i][j] += mat[i][k] * rhs.mat[k][j];
                    }
                }
            }
            *this = result;
            return *this;
        }

        Matrix4x4 operator*(const Matrix4x4& rhs) const
        {
            Matrix4x4 result;
            for (int i = 0; i < 4; ++i)
            {
                for (int j = 0; j < 4; ++j)
                {
                    result.mat[i][j] = 0.0f;
                    for (int k = 0; k < 4; ++k)
                    {
                        result.mat[i][j] += mat[i][k] * rhs.mat[k][j];
                    }
                }
            }
            return result;
        }

        void setOrthoLH(float width, float height, float znear, float zfar)
        {
            setIdentity();
            mat[0][0] = 2.0f / width;
            mat[1][1] = 2.0f / height;
            mat[2][2] = 1.0f / (zfar - znear);
            mat[3][2] = -znear / (zfar - znear);
        }

        void setPerspectiveFovLH(float fov, float aspect, float znear, float zfar)
        {
            setIdentity();
            float yScale = 1.0f / tan(fov / 2.0f);
            float xScale = yScale / aspect;

            mat[0][0] = xScale;
            mat[1][1] = yScale;
            mat[2][2] = zfar / (zfar - znear);
            mat[2][3] = 1.0f;
            mat[3][2] = (-znear * zfar) / (zfar - znear);
            mat[3][3] = 0.0f;
        }

        void setLookAtLH(const Vec3& eye, const Vec3& target, const Vec3& up)
        {
            Vec3 zAxis = (target - eye);
            zAxis = normalize(zAxis);

            Vec3 xAxis = cross(up, zAxis);
            xAxis = normalize(xAxis);

            Vec3 yAxis = cross(zAxis, xAxis);

            setIdentity();

            mat[0][0] = xAxis.x;
            mat[0][1] = yAxis.x;
            mat[0][2] = zAxis.x;

            mat[1][0] = xAxis.y;
            mat[1][1] = yAxis.y;
            mat[1][2] = zAxis.y;

            mat[2][0] = xAxis.z;
            mat[2][1] = yAxis.z;
            mat[2][2] = zAxis.z;

            mat[3][0] = -dot(xAxis, eye);
            mat[3][1] = -dot(yAxis, eye);
            mat[3][2] = -dot(zAxis, eye);
        }

        // Helper functions for vector operations
        static Vec3 normalize(const Vec3& v)
        {
            float length = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
            if (length > 0.0f)
                return Vec3(v.x / length, v.y / length, v.z / length);
            return v;
        }

        static Vec3 cross(const Vec3& a, const Vec3& b)
        {
            return Vec3(
                a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x
            );
        }

        static float dot(const Vec3& a, const Vec3& b)
        {
            return a.x * b.x + a.y * b.y + a.z * b.z;
        }

        Matrix4x4 transpose() const
        {
            Matrix4x4 result;
            for (int i = 0; i < 4; ++i)
            {
                for (int j = 0; j < 4; ++j)
                {
                    result.mat[i][j] = mat[j][i];
                }
            }
            return result;
        }
    };
}