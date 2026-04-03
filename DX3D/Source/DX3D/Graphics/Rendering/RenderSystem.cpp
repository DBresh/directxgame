#include <DX3D/Graphics/Rendering/RenderSystem.h>
#include <DX3D/Graphics/GraphicsLogUtils.h>
#include <DX3D/Graphics/Resources/Texture2D.h>
#include <DX3D/Graphics/Buffers/StructuredBuffer.h>
#include <DX3D/Graphics/Buffers/InstanceBuffer.h>
#include <DX3D/Math/Frustrum.h>

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
        
        m_instancedDepthVS = m_device->createVertexShaderFromFile("DX3D/Assets/Shaders/ShadowDepthInstancedVS.hlsl", "VSMain");
        m_cameraBuffer = m_device->createConstantBuffer({ nullptr, sizeof(CameraData) });
        m_lightManager = std::make_unique<LightManager>(m_device);
        m_lightManager->initShadowArray(512, 4);

        m_materialBuffer = m_device->createConstantBuffer({ nullptr, sizeof(MaterialDataGPU) });

        m_depthCB = m_device->createConstantBuffer({ nullptr, sizeof(XMFLOAT4X4) });
        m_depthVS = m_device->createVertexShaderFromFile("DX3D/Assets/Shaders/ShadowDepthVS.hlsl", "VSMain");
        m_lightMatrixBuffer = m_device->createConstantBuffer(
            { nullptr, sizeof(XMFLOAT4X4) * 64 });

        m_shadowSampler = m_device->createShadowSampler();

        D3D11_RASTERIZER_DESC shadowRS = {};
        shadowRS.CullMode = D3D11_CULL_BACK;
        shadowRS.FillMode = D3D11_FILL_SOLID;
        shadowRS.DepthClipEnable = TRUE;

        // ==== AUTO BIAS SETTINGS ====
        shadowRS.DepthBias = 0.0f;
        shadowRS.SlopeScaledDepthBias = 0.0f; // More bias on steep slopes
        shadowRS.DepthBiasClamp = 0.0f;

        m_device->getD3D11Device()->CreateRasterizerState(&shadowRS, &m_shadowRasterizer);
    }

    void RenderSystem::setPipeline(GraphicsPipelineStatePtr pipeline) noexcept
    {
        m_pipeline = std::move(pipeline);
    }

    void RenderSystem::setInstancedPipeline(GraphicsPipelineStatePtr pipeline) noexcept
    {
        m_instancedPipeline = std::move(pipeline);
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

    void RenderSystem::setFrameResources(DeviceContext& context)
    {
        // 1. Bind Camera (Slot 1)
        context.setVSConstantBuffer(*m_cameraBuffer, 1);
        context.setPSConstantBuffer(*m_cameraBuffer, 1);

        // 2. Bind Lights (Slot 2)
        context.setVSConstantBuffer(*m_lightMatrixBuffer, 2);
        context.setPSConstantBuffer(*m_lightMatrixBuffer, 2);

        // 3. Bind Samplers
        if (m_psSampler)
            context.setPSSampler(m_psSampler.Get(), 0);
        if (m_shadowSampler)
            context.setPSSampler(m_shadowSampler.Get(), 1);

        // 4. Bind Shadow Map Texture
        if (m_lightManager && m_lightManager->getShadowSRV()) {
            context.setPSTexture(m_lightManager->getShadowSRV(), 2);
        }

        // 5. Bind Light Data (Structured Buffers)
        if (m_lightManager) {
            m_lightManager->bind(context, 1);
        }
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

            if (m_lightManager->getShadowSRV()) {
                m_context->setPSTexture(m_lightManager->getShadowSRV(), 2);
            }

            // Fill the matrix buffer for all shadow-casting lights
            for (int i = 0; i < (int)lights.size() && i < max_lights; ++i)
            {
                const auto& L = lights[i];

                if (L.castShadows && L.shadow)
                {
                    XMMATRIX VP_gpu = XMLoadFloat4x4(&L.shadow->viewProj);
                    XMStoreFloat4x4(&lightMatrices[i], VP_gpu);
                }
            }

            // Update Constant Buffers
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

    void RenderSystem::renderShadows(InstanceBuffer& instanceBuffer)
    {
        m_context->m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_context->m_context->RSSetState(m_shadowRasterizer.Get());

        ID3D11Buffer* nullBuf = nullptr;
        ID3D11ShaderResourceView* nullSRV = nullptr;

        for (UINT slot = 0; slot < 8; ++slot) {
            auto ctx = m_context->m_context;
            ctx->VSSetConstantBuffers(slot, 1, &nullBuf);
            ctx->PSSetConstantBuffers(slot, 1, &nullBuf);
        }
        m_context->m_context->PSSetShaderResources(2, 1, &nullSRV);

        const auto& lights = m_lightManager->getLights();

        Rect vp;
        vp.width = (float)m_lightManager->getShadowMapSize();
        vp.height = (float)m_lightManager->getShadowMapSize();
        m_context->setViewportSize(vp);

        int shadowIndex = 0;

        for (int i = 0; i < lights.size(); ++i)
        {
            const auto& light = lights[i];
            if (!light.castShadows || !light.shadow) continue;

            ID3D11DepthStencilView* dsv = m_lightManager->getShadowDSV(shadowIndex);
            if (!dsv) break;

            m_context->setDepthTargetArraySlice(dsv);
            m_context->clearDepth(*dsv);

            XMMATRIX V = XMLoadFloat4x4(&light.shadow->view);
            XMMATRIX P = XMLoadFloat4x4(&light.shadow->proj);
            XMMATRIX VP = V * P;

            if (m_pipeline) m_context->setGraphicsPipelineState(*m_pipeline);
            m_context->setVertexShader(m_depthVS.Get());
            m_context->setPixelShader(nullptr);

            for (auto* obj : m_singleDrawObjects)
            {
                const auto& model = obj->model;
                if (!model || !model->mesh) continue;

                XMMATRIX W = XMLoadFloat4x4(&obj->transform.getWorldMatrix());
                XMFLOAT4X4 cb;
                XMStoreFloat4x4(&cb, XMMatrixTranspose(W * VP));

                m_context->updateConstantBuffer(*m_depthCB, &cb, sizeof(cb));
                m_context->setVSConstantBuffer(*m_depthCB, 0);

                m_context->setVertexBuffer(model->mesh->getVertexBuffer());
                m_context->setIndexBuffer(model->mesh->getIndexBuffer());
                m_context->drawIndexedTriangleList(model->mesh->getIndexBuffer().getIndexCount(), 0, 0);
            }

            if (m_instancedPipeline) m_context->setGraphicsPipelineState(*m_instancedPipeline);

            m_context->setVertexShader(m_instancedDepthVS.Get());
            m_context->setPixelShader(nullptr);

            XMFLOAT4X4 vpFloat4x4;
            XMStoreFloat4x4(&vpFloat4x4, XMMatrixTranspose(VP));
            m_context->updateConstantBuffer(*m_depthCB, &vpFloat4x4, sizeof(vpFloat4x4));
            m_context->setVSConstantBuffer(*m_depthCB, 0);

            for (const auto& batch : m_instancedBatches)
            {
                if (!batch.model) continue;

                instanceBuffer.resize((unsigned int)batch.matrices.size());
                auto mapped = m_context->mapBuffer(instanceBuffer.getBuffer());
                memcpy(mapped.pData, batch.matrices.data(), batch.matrices.size() * sizeof(XMFLOAT4X4));
                m_context->unmapBuffer(instanceBuffer.getBuffer());

                m_context->setInstanceBuffer(instanceBuffer, 1);

                if (!batch.model->submeshes.empty()) {
                    for (const auto& sm : batch.model->submeshes) {
                        if (!sm.mesh) continue;
                        m_context->setVertexBuffer(sm.mesh->getVertexBuffer());
                        m_context->setIndexBuffer(sm.mesh->getIndexBuffer());
                        m_context->drawIndexedInstanced(sm.mesh->getIndexBuffer().getIndexCount(), (unsigned int)batch.matrices.size(), 0, 0, 0);
                    }
                }
                else if (batch.model->mesh) {
                    m_context->setVertexBuffer(batch.model->mesh->getVertexBuffer());
                    m_context->setIndexBuffer(batch.model->mesh->getIndexBuffer());
                    for (const auto& group : batch.model->materialGroups) {
                        m_context->drawIndexedInstanced(group.indexCount, (unsigned int)batch.matrices.size(), group.startIndex, 0, 0);
                    }
                }
            }

            unsigned int stride = 0;
            unsigned int offset = 0;
            m_context->m_context->IASetVertexBuffers(1, 1, &nullBuf, &stride, &offset);

            shadowIndex++;
        }

        if (m_pipeline) m_context->setGraphicsPipelineState(*m_pipeline);
        else m_context->m_context->RSSetState(nullptr);

        m_context->setDepthTarget(nullptr);
        m_context->setPSConstantBuffer(*m_lightMatrixBuffer, 2);
        m_context->setVSConstantBuffer(*m_lightMatrixBuffer, 2);
    }

    void RenderSystem::drawModel(
        DeviceContext& context,
        const ModelGPU& model,
        const ConstantBuffer& objectCB,
        const XMFLOAT4X4& world)
    {
        TransformData cbData{};

        XMMATRIX W = XMLoadFloat4x4(&world);
        XMStoreFloat4x4(&cbData.world, XMMatrixTranspose(W));

        cbData.view = m_viewGPU;
        cbData.projection = m_projGPU;

        context.updateConstantBuffer(objectCB, &cbData, sizeof(cbData));
        context.setVSConstantBuffer(objectCB, 0);

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

                MaterialDataGPU matData = {};

                if (mat)
                {
                    matData.albedo = mat->diffuseColor;
                    matData.roughness = mat->roughness;
                    matData.metallic = mat->metallic;
                }
                else
                {
                    matData.albedo = { 1.0f, 1.0f, 1.0f };
                    matData.roughness = 0.5f;
                    matData.metallic = 0.0f;
                }

                context.updateConstantBuffer(*m_materialBuffer, &matData, sizeof(matData));
                context.setPSConstantBuffer(*m_materialBuffer, 3);

                if (mat && mat->diffuseTexture)
                    context.setPSTexture(mat->diffuseTexture->getSRV(), 0);
                else
                    context.setPSTexture(nullptr, 0);

                context.setVertexBuffer(sm.mesh->getVertexBuffer());
                context.setIndexBuffer(sm.mesh->getIndexBuffer());

                context.drawIndexedTriangleList(
                    sm.mesh->getIndexBuffer().getIndexCount(), 0, 0
                );
            }

            return;
        }

        if (!model.mesh)
            return;

        context.setVertexBuffer(model.mesh->getVertexBuffer());
        context.setIndexBuffer(model.mesh->getIndexBuffer());

        for (const auto& group : model.materialGroups)
        {
            const Material* mat = nullptr;
            if (group.materialIndex >= 0 &&
                group.materialIndex < static_cast<int>(model.materials.size()))
            {
                mat = &model.materials[group.materialIndex];
            }

            MaterialDataGPU matData = {};
            if (mat) {
                matData.albedo = mat->diffuseColor;
                matData.roughness = mat->roughness;
                matData.metallic = mat->metallic;
            }
            else {
                matData.albedo = { 1.0f, 1.0f, 1.0f };
                matData.roughness = 0.5f;
                matData.metallic = 0.0f;
            }
            context.updateConstantBuffer(*m_materialBuffer, &matData, sizeof(matData));
            context.setPSConstantBuffer(*m_materialBuffer, 3);

            if (mat && mat->diffuseTexture)
                context.setPSTexture(mat->diffuseTexture->getSRV(), 0);
            else
                context.setPSTexture(nullptr, 0);

            context.drawIndexedTriangleList(group.indexCount, group.startIndex, 0);
        }
    }

    void RenderSystem::setCameraMatrices(const XMFLOAT4X4& view, const XMFLOAT4X4& proj)
    {
        XMMATRIX V = XMLoadFloat4x4(&view);
        XMMATRIX P = XMLoadFloat4x4(&proj);

        // Transpose once and store
        XMStoreFloat4x4(&m_viewGPU, XMMatrixTranspose(V));
        XMStoreFloat4x4(&m_projGPU, XMMatrixTranspose(P));
    }

    void RenderSystem::endFrame(GraphicsDevice& device, SwapChain& swapChain, bool vsync)
    {
        device.executeCommandList(*m_context);
        swapChain.present(vsync);
    }

    void RenderSystem::buildBatches(SceneManager& scene, const Camera& camera)
    {
        m_singleDrawObjects.clear();
        m_instancedBatches.clear();

        Frustum frustum;
        frustum.constructFromViewProj(camera.getViewMatrix(), camera.getProjectionMatrix());

        auto& allObjects = scene.getAllObjects();
        std::unordered_map<ModelGPU*, std::vector<GameObject*>> modelGroups;

        for (auto& objPtr : allObjects)
        {
            if (objPtr->model) {
                AABB worldBounds = objPtr->getWorldAABB();
                if (frustum.checkAABB(worldBounds)) {
                    modelGroups[objPtr->model.get()].push_back(objPtr.get());
                }
            }
        }

        for (auto& [model, objects] : modelGroups)
        {
            if (objects.size() == 1)
            {
                m_singleDrawObjects.push_back(objects[0]);
            }
            else if (objects.size() > 1)
            {
                InstancedBatch batch;
                batch.model = model;
                batch.cb = objects[0]->constantBuffer.get();
                batch.matrices.reserve(objects.size());

                for (auto* obj : objects) {
                    batch.matrices.push_back(obj->transform.getWorldMatrix());
                }
                m_instancedBatches.push_back(std::move(batch));
            }
        }
    }

    void RenderSystem::drawInstancedBatches(DeviceContext& context, InstanceBuffer& instanceBuffer)
    {
        if (m_instancedBatches.empty()) return;

        context.setGraphicsPipelineState(*m_instancedPipeline);
        context.getD3D11Context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        setFrameResources(context);

        for (const auto& batch : m_instancedBatches)
        {
            instanceBuffer.resize((unsigned int)batch.matrices.size());

            auto mapped = context.mapBuffer(instanceBuffer.getBuffer());
            memcpy(mapped.pData, batch.matrices.data(), batch.matrices.size() * sizeof(XMFLOAT4X4));
            context.unmapBuffer(instanceBuffer.getBuffer());

            drawModelInstanced(context, *batch.model, *batch.cb, instanceBuffer, (unsigned int)batch.matrices.size());
        }
    }

    void RenderSystem::drawModelInstanced(
        DeviceContext& context,
        const ModelGPU& model,
        const ConstantBuffer& objectCB,
        const InstanceBuffer& instanceBuffer,
        unsigned int instanceCount)
    {
        TransformData cbData{};

        // The vertex shader ignores the world matrix, but we pass Identity just to keep memory clean
        XMStoreFloat4x4(&cbData.world, XMMatrixIdentity());
        cbData.view = m_viewGPU;
        cbData.projection = m_projGPU;

        context.updateConstantBuffer(objectCB, &cbData, sizeof(cbData));
        context.setVSConstantBuffer(objectCB, 0);

        // Bind the dynamic instance buffer to Slot 1
        context.setInstanceBuffer(instanceBuffer, 1);

        if (!model.submeshes.empty())
        {
            for (const auto& sm : model.submeshes)
            {
                if (!sm.mesh) continue;
                const Material* mat = nullptr;
                if (sm.materialIndex >= 0 && sm.materialIndex < static_cast<int>(model.materials.size())) {
                    mat = &model.materials[sm.materialIndex];
                }

                MaterialDataGPU matData = {};
                if (mat) {
                    matData.albedo = mat->diffuseColor;
                    matData.roughness = mat->roughness;
                    matData.metallic = mat->metallic;
                }
                else {
                    matData.albedo = { 1.0f, 1.0f, 1.0f };
                    matData.roughness = 0.5f;
                    matData.metallic = 0.0f;
                }

                context.updateConstantBuffer(*m_materialBuffer, &matData, sizeof(matData));
                context.setPSConstantBuffer(*m_materialBuffer, 3);

                if (mat && mat->diffuseTexture) context.setPSTexture(mat->diffuseTexture->getSRV(), 0);
                else context.setPSTexture(nullptr, 0);

                context.setVertexBuffer(sm.mesh->getVertexBuffer()); // Slot 0
                context.setIndexBuffer(sm.mesh->getIndexBuffer());

                context.drawIndexedInstanced(
                    sm.mesh->getIndexBuffer().getIndexCount(),
                    instanceCount,
                    0, 0, 0
                );
            }
        }
        else if (model.mesh)
        {
            context.setVertexBuffer(model.mesh->getVertexBuffer()); // Slot 0
            context.setIndexBuffer(model.mesh->getIndexBuffer());

            for (const auto& group : model.materialGroups)
            {
                const Material* mat = nullptr;
                if (group.materialIndex >= 0 && group.materialIndex < static_cast<int>(model.materials.size())) {
                    mat = &model.materials[group.materialIndex];
                }
                MaterialDataGPU matData = {};
                if (mat) {
                    matData.albedo = mat->diffuseColor; matData.roughness = mat->roughness; matData.metallic = mat->metallic;
                }
                else {
                    matData.albedo = { 1.0f, 1.0f, 1.0f }; matData.roughness = 0.5f; matData.metallic = 0.0f;
                }
                context.updateConstantBuffer(*m_materialBuffer, &matData, sizeof(matData));
                context.setPSConstantBuffer(*m_materialBuffer, 3);
                if (mat && mat->diffuseTexture) context.setPSTexture(mat->diffuseTexture->getSRV(), 0);
                else context.setPSTexture(nullptr, 0);

                context.drawIndexedInstanced(group.indexCount, instanceCount, group.startIndex, 0, 0);
            }
        }

        ID3D11Buffer* nullBuf = nullptr;
        unsigned int stride = 0;
        unsigned int offset = 0;
        context.getD3D11Context()->IASetVertexBuffers(1, 1, &nullBuf, &stride, &offset);
    }
}
