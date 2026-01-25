#include <DX3D/Graphics/AssetManager.h>
#include <DX3D/Graphics/ModelCache.h>
#include <DX3D/Graphics/ModelImporter.h>
#include <DX3D/Graphics/MaterialLoader.h>
#include <DX3D/Core/Logger.h>
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

        DX3D_LOG_INFO("AssetManager initialized. Root='{}'", m_assetsRoot.string());
    }

    AssetManager::~AssetManager()
    {
        clearAll();
        DX3D_LOG_DEBUG("AssetManager destroyed.");
    }

    std::shared_ptr<Texture2D> AssetManager::getTexture(std::string_view texPath)
    {
        const std::string key = makeKey(texPath);
        std::shared_lock rlock(m_cacheMutex);
        if (auto it = m_textureCache.find(key); it != m_textureCache.end())
            if (auto sp = it->second.lock()) return sp;
        rlock.unlock();

        auto tex = m_device->createTexture2D(key);
        if (!tex)
            DX3D_LOG_THROW_ERROR("AssetManager: Failed to load texture '{}'", key);

        std::unique_lock wlock(m_cacheMutex);
        m_textureCache[key] = tex;
        return tex;
    }

    bool AssetManager::hasTexture(std::string_view texPath) const
    {
        const std::string key = makeKey(texPath);
        std::shared_lock rlock(m_cacheMutex);
        auto it = m_textureCache.find(key);
        return it != m_textureCache.end() && !it->second.expired();
    }

    std::shared_ptr<Material> AssetManager::getMaterial(std::string_view mtlPath)
    {
        const std::string key = makeKey(mtlPath);
        std::shared_lock rlock(m_cacheMutex);
        if (auto it = m_materialCache.find(key); it != m_materialCache.end())
            if (auto sp = it->second.lock()) return sp;
        rlock.unlock();

        auto mats = MaterialLoader::loadMTL(key, "");
        if (mats.empty())
            DX3D_LOG_WARNING("AssetManager: No materials in '{}'", key);

        std::unique_lock wlock(m_cacheMutex);
        for (auto& m : mats)
        {
            auto matPtr = std::make_shared<Material>(m);

            if (!m.diffuseTexturePath.empty())
            {
                auto texPath = std::filesystem::path(key).parent_path() / m.diffuseTexturePath;
                matPtr->diffuseTexture = getTexture(texPath.string());
            }

            std::string matKey = makeKey(m.name);
            m_materialCache[matKey] = matPtr;
        }

        if (!mats.empty())
        {
            std::string firstKey = makeKey(mats.front().name);
            return m_materialCache[firstKey].lock();
        }

        return nullptr;
    }

    std::vector<Material> AssetManager::getMaterialsFromFile(const std::string& fullPath, const std::string& mtlFile)
    {
        std::filesystem::path mtlPath = std::filesystem::path(fullPath).parent_path() / mtlFile;
        const std::string mtlKey = makeKey(mtlPath.string());
        std::vector<Material> materials;

        {
            std::shared_lock rlock(m_cacheMutex);
            if (auto it = m_materialCache.find(mtlKey); it != m_materialCache.end())
            {
                DX3D_LOG_INFO("Using cached materials from '{}'", mtlPath.string());
                if (auto sp = it->second.lock())
                    materials.push_back(*sp);
                return materials;
            }
        }

        DX3D_LOG_INFO("Loading materials from: {}", mtlPath.string());
        materials = MaterialLoader::loadMTL(fullPath, mtlFile, this);

        {
            std::unique_lock wlock(m_cacheMutex);
            auto marker = std::make_shared<Material>();
            m_materialCache[mtlKey] = marker;

            for (auto& m : materials)
            {
                auto matPtr = std::make_shared<Material>(m);
                m_materialCache[makeKey(m.name)] = matPtr;
            }
        }

        DX3D_LOG_INFO("Cached {} new materials from '{}'", materials.size(), mtlPath.string());
        return materials;
    }



    bool AssetManager::hasMaterial(std::string_view mtlPath) const
    {
        const std::string key = makeKey(mtlPath);
        std::shared_lock rlock(m_cacheMutex);
        auto it = m_materialCache.find(key);
        return it != m_materialCache.end() && !it->second.expired();
    }

    std::shared_ptr<ModelGPU> AssetManager::getModel(std::string_view relativePath)
    {
        const std::string key = makeKey(relativePath);

        {
            std::shared_lock rlock(m_cacheMutex);
            if (auto it = m_modelCache.find(key); it != m_modelCache.end())
                if (auto sp = it->second.lock())
                    return sp;
        }

        auto importer = std::make_shared<ModelImporter>(m_device, this);
        std::shared_ptr<ModelData> cpuData;

        // --- Binary Cache Logic ---
        std::filesystem::path objPath(key);
        std::filesystem::path binPath = objPath;
        binPath += ".bin"; // e.g., "model.obj.bin"

        bool loadFromBinary = false;

        // Check if .bin exists and is newer than .obj
        if (std::filesystem::exists(binPath) && std::filesystem::exists(objPath))
        {
            auto objTime = std::filesystem::last_write_time(objPath);
            auto binTime = std::filesystem::last_write_time(binPath);

            if (binTime > objTime) {
                ModelData binData = importer->loadBinary(binPath.string());
                if (!binData.vertices.empty()) { // Check if load succeeded
                    cpuData = std::make_shared<ModelData>(std::move(binData));
                    loadFromBinary = true;
                }
            }
        }

        // Fallback to OBJ if binary invalid or outdated
        if (!loadFromBinary)
        {
            cpuData = std::make_shared<ModelData>(importer->loadOBJ(key));

            // Save binary for next time
            if (!cpuData->vertices.empty()) {
                importer->saveBinary(binPath.string(), *cpuData);
            }
        }

        return buildGPUAndCache(key, cpuData);
    }

    std::shared_ptr<ModelGPU> AssetManager::buildGPUAndCache(
        const std::string& key,
        const std::shared_ptr<const ModelData>& cpuData)
    {
        auto gpuModel = std::make_shared<ModelGPU>();
        gpuModel->materials = cpuData->materials;
        gpuModel->boundingBox = cpuData->boundingBox;
        gpuModel->materialGroups = cpuData->materialGroups;

        gpuModel->mesh = m_device->createMesh(cpuData->vertices, cpuData->indices);

        for (auto& mat : gpuModel->materials)
        {
            if (!mat.diffuseTexturePath.empty())
            {
                auto texPath = std::filesystem::path(key).parent_path() / mat.diffuseTexturePath;
                mat.diffuseTexture = getTexture(texPath.string());
            }
        }

        std::unique_lock wlock(m_cacheMutex);
        m_modelCache[key] = gpuModel;
        return gpuModel;
    }

    bool AssetManager::hasModel(std::string_view relativePath) const
    {
        const std::string key = makeKey(relativePath);
        std::shared_lock rlock(m_cacheMutex);
        auto it = m_modelCache.find(key);
        return it != m_modelCache.end() && !it->second.expired();
    }

    void AssetManager::clearGPU()
    {
        std::unique_lock wlock(m_cacheMutex);
        m_modelCache.clear();
        DX3D_LOG_INFO("AssetManager: Cleared model cache");
    }

    void AssetManager::clearAll()
    {
        std::unique_lock wlock(m_cacheMutex);
        m_modelCache.clear();
        m_materialCache.clear();
        m_textureCache.clear();
        DX3D_LOG_INFO("AssetManager: Cleared all caches");
    }

    void AssetManager::setAssetsRoot(const std::filesystem::path& root)
    {
        std::unique_lock wlock(m_cacheMutex);
        m_assetsRoot = std::filesystem::weakly_canonical(root);
    }

    std::string AssetManager::makeKey(std::string_view relativePath) const
    {
        std::filesystem::path p(relativePath);
        if (!m_assetsRoot.empty())
            p = m_assetsRoot / p;
        std::error_code ec;
        std::filesystem::path canon = std::filesystem::weakly_canonical(p, ec);
        if (ec) canon = p.lexically_normal();
        return toLowerAscii(canon.generic_string());
    }

    void AssetManager::preloadModel(std::string_view relativePath)
    {
        (void)getModel(relativePath);
    }

    void AssetManager::evictModel(std::string_view relativePath)
    {
        const std::string key = makeKey(relativePath);
        std::unique_lock wlock(m_cacheMutex);
        m_modelCache.erase(key);
        DX3D_LOG_DEBUG("AssetManager: evicted GPU entry '{}'", key);
    }
}
