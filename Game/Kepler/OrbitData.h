#pragma once
#include <DX3D/Math/Vec3d.h>
#include <DX3D/Core/Serialization.h>
#include <json.hpp>

namespace Simulator
{
    struct OrbitData
    {
        // Masses and Constants
        double AttractorMass = 0.0;
        double BodyMass = 1.0;
        double GravConst = 1.0;
        double MG = 0.0;

        // Primary Orbital Elements
        double Eccentricity = 0.0;
        double SemiMajorAxis = 0.0;
        double SemiMinorAxis = 0.0;
        double FocalParameter = 0.0;

        // Anomalies and Time
        double Period = 0.0;
        double MeanMotion = 0.0;
        double TrueAnomaly = 0.0;
        double MeanAnomaly = 0.0;
        double EccentricAnomaly = 0.0;

        // Spatial Vectors and Bases
        dx3d::Vec3d CenterPoint;
        dx3d::Vec3d OrbitNormal;
        dx3d::Vec3d SemiMinorAxisBasis;
        dx3d::Vec3d SemiMajorAxisBasis;

        dx3d::Vec3d Periapsis;
        dx3d::Vec3d Apoapsis;
        double PeriapsisDistance = 0.0;
        double ApoapsisDistance = 0.0;

        // State Vectors
        dx3d::Vec3d positionRelativeToAttractor;
        dx3d::Vec3d absoluteWorldPosition;
        dx3d::Vec3d velocityRelativeToAttractor;
        double AttractorDistance = 0.0;

        // Additional Parameters
        double OrbitCompressionRatio = 0.0;
        double OrbitNormalDotEclipticNormal = 0.0;
        double SphereOfInfluenceRadius = 0.0;

        // Rendering state
        bool isPathDirty = true; // if path dirty, then visualisator will update orbit points once
        bool freezeColor = false;
        DirectX::XMFLOAT4 orbitColor = { 1.f,1.f,1.f,1.f };
        int ParentOrbitIndex = -1;
        bool isFrozen = false;
    };

    inline void to_json(nlohmann::json& j, const OrbitData& o) {
            j = nlohmann::json{
                {"BodyMass", o.BodyMass},
                {"AttractorMass", o.AttractorMass},
                {"GravConst", o.GravConst},
                {"positionRelativeToAttractor", o.positionRelativeToAttractor},
                {"velocityRelativeToAttractor", o.velocityRelativeToAttractor},
                {"isFrozen", o.isFrozen},
                {"ParentOrbitIndex", o.ParentOrbitIndex},
                {"freezeColor", o.freezeColor},
                {"orbitColor", nlohmann::json::array({o.orbitColor.x, o.orbitColor.y, o.orbitColor.z, o.orbitColor.w})}
            };
        }

        inline void from_json(const nlohmann::json& j, OrbitData& o) {
            if (j.contains("BodyMass")) o.BodyMass = j.at("BodyMass").get<double>();
            if (j.contains("AttractorMass")) o.AttractorMass = j.at("AttractorMass").get<double>();
            if (j.contains("GravConst")) o.GravConst = j.at("GravConst").get<double>();
            if (j.contains("positionRelativeToAttractor")) o.positionRelativeToAttractor = j.at("positionRelativeToAttractor").get<dx3d::Vec3d>();
            if (j.contains("velocityRelativeToAttractor")) o.velocityRelativeToAttractor = j.at("velocityRelativeToAttractor").get<dx3d::Vec3d>();
            if (j.contains("isFrozen")) o.isFrozen = j.at("isFrozen").get<bool>();
            if (j.contains("ParentOrbitIndex")) o.ParentOrbitIndex = j.at("ParentOrbitIndex").get<int>();
            if (j.contains("freezeColor")) o.freezeColor = j.at("freezeColor").get<bool>();
            if (j.contains("orbitColor") && j.at("orbitColor").is_array()) {
                auto colorArr = j.at("orbitColor");
                o.orbitColor.x = colorArr[0].is_number() ? colorArr[0].get<float>() : 1.0f;
                o.orbitColor.y = colorArr[1].is_number() ? colorArr[1].get<float>() : 1.0f;
                o.orbitColor.z = colorArr[2].is_number() ? colorArr[2].get<float>() : 1.0f;
                o.orbitColor.w = colorArr[3].is_number() ? colorArr[3].get<float>() : 1.0f;
            }
        }
}