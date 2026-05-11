#pragma once
#include <json.hpp>
#include <DirectXMath.h>
#include <DX3D/Math/Vec3d.h>
#include <DX3D/Math/Transform.h>

namespace DirectX
{
    inline void to_json(nlohmann::json& j, const XMFLOAT3& v) {
        j = nlohmann::json::array({ v.x, v.y, v.z });
    }
    inline void from_json(const nlohmann::json& j, XMFLOAT3& v) {
        j.at(0).get_to(v.x);
        j.at(1).get_to(v.y);
        j.at(2).get_to(v.z);
    }

    inline void to_json(nlohmann::json& j, const XMFLOAT4& v) {
        j = nlohmann::json::array({ v.x, v.y, v.z, v.w });
    }
    inline void from_json(const nlohmann::json& j, XMFLOAT4& v) {
        j.at(0).get_to(v.x);
        j.at(1).get_to(v.y);
        j.at(2).get_to(v.z);
        j.at(3).get_to(v.w);
    }
}

namespace dx3d
{
    inline void to_json(nlohmann::json& j, const Vec3d& v) {
        j = nlohmann::json::array({ v.x, v.y, v.z });
    }
    inline void from_json(const nlohmann::json& j, Vec3d& v) {
        j.at(0).get_to(v.x);
        j.at(1).get_to(v.y);
        j.at(2).get_to(v.z);
    }

    inline void to_json(nlohmann::json& j, const Transform& t) {
        j = nlohmann::json{
            {"position", t.getPosition()},
            {"rotation", t.getQuaternion()},
            {"scale", t.getScale()}
        };
    }
    inline void from_json(const nlohmann::json& j, Transform& t) {
        if (j.contains("position")) t.setPosition(j.at("position").get<Vec3d>());
        if (j.contains("rotation")) t.setQuaternion(j.at("rotation").get<DirectX::XMFLOAT4>());
        if (j.contains("scale")) t.setScale(j.at("scale").get<DirectX::XMFLOAT3>());
    }
}