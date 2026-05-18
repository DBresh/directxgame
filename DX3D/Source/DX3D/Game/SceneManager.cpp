#include <DX3D/Game/SceneManager.h>
#include <DX3D/Core/Serialization.h>
#include <DX3D/Graphics/Importers/AssetManager.h>
#include <json.hpp>
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

    std::shared_ptr<GameObject> SceneManager::pickObject(const DirectX::XMVECTOR& rayOrigin, const DirectX::XMVECTOR& rayDir, const dx3d::Vec3d& cameraPos, float* outDistance) const
    {
        std::shared_ptr<GameObject> pickedObj = nullptr;
        float minDistance = FLT_MAX;

        for (const auto& obj : m_objects)
        {
            if (!obj->model) continue;

            AABB bounds = obj->getRelativeAABB(cameraPos);
            float tMin = 0.0f;

            if (bounds.intersectRay(rayOrigin, rayDir, tMin))
            {
                if (tMin < minDistance && tMin >= 0.0f)
                {
                    minDistance = tMin;
                    pickedObj = obj;
                }
            }
        }

        if (pickedObj && outDistance) {
            *outDistance = minDistance;
        }

        return pickedObj;
    }

    nlohmann::json SceneManager::saveScene() const
    {
        nlohmann::json jArray = nlohmann::json::array();
        std::unordered_map<GameObject*, int> idMap;

        int currentId = 0;
        for (const auto& obj : m_objects) {
            idMap[obj.get()] = currentId++;
        }

        for (const auto& obj : m_objects)
        {
            nlohmann::json jObj = nlohmann::json::object();

            jObj["id"] = idMap[obj.get()];
            jObj["name"] = obj->name;
            jObj["tag"] = obj->tag;
            jObj["modelName"] = obj->modelName;

            if (obj->hasParent() && idMap.find(obj->getParent().get()) != idMap.end()) {
                jObj["parentId"] = idMap[obj->getParent().get()];
            }
            else {
                jObj["parentId"] = -1;
            }

            jObj["inheritFlags"] = {
                {"position", obj->inheritPosition},
                {"rotation", obj->inheritRotation},
                {"scale", obj->inheritScale}
            };

            jObj["transform"] = obj->transform;

            nlohmann::json jComps = nlohmann::json::array();
            for (const auto& comp : obj->components) {
                nlohmann::json cData = comp->serialize();
                cData["type"] = comp->getType();
                jComps.push_back(cData);
            }
            jObj["components"] = jComps;

            jArray.push_back(jObj);
        }

        return jArray;
    }

    void SceneManager::loadScene(const nlohmann::json& jArray, AssetManager& assetManager)
    {
        clear();

        if (!jArray.is_array()) return;

        std::unordered_map<int, std::shared_ptr<GameObject>> tempMap;

        for (const auto& jObj : jArray)
        {
            int id = jObj.value("id", -1);
            if (id == -1) continue;

            auto obj = createObject(jObj.value("name", "Unnamed"));
            obj->tag = jObj.value("tag", "");
            obj->modelName = jObj.value("modelName", "");

            if (jObj.contains("inheritFlags")) {
                auto flags = jObj["inheritFlags"];
                obj->inheritPosition = flags.value("position", true);
                obj->inheritRotation = flags.value("rotation", true);
                obj->inheritScale = flags.value("scale", true);
            }

            if (jObj.contains("transform")) {
                obj->transform = jObj["transform"].get<Transform>();
            }

            if (!obj->modelName.empty()) {
                obj->model = assetManager.getModel(obj->modelName);
            }

            if (onObjectCreated) {
                onObjectCreated(obj);
            }

            tempMap[id] = obj;
        }

        for (const auto& jObj : jArray)
        {
            int id = jObj.value("id", -1);
            if (id == -1 || tempMap.find(id) == tempMap.end()) continue;

            auto obj = tempMap[id];

            int parentId = jObj.value("parentId", -1);
            if (parentId != -1 && tempMap.find(parentId) != tempMap.end()) {
                obj->setParent(tempMap[parentId]);
            }

            if (jObj.contains("components") && onComponentFactory) {
                for (const auto& cData : jObj["components"]) {
                    std::string type = cData.value("type", "");
                    onComponentFactory(obj.get(), type, cData);
                }
            }
        }
    }

}