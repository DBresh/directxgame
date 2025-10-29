#pragma once
#include <DX3D/Core/Base.h>
#include <DX3D/Core/Common.h>
#include <DX3D/Core/Core.h>
#include <DX3D/Core/Logger.h>
#include <DX3D/Graphics/GraphicsDevice.h>
#include <DX3D/Graphics/ModelData.h>
#include <DX3D/Graphics/ModelCache.h>
#include <DX3D/Graphics/Mesh.h>
#include <DX3D/Graphics/Material.h>

#include <filesystem>
#include <shared_mutex>
#include <unordered_map>
#include <string>
#include <string_view>
#include <memory>
#include <vector>

namespace dx3d
{
    struct AssetManagerDesc
    {
        BaseDesc base;
        std::shared_ptr<GraphicsDevice> graphicsDevice;
        std::filesystem::path assetsRoot;
    };

    class AssetManager final : public Base, public std::enable_shared_from_this<AssetManager>
    {
    public:
        explicit AssetManager(const AssetManagerDesc& desc);
        ~AssetManager() override;

        std::shared_ptr<struct ModelGPU> getModel(std::string_view relativePath);

        bool hasModel(std::string_view relativePath) const;
        void preloadModel(std::string_view relativePath);
        void evictModel(std::string_view relativePath);
        void clearGPU();

        void setAssetsRoot(const std::filesystem::path& root);
        const std::filesystem::path& getAssetsRoot() const { return m_assetsRoot; }

    private:
        std::string makeKey(std::string_view relativePath) const;

        std::shared_ptr<struct ModelGPU> buildGPUAndCache(
            const std::string& key,
            const std::shared_ptr<const ModelData>& cpuData);

    private:
        std::shared_ptr<GraphicsDevice> m_device;
        std::filesystem::path m_assetsRoot;

        mutable std::shared_mutex m_cacheMutex;
        std::unordered_map<std::string, std::weak_ptr<struct ModelGPU>> m_gpuCache;
    };
}
