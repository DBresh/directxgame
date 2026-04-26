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

namespace dx3d
{
    class GraphicsEngine final : public Base
    {
    public:
        explicit GraphicsEngine(const GraphicsEngineDesc& desc);
        virtual ~GraphicsEngine() override;

        GraphicsDevice& getGraphicsDevice() noexcept { return *m_graphicsDevice; };
        std::shared_ptr<GraphicsDevice> getGraphicsDevicePtr() const noexcept { return m_graphicsDevice; }
        LightManager* getLightManager() const noexcept { return m_renderSystem->getLightManager(); }

        void render(SceneManager& scene, Camera& camera, SwapChain& swapChain,
            const std::function<void()>& onGUI,
            const std::function<void(DeviceContext&, const DirectX::XMFLOAT4X4&, const DirectX::XMFLOAT4X4&)>& onDrawDebug = nullptr);
        
        void initUI(void* hwnd);

    private:
        void createCubeMesh();
        void compileShaders();
        void initializeThreading();
        void executeSingleDraws(SwapChain& swapChain, const Camera& camera);
        void executeInstancedDraws(SwapChain& swapChain);

    private:
        std::shared_ptr<GraphicsDevice> m_graphicsDevice{};
        DeviceContextPtr m_deviceContext{};
        GraphicsPipelineStatePtr m_pipeline{};
        GraphicsPipelineStatePtr m_instancedPipeline{};
        std::shared_ptr<InstanceBuffer> m_instanceBuffer{};
        std::shared_ptr<dx3d::GraphicsPipelineState> m_linePipeline;
        std::unique_ptr<RenderSystem> m_renderSystem;
        std::vector<DeviceContextPtr> m_deferredContexts;
        std::vector<Microsoft::WRL::ComPtr<ID3D11CommandList>> m_commandLists;
    };
}