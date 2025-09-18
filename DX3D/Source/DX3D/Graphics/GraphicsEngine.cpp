#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/GraphicsDevice.h>
#include <DX3D/Graphics/DeviceContext.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/VertexBuffer.h>
#include <DX3D/Graphics/IndexBuffer.h>
#include <DX3D/Graphics/ConstantBuffer.h>
#include <DX3D/Math/Vec3.h>
#include <fstream>
#include <DirectXMath.h>
using namespace dx3d;



dx3d::GraphicsEngine::GraphicsEngine(const GraphicsEngineDesc& desc) : Base(desc.base)
{
	m_graphicsDevice = std::make_shared<GraphicsDevice>(GraphicsDeviceDesc{ m_logger });

	auto& device = *m_graphicsDevice;
	m_deviceContext = device.createDeviceContext();

	constexpr char shaderFilePath[] = "DX3D/Assets/Shaders/Basic.hlsl";
	std::ifstream shaderStream(shaderFilePath);
	if (!shaderStream) DX3DLogThrowError("Failed to open shader file.");
	std::string shaderFileData{
		std::istreambuf_iterator<char>(shaderStream),
		std::istreambuf_iterator<char>()
	};

	auto shaderSourceCode = shaderFileData.c_str();
	auto shaderSourceCodeSize = shaderFileData.length();

	auto vs = device.compileShader({ shaderFilePath, shaderSourceCode, shaderSourceCodeSize, "VSMain", ShaderType::VertexShader });
	auto ps = device.compileShader({ shaderFilePath, shaderSourceCode, shaderSourceCodeSize, "PSMain", ShaderType::PixelShader });
	auto vsSig = device.createVertexShaderSignature({ vs });

	m_pipeline = device.createGraphicsPipelineState({ *vsSig, *ps });

	ConstantBufferDesc cbDesc{};
	cbDesc.data = nullptr;
	cbDesc.size = sizeof(DirectX::XMFLOAT4X4);
	m_cb = device.createConstantBuffer(cbDesc);

	const Vertex vertexList[] =
	{
		{{-0.5f, -0.5f, 0.0f},{1,0,0,1}},  // bottom left
		{{-0.5f,  0.5f, 0.0f},{0,1,0,1}},  // top left
		{{ 0.5f,  0.5f, 0.0f},{0,0,1,1}},  // top right
		{{ 0.5f, -0.5f, 0.0f},{1,0,1,1}},  // bottom right
	};

	m_vb = device.createVertexBuffer({ vertexList, std::size(vertexList), sizeof(Vertex) });

	// Define indices (two triangles ? 6 indices)
	unsigned short indices[] = { 0, 1, 2, 2, 3, 0 };

	m_ib = device.createIndexBuffer({ indices, (ui32)std::size(indices), false });
}

dx3d::GraphicsEngine::~GraphicsEngine()
{
}

GraphicsDevice& dx3d::GraphicsEngine::getGraphicsDevice() noexcept
{
	return *m_graphicsDevice;
}

void dx3d::GraphicsEngine::render(SwapChain& swapChain)
{
	auto& context = *m_deviceContext;
	context.clearAndSetBackBuffer(swapChain, { 1,1,1,1 });
	context.setGraphicsPipelineState(*m_pipeline);
	context.setViewportSize(swapChain.getSize());

	static float angle = 0.0f;
	angle += 0.01f;

	using namespace DirectX;
	XMMATRIX world = XMMatrixRotationZ(angle);
	XMMATRIX worldT = XMMatrixTranspose(world);

	XMFLOAT4X4 cbData;
	XMStoreFloat4x4(&cbData, worldT);

	auto& device = *m_graphicsDevice;
	device.updateConstantBuffer(*m_cb, &cbData, sizeof(cbData));
	context.setVertexBuffer(*m_vb);
	context.setIndexBuffer(*m_ib);
	context.setVSConstantBuffer(*m_cb, 0);
	context.drawIndexedTriangleList(m_ib->getIndexCount(), 0u, 0u);
	device.executeCommandList(context);
	swapChain.present();
}
