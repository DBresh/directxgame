#include <DX3D/Graphics/InstanceBuffer.h>
#include <DX3D/Graphics/GraphicsDevice.h>
#include <stdexcept>

namespace dx3d
{
    InstanceBuffer::InstanceBuffer(unsigned int maxInstances, unsigned int stride, const GraphicsResourceDesc& gDesc)
        : GraphicsResource(gDesc), m_maxInstances(maxInstances), m_stride(stride)
    {
        D3D11_BUFFER_DESC desc = {};
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.ByteWidth = stride * maxInstances;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;

        HRESULT hr = m_device.CreateBuffer(&desc, nullptr, &m_buffer);

        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create InstanceBuffer");
        }
    }

    unsigned int InstanceBuffer::getMaxInstances() const noexcept { return m_maxInstances; }
    unsigned int InstanceBuffer::getStride() const noexcept { return m_stride; }
}