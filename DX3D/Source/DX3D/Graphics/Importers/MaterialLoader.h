#pragma once
#include <DX3D/Graphics/Resources/Material.h>
#include <DX3D/Graphics/Importers/AssetManager.h>
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
