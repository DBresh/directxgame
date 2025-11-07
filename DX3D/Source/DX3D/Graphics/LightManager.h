#pragma once
#include <DX3D/Graphics/Light.h>
#include <DX3D/Graphics/GraphicsDevice.h>
#include <vector>

namespace dx3d
{
    class LightManager
    {
    public:
        void addDirectional(const Vec3& dir, const Vec3& color, float intensity, bool shadows = false);
        void addPoint(const Vec3& pos, const Vec3& color, float range, float intensity);
        void addSpot(const Vec3& pos, const Vec3& dir, float angle, const Vec3& color, float range, float intensity);

        void clear();

        void uploadToGPU(GraphicsDevice& device);
        void bind(DeviceContext& context, unsigned slot = 1);

        const std::vector<Light>& getLights() const { return m_lights; }

    private:
        std::vector<Light> m_lights;
        StructuredBufferPtr m_gpuBuffer;
    };
}
