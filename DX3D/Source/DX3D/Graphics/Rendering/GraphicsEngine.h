#pragma once
#include <DX3D/Graphics/Rendering/GraphicsEngine.h>
#include <DX3D/Graphics/Core/GraphicsDevice.h>
#include <DX3D/Graphics/Core/DeviceContext.h>
#include <DX3D/Graphics/Core/SwapChain.h>
#include <DX3D/Graphics/Rendering/RenderSystem.h>
#include <DX3D/InputSystem/Camera.h>
#include <DX3D/Graphics/Importers/AssetManager.h>
#include <DX3D/Game/SceneManager.h>
#include <DirectXMath.h>
#include <DX3D/InputSystem/InputSystem.h>
#include <DX3D/Graphics/Buffers/InstanceBuffer.h>
#include <DX3D/Core/Common.h>

#include <functional>
#include <vector>
#include <unordered_map>

// kepler testing
#include <Game/Kepler/OrbitData.h>
#include <Game/Kepler/KeplerPhysics.h>
#include <Game/Kepler/OrbitVisualizer.h>

namespace dx3d
{
    class GraphicsEngine final : public Base
    {
    public:
        explicit GraphicsEngine(const GraphicsEngineDesc& desc);
        virtual ~GraphicsEngine() override;

        GraphicsDevice& getGraphicsDevice() noexcept;
        void render(SwapChain& swapChain, const std::function<void()>& onGUI);
        void initUI(void* hwnd);
        void onWindowResized(int width, int height);

        // temp
        std::shared_ptr<GameObject> pickObject(int mouseX, int mouseY, int screenW, int screenH);

    private:
        SceneManager m_scene;
        void createCubeMesh();
        void executeSingleDraws(SwapChain& swapChain);
        void executeInstancedDraws(SwapChain& swapChain);

        // kepler testing
        void initSandboxSimulation();

    private:
        std::shared_ptr<GraphicsDevice> m_graphicsDevice{};
        DeviceContextPtr m_deviceContext{};
        GraphicsPipelineStatePtr m_pipeline{};
        std::unique_ptr<RenderSystem> m_renderSystem;
        std::unique_ptr<Camera> m_camera;
        std::shared_ptr<AssetManager>  m_assets;
        std::vector<DeviceContextPtr> m_deferredContexts;
        std::vector<Microsoft::WRL::ComPtr<ID3D11CommandList>> m_commandLists;
        GraphicsPipelineStatePtr m_instancedPipeline{};
        std::shared_ptr<InstanceBuffer> m_testInstanceBuffer{};


        // kepler testing

        struct CelestialBody
        {
            std::string name;
            std::shared_ptr<dx3d::GameObject> renderObject = nullptr;
            Simulator::OrbitData orbit;
            Simulator::OrbitVisualizer visualizer;

            int parentIndex = -1; // -1 means it is the center of the universe (e.g., the Sun)
            Simulator::Vec3d worldPosition;
        };

        std::vector<CelestialBody> m_celestialBodies;
        float m_timeWarp = 1.0f;
        std::shared_ptr<dx3d::GraphicsPipelineState> m_linePipeline;


    };
}