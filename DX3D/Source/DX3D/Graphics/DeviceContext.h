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
		void drawIndexedTriangleList(ui32 indexCount, ui32 startIndexLocation, ui32 baseVertexLocation);
		void setViewportSize(const Rect& size);
		void drawTriangleList(ui32 vertexCount, ui32 startVertexLocation);
		void setVSConstantBuffer(const ConstantBuffer& buffer, ui32 slot);
		void setPSConstantBuffer(const ConstantBuffer& buffer, ui32 slot);
		void updateConstantBuffer(const ConstantBuffer& buffer, const void* data, size_t dataSize);

	private:
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context{};

		friend class GraphicsDevice;
	};
}

