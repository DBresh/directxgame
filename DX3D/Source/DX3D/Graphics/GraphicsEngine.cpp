#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/GraphicsDevice.h>
#include <DX3D/Graphics/DeviceContext.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/VertexBuffer.h>
#include <DX3D/Graphics/IndexBuffer.h>
#include <DX3D/Graphics/ConstantBuffer.h>
#include <DX3D/Math/Vec3.h>
#include <DX3D/Math/Matrix4x4.h>
#include <DX3D/Core/Time.h>
#include <fstream>
#include <DirectXMath.h>
#include <DX3D/InputSystem/InputSystem.h> // temp

namespace dx3d
{

	GraphicsEngine::GraphicsEngine(const GraphicsEngineDesc& desc) : Base(desc.base)
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
		cbDesc.size = sizeof(DirectX::XMFLOAT4X4) * 3; // world, view, projection
		m_cb = device.createConstantBuffer(cbDesc);

		const Vertex vertexList[] = {
			// Front face
			{{-0.5f, -0.5f,  0.5f}, {1, 0, 0, 1}}, // bottom left front - 0
			{{-0.5f,  0.5f,  0.5f}, {0, 1, 0, 1}}, // top left front - 1
			{{ 0.5f,  0.5f,  0.5f}, {0, 0, 1, 1}}, // top right front - 2
			{{ 0.5f, -0.5f,  0.5f}, {1, 1, 0, 1}}, // bottom right front - 3

			// Back face
			{{-0.5f, -0.5f, -0.5f}, {1, 0, 1, 1}}, // bottom left back - 4
			{{-0.5f,  0.5f, -0.5f}, {0, 1, 1, 1}}, // top left back - 5
			{{ 0.5f,  0.5f, -0.5f}, {1, 1, 1, 1}}, // top right back - 6
			{{ 0.5f, -0.5f, -0.5f}, {0, 1, 0, 1}}  // bottom right back - 7
		};

		m_vb = device.createVertexBuffer({ vertexList, std::size(vertexList), sizeof(Vertex) });

		// Cube indices (12 triangles = 36 indices)
		unsigned short indices[] = {
			// Front face
			0,2,1, 0,3,2,
			// Back face
			4,5,6, 4,6,7,
			// Left face
			4,1,5, 4,0,1,
			// Right face
			3,6,2, 3,7,6,
			// Top face
			1,6,5, 1,2,6,
			// Bottom face
			4,7,0, 0,7,3
		};

		m_ib = device.createIndexBuffer({ indices, (ui32)std::size(indices), false });

		InputSystem::get()->addListener(this);
	}

	GraphicsEngine::~GraphicsEngine()
	{
		InputSystem::get()->removeListener(this);
	}

	GraphicsDevice& GraphicsEngine::getGraphicsDevice() noexcept
	{
		return *m_graphicsDevice;
	}

	void GraphicsEngine::render(SwapChain& swapChain)
	{
		auto& context = *m_deviceContext;
		context.clearAndSetBackBuffer(swapChain, { 0.2f, 0.2f, 0.2f, 1 });
		context.setGraphicsPipelineState(*m_pipeline);
		context.setViewportSize(swapChain.getSize());

		float dt = static_cast<float>(Time::Instance()->deltaTime());
		if (m_keysPressed['W']) m_angleX -= m_rotationSpeed * dt;
		if (m_keysPressed['S']) m_angleX += m_rotationSpeed * dt;
		if (m_keysPressed['A']) m_angleY -= m_rotationSpeed * dt;
		if (m_keysPressed['D']) m_angleY += m_rotationSpeed * dt;

		Matrix4x4 view, projection;

		Matrix4x4 rotY, rotX;
		rotY.setRotationY(m_angleY);
		rotX.setRotationX(m_angleX);
		Matrix4x4 world = rotY * rotX;

		view.setLookAtLH(
			Vec3(0.0f, 0.0f, -3.0f),  // Eye position
			Vec3(0.0f, 0.0f, 0.0f),   // Focus position
			Vec3(0.0f, 1.0f, 0.0f)    // Up direction
		);

		auto size = swapChain.getSize();
		float aspectRatio = static_cast<float>(size.width) / static_cast<float>(size.height);
		projection.setPerspectiveFovLH(3.14159f / 4.0f, aspectRatio, 0.1f, 100.0f);

		Matrix4x4 worldT = world.transpose();
		Matrix4x4 viewT = view.transpose();
		Matrix4x4 projectionT = projection.transpose();

		struct TransformData
		{
			float world[4][4];
			float view[4][4];
			float projection[4][4];
		};

		TransformData cbData;
		memcpy(&cbData.world, &worldT.mat, sizeof(float) * 16);
		memcpy(&cbData.view, &viewT.mat, sizeof(float) * 16);
		memcpy(&cbData.projection, &projectionT.mat, sizeof(float) * 16);

		auto& device = *m_graphicsDevice;
		device.updateConstantBuffer(*m_cb, &cbData, sizeof(cbData));
		context.setVertexBuffer(*m_vb);
		context.setIndexBuffer(*m_ib);
		context.setVSConstantBuffer(*m_cb, 0);
		context.drawIndexedTriangleList(m_ib->getIndexCount(), 0u, 0u);
		device.executeCommandList(context);
		swapChain.present();
	}
}
