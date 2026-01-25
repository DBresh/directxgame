#pragma once
#include <DX3D/Graphics/GraphicsResource.h>
#include <DX3D/Graphics/ModelData.h>
#include <DX3D/Graphics/AssetManager.h>

namespace dx3d
{
    class ModelImporter
    {
    public:
        explicit ModelImporter(std::shared_ptr<GraphicsDevice> device,
            AssetManager* assets = nullptr)
            : m_graphicsDevice(std::move(device)), m_assets(assets) {
        }

        ModelData loadOBJ(const std::string& relativePath);

        bool saveBinary(const std::string& path, const ModelData& data);
        ModelData loadBinary(const std::string& path);

    private:
        std::shared_ptr<GraphicsDevice> m_graphicsDevice;
        AssetManager* m_assets = nullptr;
    };
}
