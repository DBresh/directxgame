#pragma once
#include <DX3D/Math/Transform.h>
#include <DX3D/Graphics/Mesh.h>
#include <DX3D/Graphics/ConstantBuffer.h>
#include <DX3D/Graphics/Material.h>
#include <DX3D/Graphics/ModelData.h>
#include <memory>
#include <string>
#include <vector>

namespace dx3d {

    class GameObject : public std::enable_shared_from_this<GameObject> {
    public:
        std::string name;
        std::string tag;

        std::vector<Material> materials;
        std::vector<MaterialGroup> materialGroups;

        bool inheritPosition = true;
        bool inheritRotation = false;
        bool inheritScale = true;

        Transform transform{};
        std::shared_ptr<Mesh> mesh;
        ConstantBufferPtr constantBuffer;

        std::weak_ptr<GameObject> parent;
        std::vector<std::shared_ptr<GameObject>> children;

        GameObject() = default;
        explicit GameObject(const std::string& objectName) : name(objectName) {}

        void addChild(const std::shared_ptr<GameObject>& child);
        void removeChild(const std::shared_ptr<GameObject>& child);

        Transform getWorldTransform() const;

        bool hasMesh() const { return mesh != nullptr; }
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

    private:

    };

}