#pragma once
#include <vector>
#include <memory>
#include <DX3D/Graphics/Mesh.h>
#include <DX3D/Graphics/Material.h>
#include <DX3D/Graphics/ModelData.h>

namespace dx3d
{
    struct SubMesh
    {
        std::shared_ptr<Mesh> mesh;   // окремий GPU-меш
        int materialIndex = -1;       // індекс матеріалу з ModelData/MaterialLoader
        unsigned startIndex = 0;
        unsigned indexCount = 0;
    };

    struct ModelGPU
    {
        std::shared_ptr<Mesh> mesh;

        std::vector<SubMesh> submeshes;

        std::vector<Material> materials;
        std::vector<MaterialGroup> materialGroups;
        AABB boundingBox;
    };
}
