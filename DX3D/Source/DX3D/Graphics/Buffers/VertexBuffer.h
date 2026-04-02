#pragma once

#include <DX3D/Graphics/GraphicsResource.h>
namespace dx3d
{

	class VertexBuffer final: public GraphicsResource
	{
	public:
		VertexBuffer(const VertexBufferDesc& desc, const GraphicsResourceDesc& gDesc);
		unsigned int getVertexListSize() const noexcept;
		ID3D11Buffer* get() const noexcept { return m_buffer.Get(); }

	private:
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_buffer{};
		unsigned int m_vertexSize{};
		unsigned int m_vertexListSize{};
		friend class DeviceContext;
	};
}

