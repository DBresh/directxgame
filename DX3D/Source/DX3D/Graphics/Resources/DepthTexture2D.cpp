#include <DX3D/Graphics/Resources/DepthTexture2D.h>
#include <DX3D/Core/Logger.h>
#include <DX3D/Graphics/Core/GraphicsDevice.h>

using namespace Microsoft::WRL;

namespace dx3d
{
    DepthTexture2D::DepthTexture2D(UINT width, UINT height, const GraphicsResourceDesc& gDesc)
        : GraphicsResource(gDesc)
        , m_width(width), m_height(height)
    {
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R32_TYPELESS;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

        DX3D_GRAPHICS_LOG_THROW_ON_FAIL(
            m_device.CreateTexture2D(&desc, nullptr, &m_texture),
            "Failed to create DepthTexture2D"
        );

        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Texture2D.MipSlice = 0;

        DX3D_GRAPHICS_LOG_THROW_ON_FAIL(
            m_device.CreateDepthStencilView(m_texture.Get(), &dsvDesc, &m_dsv),
            "Failed to create DepthStencilView"
        );

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        DX3D_GRAPHICS_LOG_THROW_ON_FAIL(
            m_device.CreateShaderResourceView(m_texture.Get(), &srvDesc, &m_srv),
            "Failed to create ShaderResourceView for DepthTexture2D"
        );

        DX3D_LOG_INFO("Created DepthTexture2D {}x{}", width, height);
    }
}