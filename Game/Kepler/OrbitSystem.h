#pragma once
#include <vector>
#include <Game/Kepler/OrbitData.h>
#include <Game/Kepler/KeplerPhysics.h>

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

                Simulator::Kepler::UpdateOrbitAnomaliesByTime(orbit, dt);
            }
        }
    };
}