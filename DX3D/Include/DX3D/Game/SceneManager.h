#pragma once
#include <DX3D/Game/GameObject.h>
#include <DX3D/Graphics/Importers/AssetManager.h>
#include <json.hpp>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

namespace dx3d {

    class SceneManager {
    public:
        SceneManager() = default;

        nlohmann::json saveScene() const;
        void loadScene(const nlohmann::json& j, AssetManager& assetManager);

        std::function<void(std::shared_ptr<GameObject>)> onObjectCreated;
        std::function<void(GameObject*, const std::string&, const nlohmann::json&)> onComponentFactory;

        std::shared_ptr<GameObject> createObject(const std::string& name = "");
        void destroyObject(const std::shared_ptr<GameObject>& object);

        std::shared_ptr<GameObject> findObject(const std::string& name) const;
        std::vector<std::shared_ptr<GameObject>> findObjectsByTag(const std::string& tag) const;
        const std::vector<std::shared_ptr<GameObject>>& getAllObjects() const { return m_objects; }

        void clear();
        void update(float deltaTime);

        std::shared_ptr<GameObject> pickObject(const DirectX::XMVECTOR& rayOrigin, const DirectX::XMVECTOR& rayDir, const dx3d::Vec3d& cameraPos, float* outDistance = nullptr) const;



    private:
        std::vector<std::shared_ptr<GameObject>> m_objects;
        std::unordered_map<std::string, std::shared_ptr<GameObject>> m_objectMap;
    };

}