#pragma once
#include <DX3D/Game/Entity.h>
#include <vector>
#include <cassert>

namespace dx3d {

    class Registry {
    public:
        Registry() = default;
        ~Registry() = default;

        // Registry represents a unique world state; copying it leads to ID fractures.
        Registry(const Registry&) = delete;
        Registry& operator=(const Registry&) = delete;

        // Pops from Free List if available, otherwise expands memory contiguously.
        Entity create() {
            uint32_t index;
            uint32_t generation;

            if (!m_freeIndices.empty()) {
                // Reuse the most recently deleted index (LIFO for hot cache)
                index = m_freeIndices.back();
                m_freeIndices.pop_back();
                generation = m_generations[index];
            }
            else {
                // Monotonic expansion
                index = static_cast<uint32_t>(m_generations.size());
                assert(index < Entity::INDEX_MASK && "CRITICAL: Engine Entity Limit Exceeded!");

                // Initialize new generation version at 0
                m_generations.push_back(0);
                generation = 0;
            }

            return Entity(index, generation);
        }

        // Invalidates the current handle and pushes the slot to the recycle queue.
        void destroy(Entity e) {
            // Safety: Double-free or invalid destruction prevention
            if (!isValid(e)) {
                return;
            }

            uint32_t index = e.getIndex();

            // Increment the internal generation version.
            // Any old Entity handles floating around memory will immediately fail isValid().
            // Bitwise AND wraps it safely back to 0 if it exceeds 4095.
            m_generations[index] = (m_generations[index] + 1) & Entity::GENERATION_MASK;

            m_freeIndices.push_back(index);
        }

        // Validates structural bounds AND generational synchronization.
        bool isValid(Entity e) const {
            if (e.isNull()) {
                return false;
            }

            uint32_t index = e.getIndex();

            // 1. Is the index within allocated bounds?
            if (index >= m_generations.size()) {
                return false;
            }

            // 2. Does the requested generation explicitly match the active live generation?
            return e.getGeneration() == m_generations[index];
        }

        // Optional utility for subsystem sizing
        size_t aliveCount() const {
            return m_generations.size() - m_freeIndices.size();
        }

        void clear() {
            m_generations.clear();
            m_freeIndices.clear();
        }

    private:
        // Tightly packed array mapping an Entity Index to its current active Generation.
        // uint16_t is sufficient because max generation is 12 bits (4095).
        std::vector<uint16_t> m_generations;

        // Cache-friendly LIFO queue for index recycling.
        std::vector<uint32_t> m_freeIndices;
    };

} // namespace dx3d