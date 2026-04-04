#pragma once
#include <DX3D/Graphics/GraphicsResource.h>

namespace dx3d
{

	class GraphicsPipelineState final: public GraphicsResource
	{
	public:
		GraphicsPipelineState(const GraphicsPipelineStateDesc& desc, const GraphicsResourceDesc& gDesc);
		ID3D11RasterizerState* getRasterizerState() const { return m_rasterizerState.Get(); }


	private:
		Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vs{};
		Microsoft::WRL::ComPtr<ID3D11PixelShader> m_ps{};
		Microsoft::WRL::ComPtr<ID3D11InputLayout> m_layout{};
		Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_depthStencilState{};
		Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rasterizerState{};

		friend class DeviceContext;
	};


}

