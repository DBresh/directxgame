#pragma once
#include <DX3D/Game/Entity.h>
#include <DX3D/Math/Transform.h>
#include <DX3D/Math/Vec3d.h>
#include <vector>
#include <cassert>

namespace dx3d {

    class TransformSystem {
    public:
        TransformSystem() = default;
        ~TransformSystem() = default;

        // Mounts spatial parameters to a target Entity ID
        void assignTransform(Entity e, const Transform& t) {
            uint32_t entityIndex = e.getIndex();

            // Expand sparse map if this is a high-index entity
            if (entityIndex >= m_sparse.size()) {
                m_sparse.resize(entityIndex + 1, INVALID_INDEX);
            }

            assert(m_sparse[entityIndex] == INVALID_INDEX && "Entity already has a Transform assigned!");

            // Register into the flat arrays
            size_t denseIndex = m_denseData.size();
            m_sparse[entityIndex] = denseIndex;

            m_denseData.push_back(t);
            m_denseEntities.push_back(e);
        }

        // O(1) Swap-and-Pop algorithm for contiguous memory alignment
        void removeTransform(Entity e) {
            assert(hasTransform(e) && "Entity does not have a Transform!");

            uint32_t entityIndex = e.getIndex();
            size_t deletedDenseIndex = m_sparse[entityIndex];
            size_t lastDenseIndex = m_denseData.size() - 1;

            if (deletedDenseIndex != lastDenseIndex) {
                // 1. Move the trailing data element into the deleted hole
                m_denseData[deletedDenseIndex] = std::move(m_denseData[lastDenseIndex]);

                // 2. Identify which Entity owned that trailing data
                Entity lastEntity = m_denseEntities[lastDenseIndex];

                // 3. Move the tracking element
                m_denseEntities[deletedDenseIndex] = lastEntity;

                // 4. Update the lookup hash-map for the swapped entity
                m_sparse[lastEntity.getIndex()] = deletedDenseIndex;
            }

            // Invalidate the removed entity's map and trim the arrays
            m_sparse[entityIndex] = INVALID_INDEX;
            m_denseData.pop_back();
            m_denseEntities.pop_back();
        }

        // Instantaneous array lookup
        Transform& getTransform(Entity e) {
            assert(hasTransform(e) && "Entity does not have a Transform!");
            return m_denseData[m_sparse[e.getIndex()]];
        }

        const Transform& getTransform(Entity e) const {
            assert(hasTransform(e) && "Entity does not have a Transform!");
            return m_denseData[m_sparse[e.getIndex()]];
        }

        bool hasTransform(Entity e) const {
            uint32_t entityIndex = e.getIndex();
            return entityIndex < m_sparse.size() && m_sparse[entityIndex] != INVALID_INDEX;
        }

        size_t getSparseIndex(Entity e) const {
            assert(hasTransform(e) && "CRITICAL: Requested Sparse Index of Entity without a Transform component!");
            return m_sparse[e.getIndex()];
        }

        // Direct access to flat arrays for Render Pass sweeps
        std::vector<Transform>& getRawData() { return m_denseData; }
        const std::vector<Transform>& getRawData() const { return m_denseData; }
        std::vector<Entity>& getRawEntities() { return m_denseEntities; }

        void clear()
        {
            m_denseData.clear();
            m_denseEntities.clear();
            m_sparse.clear();
        }
    private:
        static constexpr size_t INVALID_INDEX = static_cast<size_t>(-1);

        // Sparse array mapping: Entity::getIndex() -> Dense Array Index
        std::vector<size_t> m_sparse;

        // Tightly packed memory blocks (Parallel Arrays)
        std::vector<Transform> m_denseData;
        std::vector<Entity> m_denseEntities;
    };

} // namespace dx3d