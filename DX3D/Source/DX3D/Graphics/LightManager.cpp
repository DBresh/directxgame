#include <DX3D/Graphics/LightManager.h>
#include <DX3D/Graphics/DeviceContext.h>
#include <DX3D/Graphics/GraphicsDevice.h>
#include <DX3D/Graphics/DepthTexture2D.h>
#include <DX3D/Core/Logger.h>

using namespace DirectX;

namespace dx3d
{
    void LightManager::clear()
    {
        m_lights.clear();
    }

    void LightManager::addDirectional(const XMFLOAT3& dir,
        const XMFLOAT3& color,
        float intensity,
        bool shadows)
    {
        Light l{};
        l.type = LightType::Directional;

        // Normalize direction
        XMVECTOR d = XMLoadFloat3(&dir);
        d = XMVector3Normalize(d);
        XMStoreFloat3(&l.direction, d);

        l.color = color;
        l.intensity = intensity;
        l.castShadows = shadows;

        m_lights.push_back(l);
    }

    void LightManager::addPoint(const XMFLOAT3& pos,
        const XMFLOAT3& color,
        float range,
        float intensity)
    {
        Light l{};
        l.type = LightType::Point;

        l.position = pos;
        l.color = color;
        l.range = range;
        l.intensity = intensity;

        m_lights.push_back(l);
    }

    static void LogMatrix(const char* name, DirectX::CXMMATRIX M)
    {
        DirectX::XMFLOAT4X4 m;
        DirectX::XMStoreFloat4x4(&m, M);
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

    void LightManager::addSpot(const XMFLOAT3& pos,
        const XMFLOAT3& dir,
        float angleDegrees,
        const XMFLOAT3& color,
        float range,
        float intensity,
        bool shadows)
    {
        Light l{};
        l.type = LightType::Spot;

        l.position = pos;

        // direction normalized
        XMVECTOR d = XMLoadFloat3(&dir);
        d = XMVector3Normalize(d);
        XMStoreFloat3(&l.direction, d);

        l.color = color;
        l.range = range;
        l.intensity = intensity;
        l.spotAngle = XMConvertToRadians(angleDegrees * 0.5f);

        DX3D_LOG_INFO("spot angle {}", l.spotAngle);

        l.castShadows = shadows;
        if (shadows)
            createSpotShadow(l);

        m_lights.push_back(std::move(l));
    }

    // ================================================================
    // Spot light shadow map creation (DirectXMath version)
    // ================================================================
    void LightManager::createSpotShadow(Light& l)
    {
        const UINT shadowSize = 1024;

        l.shadow = std::make_shared<LightShadowData>();
        l.shadow->shadowMap = m_device->createDepthTexture2D(shadowSize, shadowSize);

        // ====== Build View matrix (ROW-MAJOR DirectXMath) ======
        XMVECTOR pos = XMLoadFloat3(&l.position);
        XMVECTOR forward = XMLoadFloat3(&l.direction);
        XMVECTOR up = XMVectorSet(0, 1, 0, 0);

        // Fix up-vector when direction ~ Y axis
        {
            XMFLOAT3 d3;
            XMStoreFloat3(&d3, forward);
            if (fabs(d3.y) > 0.99f)
                up = XMVectorSet(0, 0, 1, 0);
        }

        XMMATRIX V = XMMatrixLookAtLH(pos, pos + forward, up);
        //Debug log (чистий row-major)
        //LogMatrix("SpotLight View (row-major)", V);

        // ====== Build Projection matrix (ROW-MAJOR DirectXMath) ======
        float fov = l.spotAngle; // spotAngle already half-angle
        float aspect = 1.0f;
        float znear = 0.1f;
        float zfar = l.range;

        XMMATRIX P = XMMatrixPerspectiveFovLH(fov, aspect, znear, zfar);
        //LogMatrix("SpotLight Proj (row-major)", P);

        // ====== ViewProj у ROW-MAJOR (як DirectXMath) ======
        XMMATRIX VP = V * P;
        //LogMatrix("SpotLight ViewProj (row-major, V*P)", VP);

        // ====== Store ВСЕ БЕЗ transpose ======
        XMStoreFloat4x4(&l.shadow->view, V);
        XMStoreFloat4x4(&l.shadow->proj, P);
        XMStoreFloat4x4(&l.shadow->viewProj, VP);

        // Shadow bias
        l.shadow->bias = 0.0065f;

        // Debug info
        XMFLOAT3 d3;
        XMStoreFloat3(&d3, forward);

        DX3D_LOG_INFO(
            "Created spot shadow: pos({:.2f},{:.2f},{:.2f}) dir({:.2f},{:.2f},{:.2f}) angleDeg={:.2f}",
            l.position.x, l.position.y, l.position.z,
            d3.x, d3.y, d3.z,
            XMConvertToDegrees(l.spotAngle)
        );
    }

    // ================================================================
    // Upload to GPU (StructuredBuffer<LightGPU>)
    // ================================================================
    void LightManager::uploadToGPU()
    {
        if (m_lights.empty())
            return;

        std::vector<LightGPU> gpuLights;
        gpuLights.reserve(m_lights.size());

        for (auto& l : m_lights)
        {
            LightGPU g{};
            g.type = static_cast<int>(l.type);

            g.posRange = XMFLOAT4(l.position.x, l.position.y, l.position.z, l.range);
            g.dirSpot = XMFLOAT4(l.direction.x, l.direction.y, l.direction.z, l.spotAngle);
            g.colInt = XMFLOAT4(l.color.x, l.color.y, l.color.z, l.intensity);

            gpuLights.push_back(g);
        }

        m_gpuBuffer = m_device->createStructuredBuffer(gpuLights);
    }

    void LightManager::bind(DeviceContext& context, unsigned slot)
    {
        if (!m_gpuBuffer) return;
        context.setStructuredBuffer(*m_gpuBuffer, slot);
    }
}
