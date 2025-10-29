#pragma once
#include <vector>
#include <memory>
#include <DX3D/Graphics/Mesh.h>
#include <DX3D/Graphics/Material.h>
#include <DX3D/Graphics/ModelData.h>

namespace dx3d
{
    struct ModelGPU
    {
        std::shared_ptr<Mesh> mesh;
        std::vector<Material> materials;
        std::vector<MaterialGroup> materialGroups;
        AABB boundingBox;
    };
}