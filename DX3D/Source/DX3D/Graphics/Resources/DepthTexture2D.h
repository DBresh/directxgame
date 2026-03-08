#pragma once
#include <DX3D/Graphics/GraphicsResource.h>
#include <wrl.h>
#include <d3d11.h>

namespace dx3d
{
    class DepthTexture2D final : public GraphicsResource
    {
    public:
        DepthTexture2D(UINT width, UINT height, const GraphicsResourceDesc& gDesc);

        ID3D11DepthStencilView* getDSV() const noexcept { return m_dsv.Get(); }
        ID3D11ShaderResourceView* getSRV() const noexcept { return m_srv.Get(); }

        UINT getWidth() const noexcept { return m_width; }
        UINT getHeight() const noexcept { return m_height; }

    private:
        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_texture;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_dsv;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_srv;

        UINT m_width{};
        UINT m_height{};
    };
}