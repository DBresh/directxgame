#pragma once
#include <DX3D/Editor/UIPanel.h>
#include <DX3D/Game/SceneManager.h>
#include <DX3D/Game/GameObject.h>
#include <memory>

namespace dx3d
{
    class HierarchyPanel : public UIPanel
    {
    public:
        HierarchyPanel(SceneManager& scene, std::shared_ptr<GameObject>& selectedObject);
    protected:
        void updateContent() override;
    private:
        SceneManager& m_scene;
        std::shared_ptr<GameObject>& m_selectedObject;
        std::shared_ptr<GameObject> m_lastSelectedObject{ nullptr };
    };
}