#include <DX3D/Graphics/ShaderBinary.h>
#include <DX3D/Graphics/GraphicsUtils.h>
#include <d3dcompiler.h>

namespace dx3d
{
	ShaderBinary::ShaderBinary(const ShaderCompileDesc& desc, const GraphicsResourceDesc& gDesc) :
		GraphicsResource(gDesc), m_type(desc.shaderType)
	{

		if (!desc.shaderSourceName) DX3D_LOG_THROW_ERROR("No shader source name provided.");
		if (!desc.shaderSourceCode) DX3D_LOG_THROW_ERROR("No shader source code provided.");
		if (!desc.shaderSourceCodeSize) DX3D_LOG_THROW_ERROR("No shader source code size provided.");
		if (!desc.shaderEntryPoint) DX3D_LOG_THROW_ERROR("No shader entry point provided.");

		UINT compileFlags{};

#ifdef _DEBUG
		compileFlags |= D3DCOMPILE_DEBUG;
#endif

		Microsoft::WRL::ComPtr<ID3DBlob> errorBlob{};
		D3DCompile(
			desc.shaderSourceCode,
			desc.shaderSourceCodeSize,
			desc.shaderSourceName,
			nullptr,
			nullptr,
			desc.shaderEntryPoint,
			getShaderModelTarget(desc.shaderType),
			compileFlags,
			0,
			&m_blob,
			&errorBlob
		);

		if (errorBlob) {
			std::string errMsg(static_cast<const char*>(errorBlob->GetBufferPointer()), errorBlob->GetBufferSize());
			DX3D_LOG_ERROR("Shader compilation failed:\n{}", errMsg);
			throw std::runtime_error(errMsg);
		}

		if (!m_blob) {
			DX3D_LOG_THROW_ERROR("Shader compilation returned null blob.");
		}
	}

	BinaryData ShaderBinary::getData() const noexcept
	{
		return
		{
			m_blob->GetBufferPointer(),
			m_blob->GetBufferSize()
		};
	}

	ShaderType ShaderBinary::getType() const noexcept
	{
		return m_type;
	}
}
