#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/GraphicsDevice.h>
#include <DX3D/Graphics/DeviceContext.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/GraphicsLogUtils.h>
#include <DX3D/Graphics/Texture2D.h>
#include <DX3D/Core/Time.h>
#include <DX3D/InputSystem/InputSystem.h>
#include <DX3D/Graphics/ModelCache.h>
#include <DX3D/Graphics/AssetManager.h>
#include <DX3D/Core/JobSystem.h>

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
		m_renderSystem->setPipeline(m_instancedPipeline);

		createCubeMesh();

		m_camera = std::make_unique<Camera>();
		InputSystem::get()->addListener(m_camera.get());
	}

	GraphicsEngine::~GraphicsEngine() = default;

	GraphicsDevice& GraphicsEngine::getGraphicsDevice() noexcept
	{
		return *m_graphicsDevice;
	}

	void GraphicsEngine::createCubeMesh()
	{
		auto model = m_assets->getModel("cube.obj");

		for (int i = 1; i <= 3; i++)
		{
			for (int j = 1; j <= 3; j++)
			{
				auto cube = m_scene.createObject("cube");
				cube->model = model;

				cube->transform.setPosition(XMFLOAT3(i * 1.5f, 0.0f, j * -2.0f));
				cube->transform.setScale(XMFLOAT3(1.0f, 1.0f, 1.0f));

				cube->constantBuffer =
					m_graphicsDevice->createConstantBuffer({ nullptr, sizeof(XMFLOAT4X4) * 3 });
			}
		}

		auto planeModel = m_assets->getModel("plane.obj");

		auto plane = m_scene.createObject("plane");
		plane->model = planeModel;
		plane->transform.setPosition(XMFLOAT3(0.0f, -2.0f, 0.0f));
		plane->transform.setScale(XMFLOAT3(5.0f, 5.0f, 5.0f));
		plane->constantBuffer =
			m_graphicsDevice->createConstantBuffer({ nullptr, sizeof(XMFLOAT4X4) * 3 });

		auto plane1 = m_scene.createObject("plane");
		plane1->model = planeModel;
		plane1->transform.setPosition(XMFLOAT3(1.5f, 1.25f, 5.0f));
		plane1->transform.setScale(XMFLOAT3(0.1f, 0.1f, 0.1f));
		plane1->constantBuffer =
			m_graphicsDevice->createConstantBuffer({ nullptr, sizeof(XMFLOAT4X4) * 3 });

		auto lights = m_renderSystem->getLightManager();
		lights->clear();

		lights->addSpot(
			XMFLOAT3(1.5f, 1.25f, 5.f),
			XMFLOAT3(1.5f, -0.59f, -2.9f),
			50.0f,
			XMFLOAT3(1.f, 0.95f, 0.85f),
			25.0f,
			25.0f,
			true
		);

		//lights->addDirectional(
		//	XMFLOAT3(0.f, -1.f, 0.f),
		//	XMFLOAT3(1.f, 0.f, 0.f),
		//	10.5f,
		//	true
		//);

		lights->addSpot(
			XMFLOAT3(1.5f, 1.25f, -10.f),
			XMFLOAT3(1.5f, -0.59f, 2.9f),
			50.0f,
			XMFLOAT3(1.f, 0.2f, 0.1f),
			25.0f,
			25.0f,
			true
		);
	}

	void GraphicsEngine::render(SwapChain& swapChain)
	{
		m_camera->update();
		const XMFLOAT4X4& view = m_camera->getViewMatrix();
		const XMFLOAT4X4& proj = m_camera->getProjectionMatrix();

		m_renderSystem->setCameraMatrices(view, proj);

		buildRenderBatches();
		m_renderSystem->renderShadows(m_singleDrawObjects, m_instancedBatches, *m_testInstanceBuffer);
		m_renderSystem->beginFrame(swapChain, { 0.2f, 0.2f, 0.2f, 1.0f });
		executeSingleDraws(swapChain);
		executeInstancedDraws(swapChain);

		m_renderSystem->endFrame(*m_graphicsDevice, swapChain, false);
	}

	void GraphicsEngine::buildRenderBatches()
	{
		m_singleDrawObjects.clear();
		m_instancedBatches.clear();

		auto& allObjects = m_scene.getAllObjects();
		std::unordered_map<ModelGPU*, std::vector<GameObject*>> modelGroups;

		for (auto& objPtr : allObjects)
		{
			if (objPtr->model) {
				modelGroups[objPtr->model.get()].push_back(objPtr.get());
			}
		}

		for (auto& [model, objects] : modelGroups)
		{
			if (objects.size() == 1)
			{
				m_singleDrawObjects.push_back(objects[0]);
			}
			else if (objects.size() > 1)
			{
				InstancedBatch batch;
				batch.model = model;
				batch.cb = objects[0]->constantBuffer.get();
				batch.matrices.reserve(objects.size());

				for (auto* obj : objects) {
					batch.matrices.push_back(obj->transform.getWorldMatrix());
				}
				m_instancedBatches.push_back(std::move(batch));
			}
		}
	}

	void GraphicsEngine::executeSingleDraws(SwapChain& swapChain)
	{
		if (m_singleDrawObjects.empty()) return;

		const uint32_t groupSize = 250;

		JobSystem::Dispatch((uint32_t)m_singleDrawObjects.size(), groupSize, [&](JobDispatchArgs args)
			{
				int ctxIndex = args.groupIndex % m_deferredContexts.size();
				auto& ctx = *m_deferredContexts[ctxIndex];

				ctx.setGraphicsPipelineState(*m_pipeline);
				ctx.setViewportSize(swapChain.getSize());
				ctx.setRenderTarget(swapChain);

				m_renderSystem->setFrameResources(ctx);

				uint32_t count = std::min(groupSize, (uint32_t)m_singleDrawObjects.size() - args.jobIndex);

				for (uint32_t i = 0; i < count; ++i)
				{
					auto* obj = m_singleDrawObjects[args.jobIndex + i];

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
		if (m_instancedBatches.empty()) return;

		m_deviceContext->setGraphicsPipelineState(*m_instancedPipeline);
		m_deviceContext->setRenderTarget(swapChain);
		m_deviceContext->setViewportSize(swapChain.getSize());
		m_deviceContext->getD3D11Context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_renderSystem->setFrameResources(*m_deviceContext);

		for (const auto& batch : m_instancedBatches)
		{
			m_testInstanceBuffer->resize((unsigned int)batch.matrices.size());

			auto mapped = m_deviceContext->mapBuffer(m_testInstanceBuffer->getBuffer());
			memcpy(mapped.pData, batch.matrices.data(), batch.matrices.size() * sizeof(XMFLOAT4X4));
			m_deviceContext->unmapBuffer(m_testInstanceBuffer->getBuffer());

			m_renderSystem->drawModelInstanced(
				*m_deviceContext,
				*batch.model,
				*batch.cb,
				*m_testInstanceBuffer,
				(unsigned int)batch.matrices.size()
			);
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
