#pragma once
#include <DX3D/Core/Core.h>
#include <DX3D/Math/Vec3.h>


namespace dx3d
{
	class Vec4
	{
	public:
		Vec4() = default;
		Vec4(f32 x, f32 y, f32 z, f32 w) : x(x), y(y), z(z), w(w) {}
		Vec4(Vec3 vec, f32 w) : x(vec.x), y(vec.y), z(vec.z), w(w) {}

	public:
		f32 x{}, y{}, z{}, w{};
	};
}