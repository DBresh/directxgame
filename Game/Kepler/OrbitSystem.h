#pragma once
#include <DX3D/Game/Entity.h>
#include <Game/Kepler/OrbitData.h>
#include <Game/Kepler/KeplerPhysics.h>
#include <vector>
#include <cassert>
#include <cmath>
#include <unordered_map>
#include <json.hpp>

namespace dx3d {

    class OrbitSystem {
    public:
        OrbitSystem() = default;
        ~OrbitSystem() = default;

        void assignOrbitToEntity(Entity e, const Simulator::OrbitData& data) {
            assert(!e.isNull() && "CRITICAL: Attempted to assign orbit to a Null Entity!");
            uint32_t entityIndex = e.getIndex();

            if (entityIndex >= m_sparse.size()) {
                m_sparse.resize(entityIndex + 1, INVALID_INDEX);
            }

            assert(m_sparse[entityIndex] == INVALID_INDEX && "Entity already has an Orbit assigned!");

            size_t denseIndex = m_denseData.size();
            m_sparse[entityIndex] = denseIndex;

            m_denseData.push_back(data);
            m_denseEntities.push_back(e);
            m_hierarchyDirty = true;
        }

        void removeOrbit(Entity e) {
            assert(hasOrbit(e) && "Entity does not have an Orbit!");

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
            m_hierarchyDirty = true;
        }

        Simulator::OrbitData& getOrbit(Entity e) {
            assert(!e.isNull() && "CRITICAL: Attempted to get orbit for a Null Entity!");
            assert(hasOrbit(e) && "Entity does not have an Orbit!");
            return m_denseData[m_sparse[e.getIndex()]];
        }

        const Simulator::OrbitData& getOrbit(Entity e) const {
            assert(!e.isNull() && "CRITICAL: Attempted to get orbit for a Null Entity!");
            assert(hasOrbit(e) && "Entity does not have an Orbit!");
            return m_denseData[m_sparse[e.getIndex()]];
        }

        bool hasOrbit(Entity e) const {
            uint32_t entityIndex = e.getIndex();
            return entityIndex < m_sparse.size() && m_sparse[entityIndex] != INVALID_INDEX;
        }

        void markHierarchyDirty() { m_hierarchyDirty = true; }

        void UpdateAll(double scaledDt) {
            ResolveHierarchyIfNeeded();
            RebuildDirtyOrbits();
            AdvanceSimulation(scaledDt);
            ResolveAbsolutePositions();
            UpdateVisualState();
        }

        size_t getSparseIndex(Entity e) const {
            assert(hasOrbit(e) && "CRITICAL: Requested Sparse Index of Entity without an Orbit component!");
            return m_sparse[e.getIndex()];
        }

        template<typename Fn>
        void forEach(Fn&& fn) {
            for (size_t i = 0; i < m_denseEntities.size(); ++i) {
                fn(m_denseEntities[i], m_denseData[i]);
            }
        }

        template<typename Fn>
        void forEach(Fn&& fn) const {
            for (size_t i = 0; i < m_denseEntities.size(); ++i) {
                fn(m_denseEntities[i], m_denseData[i]);
            }
        }

        std::vector<Simulator::OrbitData>& getRawData() { return m_denseData; }
        const std::vector<Simulator::OrbitData>& getRawData() const { return m_denseData; }
        const std::vector<Entity>& getRawEntities() const { return m_denseEntities; }



        nlohmann::json saveToJson() const
        {
            nlohmann::json out = nlohmann::json::array();
            for (size_t i = 0; i < m_denseData.size(); ++i)
            {
                nlohmann::json entry;
                entry["entityId"] = m_denseEntities[i].id;
                entry["orbit"] = m_denseData[i];
                out.push_back(entry);
            }
            return out;
        }

        void loadFromJson(const nlohmann::json& j)
        {
            clear();
            if (!j.is_array())
            {
                return;
            }

            std::unordered_map<uint32_t, Entity> remap;
            std::unordered_map<int, Entity> orbitIndexToEntity;

            for (const auto& entry : j)
            {
                if (!entry.contains("entityId") || !entry.contains("orbit"))
                {
                    continue;
                }

                Entity e(entry.at("entityId").get<uint32_t>());
                Simulator::OrbitData data = entry.at("orbit").get<Simulator::OrbitData>();
                data.ParentEntity = Entity::Null;
                assignOrbitToEntity(e, data);
                remap[e.id] = e;

                const auto& orbitJson = entry.at("orbit");
                if (orbitJson.contains("index"))
                {
                    orbitIndexToEntity[orbitJson.at("index").get<int>()] = e;
                }
            }

            for (const auto& entry : j)
            {
                if (!entry.contains("entityId") || !entry.contains("orbit"))
                {
                    continue;
                }

                Entity child(entry.at("entityId").get<uint32_t>());
                if (!hasOrbit(child))
                {
                    continue;
                }

                const auto& orbitJson = entry.at("orbit");
                if (orbitJson.contains("ParentEntityId"))
                {
                    uint32_t parentId = orbitJson.at("ParentEntityId").get<uint32_t>();
                    auto it = remap.find(parentId);
                    getOrbit(child).ParentEntity = (it != remap.end()) ? it->second : Entity::Null;
                    continue;
                }


                if (orbitJson.contains("ParentOrbitIndex"))
                {
                    int parentOrbitIndex = orbitJson.at("ParentOrbitIndex").get<int>();
                    auto it = orbitIndexToEntity.find(parentOrbitIndex);
                    getOrbit(child).ParentEntity = (it != orbitIndexToEntity.end()) ? it->second : Entity::Null;
                }
            }

            m_hierarchyDirty = true;
        }

