#include <DX3D/Graphics/RenderSystem.h>
#include <DX3D/Graphics/GraphicsLogUtils.h>
#include <DX3D/Graphics/Texture2D.h>
#include <cstring>

namespace dx3d
{
    RenderSystem::RenderSystem(std::shared_ptr<GraphicsDevice> device,
        DeviceContextPtr context)
        : m_device(std::move(device)), m_context(std::move(context))
    {
    }

    void RenderSystem::setPipeline(GraphicsPipelineStatePtr pipeline) noexcept
    {
        m_pipeline = std::move(pipeline);
    }

    void RenderSystem::setPSSampler(ID3D11SamplerState* sampler) noexcept
    {
        m_psSampler = sampler;
    }

    void RenderSystem::beginFrame(SwapChain& swapChain, const Vec4& clearColor)
    {
        m_context->clearAndSetBackBuffer(swapChain, clearColor);
        if (m_pipeline) m_context->setGraphicsPipelineState(*m_pipeline);
        m_context->setViewportSize(swapChain.getSize());
        if (m_psSampler) m_context->setPSSampler(m_psSampler.Get(), 0);
    }

    void RenderSystem::drawMesh(const Mesh& mesh,
        const ConstantBuffer& objectCB,
        const Matrix4x4& worldT,
        const Matrix4x4& viewT,
        const Matrix4x4& projT,
        const std::vector<MaterialGroup>& groups,
        const std::vector<Material>& materials)
    {
        TransformData cbData{};
        std::memcpy(&cbData.world, &worldT.mat, sizeof(float) * 16);
        std::memcpy(&cbData.view, &viewT.mat, sizeof(float) * 16);
        std::memcpy(&cbData.projection, &projT.mat, sizeof(float) * 16);

        m_context->updateConstantBuffer(objectCB, &cbData, sizeof(cbData));
        m_context->setVSConstantBuffer(objectCB, 0);

        m_context->setVertexBuffer(mesh.getVertexBuffer());
        m_context->setIndexBuffer(mesh.getIndexBuffer());

        for (const auto& group : groups)
        {
            const Material* mat = nullptr;
            if (group.materialIndex >= 0 && group.materialIndex < static_cast<int>(materials.size()))
                mat = &materials[group.materialIndex];

            if (mat && mat->diffuseTexture)
                m_context->setPSTexture(mat->diffuseTexture->getSRV(), 0);
            else
                m_context->setPSTexture(nullptr, 0);

            m_context->drawIndexedTriangleList(group.indexCount, group.startIndex, 0);
        }
    }

    void RenderSystem::endFrame(GraphicsDevice& device, SwapChain& swapChain, bool vsync)
    {
        device.executeCommandList(*m_context);
        swapChain.present(vsync);
    }
}
