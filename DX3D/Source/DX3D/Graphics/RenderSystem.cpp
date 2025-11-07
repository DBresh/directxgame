#include <DX3D/Graphics/RenderSystem.h>
#include <DX3D/Graphics/GraphicsLogUtils.h>
#include <DX3D/Graphics/Texture2D.h>
#include <DX3D/Graphics/StructuredBuffer.h>
#include <cstring>

namespace dx3d
{
    struct CameraData
    {
        Vec3 cameraPos;
        float ambientIntensity;
        int lightCount;
        float _padding[3];
    };


    RenderSystem::RenderSystem(std::shared_ptr<GraphicsDevice> device,
        DeviceContextPtr context)
        : m_device(std::move(device)), m_context(std::move(context))
    {
        m_cameraBuffer = m_device->createConstantBuffer({ nullptr, sizeof(CameraData) });
        m_lightManager = std::make_unique<LightManager>();
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
        CameraData cam{};
        cam.cameraPos = m_cameraPosition;
        cam.ambientIntensity = 0.08f;
        cam.lightCount = static_cast<int>(m_lightManager->getLights().size());

        m_context->updateConstantBuffer(*m_cameraBuffer, &cam, sizeof(cam));
        m_context->setPSConstantBuffer(*m_cameraBuffer, 1);
        if (m_psSampler) m_context->setPSSampler(m_psSampler.Get(), 0);

        if (m_lightManager)
        {
            m_lightManager->uploadToGPU(*m_device);
            m_lightManager->bind(*m_context, 1);
        }
    }

    void RenderSystem::drawModel(
        const ModelGPU& model,
        const ConstantBuffer& objectCB,
        const Matrix4x4& worldT,
        const Matrix4x4& viewT,
        const Matrix4x4& projT)
    {
        TransformData cbData{};
        std::memcpy(&cbData.world, &worldT.mat, sizeof(float) * 16);
        std::memcpy(&cbData.view, &viewT.mat, sizeof(float) * 16);
        std::memcpy(&cbData.projection, &projT.mat, sizeof(float) * 16);

        m_context->updateConstantBuffer(objectCB, &cbData, sizeof(cbData));
        m_context->setVSConstantBuffer(objectCB, 0);

        if (!model.submeshes.empty())
        {
            for (const auto& sm : model.submeshes)
            {
                if (!sm.mesh) continue;

                const Material* mat = nullptr;
                if (sm.materialIndex >= 0 && sm.materialIndex < static_cast<int>(model.materials.size()))
                    mat = &model.materials[sm.materialIndex];

                if (mat && mat->diffuseTexture)
                    m_context->setPSTexture(mat->diffuseTexture->getSRV(), 0);
                else
                    m_context->setPSTexture(nullptr, 0);

                m_context->setVertexBuffer(sm.mesh->getVertexBuffer());
                m_context->setIndexBuffer(sm.mesh->getIndexBuffer());
                m_context->drawIndexedTriangleList(
                    sm.mesh->getIndexBuffer().getIndexCount(), 0, 0);
            }
            return;
        }

        if (!model.mesh) return;
        m_context->setVertexBuffer(model.mesh->getVertexBuffer());
        m_context->setIndexBuffer(model.mesh->getIndexBuffer());

        for (const auto& group : model.materialGroups)
        {
            const Material* mat = nullptr;
            if (group.materialIndex >= 0 && group.materialIndex < static_cast<int>(model.materials.size()))
                mat = &model.materials[group.materialIndex];

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
