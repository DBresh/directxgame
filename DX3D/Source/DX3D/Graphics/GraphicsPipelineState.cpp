#include <DX3D/Graphics/GraphicsPipelineState.h>
#include <DX3D/Graphics/ShaderBinary.h>

dx3d::GraphicsPipelineState::GraphicsPipelineState(const GraphicsPipelineStateDesc& desc, const GraphicsResourceDesc& gDesc):
	GraphicsResource(gDesc)
{
	if (desc.vs.getType() != ShaderType::VertexShader)
		DX3DLogThrowInvalidArg("The 'vs' member is not a valid vertex shader binary.")

	if (desc.ps.getType() != ShaderType::PixelShader)
		DX3DLogThrowInvalidArg("The 'ps' member is not a valid pixel shader binary.")

	auto vs = desc.vs.getData();
	auto ps = desc.ps.getData();

	constexpr D3D11_INPUT_ELEMENT_DESC elements[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	DX3DGraphicsLogThrowOnFail(m_device.CreateInputLayout(elements, std::size(elements), vs.data, vs.dataSize, &m_layout),
		"CreateInputLayout failed.");

	DX3DGraphicsLogThrowOnFail(m_device.CreateVertexShader(vs.data, vs.dataSize, nullptr, &m_vs), "CreateVertexShader failed.");
	DX3DGraphicsLogThrowOnFail(m_device.CreatePixelShader(ps.data, ps.dataSize, nullptr, &m_ps), "CreatePixelShader failed.");

}
