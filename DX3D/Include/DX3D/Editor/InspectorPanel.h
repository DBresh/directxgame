#pragma once
#include <DX3D/Editor/UIPanel.h>
#include <DX3D/Game/GameObject.h>
#include <memory>

namespace dx3d
{
    class InspectorPanel : public UIPanel
    {
    public:
        InspectorPanel(std::shared_ptr<GameObject>& selectedObject);
    protected:
        void updateContent() override;
    private:
        std::shared_ptr<GameObject>& m_selectedObject;
    };
}