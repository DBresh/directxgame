#include <DX3D/Game/SceneManager.h>
#include <DX3D/Core/Serialization.h>
#include <DX3D/Graphics/Importers/AssetManager.h>
#include <json.hpp>
#include <algorithm>

namespace dx3d {

	// WARNING: This is an Editor-only function. Do not use during ECS runtime execution.
	// ECS systems must read directly from TransformSystem::getWorld().
	Transform SceneManager::getObjectWorldTransform(const std::shared_ptr<GameObject>& object) const
	{
		if (!object) {
			return Transform{};
		}

		Transform result = resolveTransform ?
			(resolveTransform(object->entity) ? *resolveTransform(object->entity) : object->cachedEditorTransform)
			: object->cachedEditorTransform;

		auto parentPtr = object->parent.lock();
		if (!parentPtr) {
			return result;
		}

		Transform parentWorld = getObjectWorldTransform(parentPtr);

		dx3d::Vec3d localPos = result.getPosition();
		DirectX::XMFLOAT3 localScale = result.getScale();
		DirectX::XMFLOAT4 localQuat = result.getQuaternion();

		dx3d::Vec3d parentPos = parentWorld.getPosition();
		DirectX::XMFLOAT3 parentScale = parentWorld.getScale();
		DirectX::XMFLOAT4 parentQuat = parentWorld.getQuaternion();

		DirectX::XMFLOAT3 worldScale = localScale;
		if (object->inheritScale) {
			worldScale.x *= parentScale.x;
			worldScale.y *= parentScale.y;
			worldScale.z *= parentScale.z;
		}

		DirectX::XMVECTOR qLocal = DirectX::XMLoadFloat4(&localQuat);
		DirectX::XMVECTOR qParent = DirectX::XMLoadFloat4(&parentQuat);
		DirectX::XMVECTOR qWorld = qLocal;
		if (object->inheritRotation) qWorld = DirectX::XMQuaternionMultiply(qLocal, qParent);
		DirectX::XMFLOAT4 worldQuat; DirectX::XMStoreFloat4(&worldQuat, qWorld);

		dx3d::Vec3d worldPos = localPos;
		if (object->inheritPosition)
		{
			DirectX::XMMATRIX S = DirectX::XMMatrixScaling(parentScale.x, parentScale.y, parentScale.z);
			DirectX::XMMATRIX R = DirectX::XMMatrixRotationQuaternion(qParent);
			DirectX::XMVECTOR localV = DirectX::XMVectorSet(static_cast<float>(localPos.x), static_cast<float>(localPos.y), static_cast<float>(localPos.z), 1.0f);

			DirectX::XMVECTOR rotatedScaled = DirectX::XMVector3Transform(localV, S * R);
			DirectX::XMFLOAT3 rsFloat;
			DirectX::XMStoreFloat3(&rsFloat, rotatedScaled);

			worldPos = parentPos + dx3d::Vec3d(rsFloat.x, rsFloat.y, rsFloat.z);
		}

		result.setScale(worldScale);
		result.setPosition(worldPos);
		result.setQuaternion(worldQuat);

		return result;
	}

	std::shared_ptr<GameObject> SceneManager::createObject(const std::string& name) {
		auto object = std::make_shared<GameObject>();
		object->name = name;

		assert(m_registry && "SceneManager requires a valid Registry to create objects!");
		object->entity = m_registry->create();

		m_objects.push_back(object);
		m_entityMap[object->entity.id] = object;
		if (!name.empty()) {
			m_objectMap[name] = object;
		}

		return object;
	}

	std::shared_ptr<GameObject> SceneManager::findObjectByEntity(Entity entity) const {
		auto it = m_entityMap.find(entity.id);
		return (it != m_entityMap.end()) ? it->second : nullptr;
	}

