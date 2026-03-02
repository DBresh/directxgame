#pragma once

#include <DX3D/Graphics/GraphicsResource.h>
#include <DirectXMath.h>

namespace dx3d
{
    class InstanceBuffer final : public GraphicsResource
    {
    public:
        InstanceBuffer(const InstanceBufferDesc& desc, const GraphicsResourceDesc& gDesc);

        unsigned int getMaxInstances() const noexcept;
        unsigned int getStride() const noexcept;
        ID3D11Buffer* getBuffer() const { return m_buffer.Get(); }

    private:
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_buffer{};
        unsigned int m_stride{};
        unsigned int m_maxInstances{};
    };
}