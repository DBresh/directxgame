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
		auto cubeModel = m_assets->getModel("cube.obj");

		m_earth = m_scene.createObject("Earth");
		m_earth->model = cubeModel;
		m_earth->transform.setPosition(XMFLOAT3(0.0f, 0.0f, 0.0f));
		m_earth->transform.setScale(XMFLOAT3(50.0f, 50.0f, 50.0f));
		m_earth->constantBuffer = m_graphicsDevice->createConstantBuffer({ nullptr, sizeof(XMFLOAT4X4) * 3 });

		m_moon = m_scene.createObject("Moon");
		m_moon->model = cubeModel;
		m_moon->transform.setScale(XMFLOAT3(10.0f, 10.0f, 10.0f));
		m_moon->constantBuffer = m_graphicsDevice->createConstantBuffer({ nullptr, sizeof(XMFLOAT4X4) * 3 });

		m_moonOrbit.AttractorMass = 100000.0;
		m_moonOrbit.GravConst = 1.0;
		m_moonOrbit.positionRelativeToAttractor = Simulator::Vec3d(100.0, 30.0, 0.0);
		m_moonOrbit.velocityRelativeToAttractor = Simulator::Vec3d(0.0, 0.0, 31.62);

		Simulator::Kepler::CalculateOrbitStateFromOrbitalVectors(m_moonOrbit);
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

		Simulator::Kepler::UpdateOrbitAnomaliesByTime(m_moonOrbit, scaledDt);

		DirectX::XMFLOAT3 newMoonPos = m_moonOrbit.positionRelativeToAttractor.toFloat3();
		m_moon->transform.setPosition(newMoonPos);

		ImGui::Begin("Simulator Test");
		ImGui::SliderFloat("Time Warp", &m_timeWarp, 0.0f, 50.0f);
		ImGui::Text("Moon Pos: X:%.1f Y:%.1f Z:%.1f", newMoonPos.x, newMoonPos.y, newMoonPos.z);
		ImGui::Text("Eccentricity: %.4f", m_moonOrbit.Eccentricity);
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
