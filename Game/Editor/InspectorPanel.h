#pragma once
#include <Game/Editor/UIPanel.h>
#include <DX3D/Game/Entity.h>
#include <DX3D/Game/SceneManager.h>
#include <DX3D/Game/TransformSystem.h>
#include <DX3D/Game/RenderComponentSystem.h>
#include <Game/Kepler/OrbitSystem.h>

namespace dx3d
{
    class InspectorPanel : public UIPanel
    {
    public:
        InspectorPanel(
            Entity& selectedEntity,
            SceneManager& scene,
            TransformSystem& transforms,
            RenderComponentSystem& renderables,
            OrbitSystem* orbitSystem
        );
        void init() override;
    protected:
        void updateContent() override;
    private:
        Entity& m_selectedEntity;
        SceneManager& m_scene;
        TransformSystem& m_transforms;
        RenderComponentSystem& m_renderables;
        OrbitSystem* m_orbitSystem = nullptr;
    };
}
