#pragma once
#include <DX3D/Game/Game.h>
#include <Game/Kepler/OrbitData.h>
#include <Game/Kepler/KeplerPhysics.h>
#include <Game/Kepler/OrbitVisualizer.h>
#include <Game/Editor/UIManager.h>
#include <Game/Kepler/TimeController.h>
#include <Game/Kepler/OrbitSystem.h>
#include <Game/Editor/KeplerEditor.h>
#include <DX3D/Game/TransformSystem.h>
#include <DX3D/Game/HierarchyBridge.h>
#include <chrono>
#include <json.hpp>
#include <unordered_map>

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
        void onWindowResized(int width, int height) override;
        void onDrawDebug(DeviceContext& ctx, const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& proj) override;

    private:
        void syncCameraOrbitTarget();
        void rebuildECSStoreFromScene();
        void mirrorTransformsToSceneObjects();

    private:
        Simulator::TimeController m_timeController;
        std::unordered_map<uint32_t, Simulator::OrbitVisualizer> m_orbitVisualizers;
        std::unique_ptr<KeplerEditor> m_editor;
        std::unique_ptr<HierarchyBridge> m_hierarchyBridge;

        // temp
        struct FrameMetrics {
            double simulationTimeMs = 0.0;
            double transformTimeMs = 0.0;
            double renderBuildTimeMs = 0.0;
            size_t activeEntities = 0;
            size_t orbitCount = 0;
            size_t drawCalls = 0;
        };

        bool m_showProfiler = false;
        FrameMetrics m_metrics;
        void stressTest();
    };
}