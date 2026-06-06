#pragma once
#include <memory>
#include <vector>
#include <wrl.h>
#include <DirectXMath.h>

#include <DX3D/Core/Core.h>
#include <DX3D/Core/Common.h>
#include <DX3D/Graphics/Core/GraphicsDevice.h>
#include <DX3D/Graphics/Core/DeviceContext.h>
#include <DX3D/Graphics/Core/SwapChain.h>
#include <DX3D/Graphics/Core/GraphicsPipelineState.h>
#include <DX3D/Graphics/Resources/Mesh.h>
#include <DX3D/Graphics/Buffers/ConstantBuffer.h>
#include <DX3D/Graphics/Resources/ModelData.h>
#include <DX3D/Graphics/Rendering/LightManager.h>
#include <DX3D/Graphics/Buffers/StructuredBuffer.h>
#include <DX3D/InputSystem/Camera.h>
#include <DX3D/Game/RenderComponentSystem.h>
#include <DX3D/Game/TransformSystem.h>
#include <DX3D/Game/RuntimeWorld.h>

namespace dx3d
{
	using namespace DirectX;

	struct MaterialDataGPU
	{
		DirectX::XMFLOAT3 albedo;
		float roughness;
		float metallic;
		DirectX::XMFLOAT3 padding;
	};

	class RenderSystem
	{
	public:
		struct SingleDrawItem
		{
			ModelGPU* model = nullptr;
			const ConstantBuffer* objectCB = nullptr;
			DirectX::XMFLOAT4X4 worldMatrix{};
		};
		struct InstancedDrawItem
		{
			ModelGPU* model = nullptr;
			uint32_t transformStartIndex = 0;
			uint32_t instanceCount = 0;
		};

		struct SceneRenderProxy
		{
			ModelGPU* model = nullptr;
			DirectX::XMFLOAT4X4 worldMatrix{};
			AABB localBounds{};
		};

		RenderSystem(std::shared_ptr<GraphicsDevice> device,
			DeviceContextPtr context);

		void setPipeline(GraphicsPipelineStatePtr pipeline) noexcept;
		void setPSSampler(ID3D11SamplerState* sampler) noexcept;
		void beginFrame(SwapChain& swapChain, const XMFLOAT4& clearColor, const Camera& camera);
		void endFrame(GraphicsDevice& device, SwapChain& swapChain, bool vsync);
		void setCameraMatrices(const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& proj);
		void setFrameResources(DeviceContext& context);

		void drawModel(
			DeviceContext& context,
			const ModelGPU& model,
			const ConstantBuffer& objectCB,
			const DirectX::XMFLOAT4X4& world);

		void drawModelInstanced(
			DeviceContext& context,
			const ModelGPU& model,
			const InstanceBuffer& instanceBuffer,
			unsigned int instanceCount);

		void setCameraPosition(const XMFLOAT3& pos) noexcept { m_cameraPosition = pos; }
		LightManager* getLightManager() const noexcept { return m_lightManager.get(); }
		void setInstancedPipeline(GraphicsPipelineStatePtr pipeline) noexcept;
		void drawInstancedBatches(DeviceContext& context, InstanceBuffer& instanceBuffer);

		void renderShadows(InstanceBuffer& instanceBuffer, const Camera& camera);
		void buildBatches(RuntimeWorld& world, const Camera& camera);

		const std::vector<SingleDrawItem>& getSingleDrawObjects() const noexcept { return m_singleDrawObjects; }

	private:
		void renderSingleLightShadows(const Light& light, int shadowIndex, InstanceBuffer& instanceBuffer, const Camera& camera);

	private:
		std::shared_ptr<GraphicsDevice> m_device;
		DeviceContextPtr                m_context;
		GraphicsPipelineStatePtr        m_pipeline;
		std::unique_ptr<LightManager>   m_lightManager;

		ConstantBufferPtr m_materialBuffer;

		Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_shadowRasterizer;

		ConstantBufferPtr m_cameraBuffer;
		XMFLOAT3          m_cameraPosition{ 0, 0, 0 };

		DirectX::XMFLOAT4X4 m_viewGPU{};
		DirectX::XMFLOAT4X4 m_projGPU{};

		Microsoft::WRL::ComPtr<ID3D11VertexShader> m_depthVS;
		ConstantBufferPtr m_depthCB;
		ConstantBufferPtr m_lightMatrixBuffer;

		Microsoft::WRL::ComPtr<ID3D11SamplerState> m_psSampler;
		Microsoft::WRL::ComPtr<ID3D11SamplerState> m_shadowSampler;

		GraphicsPipelineStatePtr m_instancedPipeline;
		Microsoft::WRL::ComPtr<ID3D11VertexShader> m_instancedDepthVS;

		std::vector<SingleDrawItem> m_singleDrawObjects;
		std::vector<InstancedDrawItem> m_instancedDraws;
		std::vector<DirectX::XMFLOAT4X4> m_instancedTransforms;
		std::vector<SceneRenderProxy> m_sceneProxies;
		ConstantBufferPtr m_instancedTransformCB;

		struct TransformData
		{
			XMFLOAT4X4 world;
			XMFLOAT4X4 view;
			XMFLOAT4X4 projection;
		};
	};
}
