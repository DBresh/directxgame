#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/GraphicsDevice.h>
#include <DX3D/Graphics/DeviceContext.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/GraphicsLogUtils.h>
#include <DX3D/Graphics/Texture2D.h>
#include <DX3D/Math/Vec3.h>
#include <DX3D/Math/Matrix4x4.h>
#include <DX3D/Core/Time.h>
#include <DX3D/InputSystem/InputSystem.h>
#include <DX3D/Graphics/ModelCache.h>
#include <DX3D/Graphics/AssetManager.h>
#include <fstream>

namespace dx3d
{
	namespace
	{
		static std::string loadFileText(const std::string& path)
		{
			std::ifstream file(path);
			if (!file.is_open())
				DX3D_LOG_THROW_ERROR("Failed to open file: {}", path);
			return { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
		}
	}

	GraphicsEngine::GraphicsEngine(const GraphicsEngineDesc& desc) : Base(desc.base)
	{
		m_graphicsDevice = std::make_shared<GraphicsDevice>(GraphicsDeviceDesc{});
		auto& device = *m_graphicsDevice;
		m_deviceContext = device.createDeviceContext();

		const std::string shaderFileData = loadFileText("DX3D/Assets/Shaders/Basic.hlsl");
		const char* shaderSourceCode = shaderFileData.c_str();
		const size_t shaderSize = shaderFileData.size();

		auto vs = device.compileShader({ "Basic.hlsl", shaderSourceCode, shaderSize, "VSMain", ShaderType::VertexShader });
		auto ps = device.compileShader({ "Basic.hlsl", shaderSourceCode, shaderSize, "PSMain", ShaderType::PixelShader });
		auto vsSig = device.createVertexShaderSignature({ vs });

		m_pipeline = device.createGraphicsPipelineState({ *vsSig, *ps });

		AssetManagerDesc aDesc{};
		aDesc.graphicsDevice = m_graphicsDevice;
		aDesc.assetsRoot = std::filesystem::path("DX3D/Assets/Models");
		m_assets = std::make_shared<AssetManager>(aDesc);

		m_renderSystem = std::make_unique<RenderSystem>(m_graphicsDevice, m_deviceContext);
		m_renderSystem->setPipeline(m_pipeline);

		createCubeMesh();

		m_camera = std::make_unique<Camera>();
		InputSystem::get()->addListener(m_camera.get());
	}

	GraphicsEngine::~GraphicsEngine()
	{
	}

	GraphicsDevice& GraphicsEngine::getGraphicsDevice() noexcept
	{
		return *m_graphicsDevice;
	}

	void GraphicsEngine::createCubeMesh()
	{
		auto model = m_assets->getModel("cube.obj");
		for (int i = 1; i <= 1; i++) {
			auto cubes = m_scene.createObject("cube");

			cubes->model = model;

			cubes->transform.setPosition(Vec3(i * 1.5f, 0.0f, 0.0f));
			cubes->transform.setScale(Vec3(1, 1, 1));
			cubes->constantBuffer = m_graphicsDevice->createConstantBuffer({ nullptr, sizeof(Matrix4x4) * 3 });
		}

		auto planeModel = m_assets->getModel("plane.obj");
		auto plane = m_scene.createObject("plane");
		plane->model = planeModel;
		plane->transform.setPosition(Vec3(0, -2, 0));
		plane->transform.setScale(Vec3(5, 5, 5));
		plane->constantBuffer = m_graphicsDevice->createConstantBuffer({ nullptr, sizeof(Matrix4x4) * 3 });

		auto plane1 = m_scene.createObject("plane");
		plane1->model = planeModel;
		plane1->transform.setPosition(Vec3(0, 10, 0));
		plane1->transform.setScale(Vec3(5, 5, 5));
		plane1->constantBuffer = m_graphicsDevice->createConstantBuffer({ nullptr, sizeof(Matrix4x4) * 3 });

		auto lights = m_renderSystem->getLightManager();
		lights->clear();

		//for (int i = -32; i <= 16; i += 2)
		//{
		//	for (int j = -24; j <= 16; j += 2)
		//	{
		//		float r = (i + 32) / 48.0f;
		//		float g = (j + 24) / 40.0f;
		//		float b = 1.0f - 0.5f * (r + g);

		//		lights->addPoint(Vec3((float)i, -2.2f, (float)j), Vec3(r, g, b), 1.0f, 1.0f);
		//	}
		//}

		//lights->addPoint(Vec3(5, 0, -2), Vec3(0.2f, 0.4f, 1.0f), 55.0f, 2.9f);
		//m_scene.getAllObjects()[1]->transform.setPosition(Vec3(4, 0, 0));
		lights->addSpot(
			Vec3(1.5f, 3.0f, 0.f),        // позиція
			Vec3(0.0f, -1.0f, 0.f),       // напрям
			55.0f,                      // кут
			Vec3(1.f, 0.95f, 0.85f),    // колір
			25.0f,                      // range
			8.0f,                        // інтенсивність
			true						// тіні
		);

		lights->addPoint(Vec3(1.5f, 0.2, 0), Vec3(0.8f, 0.4f, 0.2f), 55.0f, 2.9f);
	}

	void GraphicsEngine::render(SwapChain& swapChain)
	{
		m_camera->update();
		auto cameraPos = m_camera->getPosition();
		m_renderSystem->setCameraPosition(cameraPos);

		//DX3D_LOG_INFO("POS: x={}, y={}, z={}", cameraPos.x, cameraPos.y, cameraPos.z);

		Matrix4x4 viewT = m_camera->getViewMatrix();
		Matrix4x4 projT = m_camera->getProjectionMatrix();

		m_renderSystem->renderShadows(m_scene);
		m_renderSystem->beginFrame(swapChain, { 0.2f, 0.2f, 0.2f, 1.0f });

		for (const auto& object : m_scene.getAllObjects())
		{
			if (!object->model) continue;
			Matrix4x4 worldT = object->getWorldTransform().getWorldMatrix();

			m_renderSystem->drawModel(
				*object->model,
				*object->constantBuffer,
				worldT,
				viewT,
				projT
			);
		}

		m_renderSystem->endFrame(*m_graphicsDevice, swapChain, true);
	}
}