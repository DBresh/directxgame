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

    Transform GameObject::getWorldTransform() const
    {
        Transform result = transform;

        auto parentPtr = parent.lock();
        if (!parentPtr)
            return result;

        Transform parentWorld = parentPtr->getWorldTransform();

        // Load components
        XMFLOAT3 localPos = transform.getPosition();
        XMFLOAT3 localScale = transform.getScale();
        XMFLOAT4 localQuat = transform.getQuaternion();

        XMFLOAT3 parentPos = parentWorld.getPosition();
        XMFLOAT3 parentScale = parentWorld.getScale();
        XMFLOAT4 parentQuat = parentWorld.getQuaternion();

        // ---------------- SCALE ----------------
        XMFLOAT3 worldScale = localScale;
        if (inheritScale) {
            worldScale.x *= parentScale.x;
            worldScale.y *= parentScale.y;
            worldScale.z *= parentScale.z;
        }

        // ---------------- ROTATION ----------------
        XMVECTOR qLocal = XMLoadFloat4(&localQuat);
        XMVECTOR qParent = XMLoadFloat4(&parentQuat);

        XMVECTOR qWorld = qLocal;
        if (inheritRotation)
            qWorld = XMQuaternionMultiply(qLocal, qParent);

        XMFLOAT4 worldQuat;
        XMStoreFloat4(&worldQuat, qWorld);

        // ---------------- POSITION ----------------
        XMFLOAT3 worldPos = localPos;

        if (inheritPosition)
        {
            XMMATRIX parentM = XMLoadFloat4x4(&parentWorld.getWorldMatrix());
            XMVECTOR pos = XMVectorSet(localPos.x, localPos.y, localPos.z, 1.0f);
            pos = XMVector3Transform(pos, parentM);
            XMStoreFloat3(&worldPos, pos);
        }

        // ---------------- WRITE TO RESULT ----------------
        result.setScale(worldScale);
        result.setPosition(worldPos);
        result.setQuaternion(worldQuat);

        return result;
    }
}
