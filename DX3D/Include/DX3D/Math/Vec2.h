#pragma once
#include <DX3D/Core/Core.h>
#include <cmath>

namespace dx3d
{
	class Vec2
	{
	public:
		Vec2() = default;
		Vec2(float x, float y) : x(x), y(y) {}
		Vec2(const Vec2& vec) : x(vec.x), y(vec.y) {}

	public:
		float x{}, y{};

		static Vec2 lerp(const Vec2& start, const Vec2& end, float delta)
		{
			return Vec2(
				start.x + (end.x - start.x) * delta,
				start.y + (end.y - start.y) * delta
			);
		}

		Vec2 operator-(const Vec2& other) const
		{
			return Vec2(x - other.x, y - other.y);
		}

		Vec2 operator+(const Vec2& other) const
		{
			return Vec2(x + other.x, y + other.y);
		}

		Vec2 operator*(float scalar) const
		{
			return Vec2(x * scalar, y * scalar);
		}

		bool operator==(const Vec2& other) const {
			return x == other.x && y == other.y;
		}

		float length() const
		{
			return sqrt(x * x + y * y);
		}

		Vec2 normalize() const
		{
			float len = length();
			if (len > 0.0f)
				return Vec2(x / len, y / len);
			return *this;
		}
	};

	inline float dot(const Vec2& a, const Vec2& b)
	{
		return a.x * b.x + a.y * b.y;
	}

	inline Vec2 normalize(const Vec2& v)
	{
		float len = v.length();
		if (len > 0.0f)
			return Vec2(v.x / len, v.y / len);
		return v;
	}
}