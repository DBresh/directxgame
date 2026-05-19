#pragma once
#include <vector>
#include <Game/Kepler/OrbitData.h>
#include <Game/Kepler/KeplerPhysics.h>
#include <DX3D/Core/Serialization.h>
#include <json.hpp>

namespace Simulator
{
    class OrbitSystem
    {
    private:
        std::vector<OrbitData> m_orbits;

    public:
        int AddOrbit(const OrbitData& data)
        {
            m_orbits.push_back(data);
            return static_cast<int>(m_orbits.size() - 1);
        }

        OrbitData& GetOrbit(int index)
        {
            return m_orbits[index];
        }

        void UpdateAll(double dt)
        {
            for (auto& orbit : m_orbits)
            {
                if (orbit.ParentOrbitIndex != -1) {
                    double currentAttractorMass = m_orbits[orbit.ParentOrbitIndex].BodyMass;
                    if (orbit.AttractorMass != currentAttractorMass) {
                        orbit.AttractorMass = currentAttractorMass;
                        orbit.isPathDirty = true;
                    }
                }

                if (orbit.isPathDirty)
                {
                    Simulator::Kepler::CalculateOrbitStateFromOrbitalVectors(orbit);
                    if (!orbit.freezeColor)
                    {
                        double currentSpeed = orbit.velocityRelativeToAttractor.magnitude();
                        double referenceSpeed = sqrt(orbit.GravConst * orbit.AttractorMass / orbit.SemiMajorAxis);
                        float speedRatio = static_cast<float>(currentSpeed / referenceSpeed);

                        orbit.orbitColor.x = speedRatio - 0.5f;
                        orbit.orbitColor.y = 1.0f - abs(speedRatio - 1.0f);
                        orbit.orbitColor.z = 1.5f - speedRatio;
                        orbit.orbitColor.w = 1.0f;
                    }
                }

                if (!orbit.isFrozen)
                {
                    Simulator::Kepler::UpdateOrbitAnomaliesByTime(orbit, dt);

                    // temp fallback
                    if (std::isnan(orbit.positionRelativeToAttractor.x)) {
                        orbit.isFrozen = true;
                        orbit.positionRelativeToAttractor = dx3d::Vec3d(0.0, 0.0, 0.0);
                    }
                }
            }

            for (auto& orbit : m_orbits)
            {
                orbit.absoluteWorldPosition = orbit.positionRelativeToAttractor;
                int currentParentIdx = orbit.ParentOrbitIndex;

                while (currentParentIdx != -1) {
                    orbit.absoluteWorldPosition += m_orbits[currentParentIdx].positionRelativeToAttractor;
                    currentParentIdx = m_orbits[currentParentIdx].ParentOrbitIndex;
                }
            }
        }

        nlohmann::json saveToJson() const
        {
            nlohmann::json j = nlohmann::json::array();
            for (size_t i = 0; i < m_orbits.size(); ++i)
            {
                nlohmann::json orbitJson = m_orbits[i];
                orbitJson["index"] = i;
                j.push_back(orbitJson);
            }
            return j;
        }

        void loadFromJson(const nlohmann::json& jArray)
        {
            m_orbits.clear();
            if (!jArray.is_array()) return;

            m_orbits.reserve(jArray.size());

            for (const auto& j : jArray)
            {
                try
                {
                    OrbitData data = j.get<OrbitData>();
                    Kepler::CalculateOrbitStateFromOrbitalVectors(data);
                    data.isPathDirty = true;
                    data.freezeColor = true;
                    m_orbits.push_back(data);
                }
                catch (const nlohmann::json::exception& e)
                {
                    int errorIndex = j.value("index", -1);
                    DX3D_LOG_INFO("JSON Parse Crash Prevented on Orbit Index {}. Error: {}", errorIndex, e.what());
                }
            }
        }
    };
}