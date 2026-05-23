#include <Game/Kepler/KeplerSandbox.h>
#include <DX3D/InputSystem/InputSystem.h>
#include <DX3D/Graphics/Rendering/GraphicsEngine.h>
#include <DX3D/Game/Display.h>
#include <DX3D/Core/Logger.h>
#include <DX3D/Math/Vec3d.h>
#include <Game/Editor/HierarchyPanel.h>
#include <Game/Editor/InspectorPanel.h>
#include <Game/Editor/TimelinePanel.h>
#include <Game/Editor/AssetBrowserPanel.h>
#include <Game/Components/OrbitComponent.h>
#include <Game/Components/OrbitVisualizerComponent.h>
#include <imgui.h>

namespace dx3d
{
	KeplerSandbox::KeplerSandbox(const GameDesc& desc) : Game(desc)
	{
		m_scene.onObjectCreated = [this](std::shared_ptr<GameObject> obj) {
			obj->constantBuffer = m_graphicsEngine->getGraphicsDevice().createConstantBuffer({ nullptr, sizeof(DirectX::XMFLOAT4X4) * 3 });
			};

		m_scene.onComponentFactory = [this](GameObject* obj, const std::string& type, const nlohmann::json& j) {
			if (type == "OrbitComponent") {
				auto comp = obj->addComponent<OrbitComponent>(&m_orbitSystem, obj->entity);
				comp->deserialize(j);
				comp->visualizer.init(m_graphicsEngine->getGraphicsDevice());
			}
			};

		std::ifstream file("default_scene.json");
		if (file.is_open())
		{
			nlohmann::json loadJson;
			file >> loadJson;

			m_timeController.Epoch = loadJson.value("epoch", 0.0);
			m_orbitSystem.loadFromJson(loadJson["orbitSystem"]);
			m_scene.loadScene(loadJson["gameObjects"], *m_assets);

			DX3D_LOG_INFO("Loaded default_scene.json");
		}

		m_timeController.SetTimeWarpByIndex(2);
		auto lights = m_graphicsEngine->getLightManager();
		lights->clear();
		lights->addDirectional(DirectX::XMFLOAT3(0.f, -1.f, 0.2f), DirectX::XMFLOAT3(1.f, 1.f, 1.f), 1.2f, true);

		m_editor = std::make_unique<KeplerEditor>(m_scene, m_orbitSystem, m_timeController, *m_camera, *m_assets, m_display.get(), *m_graphicsEngine);
		m_editor->init();
	}

	KeplerSandbox::~KeplerSandbox() = default;

	void KeplerSandbox::onWindowResized(int width, int height)
	{
		Game::onWindowResized(width, height);
		m_editor->onWindowResized(width, height);
	}

	void KeplerSandbox::onUpdate(double dt, double fdt)
	{
		m_timeController.Update(dt);
		double scaledDt = m_timeController.GetScaledDeltaTime(dt);

		m_orbitSystem.UpdateAll(scaledDt);

		for (const auto& obj : m_scene.getAllObjects()) {
			auto orbitComp = obj->getComponent<OrbitComponent>();
			if (orbitComp) {
				Simulator::OrbitData& orbit = orbitComp->getOrbit();
				obj->transform.setPosition(orbit.absoluteWorldPosition);

				if (orbitComp->isVisible) {
					orbitComp->visualizer.update(m_graphicsEngine->getGraphicsDevice(), orbit);
				}

				
			}
		}

		syncCameraOrbitTarget();
	}

	void dx3d::KeplerSandbox::onGUI()
	{
		m_editor->onGUI();
	}

	void KeplerSandbox::onDrawDebug(DeviceContext& ctx, const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& proj)
	{
		m_editor->onDrawDebug(ctx, view, proj);
	}

	void KeplerSandbox::syncCameraOrbitTarget() {
		auto m_selectedObject = m_editor->getSelectedObject();
		if (m_selectedObject && m_camera->isOrbiting()) {
			auto orbitComp = m_selectedObject->getComponent<OrbitComponent>();
			if (orbitComp) {
				m_camera->setOrbitTarget(orbitComp->getOrbit().absoluteWorldPosition);
			}
			else {
				m_camera->setOrbitTarget(m_selectedObject->transform.getPosition());
			}
		}
	}

	void KeplerSandbox::onMouseDown(int button)
	{
		if (ImGui::GetIO().WantCaptureMouse) return;

		if (button == 0)
		{
			auto mouseState = InputSystem::get()->getMouseState();
			int width = m_display->getClientWidth();
			int height = m_display->getClientHeight();

			DirectX::XMVECTOR dir;
			dx3d::Vec3d origin;
			m_camera->screenPointToRay(mouseState.coords.x, mouseState.coords.y, width, height, origin, dir);

			std::shared_ptr<GameObject> pick = m_scene.pickObject(origin, dir, m_camera->getPosition());

			m_editor->setSelectedObject(pick);
		}
	}

	void KeplerSandbox::onKeyDown(int key)
	{
		if (key == 'F')
		{
			auto m_selectedObject = m_editor->getSelectedObject();
			if (m_selectedObject)
			{
				bool isOrbiting = !m_camera->isOrbiting();
				m_camera->setOrbitMode(isOrbiting);

				if (isOrbiting) {
					syncCameraOrbitTarget();
				}
			}
			else
			{
				m_camera->setOrbitMode(false);
			}
		}
		if (key == VK_OEM_PERIOD)
		{
			m_timeController.IncreaseWarp();
		}
		if (key == VK_OEM_COMMA)
		{
			m_timeController.DecreaseWarp();
		}
		if (key == 'P')
		{
			m_timeController.SetPaused(!m_timeController.IsPaused());
		}

		if (key == VK_F5)
		{
			nlohmann::json saveJson;
			saveJson["version"] = 1;
			saveJson["orbitSystem"] = m_orbitSystem.saveToJson();
			saveJson["epoch"] = m_timeController.Epoch;
			saveJson["gameObjects"] = m_scene.saveScene();

			std::ofstream file("quicksave.json");
			file << saveJson.dump(4);
			DX3D_LOG_INFO("Quick saved to quicksave.json");
		}

		if (key == VK_F6)
		{
			std::ifstream file("quicksave.json");
			if (file.is_open())
			{
				nlohmann::json loadJson;
				file >> loadJson;

				m_timeController.Epoch = loadJson.value("epoch", 0.0);
				m_orbitSystem.loadFromJson(loadJson["orbitSystem"]);
				m_scene.loadScene(loadJson["gameObjects"], *m_assets);

				DX3D_LOG_INFO("Quick loaded from quicksave.json");
			}
		}
	}
}