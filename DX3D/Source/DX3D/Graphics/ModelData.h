#pragma once
#include <DX3D/Math/Vertex.h>
#include <DX3D/Graphics/Material.h>
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
        Vec3 min;
        Vec3 max;
    };

    struct ModelData
    {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<std::string> materialNames;
        std::vector<Material> materials;
        std::string sourcePath;
        std::vector<MaterialGroup> materialGroups;

        AABB boundingBox{};
        bool hasNormals = false;
        bool hasTexcoords = false;
    };
}