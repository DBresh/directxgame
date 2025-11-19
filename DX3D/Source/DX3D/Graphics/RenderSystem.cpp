#include <DX3D/Graphics/RenderSystem.h>
#include <DX3D/Graphics/GraphicsLogUtils.h>
#include <DX3D/Graphics/Texture2D.h>
#include <DX3D/Graphics/StructuredBuffer.h>

#include <DirectXMath.h>
#include <cstring>

namespace dx3d
{
    using namespace DirectX;

    struct CameraData
    {
        XMFLOAT3 cameraPos;
        float    ambientIntensity;
        int      lightCount;
        XMFLOAT3 _padding; // дл€ 16-byte alignment
    };

    RenderSystem::RenderSystem(std::shared_ptr<GraphicsDevice> device,
        DeviceContextPtr context)
        : m_device(std::move(device)), m_context(std::move(context))
    {
        m_cameraBuffer = m_device->createConstantBuffer({ nullptr, sizeof(CameraData) });
        m_lightManager = std::make_unique<LightManager>(m_device);
        m_depthCB = m_device->createConstantBuffer({ nullptr, sizeof(XMFLOAT4X4) });
        m_depthVS = m_device->createVertexShaderFromFile("DX3D/Assets/Shaders/ShadowDepthVS.hlsl", "VSMain");
        m_lightMatrixBuffer = m_device->createConstantBuffer(
            { nullptr, sizeof(XMFLOAT4X4) * 64 });

        m_shadowSampler = m_device->createShadowSampler();
    }

    void RenderSystem::setPipeline(GraphicsPipelineStatePtr pipeline) noexcept
    {
        m_pipeline = std::move(pipeline);
    }

    void RenderSystem::setPSSampler(ID3D11SamplerState* sampler) noexcept
    {
        m_psSampler = sampler;
    }

    static void LogMatrix(const char* name, CXMMATRIX M)
    {
        XMFLOAT4X4 m;
        XMStoreFloat4x4(&m, M);
        DX3D_LOG_INFO(
            "Matrix Log: {} [as Row-Major]\n"
            "  [{: 8.2f}, {: 8.2f}, {: 8.2f}, {: 8.2f}]\n"
            "  [{: 8.2f}, {: 8.2f}, {: 8.2f}, {: 8.2f}]\n"
            "  [{: 8.2f}, {: 8.2f}, {: 8.2f}, {: 8.2f}]\n"
            "  [{: 8.2f}, {: 8.2f}, {: 8.2f}, {: 8.2f}]",
            name,
            m._11, m._12, m._13, m._14,
            m._21, m._22, m._23, m._24,
            m._31, m._32, m._33, m._34,
            m._41, m._42, m._43, m._44
        );
    }

    void RenderSystem::beginFrame(SwapChain& swapChain, const XMFLOAT4& clearColor)
    {
        m_context->clearAndSetBackBuffer(swapChain, clearColor);

        if (m_pipeline)
            m_context->setGraphicsPipelineState(*m_pipeline);

        // PS sampler 0 Ц звичайний
        if (m_psSampler)
            m_context->setPSSampler(m_psSampler.Get(), 0);

        // PS sampler 1 Ц shadow sampler (Comparison / ClampBorder)
        if (m_shadowSampler)
            m_context->setPSSampler(m_shadowSampler.Get(), 1);

        m_context->setViewportSize(swapChain.getSize());

        if (m_lightManager)
        {
            constexpr int max_lights = 64;
            XMFLOAT4X4 lightMatrices[max_lights];

            for (int i = 0; i < max_lights; ++i)
                XMStoreFloat4x4(&lightMatrices[i], XMMatrixIdentity());

            const auto& lights = m_lightManager->getLights();

            int shadowTextureLightIndex = -1;

            for (int i = 0; i < (int)lights.size() && i < max_lights; ++i)
            {
                const auto& L = lights[i];

                // shadowMap t2
                if (shadowTextureLightIndex < 0 &&
                    L.type == LightType::Spot &&
                    L.castShadows &&
                    L.shadow && L.shadow->shadowMap)
                {
                    m_context->setPSTexture(L.shadow->shadowMap->getSRV(), 2);
                    shadowTextureLightIndex = i;
                }

                if (L.castShadows && L.shadow)
                {
                    XMMATRIX VP_gpu = XMLoadFloat4x4(&L.shadow->viewProj);
                    XMStoreFloat4x4(&lightMatrices[i], VP_gpu);

                    //LogMatrix("GPU light VP (as loaded for b2)", VP_gpu);
                }
            }

            m_context->updateConstantBuffer(
                *m_lightMatrixBuffer,
                lightMatrices,
                sizeof(XMFLOAT4X4) * max_lights
            );

            m_context->setPSConstantBuffer(*m_lightMatrixBuffer, 2);
            m_context->setVSConstantBuffer(*m_lightMatrixBuffer, 2);

            m_lightManager->uploadToGPU();
            m_lightManager->bind(*m_context, 1);
        }

        CameraData cam{};
        cam.cameraPos = m_cameraPosition;
        cam.ambientIntensity = 0.25f;
        cam.lightCount = (int)m_lightManager->getLights().size();
        cam._padding = XMFLOAT3(0, 0, 0);

        m_context->updateConstantBuffer(*m_cameraBuffer, &cam, sizeof(cam));
        m_context->setPSConstantBuffer(*m_cameraBuffer, 1);
    }

