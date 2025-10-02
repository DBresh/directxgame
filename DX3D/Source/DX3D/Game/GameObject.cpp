#include <DX3D/Game/GameObject.h>
#include <algorithm>

namespace dx3d {

    void GameObject::addChild(const std::shared_ptr<GameObject>& child) {
        if (!child || child.get() == this) {
            return;
        }

        if (auto currentParent = child->parent.lock()) {
            currentParent->removeChild(child);
        }

        children.push_back(child);
        child->parent = shared_from_this();
    }

    void GameObject::removeChild(const std::shared_ptr<GameObject>& child) {
        auto it = std::find(children.begin(), children.end(), child);
        if (it != children.end()) {
            children.erase(it);
            child->parent.reset();
        }
    }

    void GameObject::setParent(const std::shared_ptr<GameObject>& newParent) {
        if (newParent.get() == this) {
            return;
        }

        if (auto currentParent = parent.lock()) {
            currentParent->removeChild(shared_from_this());
        }

        if (newParent) {
            newParent->addChild(shared_from_this());
        }
        else {
            parent.reset();
        }
    }

    Transform GameObject::getWorldTransform() const {
        Transform worldTransform = transform;

        if (auto parentPtr = parent.lock()) {
            Transform parentWorldTransform = parentPtr->getWorldTransform();
            Matrix4x4 parentWorldMatrix = parentWorldTransform.getWorldMatrix();

            Vec3 worldPosition = transform.getPosition();
            Vec3 worldScale = transform.getScale();
            Quaternion worldRotation = Quaternion::fromEuler(transform.getRotation());

            if (inheritPosition) {
                worldPosition = parentWorldMatrix.transformPoint(transform.getPosition());
            }
            else {
                worldPosition = parentWorldTransform.getPosition() + transform.getPosition();
            }

            if (inheritRotation) {
                Quaternion parentRot = Quaternion::fromEuler(parentWorldTransform.getRotation());
                worldRotation = parentRot * Quaternion::fromEuler(transform.getRotation());
            }

            if (inheritScale) {
                worldScale = parentWorldTransform.getScale() * transform.getScale();
            }

            worldTransform.setPosition(worldPosition);
            worldTransform.setScale(worldScale);
            worldTransform.setRotation(worldRotation.toEuler());
        }

        return worldTransform;
    }
}