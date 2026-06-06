#include <DX3D/Graphics/Rendering/GraphicsEngine.h>
#include <DX3D/Graphics/Core/GraphicsDevice.h>
#include <DX3D/Graphics/Core/DeviceContext.h>
#include <DX3D/Graphics/Core/SwapChain.h>
#include <DX3D/Graphics/GraphicsLogUtils.h>
#include <DX3D/Graphics/Resources/Texture2D.h>
#include <DX3D/Core/Time.h>
#include <DX3D/Graphics/Importers/ModelCache.h>
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
		m_deviceContext = m_graphicsDevice->createDeviceContext();

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();

		compileShaders();
		initializeThreading();

		m_renderSystem = std::make_unique<RenderSystem>(m_graphicsDevice, m_deviceContext);
		m_renderSystem->setPipeline(m_pipeline);
		m_renderSystem->setInstancedPipeline(m_instancedPipeline);
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

	void GraphicsEngine::render(
		RuntimeWorld& world,
		Camera& camera,
		SwapChain& swapChain,
		const std::function<void()>& onGUI,
		const std::function<void(DeviceContext&, const DirectX::XMFLOAT4X4&, const DirectX::XMFLOAT4X4&)>& onDrawDebug)
	{
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// GUI Callback
		if (onGUI) {
			onGUI();
		}

		camera.update();
		const XMFLOAT4X4& view = camera.getViewMatrix();
		const XMFLOAT4X4& proj = camera.getProjectionMatrix();

		m_renderSystem->setCameraMatrices(view, proj);
		m_renderSystem->buildBatches(world, camera);
		m_renderSystem->renderShadows(*m_instanceBuffer, camera);

		m_renderSystem->beginFrame(swapChain, { 0.2f, 0.2f, 0.2f, 1.0f }, camera);

		// Debug Lines Callback (Renderer sets the pipeline, Game draws the lines)
		if (onDrawDebug) {
			m_deviceContext->setGraphicsPipelineState(*m_linePipeline);
			onDrawDebug(*m_deviceContext, view, proj);
		}

		m_deviceContext->setGraphicsPipelineState(*m_pipeline);

		executeSingleDraws(swapChain, camera);
		executeInstancedDraws(swapChain);

		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		m_renderSystem->endFrame(*m_graphicsDevice, swapChain, false);
	}

	void GraphicsEngine::executeSingleDraws(SwapChain& swapChain, const Camera& camera)
	{
		const auto& singleDrawObjects = m_renderSystem->getSingleDrawObjects();
		if (singleDrawObjects.empty()) return;

		const uint32_t groupSize = 250;

		if (singleDrawObjects.size() <= groupSize)
		{
			m_deviceContext->setGraphicsPipelineState(*m_pipeline);
			m_deviceContext->setViewportSize(swapChain.getSize());
			m_deviceContext->setRenderTarget(swapChain);
			m_renderSystem->setFrameResources(*m_deviceContext);

				for (const auto& item : singleDrawObjects)
				{
					if (item.model && item.objectCB) {
						m_renderSystem->drawModel(
							*m_deviceContext,
							*item.model,
							*item.objectCB,
							item.worldMatrix
						);
					}
				}
			return;
		}

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
					const auto& item = singleDrawObjects[args.jobIndex + i];
					if (item.model && item.objectCB) {
						m_renderSystem->drawModel(
							ctx,
							*item.model,
							*item.objectCB,
							item.worldMatrix
						);
					}
				}
			});

		JobSystem::Wait();

		uint32_t numGroups = ((uint32_t)singleDrawObjects.size() + groupSize - 1) / groupSize;
		uint32_t contextsUsed = std::min(numGroups, (uint32_t)m_deferredContexts.size());

		for (uint32_t i = 0; i < contextsUsed; ++i)
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
		m_renderSystem->drawInstancedBatches(*m_deviceContext, *m_instanceBuffer);
	}

	void GraphicsEngine::compileShaders()
	{
		auto& device = *m_graphicsDevice;

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
		m_instanceBuffer = device.createInstanceBuffer({ 10000, sizeof(DirectX::XMFLOAT4X4) });

		const std::string lineShaderFileData = loadFileText("DX3D/Assets/Shaders/Line.hlsl");
		const char* lineShaderSourceCode = lineShaderFileData.c_str();
		const size_t lineShaderSize = lineShaderFileData.size();

		auto lineVs = device.compileShader({ "Line.hlsl", lineShaderSourceCode, lineShaderSize, "VSMain", ShaderType::VertexShader });
		auto linePs = device.compileShader({ "Line.hlsl", lineShaderSourceCode, lineShaderSize, "PSMain", ShaderType::PixelShader });
		auto lineVsSig = device.createVertexShaderSignature({ lineVs });
		m_linePipeline = device.createGraphicsPipelineState({ *lineVsSig, *linePs });
	}

	void GraphicsEngine::initializeThreading()
	{
		int numThreads = std::thread::hardware_concurrency();
		m_deferredContexts.resize(numThreads);
		m_commandLists.resize(numThreads);

		for (int i = 0; i < numThreads; ++i) {
			m_deferredContexts[i] = m_graphicsDevice->createDeferredContext();
		}
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