    void RenderSystem::renderShadows(SceneManager& scene)
    {
        if (m_pipeline)
            m_context->setGraphicsPipelineState(*m_pipeline);

        ID3D11Buffer* nullBuf = nullptr;
        for (UINT slot = 0; slot < 8; ++slot)
        {
            m_context->m_context->VSSetConstantBuffers(slot, 1, &nullBuf);
            m_context->m_context->PSSetConstantBuffers(slot, 1, &nullBuf);
        }

        const auto& lights = m_lightManager->getLights();
        //DX3D_LOG_INFO("RenderShadows: lights count = {}", (int)lights.size());

        for (auto& light : lights)
        {
            if (!light.castShadows || !light.shadow)
                continue;

            auto& shadow = light.shadow;
            auto depthTex = shadow->shadowMap;
            if (!depthTex)
                continue;

            float spotDeg = (light.type == LightType::Spot)
                ? XMConvertToDegrees(light.spotAngle * 2.0f) // full FOV for debug
                : 0.0f;

            //DX3D_LOG_INFO(
            //    "ShadowPass: light type={} pos({:.2f},{:.2f},{:.2f}) dir({:.2f},{:.2f},{:.2f}) range={:.2f} spotFovDeg={:.2f}",
            //    (int)light.type,
            //    light.position.x, light.position.y, light.position.z,
            //    light.direction.x, light.direction.y, light.direction.z,
            //    light.range,
            //    spotDeg
            //);

            Rect vp;
            vp.width = depthTex->getWidth();
            vp.height = depthTex->getHeight();
            m_context->setViewportSize(vp);

            m_context->setDepthTarget(depthTex->getDSV());
            m_context->clearDepth(*depthTex->getDSV());

            m_context->setVertexShader(m_depthVS.Get());
            m_context->setPixelShader(nullptr);

            XMMATRIX V = XMLoadFloat4x4(&shadow->view);
            XMMATRIX P = XMLoadFloat4x4(&shadow->proj);

            //LogMatrix("ShadowPass View (row-major)", V);
            //LogMatrix("ShadowPass Proj (row-major)", P);

            for (auto& objPtr : scene.getAllObjects())
            {
                auto& obj = *objPtr;
                const auto& model = obj.model;
                if (!model || !model->mesh)
                    continue;

                XMMATRIX W = XMLoadFloat4x4(&obj.transform.getWorldMatrix());
                XMMATRIX WVP = W * V * P;
                XMMATRIX WVP_t = XMMatrixTranspose(WVP);
                //LogMatrix("ShadowPass WVP_t (GPU column-major)", WVP_t);

                XMFLOAT4X4 cb;
                XMStoreFloat4x4(&cb, WVP_t);

                m_context->updateConstantBuffer(*m_depthCB, &cb, sizeof(cb));
                m_context->setVSConstantBuffer(*m_depthCB, 0);

                m_context->setVertexBuffer(model->mesh->getVertexBuffer());
                m_context->setIndexBuffer(model->mesh->getIndexBuffer());
                m_context->drawIndexedTriangleList(
                    model->mesh->getIndexBuffer().getIndexCount(), 0, 0
                );
            }
        }

        m_context->setDepthTarget(nullptr);
        m_context->setPSConstantBuffer(*m_lightMatrixBuffer, 2);
        m_context->setVSConstantBuffer(*m_lightMatrixBuffer, 2);
    }


    void RenderSystem::drawModel(
        const ModelGPU& model,
        const ConstantBuffer& objectCB,
        const XMFLOAT4X4& world,
        const XMFLOAT4X4& view,
        const XMFLOAT4X4& proj)
    {
        TransformData cbData{};

        XMMATRIX W = XMLoadFloat4x4(&world);
        //DX3D_LOG_INFO("MainPass W:");
        //LogMatrix("main W", W);
        XMMATRIX V = XMLoadFloat4x4(&view);
        XMMATRIX P = XMLoadFloat4x4(&proj);

        XMMATRIX W_t = XMMatrixTranspose(W);
        XMMATRIX V_t = XMMatrixTranspose(V);
        XMMATRIX P_t = XMMatrixTranspose(P);

        //LogMatrix("DrawModel WORLD_t (GPU column-major)", W_t);
        //LogMatrix("DrawModel VIEW_t  (GPU column-major)", V_t);
        //LogMatrix("DrawModel PROJ_t  (GPU column-major)", P_t);

        XMStoreFloat4x4(&cbData.world, W_t);
        XMStoreFloat4x4(&cbData.view, V_t);
        XMStoreFloat4x4(&cbData.projection, P_t);

        m_context->updateConstantBuffer(objectCB, &cbData, sizeof(cbData));
        m_context->setVSConstantBuffer(objectCB, 0);

        if (!model.submeshes.empty())
        {
            for (const auto& sm : model.submeshes)
            {
                if (!sm.mesh)
                    continue;

                const Material* mat = nullptr;
                if (sm.materialIndex >= 0 &&
                    sm.materialIndex < static_cast<int>(model.materials.size()))
                {
                    mat = &model.materials[sm.materialIndex];
                }

                if (mat && mat->diffuseTexture)
                    m_context->setPSTexture(mat->diffuseTexture->getSRV(), 0);
                else
                    m_context->setPSTexture(nullptr, 0);

                m_context->setVertexBuffer(sm.mesh->getVertexBuffer());
                m_context->setIndexBuffer(sm.mesh->getIndexBuffer());

                m_context->drawIndexedTriangleList(
                    sm.mesh->getIndexBuffer().getIndexCount(), 0, 0
                );
            }

            return;
        }

        if (!model.mesh)
            return;

        m_context->setVertexBuffer(model.mesh->getVertexBuffer());
        m_context->setIndexBuffer(model.mesh->getIndexBuffer());

        for (const auto& group : model.materialGroups)
        {
            const Material* mat = nullptr;
            if (group.materialIndex >= 0 &&
                group.materialIndex < static_cast<int>(model.materials.size()))
            {
                mat = &model.materials[group.materialIndex];
            }

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
