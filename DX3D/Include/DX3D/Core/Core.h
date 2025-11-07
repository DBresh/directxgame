#pragma once
#include <stdexcept>
#include <memory>


namespace dx3d
{
	class Base;
	class Window;
	class Game;

	class GraphicsEngine;
	class GraphicsDevice;
	class SwapChain;
	class Display;
	class DeviceContext;
	class Logger;
	class ShaderBinary;
	class GraphicsPipelineState;
	class VertexBuffer;
	class VertexShaderSignature;
	class IndexBuffer;
	class ConstantBuffer;
	class Mesh;
	class Texture2D;
	class ModelImporter;
	class MaterialLoader;
	class StructuredBuffer;

	using SwapChainPtr = std::shared_ptr<SwapChain>;
	using DeviceContextPtr = std::shared_ptr<DeviceContext>;
	using ShaderBinaryPtr = std::shared_ptr<ShaderBinary>;
	using GraphicsPipelineStatePtr = std::shared_ptr<GraphicsPipelineState>;
	using VertexBufferPtr = std::shared_ptr<VertexBuffer>;
	using VertexShaderSignaturePtr = std::shared_ptr<VertexShaderSignature>;
	using IndexBufferPtr = std::shared_ptr<IndexBuffer>;
	using ConstantBufferPtr = std::shared_ptr<ConstantBuffer>;
	using MeshPtr = std::shared_ptr<Mesh>;
	using Texture2DPtr = std::shared_ptr<Texture2D>;
	using StructuredBufferPtr = std::shared_ptr<StructuredBuffer>;
}