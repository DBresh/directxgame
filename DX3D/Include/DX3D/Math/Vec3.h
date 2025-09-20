#pragma once
#include <DX3D/Core/Core.h>
#include <cmath>

namespace dx3d
{
    class Vec3
    {
    public:
        Vec3() = default;
        Vec3(f32 x, f32 y, f32 z) : x(x), y(y), z(z) {}
        Vec3(const Vec3& vec) : x(vec.x), y(vec.y), z(vec.z) {}

    public:
        f32 x{}, y{}, z{};

        static Vec3 lerp(const Vec3& start, const Vec3& end, float delta)
        {
            return Vec3(
                start.x + (end.x - start.x) * delta,
                start.y + (end.y - start.y) * delta,
                start.z + (end.z - start.z) * delta
            );
        }

        Vec3 operator-(const Vec3& other) const
        {
            return Vec3(x - other.x, y - other.y, z - other.z);
        }

        Vec3 operator+(const Vec3& other) const
        {
            return Vec3(x + other.x, y + other.y, z + other.z);
        }

        Vec3 operator*(float scalar) const
        {
            return Vec3(x * scalar, y * scalar, z * scalar);
        }

        float length() const
        {
            return sqrt(x * x + y * y + z * z);
        }

        Vec3 normalize() const
        {
            float len = length();
            if (len > 0.0f)
                return Vec3(x / len, y / len, z / len);
            return *this;
        }
    };

    inline Vec3 cross(const Vec3& a, const Vec3& b)
    {
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
}