#pragma once
#include <DX3D/Game/Component.h>
#include <Game/Kepler/OrbitVisualizer.h>
#include <imgui.h>

namespace dx3d
{
    class OrbitVisualizerComponent : public Component
    {
    public:
        Simulator::OrbitVisualizer* visualizer;
        bool isVisible = true;

        OrbitVisualizerComponent(Simulator::OrbitVisualizer* visPtr)
            : visualizer(visPtr) {}
    };
}