	void SceneManager::destroyObject(const std::shared_ptr<GameObject>& object) {
		auto it = std::find(m_objects.begin(), m_objects.end(), object);
		if (it != m_objects.end()) {
			if (!object->name.empty()) {
				m_objectMap.erase(object->name);
			}
			m_entityMap.erase(object->entity.id);

			if (m_registry && !object->entity.isNull()) {
				m_registry->destroy(object->entity);
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
		m_entityMap.clear();
	}

	bool SceneManager::syncObjectTransformFromECS(Entity entity)
	{
		if (!resolveTransform) {
			return false;
		}

		auto obj = findObjectByEntity(entity);
		if (!obj) {
			return false;
		}

		const Transform* runtimeTransform = resolveTransform(entity);
		if (!runtimeTransform) {
			return false;
		}

		obj->cachedEditorTransform = *runtimeTransform;
		return true;
	}

	void SceneManager::syncAllObjectTransformsFromECS()
	{
		if (!resolveTransform) {
			return;
		}

		for (const auto& [entityId, obj] : m_entityMap)
		{
			if (!obj) {
				continue;
			}

			const Entity entity(entityId);
			const Transform* runtimeTransform = resolveTransform(entity);
			if (runtimeTransform) {
				obj->cachedEditorTransform = *runtimeTransform;
			}
		}
	}

	bool SceneManager::applyTransformToEntity(Entity entity, const Transform& transform)
	{
		auto obj = findObjectByEntity(entity);
		if (!obj) {
			return false;
		}

		obj->cachedEditorTransform = transform;
		if (assignTransform) {
			assignTransform(entity, transform);
		}

		return true;
	}

	std::shared_ptr<GameObject> SceneManager::pickObject(const Vec3d& rayOrigin, const DirectX::XMVECTOR& rayDir, const dx3d::Vec3d& cameraPos, float* outDistance) const
	{
		std::shared_ptr<GameObject> pickedObj = nullptr;
		float minDistance = FLT_MAX;

		// Shift absolute ray origin into the camera-relative space using double precision
		dx3d::Vec3d relRayOriginDouble = rayOrigin - cameraPos;
		DirectX::XMVECTOR relRayOriginFloat = relRayOriginDouble.toVector();

		for (const auto& obj : m_objects)
		{
			if (!obj->model) continue;

			AABB bounds;
			if (obj->model) {
				Transform worldTransform;
				if (resolveTransform && resolveTransform(obj->entity)) {
					worldTransform = *resolveTransform(obj->entity);
				}
				else {
					worldTransform = getObjectWorldTransform(obj);
				}
				bounds = obj->model->boundingBox.transform(worldTransform.getWorldMatrixRelative(cameraPos));
			}
			float tMin = 0.0f;

			if (bounds.intersectRay(relRayOriginFloat, rayDir, tMin))
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

	nlohmann::json SceneManager::saveRuntimeTransforms() const
	{
		nlohmann::json out = nlohmann::json::array();

		for (const auto& obj : m_objects)
		{
			nlohmann::json item;
			item["entityId"] = obj->entity.id;

			const Transform* ecsTransform = resolveTransform ? resolveTransform(obj->entity) : nullptr;
			item["transform"] = ecsTransform ? *ecsTransform : obj->cachedEditorTransform;

			out.push_back(item);
		}

		return out;
	}


	void SceneManager::applyRuntimeTransforms(const nlohmann::json& transforms)
	{
		if (!transforms.is_array()) {
			return;
		}

		std::unordered_map<uint32_t, Transform> byEntityId;
		for (const auto& item : transforms)
		{
			if (!item.contains("entityId") || !item.contains("transform")) {
				continue;
			}
			byEntityId[item["entityId"].get<uint32_t>()] = item["transform"].get<Transform>();
		}

		for (const auto& obj : m_objects)
		{
			auto it = byEntityId.find(obj->entity.id);
			if (it != byEntityId.end()) {
				applyTransformToEntity(obj->entity, it->second);
			}
		}
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

			const Transform* ecsTransform = resolveTransform ? resolveTransform(obj->entity) : nullptr;
			jObj["transform"] = ecsTransform ? *ecsTransform : obj->cachedEditorTransform;

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
				const Transform loadedTransform = jObj["transform"].get<Transform>();
				applyTransformToEntity(obj->entity, loadedTransform);
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
		}
	}

	std::shared_ptr<GameObject> SceneManager::bindEditorObject(Entity entity, const std::string& name) {
		auto object = std::make_shared<GameObject>();
		object->name = name;

		object->entity = entity;

		m_objects.push_back(object);
		m_entityMap[object->entity.id] = object;
		if (!name.empty()) {
			m_objectMap[name] = object;
		}

		return object;
	}
}