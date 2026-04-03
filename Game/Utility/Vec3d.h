#pragma once
#include <cmath>
#include <DirectXMath.h>

namespace Simulator
{
    struct Vec3d
    {
        double x;
        double y;
        double z;

        constexpr Vec3d() : x(0.0), y(0.0), z(0.0) {}
        constexpr Vec3d(double x, double y, double z) : x(x), y(y), z(z) {}

        Vec3d operator+(const Vec3d& rhs) const { return Vec3d(x + rhs.x, y + rhs.y, z + rhs.z); }
        Vec3d operator-(const Vec3d& rhs) const { return Vec3d(x - rhs.x, y - rhs.y, z - rhs.z); }
        Vec3d operator*(double scalar) const { return Vec3d(x * scalar, y * scalar, z * scalar); }
        Vec3d operator/(double scalar) const { return Vec3d(x / scalar, y / scalar, z / scalar); }

        Vec3d operator-() const { return Vec3d(-x, -y, -z); }

        Vec3d& operator+=(const Vec3d& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; return *this; }
        Vec3d& operator-=(const Vec3d& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this; }
        Vec3d& operator*=(double scalar) { x *= scalar; y *= scalar; z *= scalar; return *this; }
        Vec3d& operator/=(double scalar) { x /= scalar; y /= scalar; z /= scalar; return *this; }

        [[nodiscard]] double magnitude() const { return std::sqrt(x * x + y * y + z * z); }
        [[nodiscard]] double sqrMagnitude() const { return x * x + y * y + z * z; }

        Vec3d& normalize()
        {
            double mag = magnitude();
            if (mag > 1e-45)
            {
                *this /= mag;
            }
            else
            {
                x = y = z = 0.0;
            }
            return *this;
        }

        [[nodiscard]] Vec3d normalized() const
        {
            Vec3d result = *this;
            return result.normalize();
        }

        static double Dot(const Vec3d& a, const Vec3d& b)
        {
            return a.x * b.x + a.y * b.y + a.z * b.z;
        }

        static Vec3d Cross(const Vec3d& a, const Vec3d& b)
        {
            return Vec3d(a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x);
        }

        static double Distance(const Vec3d& a, const Vec3d& b)
        {
            return (a - b).magnitude();
        }

        [[nodiscard]] DirectX::XMFLOAT3 toFloat3() const
        {
            return DirectX::XMFLOAT3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
        }
    };

    inline Vec3d operator*(double scalar, const Vec3d& vec)
    {
        return vec * scalar;
    }
}