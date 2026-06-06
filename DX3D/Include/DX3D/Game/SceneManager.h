#pragma once
#include <DX3D/Game/GameObject.h>
#include <DX3D/Graphics/Importers/AssetManager.h>
#include <DX3D/Game/Registry.h>
#include <json.hpp>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

namespace dx3d {

    class SceneManager {
    public:
        SceneManager() = default;

        void setRegistry(Registry* registry) { m_registry = registry; }
        // Legacy combined save (editor + runtime-compatible).
        nlohmann::json saveScene() const;
        // Runtime-facing transform snapshot keyed by entity id (ECS-authoritative when resolver is present).
        nlohmann::json saveRuntimeTransforms() const;
        void applyRuntimeTransforms(const nlohmann::json& transforms);
        void loadScene(const nlohmann::json& j, AssetManager& assetManager);

        std::function<void(std::shared_ptr<GameObject>)> onObjectCreated;
        std::function<const Transform* (Entity)> resolveTransform; // ECS-authoritative transform lookup
        std::function<void(Entity, const Transform&)> assignTransform; // ECS-authoritative transform write

        std::shared_ptr<GameObject> createObject(const std::string& name = "");
        void destroyObject(const std::shared_ptr<GameObject>& object);

        std::shared_ptr<GameObject> findObject(const std::string& name) const;
        std::shared_ptr<GameObject> findObjectByEntity(Entity entity) const;
        std::vector<std::shared_ptr<GameObject>> findObjectsByTag(const std::string& tag) const;
        const std::vector<std::shared_ptr<GameObject>>& getAllObjects() const { return m_objects; }

        void clear();

        // Editor-bridge helpers: mirror ECS runtime transform state into GameObject cache.
        bool syncObjectTransformFromECS(Entity entity);
        void syncAllObjectTransformsFromECS();
        bool applyTransformToEntity(Entity entity, const Transform& transform);

        std::shared_ptr<GameObject> pickObject(const Vec3d& rayOrigin, const DirectX::XMVECTOR& rayDir, const dx3d::Vec3d& cameraPos, float* outDistance = nullptr) const;

        Transform getObjectWorldTransform(const std::shared_ptr<GameObject>& object) const;

        std::shared_ptr<GameObject> bindEditorObject(Entity entity, const std::string& name = "");

    private:
        std::vector<std::shared_ptr<GameObject>> m_objects;
        std::unordered_map<std::string, std::shared_ptr<GameObject>> m_objectMap;
        std::unordered_map<uint32_t, std::shared_ptr<GameObject>> m_entityMap;
        Registry* m_registry = nullptr;

    };

}