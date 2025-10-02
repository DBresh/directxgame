#include <DX3D/Graphics/DeviceContext.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/GraphicsPipelineState.h>
#include <DX3D/Graphics/VertexBuffer.h>
#include <DX3D/Graphics/IndexBuffer.h>
#include <DX3D/Graphics/ConstantBuffer.h>

namespace dx3d
{
	DeviceContext::DeviceContext(const GraphicsResourceDesc& gDesc) : GraphicsResource(gDesc)
	{
		DX3DGraphicsLogThrowOnFail(m_device.CreateDeferredContext(0, &m_context), "CreateDeferredContext failed.");
	}

	void DeviceContext::clearAndSetBackBuffer(const SwapChain& swapChain, const Vec4& color)
	{
		f32 fColor[] = { color.x, color.y, color.z, color.w };
		
		m_context->ClearRenderTargetView(swapChain.m_rtv.Get(), fColor);
		m_context->ClearDepthStencilView(swapChain.m_dsv.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

		auto rtv = swapChain.m_rtv.Get();
		auto dsv = swapChain.m_dsv.Get();
		m_context->OMSetRenderTargets(1, &rtv, dsv);
	}

	void DeviceContext::setGraphicsPipelineState(const GraphicsPipelineState& pipeline)
	{
		m_context->IASetInputLayout(pipeline.m_layout.Get());
		m_context->VSSetShader(pipeline.m_vs.Get(), nullptr, 0);
		m_context->PSSetShader(pipeline.m_ps.Get(), nullptr, 0);
		m_context->OMSetDepthStencilState(pipeline.m_depthStencilState.Get(), 0);
	}

	void DeviceContext::setVertexBuffer(const VertexBuffer& buffer)
	{
		auto buf = buffer.m_buffer.Get();
		auto stride = buffer.m_vertexSize;
		auto offset = 0u;
		m_context->IASetVertexBuffers(0, 1, &buf, &stride, &offset);
	}

	void DeviceContext::setIndexBuffer(const IndexBuffer& buffer)
	{
		m_context->IASetIndexBuffer(buffer.m_buffer.Get(), buffer.m_format, 0);
	}

	void DeviceContext::drawIndexedTriangleList(ui32 indexCount, ui32 startIndexLocation, ui32 baseVertexLocation)
	{
		m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_context->DrawIndexed(indexCount, startIndexLocation, baseVertexLocation);
	}

	void DeviceContext::setViewportSize(const Rect& size)
	{
		D3D11_VIEWPORT vp{};
		vp.Width = static_cast<f32>(size.width);
		vp.Height = static_cast<f32>(size.height);
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;

		m_context->RSSetViewports(1, &vp);
	}

	void DeviceContext::drawTriangleList(ui32 vertexCount, ui32 startVertexLocation)
	{
		m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_context->Draw(vertexCount, startVertexLocation);
	}

	void DeviceContext::setVSConstantBuffer(const ConstantBuffer& buffer, ui32 slot)
	{
		ID3D11Buffer* b = buffer.m_buffer.Get();
		m_context->VSSetConstantBuffers(slot, 1, &b);
	}

	void DeviceContext::setPSConstantBuffer(const ConstantBuffer& buffer, ui32 slot)
	{
		ID3D11Buffer* b = buffer.m_buffer.Get();
		m_context->PSSetConstantBuffers(slot, 1, &b);
	}

	void DeviceContext::updateConstantBuffer(const ConstantBuffer& buffer, const void* data, size_t dataSize)
	{
		if (data == nullptr || dataSize == 0) return;

		if (dataSize > buffer.getBufferSize())
			DX3DLogThrowError("updateConstantBuffer: dataSize > buffer size");

		D3D11_MAPPED_SUBRESOURCE mappedResource;
		HRESULT hr = m_context->Map(buffer.m_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		if (SUCCEEDED(hr))
		{
			memcpy(mappedResource.pData, data, dataSize);
			m_context->Unmap(buffer.m_buffer.Get(), 0);
		}
		else
		{
			DX3DGraphicsLogThrowOnFail(hr, "Map failed for constant buffer update.");
		}
	}
}