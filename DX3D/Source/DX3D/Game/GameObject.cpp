#include <DX3D/Game/GameObject.h>
#include <algorithm>
#include <DirectXMath.h>

using namespace DirectX;

namespace dx3d
{
    void GameObject::addChild(const std::shared_ptr<GameObject>& child)
    {
        if (!child || child.get() == this) return;

        if (auto currentParent = child->parent.lock())
            currentParent->removeChild(child);

        children.push_back(child);
        child->parent = shared_from_this();
    }

    void GameObject::removeChild(const std::shared_ptr<GameObject>& child)
    {
        auto it = std::find(children.begin(), children.end(), child);
        if (it != children.end()) {
            children.erase(it);
            child->parent.reset();
        }
    }

    void GameObject::setParent(const std::shared_ptr<GameObject>& newParent)
    {
        if (newParent.get() == this) return;

        if (auto currentParent = parent.lock())
            currentParent->removeChild(shared_from_this());

        if (newParent)
            newParent->addChild(shared_from_this());
        else
            parent.reset();
    }

    Transform GameObject::getEditorWorldTransform() const
    {
        Transform result = cachedEditorTransform;

        auto parentPtr = parent.lock();
        if (!parentPtr)
            return result;

        Transform parentWorld = parentPtr->getEditorWorldTransform();

        dx3d::Vec3d localPos = cachedEditorTransform.getPosition();
        XMFLOAT3 localScale = cachedEditorTransform.getScale();
        XMFLOAT4 localQuat = cachedEditorTransform.getQuaternion();

        dx3d::Vec3d parentPos = parentWorld.getPosition();
        XMFLOAT3 parentScale = parentWorld.getScale();
        XMFLOAT4 parentQuat = parentWorld.getQuaternion();

        // scale
        XMFLOAT3 worldScale = localScale;
        if (inheritScale) {
            worldScale.x *= parentScale.x;
            worldScale.y *= parentScale.y;
            worldScale.z *= parentScale.z;
        }

        // rotation
        XMVECTOR qLocal = XMLoadFloat4(&localQuat);
        XMVECTOR qParent = XMLoadFloat4(&parentQuat);
        XMVECTOR qWorld = qLocal;
        if (inheritRotation) qWorld = XMQuaternionMultiply(qLocal, qParent);
        XMFLOAT4 worldQuat; XMStoreFloat4(&worldQuat, qWorld);

        // position
        dx3d::Vec3d worldPos = localPos;
        if (inheritPosition)
        {
            // Apply parent's rotation and scale to our local offset
            XMMATRIX S = XMMatrixScaling(parentScale.x, parentScale.y, parentScale.z);
            XMMATRIX R = XMMatrixRotationQuaternion(qParent);
            XMVECTOR localV = XMVectorSet(static_cast<float>(localPos.x), static_cast<float>(localPos.y), static_cast<float>(localPos.z), 1.0f);

            XMVECTOR rotatedScaled = XMVector3Transform(localV, S * R);
            XMFLOAT3 rsFloat;
            XMStoreFloat3(&rsFloat, rotatedScaled);

            worldPos = parentPos + dx3d::Vec3d(rsFloat.x, rsFloat.y, rsFloat.z);
        }

        result.setScale(worldScale);
        result.setPosition(worldPos);
        result.setQuaternion(worldQuat);

        return result;
    }
}
