#pragma once
#include <DX3D/Graphics/GraphicsResource.h>
#include <DX3D/Math/Vec4.h>

namespace dx3d
{

	class DeviceContext final : GraphicsResource
	{
	public:
		explicit DeviceContext(const GraphicsResourceDesc& gDesc);
		void clearAndSetBackBuffer(const SwapChain& swapChain, const Vec4& color);
		void setGraphicsPipelineState(const GraphicsPipelineState& pipeline);
		void setVertexBuffer(const VertexBuffer& buffer);
		void setIndexBuffer(const IndexBuffer& buffer);
		void drawIndexedTriangleList(unsigned int indexCount, unsigned int startIndexLocation, unsigned int baseVertexLocation);
		void setViewportSize(const Rect& size);
		void drawTriangleList(unsigned int vertexCount, unsigned int startVertexLocation);
		void setVSConstantBuffer(const ConstantBuffer& buffer, unsigned int slot);
		void setPSConstantBuffer(const ConstantBuffer& buffer, unsigned int slot);
		void updateConstantBuffer(const ConstantBuffer& buffer, const void* data, size_t dataSize);
		void setPSTexture(ID3D11ShaderResourceView* srv, unsigned int slot = 0);
		void setPSSampler(ID3D11SamplerState* sampler, unsigned int slot = 0);

	private:
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context{};

		friend class GraphicsDevice;
	};
}

