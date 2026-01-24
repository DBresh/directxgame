#pragma once
#include <DirectXMath.h>
#include <vector>
#include <memory>

#include <DX3D/InputSystem/InputListener.h>
#include <DX3D/InputSystem/InputSystem.h>
#include <DX3D/Graphics/Light.h>
#include <DX3D/Graphics/StructuredBuffer.h>

namespace dx3d
{
    class GraphicsDevice;
    class DeviceContext;

    class LightManager : InputListener
    {
    public:
        explicit LightManager(std::shared_ptr<GraphicsDevice> device)
            : m_device(std::move(device)) {
            InputSystem::get()->addListener(this);
        }

        void clear();

        ID3D11DepthStencilView* getShadowDSV(int index) const {
            if (index >= 0 && index < m_shadowDSVs.size())
                return m_shadowDSVs[index].Get();
            return nullptr;
        }

        UINT getShadowMapSize() const { return m_shadowMapSize; }

        ID3D11ShaderResourceView* getShadowSRV() const { return m_shadowArraySRV.Get(); }

        void initShadowArray(UINT size, UINT maxLights);

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

        void onKeyDown(int key);
        void onKeyUp(int key);
        void onKeyPress(int key);

        void onMouseMove(Point deltaMouse);
        void onMouseDown(int button);
        void onMouseUp(int button);
        void onMouseWheel(int delta);

    private:
        void createSpotShadow(Light& l);

    private:
        std::shared_ptr<GraphicsDevice> m_device;
        std::shared_ptr<StructuredBuffer> m_gpuBuffer;
        std::vector<Light> m_lights;

        UINT m_shadowMapSize = 2048;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_shadowArray;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_shadowArraySRV;
        std::vector<Microsoft::WRL::ComPtr<ID3D11DepthStencilView>> m_shadowDSVs;
    };
}
