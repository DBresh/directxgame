#pragma once
#include <DX3D/Core/Logger.h>
#include <DX3D/Core/Core.h>
#include <DX3D/Math/Rect.h>

namespace dx3d
{
	struct BaseDesc
	{
	};

	struct WindowDesc
	{
		Rect size{};
	};

	struct GraphicsEngineDesc
	{
		BaseDesc base;
	};

	struct GraphicsDeviceDesc
	{
		BaseDesc base;
	};

	struct GameDesc
	{
		Rect windowSize{ 1280, 720 };
	};

	struct SwapChainDesc
	{
		void* winHandle{};
		Rect winSize{};
	};

	enum class ShaderType
	{
		VertexShader = 0,
		PixelShader
	};

	struct ShaderCompileDesc
	{
		const char* shaderSourceName{};
		const void* shaderSourceCode{};
		size_t shaderSourceCodeSize{};
		const char* shaderEntryPoint{};
		ShaderType shaderType{};
	};

	struct GraphicsPipelineStateDesc
	{
		const VertexShaderSignature& vs;
		const ShaderBinary& ps;
	};

	struct BinaryData
	{
		const void* data{};
		size_t dataSize{};
	};

	struct VertexBufferDesc
	{
		const void* vertexList{};
		unsigned int vertexListSize{};
		unsigned int vertexSize{};
	};

	struct VertexShaderSignatureDesc
	{
		const ShaderBinaryPtr& vsBinary;
	};

	struct IndexBufferDesc
	{
		const void* indexList{};
		unsigned int indexCount{};
		bool use32Bit{};
	};

	struct ConstantBufferDesc
	{
		const void* data{};
		size_t size{};
	};

	struct DisplayDesc
	{
		WindowDesc window;
		GraphicsDevice& graphicsDevice;
	};
}