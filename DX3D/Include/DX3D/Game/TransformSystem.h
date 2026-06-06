#pragma once
#include <DX3D/Game/Entity.h>
#include <DX3D/Math/Transform.h>
#include <DX3D/Game/HierarchyComponent.h>
#include <DirectXMath.h>
#include <vector>
#include <algorithm>
#include <cassert>

namespace dx3d {

    class TransformSystem {
    public:
        TransformSystem() = default;
        ~TransformSystem() = default;

        void assignTransform(Entity e, const Transform& localTransform) {
            uint32_t index = e.getIndex();
            if (index >= m_sparse.size()) {
                m_sparse.resize(index + 1, INVALID_INDEX);
            }

            assert(m_sparse[index] == INVALID_INDEX && "Entity already has a Transform assigned!");

            m_sparse[index] = m_local.size();
            m_local.push_back(localTransform);
            m_world.push_back(WorldTransform{});
            m_hierarchy.push_back(HierarchyComponent{});
            m_entities.push_back(e);

            m_isDirty = true;
            markTransformDirty(e);
        }

        void setParent(Entity child, Entity parent) {
            assert(hasTransform(child) && "Child missing transform component");

            unlink(child);

            if (parent == Entity::Null) {
                m_isDirty = true;
                markTransformDirty(child);
                return;
            }

            assert(hasTransform(parent) && "Parent missing transform component");

            size_t childIdx = m_sparse[child.getIndex()];
            size_t parentIdx = m_sparse[parent.getIndex()];

            m_hierarchy[childIdx].parent = parent;

            Entity currentFirst = m_hierarchy[parentIdx].firstChild;
            m_hierarchy[childIdx].nextSibling = currentFirst;

            if (currentFirst != Entity::Null) {
                m_hierarchy[m_sparse[currentFirst.getIndex()]].prevSibling = child;
            }
            m_hierarchy[parentIdx].firstChild = child;

            m_isDirty = true;
            markTransformDirty(child);
        }

        void removeTransform(Entity e) {
            assert(hasTransform(e) && "Entity does not have a Transform!");

            unlink(e); // Ensure we don't break the hierarchy tree

            uint32_t entityIndex = e.getIndex();
            size_t deletedDenseIndex = m_sparse[entityIndex];
            size_t lastDenseIndex = m_entities.size() - 1;

            if (deletedDenseIndex != lastDenseIndex) {
                m_local[deletedDenseIndex] = std::move(m_local[lastDenseIndex]);
                m_world[deletedDenseIndex] = std::move(m_world[lastDenseIndex]);
                m_hierarchy[deletedDenseIndex] = std::move(m_hierarchy[lastDenseIndex]);

                Entity lastEntity = m_entities[lastDenseIndex];
                m_entities[deletedDenseIndex] = lastEntity;
                m_sparse[lastEntity.getIndex()] = deletedDenseIndex;
            }

            m_sparse[entityIndex] = INVALID_INDEX;
            m_local.pop_back();
            m_world.pop_back();
            m_hierarchy.pop_back();
            m_entities.pop_back();

            m_isDirty = true;
            m_structuralDirty = true;
        }

        void updateWorldTransforms() {
            if (m_isDirty) {
                buildTopologicalOrder();
                m_isDirty = false;
            }

            for (Entity e : m_topologicalOrder) {
                size_t idx = m_sparse[e.getIndex()];
                Entity parent = m_hierarchy[idx].parent;
                const auto& local = m_local[idx];

                if (parent == Entity::Null) {
                    // Root object: World == Local
                    m_world[idx].position = local.getPosition();
                    m_world[idx].rotation = local.getQuaternion();
                    m_world[idx].scale = local.getScale();
                }
                else {
                    // Child object: Calculate relative to parent
                    size_t parentIdx = m_sparse[parent.getIndex()];
                    const auto& parentWorld = m_world[parentIdx];
                    const auto& flags = m_hierarchy[idx];

                    m_world[idx].scale = local.getScale();
                    if (flags.inheritScale) {
                        m_world[idx].scale.x *= parentWorld.scale.x;
                        m_world[idx].scale.y *= parentWorld.scale.y;
                        m_world[idx].scale.z *= parentWorld.scale.z;
                    }

                    DirectX::XMFLOAT4 localQuat = local.getQuaternion();
                    DirectX::XMVECTOR qLocal = DirectX::XMLoadFloat4(&localQuat);
                    DirectX::XMVECTOR qParent = DirectX::XMLoadFloat4(&parentWorld.rotation);
                    DirectX::XMVECTOR qWorld = qLocal;

                    if (flags.inheritRotation) {
                        qWorld = DirectX::XMQuaternionMultiply(qLocal, qParent);
                    }
                    DirectX::XMStoreFloat4(&m_world[idx].rotation, qWorld);

                    m_world[idx].position = local.getPosition();
                    if (flags.inheritPosition) {
                        DirectX::XMMATRIX S_mat = DirectX::XMMatrixScaling(parentWorld.scale.x, parentWorld.scale.y, parentWorld.scale.z);
                        DirectX::XMMATRIX R_mat = DirectX::XMMatrixRotationQuaternion(qParent);

                        dx3d::Vec3d localPos = local.getPosition();
                        DirectX::XMVECTOR localV = DirectX::XMVectorSet(
                            static_cast<float>(localPos.x),
                            static_cast<float>(localPos.y),
                            static_cast<float>(localPos.z),
                            1.0f
                        );

                        DirectX::XMVECTOR rotatedScaled = DirectX::XMVector3Transform(localV, S_mat * R_mat);
                        DirectX::XMFLOAT3 rsFloat;
                        DirectX::XMStoreFloat3(&rsFloat, rotatedScaled);

                        m_world[idx].position = parentWorld.position + dx3d::Vec3d(rsFloat.x, rsFloat.y, rsFloat.z);
                    }
                }
            }
        }

