#pragma once
#include <DX3D/Math/Vec2.h>
#include <DX3D/Math/Vec3.h>
#include <DX3D/Math/Vec4.h>

namespace dx3d
{

    struct Vertex
    {
        Vec3 position;
        Vec3 normal;
        Vec2 texcoord;
        Vec4 color;

        Vertex() = default;

        Vertex(const Vec3& pos, const Vec4& col) :
            position(pos), color(col)
        {
            normal = Vec3(0, 1, 0);
            texcoord = Vec2(0, 0);
        }

        Vertex(const Vec3& pos, const Vec3& norm, const Vec2& uv, const Vec4& col = Vec4(1, 1, 1, 1)) :
            position(pos), normal(norm), texcoord(uv), color(col)
        {
        }
    };

}