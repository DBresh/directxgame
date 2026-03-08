#pragma once
#include <DX3D/Graphics/GraphicsResource.h>
#include <wrl.h>

namespace dx3d
{
    class StructuredBuffer : public GraphicsResource
    {
    public:
        StructuredBuffer(const void* data, size_t elementSize, size_t elementCount,
            const GraphicsResourceDesc& gDesc)
            : GraphicsResource(gDesc),
            m_elementSize(static_cast<UINT>(elementSize)),
            m_elementCount(static_cast<UINT>(elementCount))
        {
            D3D11_BUFFER_DESC desc{};
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            desc.ByteWidth = static_cast<UINT>(elementSize * elementCount);
            desc.StructureByteStride = static_cast<UINT>(elementSize);
            desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

            D3D11_SUBRESOURCE_DATA init{};
            init.pSysMem = data;

            DX3D_GRAPHICS_LOG_THROW_ON_FAIL(
                m_device.CreateBuffer(&desc, &init, &m_buffer),
                "Failed to create StructuredBuffer");

            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
            srvDesc.Format = DXGI_FORMAT_UNKNOWN;
            srvDesc.Buffer.NumElements = m_elementCount;

            DX3D_GRAPHICS_LOG_THROW_ON_FAIL(
                m_device.CreateShaderResourceView(m_buffer.Get(), &srvDesc, &m_srv),
                "Failed to create SRV for StructuredBuffer");
        }

        ID3D11ShaderResourceView* getSRV() const noexcept { return m_srv.Get(); }
        UINT getCount() const noexcept { return m_elementCount; }

    private:
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_buffer;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_srv;
        UINT m_elementSize = 0;
        UINT m_elementCount = 0;
    };

    using StructuredBufferPtr = std::shared_ptr<StructuredBuffer>;
}
