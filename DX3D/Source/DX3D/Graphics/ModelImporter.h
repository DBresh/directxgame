#pragma once
#include <DX3D/Core/Logger.h>
#include <DX3D/Graphics/ModelData.h>
#include <string>

namespace dx3d
{
    class ModelImporter
    {
    public:
        static ModelData loadOBJ(const std::string& relativePath);
    };
}
