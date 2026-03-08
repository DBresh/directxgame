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
#include <DX3D/Graphics/Texture2D.h>
#include <DX3D/Graphics/DepthTexture2D.h>
#include <DX3D/Math/Vertex.h>
#include <DX3D/Graphics/StructuredBuffer.h>
#include <DX3D/Graphics/InstanceBuffer.h>
#include <vector>

namespace dx3d
{
	GraphicsDevice::GraphicsDevice(const GraphicsDeviceDesc& desc) : Base(desc.base), m_desc(desc)
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

	InstanceBufferPtr GraphicsDevice::createInstanceBuffer(const InstanceBufferDesc& desc)
	{
		return std::make_shared<InstanceBuffer>(desc, getGraphicsResourceDesc());
	}

	MeshPtr GraphicsDevice::createMesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
	{
		return std::make_shared<Mesh>(vertices, indices, getGraphicsResourceDesc());
	}

	Texture2DPtr GraphicsDevice::createTexture2D(const std::string& path)
	{
		return std::make_shared<Texture2D>(path, getGraphicsResourceDesc());
	}

	DepthTexture2DPtr GraphicsDevice::createDepthTexture2D(UINT width, UINT height)
	{
		return std::make_shared<DepthTexture2D>(width, height, getGraphicsResourceDesc());
	}

	Microsoft::WRL::ComPtr<ID3D11SamplerState> GraphicsDevice::createSampler()
	{
		D3D11_SAMPLER_DESC samplerDesc{};
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

		Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler;
		DX3D_GRAPHICS_LOG_THROW_ON_FAIL(
			m_d3dDevice->CreateSamplerState(&samplerDesc, &sampler),
			"Failed to create sampler state."
		);
		return sampler;
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

	ModelGPU GraphicsDevice::createModelGPU(const ModelData& data)
	{
		ModelGPU gpuModel;
		gpuModel.materials = data.materials;
		gpuModel.boundingBox = data.boundingBox;

		auto mesh = createMesh(data.vertices, data.indices);
		gpuModel.mesh = mesh;

		DX3D_LOG_INFO("Created GPU model: 1 mesh, {} vertices, {} indices",
			data.vertices.size(), data.indices.size());

		return gpuModel;
	}

	StructuredBufferPtr GraphicsDevice::createStructuredBuffer(const void* data, size_t elementSize, size_t elementCount)
	{
		return std::make_shared<StructuredBuffer>(data, elementSize, elementCount, getGraphicsResourceDesc());
	}

	Microsoft::WRL::ComPtr<ID3D11VertexShader> GraphicsDevice::createVertexShaderFromFile(const std::string& path, const char* entry)
	{
		std::ifstream file(path, std::ios::binary);
		std::string src((std::istreambuf_iterator<char>(file)), {});
		ShaderCompileDesc desc{};
		desc.shaderSourceName = path.c_str();
		desc.shaderSourceCode = src.data();
		desc.shaderSourceCodeSize = src.size();
		desc.shaderEntryPoint = entry;
		desc.shaderType = ShaderType::VertexShader;
		auto blob = compileShader(desc);
		auto data = blob->getData();

		Microsoft::WRL::ComPtr<ID3D11VertexShader> vs;
		DX3D_GRAPHICS_LOG_THROW_ON_FAIL(
			m_d3dDevice->CreateVertexShader(data.data, data.dataSize, nullptr, &vs),
			"Failed to create vertex shader from file: {}", path
		);
		return vs;
	}

	Microsoft::WRL::ComPtr<ID3D11SamplerState> GraphicsDevice::createShadowSampler()
	{
		D3D11_SAMPLER_DESC desc{};
		desc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
		desc.AddressU = desc.AddressV = desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
		desc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
		desc.BorderColor[0] = desc.BorderColor[1] = desc.BorderColor[2] = desc.BorderColor[3] = 1.0f;
		desc.MinLOD = 0;
		desc.MaxLOD = D3D11_FLOAT32_MAX;

		Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler;
		DX3D_GRAPHICS_LOG_THROW_ON_FAIL(
			m_d3dDevice->CreateSamplerState(&desc, &sampler),
			"Failed to create shadow comparison sampler."
		);
		return sampler;
	}

	DeviceContextPtr GraphicsDevice::createDeferredContext()
	{
		ID3D11DeviceContext* deferredCtx = nullptr;
		// no specific flags
		m_d3dDevice->CreateDeferredContext(0, &deferredCtx);

		if (!deferredCtx) return nullptr;

		return std::make_shared<DeviceContext>(deferredCtx, shared_from_this());
	}

}
