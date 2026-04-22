#pragma once
#include <DX3D/Game/Component.h>
#include <Game/Kepler/OrbitVisualizer.h>
#include <Game/Kepler/OrbitData.h>
#include <imgui.h>

namespace dx3d
{
    class OrbitVisualizerComponent : public Component
    {
    public:
        Simulator::OrbitVisualizer* visualizer;
        Simulator::OrbitData* orbit;

        bool isVisible = true;

        OrbitVisualizerComponent(Simulator::OrbitVisualizer* visPtr, Simulator::OrbitData* orbitPtr)
            : visualizer(visPtr), orbit(orbitPtr) {
        }

        void onInspectorGUI() override
        {
            if (ImGui::CollapsingHeader("Orbit Visualizer", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox("Show Path", &isVisible);
                ImGui::Checkbox("Freeze Color", &orbit->freezeColor);

                if (ImGui::ColorEdit4("Line Color", &orbit->orbitColor.x))
                {
                    orbit->isPathDirty = true;
                }
            }
        }
    };
}