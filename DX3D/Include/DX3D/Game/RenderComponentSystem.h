#pragma once
#include <DX3D/Game/Entity.h>
#include <DX3D/Game/RenderComponent.h>
#include <vector>
#include <algorithm>
#include <cassert>

namespace dx3d {

    class RenderComponentSystem {
    public:
        RenderComponentSystem() = default;
        ~RenderComponentSystem() = default;

        void add(Entity e, const RenderComponent& r) {
            uint32_t entityIndex = e.getIndex();
            if (entityIndex >= m_sparse.size()) {
                m_sparse.resize(entityIndex + 1, INVALID_INDEX);
            }

            assert(m_sparse[entityIndex] == INVALID_INDEX && "Entity already has a RenderComponent!");

            size_t denseIndex = m_denseData.size();
            m_sparse[entityIndex] = denseIndex;

            m_denseData.push_back(r);
            m_denseEntities.push_back(e);
            markDirty(e);
            m_structuralDirty = true;
        }

        void remove(Entity e) {
            assert(has(e) && "Entity does not have a RenderComponent!");

            uint32_t entityIndex = e.getIndex();
            size_t deletedDenseIndex = m_sparse[entityIndex];
            size_t lastDenseIndex = m_denseData.size() - 1;

            if (deletedDenseIndex != lastDenseIndex) {
                m_denseData[deletedDenseIndex] = std::move(m_denseData[lastDenseIndex]);
                Entity lastEntity = m_denseEntities[lastDenseIndex];
                m_denseEntities[deletedDenseIndex] = lastEntity;
                m_sparse[lastEntity.getIndex()] = deletedDenseIndex;
            }

            m_sparse[entityIndex] = INVALID_INDEX;
            m_denseData.pop_back();
            m_denseEntities.pop_back();
            m_dirtyEntities.clear();
            m_structuralDirty = true;
        }

        RenderComponent& get(Entity e) {
            assert(has(e) && "Entity does not have a RenderComponent!");
            markDirty(e);
            return m_denseData[m_sparse[e.getIndex()]];
        }

        void set(Entity e, const RenderComponent& r) {
            assert(has(e) && "Entity does not have a RenderComponent!");
            m_denseData[m_sparse[e.getIndex()]] = r;
            markDirty(e);
        }

        void markDirty(Entity e) {
            if (std::find(m_dirtyEntities.begin(), m_dirtyEntities.end(), e) == m_dirtyEntities.end()) {
                m_dirtyEntities.push_back(e);
            }
        }

        bool hasDirtyComponents() const { return m_structuralDirty || !m_dirtyEntities.empty(); }
        bool hasStructuralChanges() const { return m_structuralDirty; }
        const std::vector<Entity>& getDirtyEntities() const { return m_dirtyEntities; }
        void clearDirtyTracking() { m_dirtyEntities.clear(); m_structuralDirty = false; }

        const RenderComponent& get(Entity e) const {
            assert(has(e) && "Entity does not have a RenderComponent!");
            return m_denseData[m_sparse[e.getIndex()]];
        }

        bool has(Entity e) const {
            uint32_t entityIndex = e.getIndex();
            return entityIndex < m_sparse.size() && m_sparse[entityIndex] != INVALID_INDEX;
        }

        std::vector<RenderComponent>& getRawData() { return m_denseData; }
        const std::vector<RenderComponent>& getRawData() const { return m_denseData; }
        std::vector<Entity>& getRawEntities() { return m_denseEntities; }
        const std::vector<Entity>& getRawEntities() const { return m_denseEntities; }

        void clear() {
            m_denseData.clear();
            m_denseEntities.clear();
            m_sparse.clear();
            m_dirtyEntities.clear();
            m_structuralDirty = true;
        }

    private:
        static constexpr size_t INVALID_INDEX = static_cast<size_t>(-1);

        std::vector<size_t> m_sparse;
        std::vector<RenderComponent> m_denseData;
        std::vector<Entity> m_denseEntities;
        std::vector<Entity> m_dirtyEntities;
        bool m_structuralDirty = true;
    };
}