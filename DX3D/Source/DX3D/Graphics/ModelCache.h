#pragma once
#include <DX3D/Graphics/ModelData.h>
#include <DX3D/Graphics/ModelImporter.h>
#include <DX3D/Graphics/GraphicsDevice.h>
#include <DX3D/Core/Logger.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>
#include <filesystem>

namespace dx3d
{
    class ModelCache
    {
    public:
        static std::shared_ptr<ModelData> getOrLoad(
            const std::string& relativePath,
            const std::shared_ptr<GraphicsDevice>& device);

        static void clear();
        static void pruneExpired();
        static size_t size();

    private:
        static std::string makeKey(const std::string& relativePath);

    private:
        static std::unordered_map<std::string, std::weak_ptr<ModelData>> s_cache;
        static std::mutex s_mutex;
    };
}
