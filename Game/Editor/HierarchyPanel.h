#pragma once
#include <Game/Editor/UIPanel.h>
#include <DX3D/Game/Entity.h>
#include <DX3D/Game/SceneManager.h>
#include <DX3D/Game/TransformSystem.h>
#include <DX3D/Game/RenderComponentSystem.h>
#include <DX3D/InputSystem/Camera.h>

namespace dx3d
{
    class HierarchyPanel : public UIPanel
    {
    public:
        HierarchyPanel(SceneManager& scene, TransformSystem& transforms, RenderComponentSystem& renderables, Entity& selectedEntity, Camera& camera);
        void init() override;
    protected:
        void updateContent() override;
    private:
        SceneManager& m_scene;
        TransformSystem& m_transforms;
        RenderComponentSystem& m_renderables;
        Entity& m_selectedEntity;
        Entity m_lastSelectedEntity{ Entity::Null };
        Camera& m_camera;
    };
}
