#include <DX3D/Game/HierarchyBridge.h>
#include <DX3D/Core/Logger.h>

namespace dx3d {

    void HierarchyBridge::syncEditorToRuntime() {
        for (const auto& obj : m_scene.getAllObjects()) {
            if (m_transforms.hasTransform(obj->entity)) {
                m_transforms.setParent(obj->entity, Entity::Null);
            }
        }

        for (const auto& obj : m_scene.getAllObjects()) {
            if (auto parent = obj->getParent()) {
                m_transforms.setParent(obj->entity, parent->entity);
            }

            if (m_transforms.hasTransform(obj->entity)) {
                auto& hierarchy = m_transforms.getHierarchy(obj->entity);
                hierarchy.inheritPosition = obj->inheritPosition;
                hierarchy.inheritRotation = obj->inheritRotation;
                hierarchy.inheritScale = obj->inheritScale;
            }
        }
    }

    void HierarchyBridge::syncRuntimeToEditor() {
        for (const auto& obj : m_scene.getAllObjects()) {
            if (!m_transforms.hasTransform(obj->entity)) continue;

            const auto& hierarchy = m_transforms.getHierarchy(obj->entity);

            if (!hierarchy.parent.isNull()) {
                auto newParent = m_scene.findObjectByEntity(hierarchy.parent);
                if (obj->getParent() != newParent) {
                    obj->setParent(newParent);
                }
            }
            else if (obj->hasParent()) {
                obj->setParent(nullptr);
            }
        }
    }

    bool HierarchyBridge::validateHierarchyConsistency() const {
        bool consistent = true;

        for (const auto& obj : m_scene.getAllObjects()) {
            if (!m_transforms.hasTransform(obj->entity)) continue;

            Entity ecsParent = m_transforms.getHierarchy(obj->entity).parent;
            auto editorParent = obj->getParent();

            Entity editorParentEntity = editorParent ? editorParent->entity : Entity::Null;

            if (ecsParent != editorParentEntity) {
                DX3D_LOG_WARNING("Hierarchy divergence detected on Entity %d (Object: %s)!", obj->entity.id, obj->name.c_str());
                consistent = false;
            }
        }

        return consistent;
    }
}