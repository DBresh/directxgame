#include <DX3D/Graphics/InstanceBuffer.h>
#include <DX3D/Graphics/GraphicsDevice.h>
#include <stdexcept>

namespace dx3d
{
    InstanceBuffer::InstanceBuffer(const InstanceBufferDesc& iDesc, const GraphicsResourceDesc& gDesc)
        : GraphicsResource(gDesc), m_maxInstances(iDesc.maxInstances), m_stride(iDesc.stride)
    {
        D3D11_BUFFER_DESC desc = {};
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.ByteWidth = iDesc.stride * iDesc.maxInstances;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;

        HRESULT hr = m_device.CreateBuffer(&desc, nullptr, &m_buffer);

        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create InstanceBuffer");
        }
    }

    void InstanceBuffer::resize(unsigned int newMaxInstances)
    {
        if (newMaxInstances <= m_maxInstances) return;
        unsigned int targetInstances = std::max(newMaxInstances, m_maxInstances * 2);

        m_buffer.Reset(); // Release the old buffer

        D3D11_BUFFER_DESC desc = {};
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.ByteWidth = m_stride * targetInstances;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;

        HRESULT hr = m_device.CreateBuffer(&desc, nullptr, &m_buffer);
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to resize InstanceBuffer");
        }

        m_maxInstances = targetInstances;
    }

    unsigned int InstanceBuffer::getMaxInstances() const noexcept { return m_maxInstances; }
    unsigned int InstanceBuffer::getStride() const noexcept { return m_stride; }
}