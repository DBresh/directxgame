#pragma once
#include "json.hpp"
#include <DX3D/Game/Registry.h>
#include <DX3D/Game/TransformSystem.h>
#include <Game/Kepler/OrbitSystem.h>
#include <unordered_map>

namespace dx3d {

    class SceneSerializer {
    public:
        SceneSerializer(Registry& registry, TransformSystem& tSys, OrbitSystem& oSys);

        // VK_F5 Save Pass
        bool Serialize(const std::string& filepath);

        // VK_F6 Load Pass
        bool Deserialize(const std::string& filepath);

    private:
        int CalculateSimDepth(size_t orbitIndex) const;
        uint32_t GetEntityIdFromOrbitIndex(size_t orbitIndex) const;

        Registry& m_registry;
        OrbitSystem& m_orbitSystem;
        TransformSystem& m_transformSystem;
    };

} // namespace dx3d