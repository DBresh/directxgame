#pragma once
#include <DX3D/Math/Vertex.h>
#include <DX3D/Graphics/Material.h>
#include <string>
#include <vector>

namespace dx3d
{
    struct ModelData
    {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<std::string> materialNames;
        std::vector<Material> materials;
        std::string sourcePath;
    };
}