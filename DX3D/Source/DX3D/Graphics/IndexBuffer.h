#pragma once
#include <DX3D/Graphics/GraphicsResource.h>

namespace dx3d
{
    class IndexBuffer final : public GraphicsResource
    {
    public:
        IndexBuffer(const IndexBufferDesc& desc, const GraphicsResourceDesc& gDesc);

        unsigned int getIndexCount() const noexcept { return m_indexCount; }
        bool is32Bit() const noexcept { return m_format == DXGI_FORMAT_R32_UINT; }

    private:
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_buffer{};
        DXGI_FORMAT m_format{ DXGI_FORMAT_UNKNOWN };
        unsigned int m_indexCount{};

        friend class DeviceContext;
    };
}
