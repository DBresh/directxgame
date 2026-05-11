#pragma once
#include <DX3D/Game/Component.h>
#include <Game/Kepler/OrbitSystem.h>
#include <Game/Kepler/OrbitData.h>
#include <Game/Components/OrbitVisualizerComponent.h>
#include <json.hpp>
#include <imgui.h>

namespace dx3d
{
    class OrbitComponent : public Component
    {
    public:
        Simulator::OrbitVisualizer visualizer;
        bool isVisible = true;
    
    public:
        OrbitComponent(Simulator::OrbitSystem* system, int orbitIndex)
            : m_system(system), m_orbitIndex(orbitIndex) {
        }

        Simulator::OrbitData& getOrbit() const
        {
            return m_system->GetOrbit(m_orbitIndex);
        }

        int getOrbitIndex() const { return m_orbitIndex; }

        void onInspectorGUI() override
        {
            if (ImGui::CollapsingHeader("Orbit Dynamics", ImGuiTreeNodeFlags_DefaultOpen))
            {
                Simulator::OrbitData& orbit = getOrbit();
                bool physicsChanged = false;

                ImGui::TextDisabled("Relative State Vectors");

                physicsChanged |= ImGui::DragScalarN("Rel Position", ImGuiDataType_Double, &orbit.positionRelativeToAttractor.x, 3, 0.5f);
                physicsChanged |= ImGui::DragScalarN("Rel Velocity", ImGuiDataType_Double, &orbit.velocityRelativeToAttractor.x, 3, 0.05f);

                ImGui::Separator();

                ImGui::TextDisabled("Physics Parameters");

                if (ImGui::Checkbox("Freeze Body (Pause Physics)", &orbit.isFrozen)) {
                    if (!orbit.isFrozen) orbit.isPathDirty = true;
                }

                double const minMass = 0.001;
                physicsChanged |= ImGui::DragScalar("Body Mass", ImGuiDataType_Double, &orbit.BodyMass, 10.0f, &minMass);
                ImGui::Text("Attractor Mass: %.2f", orbit.AttractorMass);

                if (physicsChanged) {
                    orbit.isPathDirty = true;
                }

                ImGui::Separator();

                ImGui::TextDisabled("Orbital Elements (Read-Only)");
                ImGui::Text("Eccentricity: %.4f", orbit.Eccentricity);
                ImGui::Text("Semi-Major Axis: %.2f", orbit.SemiMajorAxis);
                ImGui::Text("Apoapsis: %.2f", orbit.ApoapsisDistance);
                ImGui::Text("Periapsis: %.2f", orbit.PeriapsisDistance);
                ImGui::Text("Period: %.2f Days", orbit.Period);

                ImGui::Separator();

                ImGui::TextDisabled("Rendering");
                ImGui::Checkbox("Show Orbit Line", &isVisible);
                ImGui::Checkbox("Freeze Color", &orbit.freezeColor);
                if (orbit.freezeColor) {
                    ImGui::ColorEdit4("Orbit Color", &orbit.orbitColor.x);
                    orbit.isPathDirty = true;
                }
            }
        }

        std::string getType() const override { return "OrbitComponent"; }

        nlohmann::json serialize() const override
        {
            return nlohmann::json{
                {"orbitIndex", m_orbitIndex},
                {"isVisible", isVisible}
            };
        }

        void deserialize(const nlohmann::json& j) override
        {
            if (j.contains("orbitIndex")) m_orbitIndex = j.at("orbitIndex").get<int>();
            if (j.contains("isVisible")) isVisible = j.at("isVisible").get<bool>();
        }

    private:
        Simulator::OrbitSystem* m_system;
        int m_orbitIndex;
    };
}