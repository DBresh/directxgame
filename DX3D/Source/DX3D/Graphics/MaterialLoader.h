#pragma once
#include <DX3D/Graphics/Material.h>
#include <DX3D/Graphics/AssetManager.h>
#include <string>
#include <vector>

namespace dx3d
{
    class MaterialLoader
    {
    public:
    public:
        static std::vector<Material> loadMTL(
            const std::string& objFilePath,
            const std::string& mtlFileName,
            AssetManager* assets = nullptr);
    };
}
