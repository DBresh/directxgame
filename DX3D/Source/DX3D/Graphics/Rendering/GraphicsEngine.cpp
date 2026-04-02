#include <DX3D/Graphics/Rendering/GraphicsEngine.h>
#include <DX3D/Graphics/Core/GraphicsDevice.h>
#include <DX3D/Graphics/Core/DeviceContext.h>
#include <DX3D/Graphics/Core/SwapChain.h>
#include <DX3D/Graphics/GraphicsLogUtils.h>
#include <DX3D/Graphics/Resources/Texture2D.h>
#include <DX3D/Core/Time.h>
#include <DX3D/InputSystem/InputSystem.h>
#include <DX3D/Graphics/Importers/ModelCache.h>
#include <DX3D/Graphics/Importers/AssetManager.h>
#include <DX3D/Core/JobSystem.h>

#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <imgui.h>
#include <unordered_map>
#include <DirectXMath.h>
#include <fstream>

using namespace DirectX;

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

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();

		const std::string shaderFileData = loadFileText("DX3D/Assets/Shaders/Basic.hlsl");
		const char* shaderSourceCode = shaderFileData.c_str();
		const size_t shaderSize = shaderFileData.size();

		auto vs = device.compileShader({ "Basic.hlsl", shaderSourceCode, shaderSize, "VSMain", ShaderType::VertexShader });
		auto ps = device.compileShader({ "Basic.hlsl", shaderSourceCode, shaderSize, "PSMain", ShaderType::PixelShader });
		auto vsSig = device.createVertexShaderSignature({ vs });
		m_pipeline = device.createGraphicsPipelineState({ *vsSig, *ps });

		const std::string instancedShaderFileData = loadFileText("DX3D/Assets/Shaders/BasicInstancedVS.hlsl");
		auto vsInstanced = device.compileShader({ "BasicInstancedVS.hlsl", instancedShaderFileData.c_str(), instancedShaderFileData.size(), "VSMain", ShaderType::VertexShader });
		auto vsSigInstanced = device.createVertexShaderSignature({ vsInstanced });
		m_instancedPipeline = device.createGraphicsPipelineState({ *vsSigInstanced, *ps });
		m_testInstanceBuffer = device.createInstanceBuffer({ 10000, sizeof(XMFLOAT4X4) });

		const std::string lineShaderFileData = loadFileText("DX3D/Assets/Shaders/Line.hlsl");
		const char* lineShaderSourceCode = lineShaderFileData.c_str();
		const size_t lineShaderSize = lineShaderFileData.size();

		auto lineVs = device.compileShader({ "Line.hlsl", lineShaderSourceCode, lineShaderSize, "VSMain", ShaderType::VertexShader });
		auto linePs = device.compileShader({ "Line.hlsl", lineShaderSourceCode, lineShaderSize, "PSMain", ShaderType::PixelShader });
		auto lineVsSig = device.createVertexShaderSignature({ lineVs });
		m_linePipeline = device.createGraphicsPipelineState({ *lineVsSig, *linePs });

		AssetManagerDesc aDesc{};
		aDesc.graphicsDevice = m_graphicsDevice;
		aDesc.assetsRoot = std::filesystem::path("DX3D/Assets/Models");
		m_assets = std::make_shared<AssetManager>(aDesc);

		int numThreads = std::thread::hardware_concurrency();
		m_deferredContexts.resize(numThreads);
		m_commandLists.resize(numThreads);

		for (int i = 0; i < numThreads; ++i) {
			m_deferredContexts[i] = m_graphicsDevice->createDeferredContext();
		}

		m_renderSystem = std::make_unique<RenderSystem>(m_graphicsDevice, m_deviceContext);
		m_renderSystem->setPipeline(m_pipeline);
		m_renderSystem->setInstancedPipeline(m_instancedPipeline);

		createCubeMesh();

		m_camera = std::make_unique<Camera>();
		InputSystem::get()->addListener(m_camera.get());
	}

	GraphicsEngine::~GraphicsEngine()
	{
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}

	void GraphicsEngine::initUI(void* hwnd)
	{
		ImGui_ImplWin32_Init(static_cast<HWND>(hwnd));
		ImGui_ImplDX11_Init(m_graphicsDevice->getD3D11Device(),
			m_deviceContext->getD3D11Context());
	}

	void GraphicsEngine::onWindowResized(int width, int height)
	{
		if (m_camera)
		{
			m_camera->setScreenSize((float)width, (float)height);
		}

		if (ImGui::GetCurrentContext())
		{
			float scaleFactor = (static_cast<float>(height) / 1080.0f) * 1.25f;

			if (scaleFactor < 1.0f) {
				scaleFactor = 1.0f;
			}

			ImGui::GetIO().FontGlobalScale = scaleFactor;

			ImGuiStyle& style = ImGui::GetStyle();
			style = ImGuiStyle();
			ImGui::StyleColorsDark();
			style.ScaleAllSizes(scaleFactor);
		}
	}

	GraphicsDevice& GraphicsEngine::getGraphicsDevice() noexcept
	{
		return *m_graphicsDevice;
	}

	void GraphicsEngine::initSandboxSimulation()
	{
		auto bodyModel = m_assets->getModel("cube.obj");

		m_celestialBodies.clear();
		m_celestialBodies.resize(35);

		auto setupBody = [&](int index, const std::string& name, int parent, double mass, double distance, Simulator::Vec3d velocity, float scale)
			{
				auto& body = m_celestialBodies[index];
				body.name = name;
				body.parentIndex = parent;

				body.renderObject = m_scene.createObject(name);
				body.renderObject->model = bodyModel;
				body.renderObject->transform.setScale(DirectX::XMFLOAT3(scale, scale, scale));
				body.renderObject->constantBuffer = m_graphicsDevice->createConstantBuffer({ nullptr, sizeof(DirectX::XMFLOAT4X4) * 3 });

				body.visualizer.init(*m_graphicsDevice);

				if (parent != -1)
				{
					double attractorMass = m_celestialBodies[parent].orbit.BodyMass;
					body.orbit.BodyMass = mass;
					body.orbit.AttractorMass = attractorMass;
					body.orbit.GravConst = 1.0;

					body.orbit.positionRelativeToAttractor = Simulator::Vec3d(distance, 0.0, 0.0);
					body.orbit.velocityRelativeToAttractor = velocity;

					Simulator::Kepler::CalculateOrbitStateFromOrbitalVectors(body.orbit);
					body.orbit.isPathDirty = true;
				}
				else
				{
					body.orbit.BodyMass = mass;
					body.worldPosition = Simulator::Vec3d(0.0, 0.0, 0.0);
				}
			};

		// 0: Sun
		setupBody(0, "Sun", -1, 1000000.0, 0.0, Simulator::Vec3d(), 100.0f);

		// --- First Layer: Inner Planets ---
		setupBody(1, "Planet Alpha", 0, 5000.0, 500.0, Simulator::Vec3d(0.0, 5.0, 44.7), 30.0f);
		setupBody(2, "Planet Beta", 0, 8000.0, 900.0, Simulator::Vec3d(0.0, 0.0, 33.3), 40.0f);
		setupBody(3, "Planet Gamma", 0, 4000.0, 1400.0, Simulator::Vec3d(0.0, -8.0, 26.7), 25.0f);

		// --- First Layer: Outer Giants ---
		setupBody(15, "Planet Delta", 0, 15000.0, 4000.0, Simulator::Vec3d(0.0, 0.0, 15.8), 60.0f);
		setupBody(16, "Planet Epsilon", 0, 9000.0, 6000.0, Simulator::Vec3d(0.0, 0.0, 12.9), 45.0f);

		// --- Second Layer: Inner Moons ---
		setupBody(4, "Alpha Moon", 1, 100.0, 80.0, Simulator::Vec3d(0.0, 0.0, 7.9), 8.0f);
		setupBody(5, "Beta Moon 1", 2, 150.0, 120.0, Simulator::Vec3d(0.0, 2.0, 8.1), 10.0f);
		setupBody(6, "Beta Moon 2", 2, 80.0, 200.0, Simulator::Vec3d(0.0, 0.0, 6.3), 6.0f);
		setupBody(7, "Gamma Moon", 3, 50.0, 60.0, Simulator::Vec3d(0.0, 0.0, 8.1), 5.0f);

		// --- Second Layer: Outer Moons ---
		setupBody(17, "Delta Moon 1", 15, 200.0, 150.0, Simulator::Vec3d(0.0, 0.0, 10.0), 12.0f);
		setupBody(18, "Delta Moon 2", 15, 180.0, 250.0, Simulator::Vec3d(0.0, 5.0, 7.7), 10.0f);
		setupBody(19, "Delta Moon 3", 15, 100.0, 400.0, Simulator::Vec3d(0.0, 0.0, 6.1), 8.0f);
		setupBody(20, "Delta Moon 4", 15, 50.0, 600.0, Simulator::Vec3d(0.0, -2.0, 5.0), 5.0f);
		setupBody(21, "Epsilon Moon 1", 16, 100.0, 200.0, Simulator::Vec3d(0.0, 0.0, 6.7), 9.0f);
		setupBody(22, "Epsilon Moon 2", 16, 80.0, 350.0, Simulator::Vec3d(0.0, 0.0, 5.0), 7.0f);

		// --- Third Layer: Deep nesting satellites ---
		setupBody(8, "Alpha Station", 4, 1.0, 25.0, Simulator::Vec3d(0.0, 0.0, 2.0), 12.0f);
		setupBody(9, "Beta Relay", 5, 1.0, 35.0, Simulator::Vec3d(0.0, 1.0, 2.0), 12.0f);

		// --- Asteroids and Comets ---
		setupBody(10, "Asteroid 1", 0, 10.0, 2000.0, Simulator::Vec3d(0.0, 15.0, 25.0), 14.0f);
		setupBody(11, "Asteroid 2", 0, 15.0, 2200.0, Simulator::Vec3d(-5.0, -5.0, 18.0), 15.0f);
		setupBody(12, "Asteroid 3", 0, 12.0, 2400.0, Simulator::Vec3d(0.0, 15.0, 20.0), 14.5f);
		setupBody(13, "Asteroid 4", 0, 20.0, 2700.0, Simulator::Vec3d(0.0, 0.0, 15.0), 16.0f);
		setupBody(14, "Asteroid 5", 0, 5.0, 3000.0, Simulator::Vec3d(10.0, 10.0, 18.0), 13.0f);
		setupBody(23, "Comet Halley", 0, 2.0, 500.0, Simulator::Vec3d(0.0, 10.0, 59.0), 13.0f);
		setupBody(24, "Outer Rim Debris", 0, 1.0, 8000.0, Simulator::Vec3d(0.0, 0.0, 11.1), 15.0f);

		// 25-29: The "Ghost" Belt (Extreme Inclinations)
		for (int i = 25; i < 30; ++i) {
			double dist = 4500.0 + (static_cast<double>(i) * 100.0);
			double orbitalSpeed = std::sqrt(1000000.0 / dist);
			Simulator::Vec3d velocityDir(0.0, 1.0, 1.0);
			velocityDir = velocityDir.normalized();
			Simulator::Vec3d stableVelocity = velocityDir * orbitalSpeed;
			setupBody(i, "Polar Debris " + std::to_string(i), 0, 5.0, dist, stableVelocity, 15.0f);
		}

		// 30-34: Nested Moon Swarm for Planet Delta
		for (int i = 30; i < 35; ++i) {
			double dist = 700.0 + (static_cast<double>(i - 30) * 150.0);
			double vMag = std::sqrt(15000.0 / dist);
			setupBody(i, "Delta Sub-Moon " + std::to_string(i - 29), 15, 20.0, dist,
				Simulator::Vec3d(0.0, 0.5, vMag), 6.0f);
		}
	}

	void GraphicsEngine::createCubeMesh()
	{
		initSandboxSimulation();

		auto planeModel = m_assets->getModel("plane.obj");

		auto plane = m_scene.createObject("plane");
		plane->model = planeModel;
		plane->transform.setPosition(XMFLOAT3(0.0f, -70.0f, 0.0f));
		plane->transform.setScale(XMFLOAT3(50.0f, 10.0f, 50.0f));
		plane->constantBuffer =
			m_graphicsDevice->createConstantBuffer({ nullptr, sizeof(XMFLOAT4X4) * 3 });

		auto lights = m_renderSystem->getLightManager();
		lights->clear();

		lights->addDirectional(XMFLOAT3(0.f, -1.f, 0.2f), XMFLOAT3(1.f, 1.f, 1.f), 10.f, true);
	}

	void GraphicsEngine::render(SwapChain& swapChain, const std::function<void()>& onGUI)
	{
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		double dt = static_cast<double>(ImGui::GetIO().DeltaTime);
		double scaledDt = dt * static_cast<double>(m_timeWarp);

		for (size_t i = 0; i < m_celestialBodies.size(); ++i)
		{
			auto& body = m_celestialBodies[i];

			if (body.parentIndex != -1)
			{
				Simulator::Kepler::UpdateOrbitAnomaliesByTime(body.orbit, scaledDt);
			}

			if (body.parentIndex != -1)
			{
				body.worldPosition = m_celestialBodies[body.parentIndex].worldPosition + body.orbit.positionRelativeToAttractor;
			}
			else
			{
				body.worldPosition = Simulator::Vec3d(0.0, 0.0, 0.0);
			}

			if (body.renderObject)
			{
				body.renderObject->transform.setPosition(body.worldPosition.toFloat3());
			}

			if (body.parentIndex != -1)
			{
				body.visualizer.update(*m_graphicsDevice, body.orbit);
			}
		}

		ImGui::Begin("Simulator Test");
		ImGui::SliderFloat("Time Warp", &m_timeWarp, 0.0f, 5000.0f);
		ImGui::SliderFloat("Player Speed", &m_camera->m_speed, 2.0f, 500.0f);
		ImGui::End();

		if (onGUI) {
			onGUI();
		}

		m_camera->update();
		const XMFLOAT4X4& view = m_camera->getViewMatrix();
		const XMFLOAT4X4& proj = m_camera->getProjectionMatrix();

		m_renderSystem->setCameraMatrices(view, proj);
		m_renderSystem->buildBatches(m_scene);
		m_renderSystem->renderShadows(*m_testInstanceBuffer);

		m_renderSystem->beginFrame(swapChain, { 0.2f, 0.2f, 0.2f, 1.0f });

		m_deviceContext->setGraphicsPipelineState(*m_linePipeline);
		
		for (auto& body : m_celestialBodies)
		{
			if (body.parentIndex != -1)
			{
				body.visualizer.draw(*m_deviceContext, view, proj, m_celestialBodies[body.parentIndex].worldPosition);
			}
		}
		
		m_deviceContext->setGraphicsPipelineState(*m_pipeline);

		executeSingleDraws(swapChain);
		executeInstancedDraws(swapChain);

		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		m_renderSystem->endFrame(*m_graphicsDevice, swapChain, false);
	}

	void GraphicsEngine::executeSingleDraws(SwapChain& swapChain)
	{
		const auto& singleDrawObjects = m_renderSystem->getSingleDrawObjects();
		if (singleDrawObjects.empty()) return;

		const uint32_t groupSize = 250;

		JobSystem::Dispatch((uint32_t)singleDrawObjects.size(), groupSize, [&](JobDispatchArgs args)
			{
				int ctxIndex = args.groupIndex % m_deferredContexts.size();
				auto& ctx = *m_deferredContexts[ctxIndex];

				ctx.setGraphicsPipelineState(*m_pipeline);
				ctx.setViewportSize(swapChain.getSize());
				ctx.setRenderTarget(swapChain);

				m_renderSystem->setFrameResources(ctx);

				uint32_t count = std::min(groupSize, (uint32_t)singleDrawObjects.size() - args.jobIndex);

				for (uint32_t i = 0; i < count; ++i)
				{
					auto* obj = singleDrawObjects[args.jobIndex + i];

					if (obj->model) {
						m_renderSystem->drawModel(
							ctx,
							*obj->model,
							*obj->constantBuffer,
							obj->transform.getWorldMatrix()
						);
					}
				}
			});

		JobSystem::Wait();

		for (int i = 0; i < m_deferredContexts.size(); ++i)
		{
			HRESULT hr = m_deferredContexts[i]->getD3D11Context()->FinishCommandList(
				false, &m_commandLists[i]
			);

			if (SUCCEEDED(hr) && m_commandLists[i]) {
				m_deviceContext->getD3D11Context()->ExecuteCommandList(
					m_commandLists[i].Get(), false
				);
			}
			m_commandLists[i].Reset();
		}
	}

	void GraphicsEngine::executeInstancedDraws(SwapChain& swapChain)
	{
		m_deviceContext->setRenderTarget(swapChain);
		m_deviceContext->setViewportSize(swapChain.getSize());
		m_renderSystem->drawInstancedBatches(*m_deviceContext, *m_testInstanceBuffer);
	}

	static void LogMatrix(const char* name, CXMMATRIX M)
	{
		XMFLOAT4X4 m;
		XMStoreFloat4x4(&m, M);
		DX3D_LOG_INFO(
			"Matrix Log: {} [as Row-Major]\n"
			"  [{: 8.2f}, {: 8.2f}, {: 8.2f}, {: 8.2f}]\n"
			"  [{: 8.2f}, {: 8.2f}, {: 8.2f}, {: 8.2f}]\n"
			"  [{: 8.2f}, {: 8.2f}, {: 8.2f}, {: 8.2f}]\n"
			"  [{: 8.2f}, {: 8.2f}, {: 8.2f}, {: 8.2f}]",
			name,
			m._11, m._12, m._13, m._14,
			m._21, m._22, m._23, m._24,
			m._31, m._32, m._33, m._34,
			m._41, m._42, m._43, m._44
		);
	}
}
