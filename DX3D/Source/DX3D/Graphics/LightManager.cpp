#include <DX3D/Graphics/LightManager.h>
#include <DX3D/Graphics/DeviceContext.h>
#include <DX3D/Graphics/DepthTexture2D.h>
#include <DX3D/Math/Utils.h>
#include <DX3D/Core/Logger.h>

namespace dx3d
{
    void LightManager::clear()
    {
        m_lights.clear();
    }

    void LightManager::addDirectional(const Vec3& dir, const Vec3& color, float intensity, bool shadows)
    {
        Light l;
        l.type = LightType::Directional;
        l.direction = dir.normalize();
        l.color = color;
        l.intensity = intensity;
        l.castShadows = shadows;

        //if (shadows) 
            //createDirectionalShadow(l);

        m_lights.push_back(l);
    }

    void LightManager::addPoint(const Vec3& pos, const Vec3& color, float range, float intensity)
    {
        Light l;
        l.type = LightType::Point;
        l.position = pos;
        l.range = range;
        l.color = color;
        l.intensity = intensity;
        m_lights.push_back(l);
    }

    void LightManager::addSpot(const Vec3& pos, const Vec3& dir, float angle,
        const Vec3& color, float range, float intensity, bool shadows)
    {
        Light l;
        l.type = LightType::Spot;
        l.position = pos;
        l.direction = dir.normalize();
        l.spotAngle = angle;
        l.range = range;
        l.color = color;
        l.intensity = intensity;
        l.castShadows = shadows;

        if (shadows)
            createSpotShadow(l);

        m_lights.push_back(std::move(l));
    }

    void LightManager::createSpotShadow(Light& l)
    {
        const UINT shadowSize = 1024;

        l.shadow = std::make_shared<LightShadowData>();
        l.shadow->shadowMap = m_device->createDepthTexture2D(shadowSize, shadowSize);

        l.shadow->view.setLookAtLH(l.position, l.position + l.direction, Vec3(0, 1, 0));

        l.shadow->proj.setPerspectiveFovLH(
            deg2rad(l.spotAngle * 2.0f),
            1.0f,  // aspect
            0.1f,  // znear
            l.range // zfar
        );

        l.shadow->viewProj = l.shadow->view * l.shadow->proj;

        DX3D_LOG_INFO("Created shadow map for spot light ({:.1f},{:.1f},{:.1f}) - angle {:.1f}",
            l.position.x, l.position.y, l.position.z, l.spotAngle);
    }

    void LightManager::uploadToGPU()
    {
        if (m_lights.empty()) return;

        std::vector<LightGPU> gpuLights;
        gpuLights.reserve(m_lights.size());

        for (auto& l : m_lights)
        {
            LightGPU g{};
            g.type = static_cast<int>(l.type);
            g.posRange = Vec4(l.position.x, l.position.y, l.position.z, l.range);
            g.dirSpot = Vec4(l.direction.x, l.direction.y, l.direction.z, l.spotAngle);
            g.colInt = Vec4(l.color.x, l.color.y, l.color.z, l.intensity);
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
