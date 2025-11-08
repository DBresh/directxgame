#pragma once
#include <memory>
#include <vector>
#include <wrl.h>
#include <DX3D/Core/Core.h>
#include <DX3D/Core/Common.h>
#include <DX3D/Math/Matrix4x4.h>
#include <DX3D/Graphics/GraphicsDevice.h>
#include <DX3D/Graphics/DeviceContext.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/GraphicsPipelineState.h>
#include <DX3D/Graphics/Mesh.h>
#include <DX3D/Graphics/ConstantBuffer.h>
#include <DX3D/Graphics/ModelData.h>
#include <DX3D/Graphics/LightManager.h>
#include <DX3D/Graphics/StructuredBuffer.h>
#include <DX3D/Game/SceneManager.h>

namespace dx3d
{
    class RenderSystem
    {
    public:
        RenderSystem(std::shared_ptr<GraphicsDevice> device,
            DeviceContextPtr context);

        void setPipeline(GraphicsPipelineStatePtr pipeline) noexcept;
        void setPSSampler(ID3D11SamplerState* sampler) noexcept;

        void beginFrame(SwapChain& swapChain, const Vec4& clearColor);
        void endFrame(GraphicsDevice& device, SwapChain& swapChain, bool vsync);

        void drawModel(
            const ModelGPU& model,
            const ConstantBuffer& objectCB,
            const Matrix4x4& worldT,
            const Matrix4x4& viewT,
            const Matrix4x4& projT);

        void setCameraPosition(const Vec3& pos) noexcept { m_cameraPosition = pos; }

        LightManager* getLightManager() const noexcept { return m_lightManager.get(); }

        void renderShadows(SceneManager& scene);

    private:
        std::shared_ptr<GraphicsDevice> m_device;
        DeviceContextPtr                m_context;
        GraphicsPipelineStatePtr        m_pipeline;
        std::unique_ptr<LightManager>   m_lightManager;
        ConstantBufferPtr m_cameraBuffer;
        Vec3 m_cameraPosition{ 0, 0, 0 };

        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_depthVS;
        ConstantBufferPtr m_depthCB;
        ConstantBufferPtr m_lightMatrixBuffer;

        Microsoft::WRL::ComPtr<ID3D11SamplerState> m_psSampler;
        Microsoft::WRL::ComPtr<ID3D11SamplerState> m_shadowSampler;

        struct TransformData
        {
            float world[4][4];
            float view[4][4];
            float projection[4][4];
        };
    };
}
