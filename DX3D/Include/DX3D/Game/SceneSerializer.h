#pragma once
#include "json.hpp"
#include <DX3D/Game/Registry.h>
#include <DX3D/Game/TransformSystem.h>
#include <Game/Kepler/OrbitSystem.h>
#include <DX3D/Game/SceneManager.h>
#include <DX3D/Graphics/Importers/AssetManager.h>
#include <unordered_map>

namespace dx3d {

    class SceneSerializer {
    public:
        SceneSerializer(Registry& registry, TransformSystem& tSys, OrbitSystem& oSys, SceneManager& sMan, AssetManager& aMan);

        bool Serialize(const std::string& filepath);
        bool Deserialize(const std::string& filepath);

    private:
        int CalculateSimDepth(Entity entity) const;
        uint32_t GetEntityIdFromOrbitEntity(Entity entity) const;

        Registry& m_registry;
        OrbitSystem& m_orbitSystem;
        TransformSystem& m_transformSystem;
        SceneManager& m_sceneManager;
        AssetManager& m_assetManager;
    };

}