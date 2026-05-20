#pragma once
#include <memory>
#include <string>
#include <DX3D/Game/SceneManager.h>
#include <DX3D/InputSystem/Camera.h>
#include <DX3D/Graphics/Importers/AssetManager.h>
#include <DX3D/Game/Display.h>
#include <DX3D/Graphics/Core/DeviceContext.h>
#include <Game/Kepler/OrbitSystem.h>
#include <Game/Kepler/TimeController.h>
#include <Game/Editor/UIManager.h>
#include <DX3D/Graphics/Rendering/GraphicsEngine.h>

namespace dx3d
{
	class KeplerEditor
	{
	public:
		KeplerEditor(SceneManager& scene, Simulator::OrbitSystem& orbitSystem,
			Simulator::TimeController& timeController, Camera& camera,
			AssetManager& assets, Display* display, GraphicsEngine& graphicsEngine);
		~KeplerEditor();

		void init();
		void onUpdate(double dt);
		void onGUI();

		void onDrawDebug(DeviceContext& ctx, const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& proj);

		void onWindowResized(int width, int height);

		std::shared_ptr<GameObject> getSelectedObject() const { return m_selectedObject; }
		void setSelectedObject(std::shared_ptr<GameObject> obj) { m_selectedObject = obj; }

	private:
		void handleMouseWrapping();
		void handleViewportDragAndDrop();

	private:
		UIManager m_uiManager;

		SceneManager& m_scene;
		Simulator::OrbitSystem& m_orbitSystem;
		Simulator::TimeController& m_timeController;
		Camera& m_camera;
		AssetManager& m_assets;
		Display* m_display;
		GraphicsEngine& m_graphicsEngine;

		std::shared_ptr<GameObject> m_selectedObject{ nullptr };
		std::shared_ptr<GameObject> m_dragPreviewObject{ nullptr };
		bool m_isDraggingAsset = false;
	};
}