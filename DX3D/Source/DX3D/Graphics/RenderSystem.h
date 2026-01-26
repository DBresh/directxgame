#pragma once
#include <memory>
#include <vector>
#include <wrl.h>
#include <DirectXMath.h>

#include <DX3D/Core/Core.h>
#include <DX3D/Core/Common.h>
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
    using namespace DirectX;

    struct MaterialDataGPU
    {
        DirectX::XMFLOAT3 albedo;
        float roughness;
        float metallic;
        DirectX::XMFLOAT3 padding;
    };

    class RenderSystem
    {
    public:
        RenderSystem(std::shared_ptr<GraphicsDevice> device,
            DeviceContextPtr context);

        void setPipeline(GraphicsPipelineStatePtr pipeline) noexcept;
        void setPSSampler(ID3D11SamplerState* sampler) noexcept;

        void beginFrame(SwapChain& swapChain, const XMFLOAT4& clearColor);
        void endFrame(GraphicsDevice& device, SwapChain& swapChain, bool vsync);

        void drawModel(
            const ModelGPU& model,
            const ConstantBuffer& objectCB,
            const XMFLOAT4X4& world,
            const XMFLOAT4X4& view,
            const XMFLOAT4X4& proj);

        void setCameraPosition(const XMFLOAT3& pos) noexcept { m_cameraPosition = pos; }

        LightManager* getLightManager() const noexcept { return m_lightManager.get(); }

        void renderShadows(SceneManager& scene);

    private:
        std::shared_ptr<GraphicsDevice> m_device;
        DeviceContextPtr                m_context;
        GraphicsPipelineStatePtr        m_pipeline;
        std::unique_ptr<LightManager>   m_lightManager;

        ConstantBufferPtr m_materialBuffer;

        Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_shadowRasterizer;

        ConstantBufferPtr m_cameraBuffer;
        XMFLOAT3          m_cameraPosition{ 0, 0, 0 };

        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_depthVS;
        ConstantBufferPtr m_depthCB;           // worldViewProj для shadow pass
        ConstantBufferPtr m_lightMatrixBuffer; // масив матриць світла (viewProj)

        Microsoft::WRL::ComPtr<ID3D11SamplerState> m_psSampler;
        Microsoft::WRL::ComPtr<ID3D11SamplerState> m_shadowSampler;

        struct TransformData
        {
            XMFLOAT4X4 world;
            XMFLOAT4X4 view;
            XMFLOAT4X4 projection;
        };
    };
}
