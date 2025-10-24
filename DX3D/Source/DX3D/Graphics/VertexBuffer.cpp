#include <DX3D/Graphics/VertexBuffer.h>

dx3d::VertexBuffer::VertexBuffer(const VertexBufferDesc& desc, const GraphicsResourceDesc& gDesc) :
	GraphicsResource(gDesc), m_vertexSize(desc.vertexSize), m_vertexListSize(desc.vertexListSize)
{
	if (!desc.vertexList) DX3D_LOG_THROW_ERROR("No vertex list provided.");
	if (!desc.vertexListSize) DX3D_LOG_THROW_ERROR("Vertex list size must be non-zero.");
	if (!desc.vertexSize) DX3D_LOG_THROW_ERROR("Vertex size must be non-zero.");

	D3D11_BUFFER_DESC buffDesc{};
	buffDesc.ByteWidth = desc.vertexListSize * desc.vertexSize;
	buffDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA initData{};
	initData.pSysMem = desc.vertexList;

	DX3D_GRAPHICS_LOG_THROW_ON_FAIL(
		m_device.CreateBuffer(&buffDesc, &initData, &m_buffer)
		, "CreateBuffer failed.");
}

unsigned int dx3d::VertexBuffer::getVertexListSize() const noexcept
{
	return m_vertexListSize;
}
