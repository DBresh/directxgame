#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/GraphicsDevice.h>
#include <DX3D/Graphics/DeviceContext.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/Mesh.h>
#include <DX3D/Graphics/ConstantBuffer.h>
#include <DX3D/Graphics/GraphicsLogUtils.h>
#include <DX3D/Graphics/Texture2D.h>
#include <DX3D/Math/Vec3.h>
#include <DX3D/Math/Matrix4x4.h>
#include <DX3D/Core/Time.h>
#include <fstream>
#include <DirectXMath.h>
#include <DX3D/InputSystem/InputSystem.h>

namespace dx3d
{

	GraphicsEngine::GraphicsEngine(const GraphicsEngineDesc& desc) : Base(desc.base)
	{
		m_graphicsDevice = std::make_shared<GraphicsDevice>(GraphicsDeviceDesc{});

		auto& device = *m_graphicsDevice;
		m_deviceContext = device.createDeviceContext();

		HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
		{
			DX3D_LOG_THROW_ERROR("Failed to initialize COM library. HRESULT: {:#010x}", static_cast<unsigned int>(hr));
		}

		constexpr char shaderFilePath[] = "DX3D/Assets/Shaders/Basic.hlsl";
		std::ifstream shaderStream(shaderFilePath);
		if (!shaderStream) DX3D_LOG_THROW_ERROR("Failed to open shader file.");
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
		cbDesc.size = sizeof(DirectX::XMFLOAT4X4) * 3;
		m_cb = device.createConstantBuffer(cbDesc);

		m_modelImporter = std::make_shared<ModelImporter>(m_graphicsDevice);

		createCubeMesh();

		InputSystem::get()->addListener(this);
		m_cameraPosition.setIdentity();
		m_cameraPosition.setTranslate(Vec3(-3, 0, 0));
		m_forward = 0;
		m_right = 0;
	}

	void GraphicsEngine::createCubeMesh()
	{
		ConstantBufferDesc cbDesc{};
		cbDesc.data = nullptr;
		cbDesc.size = sizeof(DirectX::XMFLOAT4X4) * 3;

		auto cubeModel = m_modelImporter->loadOBJ("Models\\cube.obj");
		auto cubeMesh = m_graphicsDevice->createMesh(cubeModel.vertices, cubeModel.indices);
		auto cube = m_scene.createObject("Cube1");
		cube->mesh = cubeMesh;
		cube->materials = cubeModel.materials;
		cube->materialGroups = cubeModel.materialGroups;
		cube->transform.setPosition(Vec3(-2.0f, 0.0f, 0.0f));
		cube->transform.setScale(Vec3(0.5f, 0.5f, 0.5f));
		cube->constantBuffer = m_graphicsDevice->createConstantBuffer(cbDesc);
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

		Matrix4x4 projection, view;

		Vec3 forward;
		forward.x = sin(m_angleY) * cos(m_angleX);
		forward.y = sin(m_angleX);
		forward.z = cos(m_angleY) * cos(m_angleX);
		forward = forward.normalize();

		Vec3 right = cross(forward, Vec3(0, 1, 0)).normalize();
		Vec3 up = cross(right, forward).normalize();

		Vec3 cameraPos = m_cameraPosition.getTranslation();
		cameraPos = cameraPos + forward * (m_forward * 0.1f);
		cameraPos = cameraPos + right * (m_right * 0.1f);

		Vec3 target = cameraPos + forward;
		view.setLookAtLH(cameraPos, target, up);

		auto size = swapChain.getSize();
		float aspectRatio = static_cast<float>(size.width) / static_cast<float>(size.height);
		projection.setPerspectiveFovLH(3.14159f / 4.0f, aspectRatio, 0.1f, 100.0f);

		Matrix4x4 viewT = view.transpose();
		Matrix4x4 projectionT = projection.transpose();

		for (const auto& object : m_scene.getAllObjects())
		{
			if (!object->mesh) continue;

			Matrix4x4 worldMatrix = object->getWorldTransform().getWorldMatrix();
			Matrix4x4 worldT = worldMatrix.transpose();
			Matrix4x4 viewT = view.transpose();
			Matrix4x4 projectionT = projection.transpose();

			struct TransformData
			{
				float world[4][4];
				float view[4][4];
				float projection[4][4];
			} cbData{};

			memcpy(&cbData.world, &worldT.mat, sizeof(float) * 16);
			memcpy(&cbData.view, &viewT.mat, sizeof(float) * 16);
			memcpy(&cbData.projection, &projectionT.mat, sizeof(float) * 16);

			context.updateConstantBuffer(*object->constantBuffer, &cbData, sizeof(cbData));
			context.setVSConstantBuffer(*object->constantBuffer, 0);

			// 2) Bind mesh once (VB/IB)
			context.setVertexBuffer(object->mesh->getVertexBuffer());
			context.setIndexBuffer(object->mesh->getIndexBuffer());

			// 3) Bind sampler once (or per-group if you want different samplers)
			context.setPSSampler(m_linearSampler.Get(), 0);

			// 4) Draw each material group
			for (const auto& group : object->materialGroups)
			{
				const Material* mat = nullptr;
				if (group.materialIndex >= 0 && group.materialIndex < (int)object->materials.size())
					mat = &object->materials[group.materialIndex];

				if (mat && mat->diffuseTexture)
					context.setPSTexture(mat->diffuseTexture->getSRV(), 0);
				else
					context.setPSTexture(nullptr, 0);

				context.drawIndexedTriangleList(group.indexCount, group.startIndex, 0);
			}
		}

		m_graphicsDevice->executeCommandList(context);
		swapChain.present(true);

		m_cameraPosition.setTranslate(cameraPos);
	}

	void GraphicsEngine::onKeyDown(int key) {}
	void GraphicsEngine::onKeyUp(int key)
	{
		if ((char)key == 'W' || (char)key == 'S') m_forward = 0;
		if ((char)key == 'D' || (char)key == 'A') m_right = 0;
	}

	void GraphicsEngine::onKeyPress(int key)
	{
		float dt = static_cast<float>(Time::Instance()->deltaTime());
		if ((char)key == 'W') m_forward = 1.0f;
		if ((char)key == 'S') m_forward = -1.0f;
		if ((char)key == 'D') m_right = -1.0f;
		if ((char)key == 'A') m_right = 1.0f;

		if ((char)key == 'I') m_angleX += m_rotationSpeed * 10 * dt;
		if ((char)key == 'K') m_angleX -= m_rotationSpeed * 10 * dt;
		if ((char)key == 'J') m_angleY -= m_rotationSpeed * 10 * dt;
		if ((char)key == 'L') m_angleY += m_rotationSpeed * 10 * dt;

	}

	void GraphicsEngine::onMouseMove(Point deltaMouse)
	{
		float dt = static_cast<float>(Time::Instance()->deltaTime());
		m_angleX -= deltaMouse.y * m_rotationSpeed * dt;
		m_angleY += deltaMouse.x * m_rotationSpeed * dt;
	}

	void GraphicsEngine::onMouseUp(int button) {}
	void GraphicsEngine::onMouseDown(int button) {}
	void GraphicsEngine::onMouseWheel(int delta) {}

}