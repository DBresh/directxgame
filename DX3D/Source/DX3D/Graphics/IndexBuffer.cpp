#include <DX3D/Graphics/IndexBuffer.h>
#include <DX3D/Graphics/GraphicsLogUtils.h>

dx3d::IndexBuffer::IndexBuffer(const IndexBufferDesc& desc, const GraphicsResourceDesc& gDesc)
	: GraphicsResource(gDesc),
	m_indexCount(desc.indexCount)
{
	if (desc.indexList == nullptr || desc.indexCount == 0)
	{
		throw std::invalid_argument("IndexBuffer: invalid index data");
	}

	m_format = desc.use32Bit ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
	auto indexSize = desc.use32Bit ? sizeof(unsigned int) : sizeof(unsigned short);

	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = static_cast<UINT>(indexSize * desc.indexCount);
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA initData{};
	initData.pSysMem = desc.indexList;

	DX3D_GRAPHICS_LOG_THROW_ON_FAIL(
		m_device.CreateBuffer(&bd, &initData, &m_buffer),
		"Failed to create IndexBuffer."
	);
}