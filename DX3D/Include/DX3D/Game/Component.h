#pragma once
#include <DX3D/Game/GameObject.h>
#include <json.hpp>

namespace dx3d
{
    class Component
    {
    public:
        virtual ~Component() = default;

        virtual void onInspectorGUI() {}

        virtual std::string getType() const { return "Component"; }
        virtual nlohmann::json serialize() const { return nlohmann::json::object(); }
        virtual void deserialize(const nlohmann::json& j) {}

        GameObject* gameObject = nullptr;
    };
}