#include <DX3D/Graphics/GraphicsPipelineState.h>
#include <DX3D/Graphics/ShaderBinary.h>
#include <DX3D/Graphics/VertexShaderSignature.h>

namespace dx3d
{

	dx3d::GraphicsPipelineState::GraphicsPipelineState(const GraphicsPipelineStateDesc& desc, const GraphicsResourceDesc& gDesc) :
		GraphicsResource(gDesc)
	{
		if (desc.ps.getType() != ShaderType::PixelShader)
			DX3D_LOG_THROW_ERROR("The 'ps' member is not a valid pixel shader binary.");

		auto vs = desc.vs.getShaderBinaryData();
		auto ps = desc.ps.getData();
		auto vsInputElements = desc.vs.getInputElementsData();


		DX3D_GRAPHICS_LOG_THROW_ON_FAIL(m_device.CreateInputLayout(
			static_cast<const D3D11_INPUT_ELEMENT_DESC*>(vsInputElements.data),
			static_cast<unsigned int>(vsInputElements.dataSize),
			vs.data,
			vs.dataSize,
			&m_layout),
			"CreateInputLayout failed.");

		DX3D_GRAPHICS_LOG_THROW_ON_FAIL(m_device.CreateVertexShader(vs.data, vs.dataSize, nullptr, &m_vs), "CreateVertexShader failed.");
		DX3D_GRAPHICS_LOG_THROW_ON_FAIL(m_device.CreatePixelShader(ps.data, ps.dataSize, nullptr, &m_ps), "CreatePixelShader failed.");

		D3D11_DEPTH_STENCIL_DESC dsDesc{};
		dsDesc.DepthEnable = TRUE;
		dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
		dsDesc.StencilEnable = FALSE;
		DX3D_GRAPHICS_LOG_THROW_ON_FAIL(m_device.CreateDepthStencilState(&dsDesc, &m_depthStencilState), "CreateDepthStencilState failed.");

		D3D11_RASTERIZER_DESC rsDesc = {};
		rsDesc.CullMode = D3D11_CULL_BACK;   // Standard culling
		rsDesc.FillMode = D3D11_FILL_SOLID;  // Render triangles
		rsDesc.FrontCounterClockwise = FALSE;
		rsDesc.DepthClipEnable = TRUE;
		rsDesc.MultisampleEnable = FALSE;
		rsDesc.AntialiasedLineEnable = FALSE;

		// Default bias for normal rendering is 0
		rsDesc.DepthBias = 0;
		rsDesc.SlopeScaledDepthBias = 0.0f;
		rsDesc.DepthBiasClamp = 0.0f;

		DX3D_GRAPHICS_LOG_THROW_ON_FAIL(m_device.CreateRasterizerState(&rsDesc, &m_rasterizerState), "CreateRasterizerState failed.");
	}
}
