#pragma once
#include <DX3D/Core/Core.h>
#include <cmath>

namespace dx3d
{
    class Vec3
    {
    public:
        Vec3() = default;
        Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
        Vec3(const Vec3& vec) : x(vec.x), y(vec.y), z(vec.z) {}

    public:
        float x{}, y{}, z{};

        Vec3 operator-(const Vec3& other) const
        {
            return Vec3(x - other.x, y - other.y, z - other.z);
        }

        Vec3 operator+(const Vec3& other) const
        {
            return Vec3(x + other.x, y + other.y, z + other.z);
        }

        Vec3 operator*(const Vec3& other) const {
            return Vec3(x * other.x, y * other.y, z * other.z);
        }

        Vec3 operator/(const Vec3& other) const {
            return Vec3(x / other.x, y / other.y, z / other.z);
        }

        Vec3 operator*(float scalar) const
        {
            return Vec3(x * scalar, y * scalar, z * scalar);
        }

        Vec3 operator/(float scalar) const {
            float invScalar = 1.0f / scalar;
            return Vec3(x * invScalar, y * invScalar, z * invScalar);
        }

        Vec3& operator+=(const Vec3& other) {
            x += other.x;
            y += other.y;
            z += other.z;
            return *this;
        }

        Vec3& operator-=(const Vec3& other) {
            x -= other.x;
            y -= other.y;
            z -= other.z;
            return *this;
        }

        Vec3& operator*=(const Vec3& other) {
            x *= other.x;
            y *= other.y;
            z *= other.z;
            return *this;
        }

        Vec3& operator/=(const Vec3& other) {
            x /= other.x;
            y /= other.y;
            z /= other.z;
            return *this;
        }

        Vec3& operator*=(float scalar) {
            x *= scalar;
            y *= scalar;
            z *= scalar;
            return *this;
        }

        Vec3& operator/=(float scalar) {
            float invScalar = 1.0f / scalar;
            x *= invScalar;
            y *= invScalar;
            z *= invScalar;
            return *this;
        }

        Vec3 operator-() const {
            return Vec3(-x, -y, -z);
        }

        bool operator==(const Vec3& other) const {
            return x == other.x && y == other.y && z == other.z;
        }

        bool operator!=(const Vec3& other) const {
            return !(*this == other);
        }

        float length() const
        {
            return sqrt(x * x + y * y + z * z);
        }

        float lengthSquared() const {
            return x * x + y * y + z * z;
        }

        Vec3 normalize() const {
            float len = length();
            if (len > 0.0f) {
                return *this / len;
            }
            return *this;
        }

        static float dot(const Vec3& a, const Vec3& b) {
            return a.x * b.x + a.y * b.y + a.z * b.z;
        }

        static Vec3 cross(const Vec3& a, const Vec3& b) {
            return Vec3(
                a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x
            );
        }

        static Vec3 lerp(const Vec3& a, const Vec3& b, float t) {
            return a + (b - a) * t;
        }
    };

    inline static Vec3 cross(const Vec3& a, const Vec3& b) {
        return Vec3(
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        );
    }

    inline float dot(const Vec3& a, const Vec3& b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    inline Vec3 normalize(const Vec3& v)
    {
        float len = v.length();
        if (len > 0.0f)
            return Vec3(v.x / len, v.y / len, v.z / len);
        return v;
    }

    inline Vec3 operator*(float scalar, const Vec3& vec) {
        return vec * scalar;
    }
}