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
