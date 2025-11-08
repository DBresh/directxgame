#pragma once
#include <DX3D/Graphics/Light.h>
#include <DX3D/Graphics/GraphicsDevice.h>
#include <DX3D/Graphics/DepthTexture2D.h>
#include <vector>

namespace dx3d
{
    class LightManager
    {
    public:
        explicit LightManager(std::shared_ptr<GraphicsDevice> device)
            : m_device(std::move(device)) {
        }

        void clear();

        void addDirectional(const Vec3& dir, const Vec3& color, float intensity, bool shadows = false);
        void addPoint(const Vec3& pos, const Vec3& color, float range, float intensity);
        void addSpot(const Vec3& pos, const Vec3& dir, float angle, const Vec3& color, float range, float intensity, bool shadows = false);

        void uploadToGPU();
        void bind(DeviceContext& context, unsigned slot = 1);

        const std::vector<Light>& getLights() const { return m_lights; }

    private:
        void createSpotShadow(Light& l);

    private:
        std::shared_ptr<GraphicsDevice> m_device;
        std::vector<Light> m_lights;
        StructuredBufferPtr m_gpuBuffer;
    };
}