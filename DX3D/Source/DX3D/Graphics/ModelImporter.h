#pragma once
#include <DX3D/Graphics/GraphicsResource.h>
#include <DX3D/Graphics/ModelData.h>

namespace dx3d
{
    class ModelImporter
    {
    public:
        explicit ModelImporter(std::shared_ptr<GraphicsDevice> device)
            : m_graphicsDevice(std::move(device)) {
        }

        ModelData loadOBJ(const std::string& relativePath);

    private:
        std::shared_ptr<GraphicsDevice> m_graphicsDevice;
    };
}
