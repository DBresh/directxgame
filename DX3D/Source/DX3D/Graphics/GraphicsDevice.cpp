#include <DX3D/Graphics/GraphicsDevice.h>
#include <DX3D/Graphics/GraphicsLogUtils.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/DeviceContext.h>
#include <DX3D/Graphics/ShaderBinary.h>
#include <DX3D/Graphics/GraphicsPipelineState.h>
#include <DX3D/Graphics/VertexBuffer.h>
#include <DX3D/Graphics/VertexShaderSignature.h>
#include <DX3D/Graphics/IndexBuffer.h>
#include <DX3D/Graphics/ConstantBuffer.h>
#include <DX3D/Graphics/Mesh.h>
#include <DX3D/Math/Vertex.h>
#include <vector>

namespace dx3d
{
	GraphicsDevice::GraphicsDevice(const GraphicsDeviceDesc& desc) : Base(desc.base)
	{
		D3D_FEATURE_LEVEL featureLevel{};

		UINT createDeviceFlags{};

#ifdef _DEBUG
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif


		DX3D_GRAPHICS_LOG_THROW_ON_FAIL(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, NULL, 0, D3D11_SDK_VERSION, &m_d3dDevice, &featureLevel, &m_d3dContext),
			"Direct3D11 initialization failed.");

		DX3D_GRAPHICS_LOG_THROW_ON_FAIL(m_d3dDevice->QueryInterface(IID_PPV_ARGS(&m_dxgiDevice)), "QueryInterface failed to retrieve IDXGIDevice.");
		DX3D_GRAPHICS_LOG_THROW_ON_FAIL(m_dxgiDevice->GetParent(IID_PPV_ARGS(&m_dxgiAdapter)), "GetParent failed to retrieve IDXGIAdapter.");
		DX3D_GRAPHICS_LOG_THROW_ON_FAIL(m_dxgiAdapter->GetParent(IID_PPV_ARGS(&m_dxgiFactory)), "GetParent failed to retrieve IDXGIFactory.");
	}

	GraphicsDevice::~GraphicsDevice()
	{
	}

	SwapChainPtr GraphicsDevice::createSwapChain(const SwapChainDesc& desc) const
	{
		return std::make_shared<SwapChain>(desc, getGraphicsResourceDesc());
	}

	DeviceContextPtr GraphicsDevice::createDeviceContext()
	{
		return std::make_shared<DeviceContext>(getGraphicsResourceDesc());
	}

	ShaderBinaryPtr GraphicsDevice::compileShader(const ShaderCompileDesc& desc)
	{
		return std::make_shared<ShaderBinary>(desc, getGraphicsResourceDesc());
	}

	GraphicsPipelineStatePtr GraphicsDevice::createGraphicsPipelineState(const GraphicsPipelineStateDesc& desc)
	{
		return std::make_shared<GraphicsPipelineState>(desc, getGraphicsResourceDesc());
	}

	VertexBufferPtr GraphicsDevice::createVertexBuffer(const VertexBufferDesc& desc)
	{
		return std::make_shared<VertexBuffer>(desc, getGraphicsResourceDesc());
	}

	VertexShaderSignaturePtr GraphicsDevice::createVertexShaderSignature(const VertexShaderSignatureDesc& desc)
	{
		return std::make_shared<VertexShaderSignature>(desc, getGraphicsResourceDesc());
	}

	IndexBufferPtr GraphicsDevice::createIndexBuffer(const IndexBufferDesc& desc)
	{
		return std::make_shared<IndexBuffer>(desc, getGraphicsResourceDesc());
	}

	ConstantBufferPtr GraphicsDevice::createConstantBuffer(const ConstantBufferDesc& desc)
	{
		return std::make_shared<ConstantBuffer>(desc, getGraphicsResourceDesc());
	}

	MeshPtr GraphicsDevice::createMesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
	{
		return std::make_shared<Mesh>(vertices, indices, getGraphicsResourceDesc());
	}

	void GraphicsDevice::executeCommandList(DeviceContext& context)
	{
		Microsoft::WRL::ComPtr<ID3D11CommandList> list{};
		DX3D_GRAPHICS_LOG_THROW_ON_FAIL(context.m_context->FinishCommandList(false, &list), "FinishCommandList failed.");
		m_d3dContext->ExecuteCommandList(list.Get(), false);
	}

	GraphicsResourceDesc GraphicsDevice::getGraphicsResourceDesc() const noexcept
	{
		return { BaseDesc{}, shared_from_this(), *m_d3dDevice.Get(), *m_dxgiFactory.Get() };
	}
}
