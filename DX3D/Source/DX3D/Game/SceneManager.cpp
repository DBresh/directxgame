#include <DX3D/Game/SceneManager.h>
#include <algorithm>

namespace dx3d {

    std::shared_ptr<GameObject> SceneManager::createObject(const std::string& name) {
        auto object = std::make_shared<GameObject>();
        object->name = name;

        m_objects.push_back(object);
        if (!name.empty()) {
            m_objectMap[name] = object;
        }

        return object;
    }

    void SceneManager::destroyObject(const std::shared_ptr<GameObject>& object) {
        auto it = std::find(m_objects.begin(), m_objects.end(), object);
        if (it != m_objects.end()) {
            if (!object->name.empty()) {
                m_objectMap.erase(object->name);
            }
            m_objects.erase(it);
        }
    }

    std::shared_ptr<GameObject> SceneManager::findObject(const std::string& name) const {
        auto it = m_objectMap.find(name);
        return (it != m_objectMap.end()) ? it->second : nullptr;
    }

    std::vector<std::shared_ptr<GameObject>> SceneManager::findObjectsByTag(const std::string& tag) const {
        std::vector<std::shared_ptr<GameObject>> result;
        for (const auto& obj : m_objects) {
            if (obj->tag == tag) {
                result.push_back(obj);
            }
        }
        return result;
    }

    void SceneManager::clear() {
        m_objects.clear();
        m_objectMap.clear();
    }

    void SceneManager::update(float deltaTime) {
        // Update all objects (for game logic, animations, etc.)
        for (auto& obj : m_objects) {
            
        }
    }

    void SceneManager::shiftUniverse(double cameraX, double cameraY, double cameraZ)
    {
        for (auto& obj : m_objects)
        {
            obj->applyFloatingOriginOffset(cameraX, cameraY, cameraZ);
        }
    }

    std::shared_ptr<GameObject> SceneManager::pickObject(const DirectX::XMVECTOR& rayOrigin, const DirectX::XMVECTOR& rayDir) const
    {
        std::shared_ptr<GameObject> pickedObj = nullptr;
        float minDistance = FLT_MAX;

        for (const auto& obj : m_objects)
        {
            if (!obj->model) continue;

            AABB bounds = obj->getWorldAABB();
            float tMin = 0.0f;

            if (bounds.intersectRay(rayOrigin, rayDir, tMin))
            {
                if (tMin < minDistance)
                {
                    minDistance = tMin;
                    pickedObj = obj;
                }
            }
        }

        return pickedObj;
    }

}