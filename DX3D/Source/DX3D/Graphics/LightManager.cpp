#include <DX3D/Graphics/LightManager.h>
#include <DX3D/Graphics/DeviceContext.h>
#include <DX3D/Graphics/StructuredBuffer.h>
#include <DX3D/Graphics/Light.h>

namespace dx3d
{
    void LightManager::addDirectional(const Vec3& dir, const Vec3& color, float intensity, bool shadows)
    {
        Light l;
        l.type = LightType::Directional;
        l.direction = dir.normalize();
        l.color = color;
        l.intensity = intensity;
        l.castShadows = shadows;
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

    void LightManager::addSpot(const Vec3& pos, const Vec3& dir, float angle, const Vec3& color, float range, float intensity)
    {
        Light l;
        l.type = LightType::Spot;
        l.position = pos;
        l.direction = dir.normalize();
        l.spotAngle = angle;
        l.range = range;
        l.color = color;
        l.intensity = intensity;
        m_lights.push_back(l);
    }

    void LightManager::clear()
    {
        m_lights.clear();
    }

    void LightManager::uploadToGPU(GraphicsDevice& device)
    {
        if (m_lights.empty()) return;

        std::vector<LightGPU> gpuLights;
        gpuLights.reserve(m_lights.size());

        for (const auto& l : m_lights)
        {
            LightGPU g{};
            g.type = static_cast<int>(l.type);

            g.posRange = Vec4(l.position.x, l.position.y, l.position.z, l.range);
            g.dirSpot = Vec4(l.direction.x, l.direction.y, l.direction.z, l.spotAngle);
            g.colInt = Vec4(l.color.x, l.color.y, l.color.z, l.intensity);

            gpuLights.push_back(g);
        }

        m_gpuBuffer = device.createStructuredBuffer(gpuLights);
    }

    void LightManager::bind(DeviceContext& context, unsigned slot)
    {
        if (!m_gpuBuffer) return;
        context.setStructuredBuffer(*m_gpuBuffer, slot);
    }
}
