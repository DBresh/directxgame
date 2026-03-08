#include <DX3D/Graphics/Importers/ModelCache.h>

namespace dx3d
{
    std::unordered_map<std::string, std::weak_ptr<ModelData>> ModelCache::s_cache{};
    std::mutex ModelCache::s_mutex{};

    std::string ModelCache::makeKey(const std::string& relativePath)
    {
        std::filesystem::path base = std::filesystem::path("DX3D/Assets");
        std::filesystem::path rel = std::filesystem::path(relativePath);

        auto joined = (base / rel).lexically_normal();
#ifdef _WIN32
        return joined.generic_string();
#else
        return joined.string();
#endif
    }

    std::shared_ptr<ModelData> ModelCache::getOrLoad(
        const std::string& relativePath,
        const std::shared_ptr<GraphicsDevice>& device)
    {
        const std::string key = makeKey(relativePath);

        {
            std::lock_guard<std::mutex> lock(s_mutex);
            if (auto it = s_cache.find(key); it != s_cache.end())
                if (auto existing = it->second.lock())
                    return existing;
        }

        auto importer = std::make_shared<ModelImporter>(device, nullptr);
        auto loaded = std::make_shared<ModelData>(importer->loadOBJ(relativePath));

        {
            std::lock_guard<std::mutex> lock(s_mutex);
            s_cache[key] = loaded;
        }

        DX3D_LOG_INFO("ModelCache load: {} (verts: {}, tris: {})",
            key, loaded->vertices.size(), loaded->indices.size() / 3);

        return loaded;
    }


    void ModelCache::clear()
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_cache.clear();
        DX3D_LOG_INFO("ModelCache cleared");
    }

    void ModelCache::pruneExpired()
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        size_t before = s_cache.size();
        for (auto it = s_cache.begin(); it != s_cache.end(); )
        {
            if (it->second.expired())
                it = s_cache.erase(it);
            else
                ++it;
        }
        DX3D_LOG_INFO("ModelCache pruned: {} - {}", before, s_cache.size());
    }

    size_t ModelCache::size()
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        return s_cache.size();
    }
}
