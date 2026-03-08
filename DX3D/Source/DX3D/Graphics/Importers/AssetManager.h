#pragma once
#include <DX3D/Core/Base.h>
#include <DX3D/Core/Common.h>
#include <DX3D/Graphics/Core/GraphicsDevice.h>
#include <DX3D/Graphics/Resources/ModelData.h>
#include <DX3D/Graphics/Resources/ModelGPU.h>
#include <DX3D/Graphics/Resources/Material.h>
#include <DX3D/Graphics/Resources/Texture2D.h>

#include <filesystem>
#include <shared_mutex>
#include <unordered_map>
#include <string>
#include <memory>

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

        std::shared_ptr<ModelGPU> getModel(std::string_view relativePath);
        std::shared_ptr<Material> getMaterial(std::string_view mtlPath);
        std::shared_ptr<Texture2D> getTexture(std::string_view texPath);

        bool hasModel(std::string_view relativePath) const;
        bool hasMaterial(std::string_view mtlPath) const;
        bool hasTexture(std::string_view texPath) const;

        void preloadModel(std::string_view relativePath);
        void evictModel(std::string_view relativePath);
        void clearGPU();
        void clearAll();

        void setAssetsRoot(const std::filesystem::path& root);
        const std::filesystem::path& getAssetsRoot() const { return m_assetsRoot; }

        std::vector<Material> getMaterialsFromFile(const std::string& fullPath, const std::string& mtlPath);
    private:
        std::string makeKey(std::string_view relativePath) const;

        std::shared_ptr<ModelGPU> buildGPUAndCache(
            const std::string& key,
            const std::shared_ptr<const ModelData>& cpuData);

    private:
        std::shared_ptr<GraphicsDevice> m_device;
        std::filesystem::path m_assetsRoot;

        mutable std::shared_mutex m_cacheMutex;

        std::unordered_map<std::string, std::weak_ptr<ModelGPU>>    m_modelCache;
        std::unordered_map<std::string, std::weak_ptr<Material>>    m_materialCache;
        std::unordered_map<std::string, std::weak_ptr<Texture2D>>   m_textureCache;
    };
}
