#pragma once
#include <DX3D/Core/Core.h>
#include <DX3D/Math/Vec3.h>


namespace dx3d
{
	class Vec4
	{
	public:
		Vec4() = default;
		Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
		Vec4(Vec3 vec, float w) : x(vec.x), y(vec.y), z(vec.z), w(w) {}


		void cross(const Vec4& v1, const Vec4& v2, const Vec4& v3)
		{
			x = v1.y * (v2.z * v3.w - v2.w * v3.z) -
				v1.z * (v2.y * v3.w - v2.w * v3.y) +
				v1.w * (v2.y * v3.z - v2.z * v3.y);

			y = -(v1.x * (v2.z * v3.w - v2.w * v3.z) -
				v1.z * (v2.x * v3.w - v2.w * v3.x) +
				v1.w * (v2.x * v3.z - v2.z * v3.x));

			z = v1.x * (v2.y * v3.w - v2.w * v3.y) -
				v1.y * (v2.x * v3.w - v2.w * v3.x) +
				v1.w * (v2.x * v3.y - v2.y * v3.x);

			w = -(v1.x * (v2.y * v3.z - v2.z * v3.y) -
				v1.y * (v2.x * v3.z - v2.z * v3.x) +
				v1.z * (v2.x * v3.y - v2.y * v3.x));
		}

	public:
		float x{}, y{}, z{}, w{};
	};
}