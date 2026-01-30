#pragma once
#include <DX3D/Graphics/GraphicsResource.h>
#include <DX3D/Core/Common.h>
#include <DX3D/Core/Base.h>
#include <DX3D/Math/Vertex.h>
#include <DX3D/Graphics/ModelGPU.h>
#include <d3d11.h>
#include <wrl.h>
#include <vector>

namespace dx3d
{

	class GraphicsDevice final: public Base, public std::enable_shared_from_this<GraphicsDevice>
	{
	public:
		explicit GraphicsDevice(const GraphicsDeviceDesc& desc);
		virtual ~GraphicsDevice() override;
		
		SwapChainPtr createSwapChain(const SwapChainDesc& desc) const;
		DeviceContextPtr createDeviceContext();
		ShaderBinaryPtr compileShader(const ShaderCompileDesc& desc);
		GraphicsPipelineStatePtr createGraphicsPipelineState(const GraphicsPipelineStateDesc& desc);
		VertexBufferPtr createVertexBuffer(const VertexBufferDesc& desc);
		VertexShaderSignaturePtr createVertexShaderSignature(const VertexShaderSignatureDesc& desc);
		IndexBufferPtr createIndexBuffer(const IndexBufferDesc& desc);
		ConstantBufferPtr createConstantBuffer(const ConstantBufferDesc& desc);
		MeshPtr createMesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
		Texture2DPtr createTexture2D(const std::string& path);
		ModelGPU createModelGPU(const ModelData& data);
		StructuredBufferPtr createStructuredBuffer(const void* data, size_t elementSize, size_t elementCount);
		DepthTexture2DPtr createDepthTexture2D(UINT width, UINT height);
		DeviceContextPtr createDeferredContext();

		template<typename T>
		StructuredBufferPtr createStructuredBuffer(const std::vector<T>& elements)
		{
			if (elements.empty())
				return nullptr;
			return createStructuredBuffer(elements.data(), sizeof(T), elements.size());
		}

		Microsoft::WRL::ComPtr<ID3D11SamplerState> createSampler();
		Microsoft::WRL::ComPtr<ID3D11SamplerState> createShadowSampler();
		Microsoft::WRL::ComPtr<ID3D11VertexShader> createVertexShaderFromFile(const std::string& path, const char* entry);

		ID3D11Device* getD3D11Device() const noexcept { return m_d3dDevice.Get(); }
		IDXGIFactory* getDXGIFactory() const { return m_dxgiFactory.Get(); }
		const GraphicsDeviceDesc& getDesc() const { return m_desc; }

		void executeCommandList(DeviceContext& context);
	protected:
		GraphicsResourceDesc getGraphicsResourceDesc() const noexcept;

	private:
		GraphicsDeviceDesc m_desc;
		Microsoft::WRL::ComPtr<ID3D11Device> m_d3dDevice{};
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_d3dContext{};
		Microsoft::WRL::ComPtr<IDXGIDevice> m_dxgiDevice{};
		Microsoft::WRL::ComPtr<IDXGIAdapter> m_dxgiAdapter{};
		Microsoft::WRL::ComPtr<IDXGIFactory> m_dxgiFactory{};	
	};
}

