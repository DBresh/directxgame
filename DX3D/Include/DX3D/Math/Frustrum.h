#pragma once
#include <DirectXMath.h>
#include <DX3D/Graphics/Resources/ModelData.h>

namespace dx3d
{
    struct Frustum
    {
        DirectX::XMVECTOR planes[6];

        void constructFromViewProj(const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& proj)
        {
            DirectX::XMMATRIX V = DirectX::XMLoadFloat4x4(&view);
            DirectX::XMMATRIX P = DirectX::XMLoadFloat4x4(&proj);
            DirectX::XMMATRIX VP = V * P;

            DirectX::XMFLOAT4X4 vp;
            DirectX::XMStoreFloat4x4(&vp, VP);

            planes[0] = DirectX::XMVectorSet(vp._14 + vp._11, vp._24 + vp._21, vp._34 + vp._31, vp._44 + vp._41); // Left
            planes[1] = DirectX::XMVectorSet(vp._14 - vp._11, vp._24 - vp._21, vp._34 - vp._31, vp._44 - vp._41); // Right
            planes[2] = DirectX::XMVectorSet(vp._14 + vp._12, vp._24 + vp._22, vp._34 + vp._32, vp._44 + vp._42); // Bottom
            planes[3] = DirectX::XMVectorSet(vp._14 - vp._12, vp._24 - vp._22, vp._34 - vp._32, vp._44 - vp._42); // Top
            planes[4] = DirectX::XMVectorSet(vp._13, vp._23, vp._33, vp._43);                                     // Near
            planes[5] = DirectX::XMVectorSet(vp._14 - vp._13, vp._24 - vp._23, vp._34 - vp._33, vp._44 - vp._43); // Far

            for (int i = 0; i < 6; ++i) {
                planes[i] = DirectX::XMPlaneNormalize(planes[i]);
            }
        }

        bool checkAABB(const AABB& box) const
        {
            for (int i = 0; i < 6; ++i) {
                DirectX::XMVECTOR p = DirectX::XMVectorSet(
                    DirectX::XMVectorGetX(planes[i]) > 0.0f ? box.max.x : box.min.x,
                    DirectX::XMVectorGetY(planes[i]) > 0.0f ? box.max.y : box.min.y,
                    DirectX::XMVectorGetZ(planes[i]) > 0.0f ? box.max.z : box.min.z,
                    1.0f
                );

                if (DirectX::XMVectorGetX(DirectX::XMPlaneDotCoord(planes[i], p)) < 0.0f) {
                    return false;
                }
            }
            return true;
        }
    };
}