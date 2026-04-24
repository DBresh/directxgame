#pragma once
#include <DX3D/Game/Component.h>
#include <Game/Kepler/OrbitSystem.h>
#include <Game/Kepler/OrbitData.h>
#include <imgui.h>

namespace dx3d
{
    class OrbitComponent : public Component
    {
    public:

        OrbitComponent(Simulator::OrbitSystem* system, int orbitIndex)
            : m_system(system), m_orbitIndex(orbitIndex) {
        }

        Simulator::OrbitData& getOrbit() const
        {
            return m_system->GetOrbit(m_orbitIndex);
        }

        int getOrbitIndex() const { return m_orbitIndex; }

    private:
        Simulator::OrbitSystem* m_system;
        int m_orbitIndex;
    };
}