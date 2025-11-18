#pragma once
#include <DirectXMath.h>

namespace dx3d
{
    using namespace DirectX;

    struct Vertex
    {
        XMFLOAT3 position;
        XMFLOAT3 normal;
        XMFLOAT2 texcoord;
        XMFLOAT4 color;

        Vertex() :
            position(0.f, 0.f, 0.f),
            normal(0.f, 1.f, 0.f),
            texcoord(0.f, 0.f),
            color(1.f, 1.f, 1.f, 1.f)
        {
        }

        Vertex(const XMFLOAT3& pos, const XMFLOAT4& col) :
            position(pos),
            normal(0.f, 1.f, 0.f),
            texcoord(0.f, 0.f),
            color(col)
        {
        }

        Vertex(const XMFLOAT3& pos, const XMFLOAT3& norm, const XMFLOAT2& uv,
            const XMFLOAT4& col = XMFLOAT4(1.f, 1.f, 1.f, 1.f)) :
            position(pos), normal(norm), texcoord(uv), color(col)
        {
        }
    };
}