        bool hasTransform(Entity e) const { return e.getIndex() < m_sparse.size() && m_sparse[e.getIndex()] != INVALID_INDEX; }

        Transform& getLocal(Entity e) { return m_local[m_sparse[e.getIndex()]]; }
        const Transform& getLocal(Entity e) const { return m_local[m_sparse[e.getIndex()]]; }

        void setTransform(Entity e, const Transform& localTransform) {
            assert(hasTransform(e) && "Entity does not have a Transform!");
            m_local[m_sparse[e.getIndex()]] = localTransform;
            markTransformDirty(e);
        }

        void markTransformDirty(Entity e) {
            assert(hasTransform(e) && "Entity does not have a Transform!");
            if (std::find(m_dirtyEntities.begin(), m_dirtyEntities.end(), e) == m_dirtyEntities.end()) {
                m_dirtyEntities.push_back(e);
            }
        }

        bool hasDirtyTransforms() const { return m_structuralDirty || !m_dirtyEntities.empty(); }
        bool hasStructuralChanges() const { return m_structuralDirty; }
        const std::vector<Entity>& getDirtyEntities() const { return m_dirtyEntities; }
        void clearDirtyTracking() { m_dirtyEntities.clear(); m_structuralDirty = false; }

        const WorldTransform& getWorld(Entity e) const { return m_world[m_sparse[e.getIndex()]]; }
        HierarchyComponent& getHierarchy(Entity e) { return m_hierarchy[m_sparse[e.getIndex()]]; }

        Transform& getTransform(Entity e) { return getLocal(e); }
        const Transform& getTransform(Entity e) const { return getLocal(e); }
        bool has(Entity e) const { return hasTransform(e); }

        const std::vector<Entity>& getRawEntities() const { return m_entities; }
        std::vector<Entity>& getRawEntities() { return m_entities; }

        const std::vector<Transform>& getRawData() const { return m_local; }
        std::vector<Transform>& getRawData() { return m_local; }

        const std::vector<WorldTransform>& getRawWorldData() const { return m_world; }
        const std::vector<HierarchyComponent>& getRawHierarchyData() const { return m_hierarchy; }

        const std::vector<Entity>& getTopologicalOrder() const { return m_topologicalOrder; }

        void clear() {
            m_local.clear(); m_world.clear(); m_hierarchy.clear();
            m_entities.clear(); m_sparse.clear(); m_topologicalOrder.clear();
            m_dirtyEntities.clear();
            m_isDirty = true;
            m_structuralDirty = true;
        }

    private:
        static constexpr size_t INVALID_INDEX = static_cast<size_t>(-1);

        std::vector<size_t> m_sparse;
        std::vector<Transform> m_local;
        std::vector<WorldTransform> m_world;
        std::vector<HierarchyComponent> m_hierarchy;
        std::vector<Entity> m_entities;

        std::vector<Entity> m_topologicalOrder;
        std::vector<Entity> m_dirtyEntities;
        bool m_isDirty = true;
        bool m_structuralDirty = true;

        void unlink(Entity e) {
            size_t idx = m_sparse[e.getIndex()];
            Entity p = m_hierarchy[idx].parent;

            if (p == Entity::Null) return;

            size_t pIdx = m_sparse[p.getIndex()];
            Entity prev = m_hierarchy[idx].prevSibling;
            Entity next = m_hierarchy[idx].nextSibling;

            if (prev != Entity::Null) {
                m_hierarchy[m_sparse[prev.getIndex()]].nextSibling = next;
            }
            else {
                m_hierarchy[pIdx].firstChild = next;
            }

            if (next != Entity::Null) {
                m_hierarchy[m_sparse[next.getIndex()]].prevSibling = prev;
            }

            m_hierarchy[idx].parent = Entity::Null;
            m_hierarchy[idx].prevSibling = Entity::Null;
            m_hierarchy[idx].nextSibling = Entity::Null;
        }

        void buildTopologicalOrder() {
            m_topologicalOrder.clear();
            m_topologicalOrder.reserve(m_entities.size());

            // Add all roots (parent == Null)
            for (size_t i = 0; i < m_entities.size(); ++i) {
                if (m_hierarchy[i].parent == Entity::Null) {
                    m_topologicalOrder.push_back(m_entities[i]);
                }
            }

            // BFS traversal to flatten the tree linearly
            size_t head = 0;
            while (head < m_topologicalOrder.size()) {
                Entity current = m_topologicalOrder[head++];
                Entity child = m_hierarchy[m_sparse[current.getIndex()]].firstChild;

                while (child != Entity::Null) {
                    m_topologicalOrder.push_back(child);
                    child = m_hierarchy[m_sparse[child.getIndex()]].nextSibling;
                }
            }
        }
    };
}