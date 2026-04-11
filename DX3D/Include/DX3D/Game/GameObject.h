#pragma once
#include <DX3D/Math/Transform.h>
#include <DX3D/Graphics/Resources/Mesh.h>
#include <DX3D/Graphics/Buffers/ConstantBuffer.h>
#include <DX3D/Graphics/Resources/Material.h>
#include <DX3D/Graphics/Resources/ModelData.h>
#include <DX3D/Graphics/Resources/ModelGPU.h>
#include <DX3D/Game/Component.h>
#include <memory>
#include <string>
#include <vector>

namespace dx3d {

    class GameObject : public std::enable_shared_from_this<GameObject> {
    public:
        std::string name;
        std::string tag;

        bool inheritPosition = true;
        bool inheritRotation = true;
        bool inheritScale = true;

        Transform transform{};
        std::shared_ptr<ModelGPU> model;
        ConstantBufferPtr constantBuffer;

        std::weak_ptr<GameObject> parent;
        std::vector<std::shared_ptr<GameObject>> children;

        GameObject() = default;
        explicit GameObject(const std::string& objectName) : name(objectName) {}

        void addChild(const std::shared_ptr<GameObject>& child);
        void removeChild(const std::shared_ptr<GameObject>& child);

        Transform getWorldTransform() const;

        bool hasMesh() const { return model != nullptr; }
        bool hasParent() const { return !parent.expired(); }
        void setParent(const std::shared_ptr<GameObject>& newParent);

        bool isChildOf(const std::shared_ptr<GameObject>& potentialParent) const {
            if (auto currentParent = parent.lock()) {
                return currentParent == potentialParent;
            }
            return false;
        }

        std::shared_ptr<GameObject> getParent() const {
            return parent.lock();
        }

        AABB getWorldAABB() const {
            if (model) {
                return model->boundingBox.transform(getWorldTransform().getWorldMatrix());
            }
            return AABB{};
        }

        std::vector<std::shared_ptr<Component>> components;

        template<typename T, typename... Args>
        std::shared_ptr<T> addComponent(Args&&... args)
        {
            auto comp = std::make_shared<T>(std::forward<Args>(args)...);
            comp->gameObject = this;
            components.push_back(comp);
            return comp;
        }

        template<typename T>
        std::shared_ptr<T> getComponent() const
        {
            for (auto& comp : components)
            {
                if (auto casted = std::dynamic_pointer_cast<T>(comp))
                {
                    return casted;
                }
            }
            return nullptr;
        }

    private:

    };

}