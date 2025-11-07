#include <DX3D/Graphics/AssetManager.h>
#include <algorithm>
#include <cctype>
#include <system_error>

namespace dx3d
{
    static std::string toLowerAscii(std::string s)
    {
        for (char& c : s)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return s;
    }

    AssetManager::AssetManager(const AssetManagerDesc& desc)
        : Base(desc.base)
        , m_device(desc.graphicsDevice)
        , m_assetsRoot(desc.assetsRoot)
    {
        if (!m_device)
            DX3D_LOG_THROW_ERROR("AssetManager: GraphicsDevice is null.");

        if (!m_assetsRoot.empty())
            m_assetsRoot = std::filesystem::weakly_canonical(m_assetsRoot);

        DX3D_LOG_INFO("AssetManager created. Root='{}'", m_assetsRoot.string());
    }

    AssetManager::~AssetManager()
    {
        clearGPU();
        DX3D_LOG_DEBUG("AssetManager destroyed.");
    }

    std::shared_ptr<ModelGPU> AssetManager::getModel(std::string_view relativePath)
    {
        const std::string key = makeKey(relativePath);

        {
            std::shared_lock rlock(m_cacheMutex);
            if (auto it = m_gpuCache.find(key); it != m_gpuCache.end())
                if (auto sp = it->second.lock())
                    return sp;
        }

        std::shared_ptr<const ModelData> cpuData;
        try
        {
            cpuData = ModelCache::getOrLoad(key, m_device);
            if (!cpuData)
                DX3D_LOG_THROW_ERROR("AssetManager: ModelCache returned null for '{}'", key);
        }
        catch (const std::exception& e)
        {
            DX3D_LOG_ERROR("AssetManager: CPU load failed for '{}': {}", key, e.what());
            throw;
        }

        return buildGPUAndCache(key, cpuData);
    }

    bool AssetManager::hasModel(std::string_view relativePath) const
    {
        const std::string key = makeKey(relativePath);
        std::shared_lock rlock(m_cacheMutex);
        auto it = m_gpuCache.find(key);
        return it != m_gpuCache.end() && !it->second.expired();
    }

    void AssetManager::preloadModel(std::string_view relativePath)
    {
        (void)getModel(relativePath);
    }

    void AssetManager::evictModel(std::string_view relativePath)
    {
        const std::string key = makeKey(relativePath);
        std::unique_lock wlock(m_cacheMutex);
        m_gpuCache.erase(key);
        DX3D_LOG_DEBUG("AssetManager: evicted GPU entry '{}'", key);
    }

    void AssetManager::clearGPU()
    {
        std::unique_lock wlock(m_cacheMutex);
        m_gpuCache.clear();
        DX3D_LOG_INFO("AssetManager: cleared all GPU entries");
    }

    void AssetManager::setAssetsRoot(const std::filesystem::path& root)
    {
        std::unique_lock wlock(m_cacheMutex);
        m_assetsRoot = std::filesystem::weakly_canonical(root);
        DX3D_LOG_INFO("AssetManager: assets root set to '{}'", m_assetsRoot.string());
    }

    std::string AssetManager::makeKey(std::string_view relativePath) const
    {
        std::filesystem::path p(relativePath);
        if (!m_assetsRoot.empty())
            p = m_assetsRoot / p;

        std::error_code ec;
        std::filesystem::path canon = std::filesystem::weakly_canonical(p, ec);
        if (ec) canon = p.lexically_normal();

        std::string s = canon.generic_string();
        return toLowerAscii(std::move(s));
    }

    std::shared_ptr<ModelGPU> AssetManager::buildGPUAndCache(
        const std::string& key,
        const std::shared_ptr<const ModelData>& cpuData)
    {
        {
            std::shared_lock rlock(m_cacheMutex);
            if (auto it = m_gpuCache.find(key); it != m_gpuCache.end())
                if (auto sp = it->second.lock())
                    return sp;
        }

        std::shared_ptr<ModelGPU> gpuModel = std::make_shared<ModelGPU>();

        try
        {
            auto wholeMesh = m_device->createMesh(cpuData->vertices, cpuData->indices);
            if (!wholeMesh)
                DX3D_LOG_THROW_ERROR("AssetManager: createMesh (whole) returned null for '{}'", key);

            gpuModel->mesh = std::move(wholeMesh);
            gpuModel->materials = cpuData->materials;
            gpuModel->boundingBox = cpuData->boundingBox;
            gpuModel->materialGroups = cpuData->materialGroups;

            gpuModel->submeshes.clear();
            gpuModel->submeshes.reserve(cpuData->materialGroups.size());

            for (const auto& group : cpuData->materialGroups)
            {
                if (group.indexCount == 0) continue;

                const unsigned start = group.startIndex;
                const unsigned end = group.startIndex + group.indexCount;

                std::unordered_map<uint32_t, uint32_t> remap;
                remap.reserve(group.indexCount);

                std::vector<Vertex> subVertices;
                subVertices.reserve(group.indexCount);

                std::vector<unsigned> subIndices;
                subIndices.reserve(group.indexCount);

                for (unsigned i = start; i < end; ++i)
                {
                    uint32_t oldIndex = cpuData->indices[i];

                    auto it = remap.find(oldIndex);
                    if (it == remap.end())
                    {
                        uint32_t newIndex = static_cast<uint32_t>(subVertices.size());
                        remap.emplace(oldIndex, newIndex);
                        subVertices.push_back(cpuData->vertices[oldIndex]);
                        subIndices.push_back(newIndex);
                    }
                    else
                    {
                        subIndices.push_back(it->second);
                    }
                }

                auto subMesh = m_device->createMesh(subVertices, subIndices);
                if (!subMesh)
                    DX3D_LOG_THROW_ERROR("AssetManager: createMesh (sub) returned null for '{}' group '{}'",
                        key, group.name);

                SubMesh sm{};
                sm.mesh = std::move(subMesh);
                sm.materialIndex = group.materialIndex;
                sm.startIndex = group.startIndex;
                sm.indexCount = group.indexCount;

                gpuModel->submeshes.push_back(std::move(sm));

                DX3D_LOG_DEBUG("AssetManager: built submesh '{}' (verts: {}, idx: {}, mat: {})",
                    group.name, subVertices.size(), subIndices.size(), group.materialIndex);
            }
        }
        catch (const std::exception& e)
        {
            DX3D_LOG_ERROR("AssetManager: GPU build failed for '{}': {}", key, e.what());
            throw;
        }

        {
            std::unique_lock wlock(m_cacheMutex);
            m_gpuCache[key] = gpuModel;
        }

        DX3D_LOG_DEBUG("AssetManager: cached GPU model '{}' (whole+{} submeshes)",
            key, gpuModel->submeshes.size());

        return gpuModel;
    }
}
