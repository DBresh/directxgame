#pragma once
#include <DX3D/Math/Transform.h>
#include <DX3D/Graphics/Resources/Mesh.h>
#include <DX3D/Graphics/Buffers/ConstantBuffer.h>
#include <DX3D/Graphics/Resources/Material.h>
#include <DX3D/Graphics/Resources/ModelData.h>
#include <DX3D/Graphics/Resources/ModelGPU.h>
#include <DX3D/Game/Entity.h>
#include <memory>
#include <string>
#include <vector>

namespace dx3d {

    class GameObject : public std::enable_shared_from_this<GameObject> {
    public:
        std::string name;
        std::string tag;
        std::string modelName;

        Entity entity = Entity::Null;

        bool inheritPosition = true;
        bool inheritRotation = true;
        bool inheritScale = true;

        Transform cachedEditorTransform{};
        std::shared_ptr<ModelGPU> model;
        ConstantBufferPtr constantBuffer;

        // ====================================================================
        // EDITOR-ONLY METADATA
        // Do NOT use these methods during the runtime simulation loop.
        // Runtime hierarchy is strictly owned by TransformSystem::setParent().
        // ====================================================================
        std::weak_ptr<GameObject> parent;
        std::vector<std::shared_ptr<GameObject>> children;

        GameObject() = default;
        explicit GameObject(const std::string& objectName) : name(objectName) {}

        void addChild(const std::shared_ptr<GameObject>& child);
        void removeChild(const std::shared_ptr<GameObject>& child);

        Transform getEditorWorldTransform() const;

        bool hasMesh() const { return model != nullptr; }
        bool hasParent() const { return !parent.expired(); }
        void setParent(const std::shared_ptr<GameObject>& newParent);

        bool isChildOf(const std::shared_ptr<GameObject>& potentialParent) const {
            if (auto currentParent = parent.lock()) {
                return currentParent == potentialParent;
            }
            return false;
        }
        // ====================================================================

        std::shared_ptr<GameObject> getParent() const {
            return parent.lock();
        }

        AABB getRelativeAABB(const dx3d::Vec3d& cameraPos) const
        {
            if (model) {
                return model->boundingBox.transform(getEditorWorldTransform().getWorldMatrixRelative(cameraPos));
            }
            return AABB{};
        }

    private:

    };

}