#pragma once
#include <DX3D/Math/Vec3.h>
#include <DX3D/Math/Matrix4x4.h>
#include <cmath>

namespace dx3d {

    class Quaternion {
    public:
        float x, y, z, w;

        Quaternion() : x(0), y(0), z(0), w(1) {}
        Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

        static Quaternion identity() { return Quaternion(0, 0, 0, 1); }

        static Quaternion fromEuler(const Vec3& euler) {
            return fromEuler(euler.x, euler.y, euler.z);
        }

        static Quaternion fromEuler(float pitch, float yaw, float roll) {
            float halfPitch = pitch * 0.5f;
            float halfYaw = yaw * 0.5f;
            float halfRoll = roll * 0.5f;

            float sinPitch = std::sin(halfPitch);
            float cosPitch = std::cos(halfPitch);
            float sinYaw = std::sin(halfYaw);
            float cosYaw = std::cos(halfYaw);
            float sinRoll = std::sin(halfRoll);
            float cosRoll = std::cos(halfRoll);

            return Quaternion(
                cosRoll * sinPitch * cosYaw + sinRoll * cosPitch * sinYaw,
                cosRoll * cosPitch * sinYaw - sinRoll * sinPitch * cosYaw,
                sinRoll * cosPitch * cosYaw - cosRoll * sinPitch * sinYaw,
                cosRoll * cosPitch * cosYaw + sinRoll * sinPitch * sinYaw
            );
        }

        Vec3 toEuler() const {
            Vec3 euler;

            float sinr_cosp = 2.0f * (w * x + y * z);
            float cosr_cosp = 1.0f - 2.0f * (x * x + y * y);
            euler.z = std::atan2(sinr_cosp, cosr_cosp);

            float sinp = 2.0f * (w * y - z * x);
            if (std::abs(sinp) >= 1.0f) {
                euler.x = std::copysign(3.14159f / 2.0f, sinp); // Use 90 degrees if out of range
            }
            else {
                euler.x = std::asin(sinp);
            }

            // Yaw (z-axis rotation)
            float siny_cosp = 2.0f * (w * z + x * y);
            float cosy_cosp = 1.0f - 2.0f * (y * y + z * z);
            euler.y = std::atan2(siny_cosp, cosy_cosp);

            return euler;
        }

        // Convert to rotation matrix
        Matrix4x4 toMatrix() const {
            Matrix4x4 result;

            float xx = x * x;
            float yy = y * y;
            float zz = z * z;
            float xy = x * y;
            float xz = x * z;
            float yz = y * z;
            float wx = w * x;
            float wy = w * y;
            float wz = w * z;

            result.mat[0][0] = 1.0f - 2.0f * (yy + zz);
            result.mat[0][1] = 2.0f * (xy + wz);
            result.mat[0][2] = 2.0f * (xz - wy);
            result.mat[0][3] = 0.0f;

            result.mat[1][0] = 2.0f * (xy - wz);
            result.mat[1][1] = 1.0f - 2.0f * (xx + zz);
            result.mat[1][2] = 2.0f * (yz + wx);
            result.mat[1][3] = 0.0f;

            result.mat[2][0] = 2.0f * (xz + wy);
            result.mat[2][1] = 2.0f * (yz - wx);
            result.mat[2][2] = 1.0f - 2.0f * (xx + yy);
            result.mat[2][3] = 0.0f;

            result.mat[3][0] = 0.0f;
            result.mat[3][1] = 0.0f;
            result.mat[3][2] = 0.0f;
            result.mat[3][3] = 1.0f;

            return result;
        }

        Quaternion operator*(const Quaternion& other) const {
            return Quaternion(
                w * other.x + x * other.w + y * other.z - z * other.y,
                w * other.y - x * other.z + y * other.w + z * other.x,
                w * other.z + x * other.y - y * other.x + z * other.w,
                w * other.w - x * other.x - y * other.y - z * other.z
            );
        }

        Quaternion& operator*=(const Quaternion& other) {
            *this = *this * other;
            return *this;
        }

        Quaternion normalized() const {
            float len = length();
            if (len > 0.0f) {
                return Quaternion(x / len, y / len, z / len, w / len);
            }
            return Quaternion::identity();
        }

        float length() const {
            return std::sqrt(x * x + y * y + z * z + w * w);
        }

        float lengthSquared() const {
            return x * x + y * y + z * z + w * w;
        }

        // Linear interpolation (for simple rotations)
        static Quaternion lerp(const Quaternion& a, const Quaternion& b, float t) {
            t = (t < 0.0f) ? 0.0f : (t > 1.0f) ? 1.0f : t;
            return Quaternion(
                a.x + (b.x - a.x) * t,
                a.y + (b.y - a.y) * t,
                a.z + (b.z - a.z) * t,
                a.w + (b.w - a.w) * t
            ).normalized();
        }

        // Spherical linear interpolation (for smooth rotations)
        static Quaternion slerp(const Quaternion& a, const Quaternion& b, float t) {
            t = (t < 0.0f) ? 0.0f : (t > 1.0f) ? 1.0f : t;

            float cosHalfTheta = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;

            // If cosHalfTheta < 0, the interpolation will take the long way around
            if (cosHalfTheta < 0.0f) {
                return slerp(Quaternion(-a.x, -a.y, -a.z, -a.w), b, t);
            }

            // Perform slerp
            float halfTheta = std::acos(cosHalfTheta);
            float sinHalfTheta = std::sqrt(1.0f - cosHalfTheta * cosHalfTheta);

            if (std::abs(sinHalfTheta) < 0.001f) {
                // Linear interpolation for very small angles
                return lerp(a, b, t);
            }

            float ratioA = std::sin((1.0f - t) * halfTheta) / sinHalfTheta;
            float ratioB = std::sin(t * halfTheta) / sinHalfTheta;

            return Quaternion(
                a.x * ratioA + b.x * ratioB,
                a.y * ratioA + b.y * ratioB,
                a.z * ratioA + b.z * ratioB,
                a.w * ratioA + b.w * ratioB
            );
        }
    };

}