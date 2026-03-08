#pragma once
#include <DX3D/Graphics/GraphicsResource.h>
#include <d3d11.h>
#include <wrl.h>

namespace dx3d
{
    struct ConstantBufferDesc;

    class ConstantBuffer final : public GraphicsResource
    {
    public:
        ConstantBuffer(const ConstantBufferDesc& desc, const GraphicsResourceDesc& gDesc);

        unsigned int getBufferSize() const noexcept { return m_size; }

    private:
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_buffer{};
        unsigned int m_size{ 0 };

        friend class DeviceContext;
        friend class GraphicsDevice;
    };
}
