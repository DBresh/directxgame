#pragma once
#include <DX3D/Game/Entity.h>
#include <Game/Kepler/OrbitData.h>
#include <vector>
#include <cassert>

namespace dx3d {

    class OrbitSystem {
    public:
        OrbitSystem() = default;
        ~OrbitSystem() = default;

        void assignOrbitToEntity(Entity e, const Simulator::OrbitData& data) {
            uint32_t entityIndex = e.getIndex();

            if (entityIndex >= m_sparse.size()) {
                m_sparse.resize(entityIndex + 1, INVALID_INDEX);
            }

            assert(m_sparse[entityIndex] == INVALID_INDEX && "Entity already has an Orbit assigned!");

            size_t denseIndex = m_denseData.size();
            m_sparse[entityIndex] = denseIndex;

            m_denseData.push_back(data);
            m_denseEntities.push_back(e);
        }

        void removeOrbit(Entity e) {
            assert(hasOrbit(e) && "Entity does not have an Orbit!");

            uint32_t entityIndex = e.getIndex();
            size_t deletedDenseIndex = m_sparse[entityIndex];
            size_t lastDenseIndex = m_denseData.size() - 1;

            if (deletedDenseIndex != lastDenseIndex) {
                // Swap the trailing memory to the deleted hole
                m_denseData[deletedDenseIndex] = std::move(m_denseData[lastDenseIndex]);

                Entity lastEntity = m_denseEntities[lastDenseIndex];
                m_denseEntities[deletedDenseIndex] = lastEntity;

                m_sparse[lastEntity.getIndex()] = deletedDenseIndex;

                // --- ARCHITECTURE FIX: The Index Patch ---
                // Because we moved an element from 'lastDenseIndex' to 'deletedDenseIndex',
                // any active orbit that relied on 'lastDenseIndex' as its parent must be patched.
                for (auto& orbit : m_denseData) {
                    if (orbit.ParentOrbitIndex == lastDenseIndex) {
                        orbit.ParentOrbitIndex = deletedDenseIndex;
                    }
                }
            }

            m_sparse[entityIndex] = INVALID_INDEX;
            m_denseData.pop_back();
            m_denseEntities.pop_back();
        }

        Simulator::OrbitData& getOrbit(Entity e) {
            assert(hasOrbit(e) && "Entity does not have an Orbit!");
            return m_denseData[m_sparse[e.getIndex()]];
        }

        bool hasOrbit(Entity e) const {
            uint32_t entityIndex = e.getIndex();
            return entityIndex < m_sparse.size() && m_sparse[entityIndex] != INVALID_INDEX;
        }

        // The physics execution pipeline sweeps contiguous arrays natively
        void UpdateAll(double scaledDt) {
            // Because m_denseData is perfectly contiguous and contains strictly POD,
            // we can safely slice this vector and feed it directly into your JobSystem.

            /* Job System Integration Target:
            JobSystem::Dispatch(m_denseData.size(), chunkSize, [&](size_t start, size_t end) {
                for (size_t i = start; i < end; ++i) {
                    // Update anomalies and calculate instantaneous absoluteWorldPosition
                    CalculateOrbitStateFromElements(m_denseData[i], scaledDt);
                }
            });
            */
        }

        size_t getSparseIndex(Entity e) const {
            assert(hasOrbit(e) && "CRITICAL: Requested Sparse Index of Entity without an Orbit component!");
            return m_sparse[e.getIndex()];
        }

        // Used by Phase 3 (Visual Transform Alignment) to sync rendering data
        std::vector<Simulator::OrbitData>& getRawData() { return m_denseData; }
        const std::vector<Entity>& getRawEntities() const { return m_denseEntities; }

        void clear()
        {
            m_sparse.clear();
            m_denseData.clear();
            m_denseEntities.clear();
        }

    private:
        static constexpr size_t INVALID_INDEX = static_cast<size_t>(-1);

        // Sparse-Set map: Entity::getIndex() -> Dense Array Index
        std::vector<size_t> m_sparse;

        // Cache-dense memory layout
        std::vector<Simulator::OrbitData> m_denseData;
        std::vector<Entity> m_denseEntities;
    };

} // namespace dx3d