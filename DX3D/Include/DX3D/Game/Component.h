#pragma once
#include <DX3D/Game/GameObject.h>

namespace dx3d
{
    class Component
    {
    public:
        virtual ~Component() = default;

        virtual void onInspectorGUI() {}

        GameObject* gameObject = nullptr;
    };
}