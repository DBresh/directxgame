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
        float mat[4][4];

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

        Vec3 getXDirection()
        {
            return Vec3(mat[0][0], mat[0][1], mat[0][2]);
        }

        Vec3 getYDirection()
        {
            return Vec3(mat[1][0], mat[1][1], mat[1][2]);
        }

        Vec3 getZDirection()
        {
            return Vec3(mat[2][0], mat[2][1], mat[2][2]);
        }

        Vec3 getTranslation()
        {
            return Vec3(mat[3][0], mat[3][1], mat[3][2]);
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
            Vec3 zAxis = normalize(target - eye);        // вперед
            Vec3 xAxis = normalize(cross(up, zAxis));    // вправо
            Vec3 yAxis = cross(zAxis, xAxis);            // вгору

            setIdentity();

            mat[0][0] = xAxis.x; mat[1][0] = xAxis.y; mat[2][0] = xAxis.z; mat[3][0] = -dot(xAxis, eye);
            mat[0][1] = yAxis.x; mat[1][1] = yAxis.y; mat[2][1] = yAxis.z; mat[3][1] = -dot(yAxis, eye);
            mat[0][2] = zAxis.x; mat[1][2] = zAxis.y; mat[2][2] = zAxis.z; mat[3][2] = -dot(zAxis, eye);
            mat[0][3] = 0;        mat[1][3] = 0;        mat[2][3] = 0;        mat[3][3] = 1;
        }


        Vec3 transformPoint(const Vec3& point) const {
            float x = point.x * mat[0][0] + point.y * mat[1][0] + point.z * mat[2][0] + mat[3][0];
            float y = point.x * mat[0][1] + point.y * mat[1][1] + point.z * mat[2][1] + mat[3][1];
            float z = point.x * mat[0][2] + point.y * mat[1][2] + point.z * mat[2][2] + mat[3][2];
            float w = point.x * mat[0][3] + point.y * mat[1][3] + point.z * mat[2][3] + mat[3][3];

            if (w != 0.0f && w != 1.0f) {
                x /= w;
                y /= w;
                z /= w;
            }

            return Vec3(x, y, z);
        }

        Vec3 transformVector(const Vec3& vector) const {
            float x = vector.x * mat[0][0] + vector.y * mat[1][0] + vector.z * mat[2][0];
            float y = vector.x * mat[0][1] + vector.y * mat[1][1] + vector.z * mat[2][1];
            float z = vector.x * mat[0][2] + vector.y * mat[1][2] + vector.z * mat[2][2];
            return Vec3(x, y, z);
        }

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

        void inverse() 
        {
            int a, i, j;
            Matrix4x4 out;
            Vec4 v, vec[3];
            float det = 0.0f;

            det = this->getDeterminant();
            if (!det) return;
            for (i = 0; i < 4; i++)
            {
                for (j = 0; j < 4; j++)
                {
                    if (j != i)
                    {
                        a = j;
                        if (j > i) a = a - 1;
                        vec[a].x = this->mat[j][0];
                        vec[a].y = this->mat[j][1];
                        vec[a].z = this->mat[j][2];
                        vec[a].w = this->mat[j][3];
                    }
                }
                v.cross(vec[0], vec[1], vec[2]);

                out.mat[0][i] = static_cast<float>(pow(-1.0f, i)) * v.x / det;
                out.mat[1][i] = static_cast<float>(pow(-1.0f, i)) * v.y / det;
                out.mat[2][i] = static_cast<float>(pow(-1.0f, i)) * v.z / det;
                out.mat[3][i] = static_cast<float>(pow(-1.0f, i)) * v.w / det;
            }

            setMatrix(out);
        }

        void setMatrix(Matrix4x4 matrix)
        {
            ::memcpy(mat, matrix.mat, sizeof(float) * 16);
        }

        float getDeterminant()
        {
            Vec4 minor, v1, v2, v3;
            float det;

            v1 = Vec4(mat[0][0], mat[1][0], mat[2][0], mat[3][0]);
            v2 = Vec4(mat[0][1], mat[1][1], mat[2][1], mat[3][1]);
            v3 = Vec4(mat[0][2], mat[1][2], mat[2][2], mat[3][2]);

            minor.cross(v1, v2, v3);
            det = -(mat[0][3] * minor.x + mat[1][3] * minor.y + mat[2][3] * minor.z + mat[3][3] * minor.w);
            return det;
        }
    };
}