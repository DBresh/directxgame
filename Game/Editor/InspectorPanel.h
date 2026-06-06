#pragma once
#include <Game/Editor/UIPanel.h>
#include <DX3D/Game/GameObject.h>
#include <Game/Kepler/OrbitSystem.h>
#include <memory>
#include <functional>

namespace dx3d
{
    class InspectorPanel : public UIPanel
    {
    public:
        InspectorPanel(
            std::shared_ptr<GameObject>& selectedObject,
            OrbitSystem* orbitSystem,
            std::function<const Transform* (Entity)> resolveTransform = {},
            std::function<void(Entity, const Transform&)> assignTransform = {},
            std::function<bool(Entity, const Transform&)> applyTransform = {}
        );
        void init() override;
    protected:
        void updateContent() override;
    private:
        OrbitSystem* m_orbitSystem = nullptr;
        std::shared_ptr<GameObject>& m_selectedObject;
        std::function<const Transform* (Entity)> m_resolveTransform;
        std::function<void(Entity, const Transform&)> m_assignTransform;
        std::function<bool(Entity, const Transform&)> m_applyTransform;
    };
}
