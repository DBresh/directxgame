#pragma once

#include <DX3D/Graphics/GraphicsResource.h>
#include <DX3D/Core/Common.h>
#include <DX3D/Core/Base.h>
#include <DX3D/Math/Vertex.h>
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
		
		Microsoft::WRL::ComPtr<ID3D11SamplerState> createSampler();

		void executeCommandList(DeviceContext& context);
	protected:
		GraphicsResourceDesc getGraphicsResourceDesc() const noexcept;

	private:
		Microsoft::WRL::ComPtr<ID3D11Device> m_d3dDevice{};
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_d3dContext{};
		Microsoft::WRL::ComPtr<IDXGIDevice> m_dxgiDevice{};
		Microsoft::WRL::ComPtr<IDXGIAdapter> m_dxgiAdapter{};
		Microsoft::WRL::ComPtr<IDXGIFactory> m_dxgiFactory{};
	};
}

