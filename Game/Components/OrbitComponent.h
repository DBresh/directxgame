#pragma once
#include <DX3D/Game/Component.h>
#include <Game/Kepler/OrbitData.h>
#include <imgui.h>

namespace dx3d
{
    class OrbitComponent : public Component
    {
    public:
        Simulator::OrbitData* orbit;

        OrbitComponent(Simulator::OrbitData* orbitPtr) : orbit(orbitPtr) {}

        void onInspectorGUI() override
        {
            if (ImGui::CollapsingHeader("Orbit Parameters", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::DragScalar("Body Mass", ImGuiDataType_Double, &orbit->BodyMass, 1.0f))
                {
                    orbit->isPathDirty = true;

                    for (auto& child : gameObject->children)
                    {
                        auto childOrbit = child->getComponent<OrbitComponent>();
                        if (childOrbit)
                        {
                            childOrbit->orbit->AttractorMass = orbit->BodyMass;
                            childOrbit->orbit->isPathDirty = true;
                        }
                    }
                }

                if (ImGui::DragScalar("Attractor Mass", ImGuiDataType_Double, &orbit->AttractorMass, 1.0f))
                {
                    orbit->isPathDirty = true;
                }

                float pos[3] = { (float)orbit->positionRelativeToAttractor.x, (float)orbit->positionRelativeToAttractor.y, (float)orbit->positionRelativeToAttractor.z };
                if (ImGui::DragFloat3("Relative Pos", pos, 1.0f))
                {
                    orbit->positionRelativeToAttractor = Simulator::Vec3d(pos[0], pos[1], pos[2]);
                    orbit->isPathDirty = true;
                }

                float vel[3] = { (float)orbit->velocityRelativeToAttractor.x, (float)orbit->velocityRelativeToAttractor.y, (float)orbit->velocityRelativeToAttractor.z };
                if (ImGui::DragFloat3("Relative Vel", vel, 0.1f))
                {
                    orbit->velocityRelativeToAttractor = Simulator::Vec3d(vel[0], vel[1], vel[2]);
                    orbit->isPathDirty = true;
                }
            }
        }
    };
}