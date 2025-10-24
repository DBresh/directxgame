#pragma once
#include <DX3D/Graphics/Material.h>
#include <string>
#include <vector>

namespace dx3d
{
    class MaterialLoader
    {
    public:
        static std::vector<Material> loadMTL(const std::string& objFilePath, const std::string& mtlFileName);
    };
}
