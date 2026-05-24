#pragma once
#include <cstdint>

namespace dx3d {

    // An Entity is purely a lightweight 32-bit handle, passed by value.
    struct Entity {
        uint32_t id;

        static constexpr uint32_t INDEX_BITS = 20;
        static constexpr uint32_t INDEX_MASK = (1 << INDEX_BITS) - 1; // 0xFFFFF

        static constexpr uint32_t GENERATION_BITS = 12;
        static constexpr uint32_t GENERATION_MASK = (1 << GENERATION_BITS) - 1; // 0xFFF
        static constexpr uint32_t GENERATION_SHIFT = INDEX_BITS;

        // Default constructor creates a strictly null/invalid entity
        constexpr Entity() : id(INDEX_MASK) {}

        constexpr explicit Entity(uint32_t rawId) : id(rawId) {}

        // Packs the index and generation into a single 32-bit integer
        constexpr Entity(uint32_t index, uint32_t generation)
            : id((index& INDEX_MASK) | ((generation & GENERATION_MASK) << GENERATION_SHIFT)) {
        }

        // Extractors
        constexpr uint32_t getIndex() const {
            return id & INDEX_MASK;
        }

        constexpr uint32_t getGeneration() const {
            return (id >> GENERATION_SHIFT) & GENERATION_MASK;
        }

        constexpr bool operator==(const Entity& other) const { return id == other.id; }
        constexpr bool operator!=(const Entity& other) const { return id != other.id; }

        // Null check validation
        constexpr bool isNull() const { return getIndex() == INDEX_MASK; }

        static const Entity Null;
    };

    // Global Null Definition
    inline const Entity Entity::Null = Entity();

}