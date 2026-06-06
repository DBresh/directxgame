#pragma once
#include <DX3D/Game/Entity.h>
#include <DX3D/Math/Transform.h>
#include <DirectXMath.h>

namespace dx3d {

    struct HierarchyComponent {
        Entity parent = Entity::Null;
        Entity firstChild = Entity::Null;
        Entity nextSibling = Entity::Null;
        Entity prevSibling = Entity::Null;

        bool inheritPosition = true;
        bool inheritRotation = true;
        bool inheritScale = true;
    };

    struct WorldTransform {
        DirectX::XMFLOAT4X4 worldMatrix; // For the GPU
        dx3d::Vec3d position;            // For CPU logic (Camera targeting, orbit, etc.)
        DirectX::XMFLOAT4 rotation;
        DirectX::XMFLOAT3 scale;
    };

}