        void clear()
        {
            m_sparse.clear();
            m_denseData.clear();
            m_denseEntities.clear();
            m_parentDense.clear();
            m_updateOrder.clear();
            m_hierarchyDirty = true;
        }

    private:
        static constexpr size_t INVALID_INDEX = static_cast<size_t>(-1);

        void ResolveHierarchyIfNeeded()
        {
            if (!m_hierarchyDirty) {
                return;
            }

            m_parentDense.resize(m_denseData.size(), INVALID_INDEX);

            for (size_t i = 0; i < m_denseData.size(); ++i)
            {
                Entity parent = m_denseData[i].ParentEntity;
                if (parent != Entity::Null && hasOrbit(parent))
                {
                    m_parentDense[i] = getSparseIndex(parent);
                }
            }

            BuildTopologicalOrder();
            m_hierarchyDirty = false;
        }

        void BuildTopologicalOrder()
        {
            const size_t count = m_denseData.size();
            m_updateOrder.clear();
            m_updateOrder.reserve(count);

            std::vector<std::vector<size_t>> children(count);
            std::vector<size_t> indegree(count, 0);

            for (size_t i = 0; i < count; ++i)
            {
                size_t parent = m_parentDense[i];
                if (parent != INVALID_INDEX && parent < count)
                {
                    children[parent].push_back(i);
                    ++indegree[i];
                }
            }

            std::vector<size_t> frontier;
            frontier.reserve(count);
            for (size_t i = 0; i < count; ++i)
            {
                if (indegree[i] == 0)
                {
                    frontier.push_back(i);
                }
            }

            for (size_t idx = 0; idx < frontier.size(); ++idx)
            {
                size_t node = frontier[idx];
                m_updateOrder.push_back(node);
                for (size_t child : children[node])
                {
                    if (--indegree[child] == 0)
                    {
                        frontier.push_back(child);
                    }
                }
            }

            if (m_updateOrder.size() != count)
            {
                m_updateOrder.clear();
                for (size_t i = 0; i < count; ++i)
                {
                    m_updateOrder.push_back(i);
                }
            }
        }

        void RebuildDirtyOrbits()
        {
            for (size_t denseIndex : m_updateOrder)
            {
                auto& orbit = m_denseData[denseIndex];
                if (!orbit.elementsDirty)
                {
                    continue;
                }

                Simulator::Kepler::CalculateOrbitStateFromOrbitalVectors(orbit);
                orbit.elementsDirty = false;
                orbit.visualDirty = true;
            }
        }

        void AdvanceSimulation(double dt)
        {
            for (size_t denseIndex : m_updateOrder)
            {
                auto& orbit = m_denseData[denseIndex];
                if (orbit.isFrozen)
                {
                    continue;
                }

                Simulator::Kepler::UpdateOrbitAnomaliesByTime(orbit, dt);

                if (std::isnan(orbit.positionRelativeToAttractor.x))
                {
                    orbit.isFrozen = true;
                    orbit.positionRelativeToAttractor = Vec3d(0.0, 0.0, 0.0);
                }
            }
        }

        void ResolveAbsolutePositions()
        {
            for (size_t denseIndex : m_updateOrder)
            {
                auto& orbit = m_denseData[denseIndex];
                size_t parentDense = m_parentDense[denseIndex];

                if (parentDense == INVALID_INDEX)
                {
                    orbit.absoluteWorldPosition = orbit.positionRelativeToAttractor;
                }
                else
                {
                    orbit.absoluteWorldPosition = m_denseData[parentDense].absoluteWorldPosition + orbit.positionRelativeToAttractor;
                }
            }
        }

        void UpdateVisualState()
        {
            for (auto& orbit : m_denseData)
            {
                if (!orbit.visualDirty)
                {
                    continue;
                }

                if (!orbit.freezeColor)
                {
                    double currentSpeed = orbit.velocityRelativeToAttractor.magnitude();
                    double referenceSpeed = std::sqrt(orbit.GravConst * orbit.AttractorMass / orbit.SemiMajorAxis);
                    float speedRatio = static_cast<float>(currentSpeed / referenceSpeed);

                    orbit.orbitColor.x = speedRatio - 0.5f;
                    orbit.orbitColor.y = 1.0f - std::abs(speedRatio - 1.0f);
                    orbit.orbitColor.z = 1.5f - speedRatio;
                    orbit.orbitColor.w = 1.0f;
                }
            }
        }

        std::vector<size_t> m_sparse;
        std::vector<Simulator::OrbitData> m_denseData;
        std::vector<Entity> m_denseEntities;

        std::vector<size_t> m_parentDense;
        std::vector<size_t> m_updateOrder;
        bool m_hierarchyDirty = true;
    };

}