#pragma once
#include <DX3D/Math/Vertex.h>
#include <DX3D/Graphics/Resources/Material.h>

#include <DirectXMath.h>
#include <string>
#include <vector>

namespace dx3d
{
    struct MaterialGroup
    {
        std::string name;
        unsigned int startIndex = 0;
        unsigned int indexCount = 0;
        int materialIndex = -1;
    };

    struct AABB
    {
        DirectX::XMFLOAT3 min = DirectX::XMFLOAT3(FLT_MAX, FLT_MAX, FLT_MAX);
        DirectX::XMFLOAT3 max = DirectX::XMFLOAT3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

        AABB transform(const DirectX::XMFLOAT4X4& m) const
        {
            DirectX::XMFLOAT3 corners[8] = {
                {min.x, min.y, min.z}, {min.x, min.y, max.z},
                {min.x, max.y, min.z}, {min.x, max.y, max.z},
                {max.x, min.y, min.z}, {max.x, min.y, max.z},
                {max.x, max.y, min.z}, {max.x, max.y, max.z}
            };

            DirectX::XMMATRIX mat = DirectX::XMLoadFloat4x4(&m);
            AABB result;

            for (int i = 0; i < 8; ++i) {
                DirectX::XMVECTOR v = DirectX::XMLoadFloat3(&corners[i]);
                v = DirectX::XMVector3TransformCoord(v, mat);
                DirectX::XMFLOAT3 t;
                DirectX::XMStoreFloat3(&t, v);

                result.min.x = std::min(result.min.x, t.x);
                result.min.y = std::min(result.min.y, t.y);
                result.min.z = std::min(result.min.z, t.z);
                result.max.x = std::max(result.max.x, t.x);
                result.max.y = std::max(result.max.y, t.y);
                result.max.z = std::max(result.max.z, t.z);
            }
            return result;
        }

        bool intersectRay(const DirectX::XMVECTOR& rayOrigin, const DirectX::XMVECTOR& rayDir, float& tMinOut) const
        {
            using namespace DirectX;
            XMVECTOR boxMin = XMLoadFloat3(&min);
            XMVECTOR boxMax = XMLoadFloat3(&max);

            XMVECTOR invDir = XMVectorReciprocal(rayDir);

            XMVECTOR t1 = XMVectorMultiply(XMVectorSubtract(boxMin, rayOrigin), invDir);
            XMVECTOR t2 = XMVectorMultiply(XMVectorSubtract(boxMax, rayOrigin), invDir);

            XMVECTOR vMin = XMVectorMin(t1, t2);
            XMVECTOR vMax = XMVectorMax(t1, t2);

            float tmin = std::max({ XMVectorGetX(vMin), XMVectorGetY(vMin), XMVectorGetZ(vMin) });
            float tmax = std::min({ XMVectorGetX(vMax), XMVectorGetY(vMax), XMVectorGetZ(vMax) });

            if (tmax >= tmin && tmax > 0.0f) {
                tMinOut = tmin;
                return true;
            }
            return false;
        }
    };

    struct ModelData
    {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        std::vector<std::string> materialNames;
        std::vector<Material> materials;
        std::vector<MaterialGroup> materialGroups;

        std::string sourcePath;

        AABB boundingBox;

        bool hasNormals = false;
        bool hasTexcoords = false;
    };
}
