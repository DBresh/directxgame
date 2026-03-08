#include <DX3D/Graphics/Importers/MaterialLoader.h>
#include <DX3D/Graphics/Resources/Texture2D.h>
#include <DX3D/Graphics/Core/GraphicsDevice.h>
#include <DX3D/Graphics/Importers/AssetManager.h>
#include <DX3D/Core/Logger.h>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace dx3d
{
    std::vector<Material> MaterialLoader::loadMTL(
        const std::string& objFilePath,
        const std::string& mtlFileName,
        AssetManager* assets)
    {
        std::vector<Material> materials;
        std::filesystem::path objPath(objFilePath);
        std::filesystem::path mtlPath = objPath.parent_path() / mtlFileName;

        std::ifstream file(mtlPath);
        if (!file.is_open())
        {
            DX3D_LOG_WARNING("Material file not found: {}", mtlPath.string());
            return materials;
        }

        bool fromCache = false;
        if (assets && assets->hasMaterial(mtlPath.string()))
        {
            //DX3D_LOG_INFO("Using cached materials from '{}'", mtlPath.string());
            fromCache = true;
        }

        Material currentMat;
        std::string line;

        while (std::getline(file, line))
        {
            if (line.empty() || line[0] == '#')
                continue;

            std::istringstream iss(line);
            std::string prefix;
            iss >> prefix;

            if (prefix == "newmtl")
            {
                if (!currentMat.name.empty())
                    materials.push_back(currentMat);

                std::string name;
                iss >> name;

                currentMat = Material();
                currentMat.name = name;
            }
            else if (prefix == "Kd")
            {
                iss >> currentMat.diffuseColor.x >> currentMat.diffuseColor.y >> currentMat.diffuseColor.z;
            }
            else if (prefix == "Ka")
            {
                iss >> currentMat.ambientColor.x >> currentMat.ambientColor.y >> currentMat.ambientColor.z;
            }
            else if (prefix == "Ks")
            {
                iss >> currentMat.specularColor.x >> currentMat.specularColor.y >> currentMat.specularColor.z;
            }
            else if (prefix == "Ns")
            {
                iss >> currentMat.shininess;
            }
            else if (prefix == "d")
            {
                iss >> currentMat.opacity;
            }
            else if (prefix == "map_Kd")
            {
                iss >> currentMat.diffuseTexturePath;

                if (assets && !currentMat.diffuseTexturePath.empty())
                {
                    auto texPath = mtlPath.parent_path() / currentMat.diffuseTexturePath;
                    if (assets->hasTexture(texPath.string()))
                    {
                        DX3D_LOG_INFO("  Using cached texture '{}'", texPath.string());
                    }
                    currentMat.diffuseTexture = assets->getTexture(texPath.string());
                }
            }
        }

        if (!currentMat.name.empty())
            materials.push_back(currentMat);

        if (!fromCache)
        {
            DX3D_LOG_INFO("Loaded {} materials from {}", materials.size(), mtlPath.string());
            for (const auto& mat : materials)
            {
                DX3D_LOG_INFO("  Material [{}]: Kd({:.2f}, {:.2f}, {:.2f})",
                    mat.name, mat.diffuseColor.x, mat.diffuseColor.y, mat.diffuseColor.z);
            }
        }
        return materials;
    }
}