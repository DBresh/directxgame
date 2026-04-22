#pragma once
#include <DX3D/Game/Game.h>
#include <Game/Kepler/OrbitData.h>
#include <Game/Kepler/KeplerPhysics.h>
#include <Game/Kepler/OrbitVisualizer.h>
#include <Game/Editor/UIManager.h>
#include <Game/Kepler/TimeController.h>

namespace dx3d
{
    class KeplerSandbox : public Game
    {
    public:
        explicit KeplerSandbox(const GameDesc& desc);
        ~KeplerSandbox() override;

        void onMouseDown(int button) override;
        void onKeyDown(int key) override;

    protected:
        void onUpdate(double dt, double fdt) override;
        void onGUI() override;
        void onDrawDebug(DeviceContext& ctx, const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& proj) override;
        void onWindowResized(int width, int height) override;

    private:
        void initUI();
        void initSandboxSimulation();

    private:
        UIManager m_uiManager;

        struct CelestialBody
        {
            std::string name;
            std::shared_ptr<dx3d::GameObject> renderObject = nullptr;
            Simulator::OrbitData orbit;
            Simulator::OrbitVisualizer visualizer;
            int parentIndex = -1;
            Simulator::Vec3d worldPosition;
        };

        std::vector<CelestialBody> m_celestialBodies;
        Simulator::TimeController m_timeController;
        
        std::shared_ptr<GameObject> m_selectedObject{ nullptr };
    };
}