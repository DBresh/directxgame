#pragma once
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/GraphicsDevice.h>
#include <DX3D/Graphics/DeviceContext.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/RenderSystem.h>
#include <DX3D/InputSystem/Camera.h>

// temp 
#include <DX3D/Game/SceneManager.h>
#include <DirectXMath.h>
#include <DX3D/InputSystem/InputSystem.h>
#include <DX3D/Graphics/ModelImporter.h>
#include <vector>

namespace dx3d
{
    class GraphicsEngine final : public Base
    {
    public:
        explicit GraphicsEngine(const GraphicsEngineDesc& desc);
        virtual ~GraphicsEngine() override;

        GraphicsDevice& getGraphicsDevice() noexcept;
        void render(SwapChain& swapChain);

    private:
        // temp
        SceneManager m_scene;
        void createCubeMesh();

    private:
        std::shared_ptr<GraphicsDevice> m_graphicsDevice{};
        DeviceContextPtr m_deviceContext{};
        GraphicsPipelineStatePtr m_pipeline{};
        std::unique_ptr<RenderSystem> m_renderSystem;
        std::unique_ptr<Camera> m_camera;

        // temp
        std::shared_ptr<ModelImporter> m_modelImporter{};
    };
}