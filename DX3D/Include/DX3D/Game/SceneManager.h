#pragma once
#include <DX3D/Game/GameObject.h>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

namespace dx3d {

    class SceneManager {
    public:
        SceneManager() = default;

        std::shared_ptr<GameObject> createObject(const std::string& name = "");
        void destroyObject(const std::shared_ptr<GameObject>& object);

        std::shared_ptr<GameObject> findObject(const std::string& name) const;
        std::vector<std::shared_ptr<GameObject>> findObjectsByTag(const std::string& tag) const;
        const std::vector<std::shared_ptr<GameObject>>& getAllObjects() const { return m_objects; }
        void shiftUniverse(double cameraX, double cameraY, double cameraZ);

        void clear();
        void update(float deltaTime);

        std::shared_ptr<GameObject> pickObject(const DirectX::XMVECTOR& rayOrigin, const DirectX::XMVECTOR& rayDir) const;

    private:
        std::vector<std::shared_ptr<GameObject>> m_objects;
        std::unordered_map<std::string, std::shared_ptr<GameObject>> m_objectMap;
    };

}