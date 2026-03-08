#include <DX3D/Graphics/Core/VertexShaderSignature.h>
#include <DX3D/Graphics/Core/ShaderBinary.h>
#include <DX3D/Graphics/GraphicsUtils.h>
#include <d3dcompiler.h>
#include <ranges>
#include <string_view>

namespace dx3d
{

	VertexShaderSignature::VertexShaderSignature(const VertexShaderSignatureDesc& desc, const GraphicsResourceDesc& gDesc) :
		GraphicsResource(gDesc), m_vsBinary(desc.vsBinary)
	{
		if (!desc.vsBinary) DX3D_LOG_THROW_ERROR("No shader binary provided.");
		if (desc.vsBinary->getType() != ShaderType::VertexShader) DX3D_LOG_THROW_ERROR("The 'vsBinary' is not a valid vertex shader binary");

		auto vsData = m_vsBinary->getData();
		DX3D_GRAPHICS_LOG_THROW_ON_FAIL(D3DReflect(
			vsData.data,
			vsData.dataSize,
			IID_PPV_ARGS(&m_shaderReflection)
		), "D3DReflect failed.");

		D3D11_SHADER_DESC shaderDesc{};
		DX3D_GRAPHICS_LOG_THROW_ON_FAIL(m_shaderReflection->GetDesc(&shaderDesc), "ID3D11ShaderReflection::GetDesc failed.");

		m_numElements = shaderDesc.InputParameters;
		D3D11_SIGNATURE_PARAMETER_DESC params[D3D11_STANDARD_VERTEX_ELEMENT_COUNT]{};

		for (auto i : std::views::iota(0u, m_numElements))
		{
			DX3D_GRAPHICS_LOG_THROW_ON_FAIL(m_shaderReflection->GetInputParameterDesc(i, &params[i]),
				"ID3D11ShaderReflection::GetInputParameterDesc failed.");
		}


		//constexpr D3D11_INPUT_ELEMENT_DESC elements[] =
		//{
		//	{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		//	{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		//};
		for (auto i : std::views::iota(0u, m_numElements))
		{
			auto param = params[i];

			// TEXCOORD 4, 5, 6, 7 for the 4x4 Instance Matrix
			bool isInstanceData = false;
			std::string_view semanticName(param.SemanticName);

			if (semanticName == "TEXCOORD" && param.SemanticIndex >= 4 && param.SemanticIndex <= 7)
			{
				isInstanceData = true;
			}

			m_elements[i] = {
				param.SemanticName,
				param.SemanticIndex,
				getDXGIFormatFromMask(param.ComponentType, param.Mask),
				isInstanceData ? 1u : 0u, // Input Slot: 1 for instances, 0 for vertices
				D3D11_APPEND_ALIGNED_ELEMENT,
				isInstanceData ? D3D11_INPUT_PER_INSTANCE_DATA : D3D11_INPUT_PER_VERTEX_DATA,
				isInstanceData ? 1u : 0u  // InstanceDataStepRate: 1 for instances, 0 for vertices
			};
		}
	}

	BinaryData VertexShaderSignature::getShaderBinaryData() const noexcept
	{
		return m_vsBinary->getData();
	}

	BinaryData VertexShaderSignature::getInputElementsData() const noexcept
	{
		return
		{
			m_elements,
			m_numElements
		};
	}
}
