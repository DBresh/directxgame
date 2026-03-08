#include <DX3D/Graphics/Buffers/ConstantBuffer.h>
#include <DX3D/Graphics/GraphicsLogUtils.h>
#include <algorithm>

namespace
{
    inline UINT AlignTo16(size_t s)
    {
        return static_cast<UINT>((s + 15u) & ~15u);
    }
}

dx3d::ConstantBuffer::ConstantBuffer(const ConstantBufferDesc& desc, const GraphicsResourceDesc& gDesc)
    : GraphicsResource(gDesc),
    m_size(AlignTo16(desc.size))
{
    if (desc.size == 0)
    {
        throw std::invalid_argument("ConstantBuffer: size must be > 0");
    }

    D3D11_BUFFER_DESC bd{};
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = m_size;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bd.MiscFlags = 0;
    bd.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA initData{};
    D3D11_SUBRESOURCE_DATA* pInit = nullptr;
    if (desc.data != nullptr)
    {
        initData.pSysMem = desc.data;
        pInit = &initData;
    }

    DX3D_GRAPHICS_LOG_THROW_ON_FAIL(
        m_device.CreateBuffer(&bd, pInit, &m_buffer),
        "Failed to create ConstantBuffer."
    );
}