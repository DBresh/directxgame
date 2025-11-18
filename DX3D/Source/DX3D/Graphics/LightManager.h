#pragma once
#include <DirectXMath.h>
#include <vector>
#include <memory>

#include <DX3D/Graphics/Light.h>
#include <DX3D/Graphics/StructuredBuffer.h>

namespace dx3d
{
    class GraphicsDevice;
    class DeviceContext;

    class LightManager
    {
    public:
        explicit LightManager(std::shared_ptr<GraphicsDevice> device)
            : m_device(std::move(device)) {
        }

        void clear();

        void addDirectional(const DirectX::XMFLOAT3& dir,
            const DirectX::XMFLOAT3& color,
            float intensity,
            bool shadows);

        void addPoint(const DirectX::XMFLOAT3& pos,
            const DirectX::XMFLOAT3& color,
            float range,
            float intensity);

        void addSpot(const DirectX::XMFLOAT3& pos,
            const DirectX::XMFLOAT3& dir,
            float angleDegrees,
            const DirectX::XMFLOAT3& color,
            float range,
            float intensity,
            bool shadows);

        void uploadToGPU();
        void bind(DeviceContext& context, unsigned slot);

        const std::vector<Light>& getLights() const { return m_lights; }

    private:
        void createSpotShadow(Light& l);

    private:
        std::shared_ptr<GraphicsDevice> m_device;
        std::shared_ptr<StructuredBuffer> m_gpuBuffer;
        std::vector<Light> m_lights;
    };
}
