#pragma once
#include <DX3D/Game/SceneManager.h>
#include <DX3D/Game/TransformSystem.h>

namespace dx3d {
    class HierarchyBridge {
    public:
        HierarchyBridge(SceneManager& sceneManager, TransformSystem& transformSystem)
            : m_scene(sceneManager), m_transforms(transformSystem) {
        }

        void syncEditorToRuntime();
        void syncRuntimeToEditor();
        bool validateHierarchyConsistency() const;

    private:
        SceneManager& m_scene;
        TransformSystem& m_transforms;
    };

}