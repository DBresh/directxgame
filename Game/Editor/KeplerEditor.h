#pragma once
#include <memory>
#include <string>
#include <DX3D/Game/SceneManager.h>
#include <DX3D/Game/TransformSystem.h>
#include <DX3D/Game/RenderComponentSystem.h>
#include <DX3D/InputSystem/Camera.h>
#include <DX3D/Graphics/Importers/AssetManager.h>
#include <DX3D/Game/Display.h>
#include <DX3D/Graphics/Core/DeviceContext.h>
#include <DX3D/Graphics/Resources/ModelGPU.h>
#include <DX3D/Graphics/Buffers/ConstantBuffer.h>
#include <DX3D/Math/Transform.h>
#include <Game/Kepler/OrbitSystem.h>
#include <Game/Kepler/TimeController.h>
#include <Game/Editor/UIManager.h>
#include <DX3D/Graphics/Rendering/GraphicsEngine.h>

namespace dx3d
{
	class KeplerEditor
	{
	public:
		KeplerEditor(SceneManager& scene, TransformSystem& transforms, RenderComponentSystem& renderables, OrbitSystem& orbitSystem,
			Simulator::TimeController& timeController, Camera& camera,
			AssetManager& assets, Display* display, GraphicsEngine& graphicsEngine);
		~KeplerEditor();

		void init();
		void onUpdate(double dt);
		void onGUI();

		void onDrawDebug(DeviceContext& ctx, const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& proj);

		void onWindowResized(int width, int height);

		Entity getSelectedEntity() const { return m_selectedEntity; }
		void setSelectedEntity(Entity entity) { m_selectedEntity = entity; }

	private:
		void handleMouseWrapping();
		void handleViewportDragAndDrop();

	private:
		UIManager m_uiManager;

		SceneManager& m_scene;
		TransformSystem& m_transforms;
		RenderComponentSystem& m_renderables;
		OrbitSystem& m_orbitSystem;
		Simulator::TimeController& m_timeController;
		Camera& m_camera;
		AssetManager& m_assets;
		Display* m_display;
		GraphicsEngine& m_graphicsEngine;

		Entity m_selectedEntity{ Entity::Null };
		std::shared_ptr<ModelGPU> m_dragPreviewModel{ nullptr };
		ConstantBufferPtr m_dragPreviewConstantBuffer{ nullptr };
		Transform m_dragPreviewTransform{};
		bool m_isDraggingAsset = false;
	};
}