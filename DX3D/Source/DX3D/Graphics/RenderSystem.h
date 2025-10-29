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
        void drawMesh(const Mesh& mesh,
            const ConstantBuffer& objectCB,
            const Matrix4x4& worldT,
            const Matrix4x4& viewT,
            const Matrix4x4& projT,
            const std::vector<MaterialGroup>& groups,
            const std::vector<Material>& materials);
        void endFrame(GraphicsDevice& device, SwapChain& swapChain, bool vsync);

    private:
        std::shared_ptr<GraphicsDevice> m_device;
        DeviceContextPtr                m_context;
        GraphicsPipelineStatePtr        m_pipeline;
        Microsoft::WRL::ComPtr<ID3D11SamplerState> m_psSampler;

        struct TransformData
        {
            float world[4][4];
            float view[4][4];
            float projection[4][4];
        };
    };
}
