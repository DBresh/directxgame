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
				int orbitIndex = j.value("orbitIndex", -1);
				if (orbitIndex != -1) {
					auto comp = obj->addComponent<OrbitComponent>(&m_orbitSystem, orbitIndex);
					comp->deserialize(j);
					comp->visualizer.init(m_graphicsEngine->getGraphicsDevice());
				}
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


		initUI();
	}

	KeplerSandbox::~KeplerSandbox() = default;

	void KeplerSandbox::initUI()
	{
		m_uiManager.addPanel(std::make_shared<HierarchyPanel>(m_scene, m_selectedObject, *m_camera));
		m_uiManager.addPanel(std::make_shared<InspectorPanel>(m_selectedObject));
		m_uiManager.addPanel(std::make_shared<TimelinePanel>(m_timeController, *m_camera));
		m_uiManager.addPanel(std::make_shared<AssetBrowserPanel>());
	}

	void KeplerSandbox::onWindowResized(int width, int height)
	{
		Game::onWindowResized(width, height);

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

			m_uiManager.setScale(scaleFactor);
		}
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

				orbit.isPathDirty = false;
			}
		}

		syncCameraOrbitTarget();
	}

	void dx3d::KeplerSandbox::onGUI()
	{
		handleMouseWrapping();
		m_uiManager.update();
		handleViewportDragAndDrop();
	}

	void KeplerSandbox::handleMouseWrapping()
	{
		ImGuiIO& io = ImGui::GetIO();
		if (ImGui::IsAnyItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
		{
			ImVec2 pos = io.MousePos;
			bool wrapped = false;

			if (pos.x <= 0.0f) { pos.x = io.DisplaySize.x - 2.0f; wrapped = true; }
			else if (pos.x >= io.DisplaySize.x - 1.0f) { pos.x = 1.0f; wrapped = true; }

			if (wrapped)
			{
				io.WantSetMousePos = true;
				io.MousePos = pos;
				io.MouseDelta = ImVec2(0.0f, 0.0f);
				io.MousePosPrev = pos;
			}
		}
	}

	void KeplerSandbox::handleViewportDragAndDrop()
	{
		if (const ImGuiPayload* activePayload = ImGui::GetDragDropPayload())
		{
			if (activePayload->IsDataType("DND_ASSET_MODEL"))
			{
				ImGuiViewport* viewport = ImGui::GetMainViewport();
				ImGui::SetNextWindowPos(viewport->WorkPos);
				ImGui::SetNextWindowSize(viewport->WorkSize);
				ImGui::SetNextWindowBgAlpha(0.0f);

				ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
					ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
					ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
					ImGuiWindowFlags_NoBackground;

				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
				ImGui::Begin("ViewportDropTarget", nullptr, window_flags);
				ImGui::PopStyleVar();

				ImGui::Dummy(ImGui::GetContentRegionAvail());

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_ASSET_MODEL"))
					{
						const char* modelName = (const char*)payload->Data;

						ImVec2 mousePos = ImGui::GetMousePos();
						int width = m_display->getClientWidth();
						int height = m_display->getClientHeight();

						DirectX::XMVECTOR rayOrigin, rayDir;
						m_camera->screenPointToRay(mousePos.x, mousePos.y, width, height, rayOrigin, rayDir);

						float hitDistance = -1.0f;
						auto hitObj = m_scene.pickObject(rayOrigin, rayDir, m_camera->getPosition(), &hitDistance);

						float scaleSize = 30.0f;

						DirectX::XMVECTOR relativeOffset;

						if (hitObj && hitDistance > 0.0f) {
							float backStepOffset = scaleSize * 0.5f;
							relativeOffset = DirectX::XMVectorScale(rayDir, hitDistance - backStepOffset);
						}
						else {
							relativeOffset = DirectX::XMVectorScale(rayDir, 400.0f);
						}

						dx3d::Vec3d camPos = m_camera->getPosition();
						dx3d::Vec3d finalAbsolutePos(
							camPos.x + DirectX::XMVectorGetX(relativeOffset),
							camPos.y + DirectX::XMVectorGetY(relativeOffset),
							camPos.z + DirectX::XMVectorGetZ(relativeOffset)
						);

						std::string assetStr(modelName);
						std::string entityName = assetStr.substr(0, assetStr.find_last_of('.'));

						auto newObj = m_scene.createObject(entityName);
						newObj->modelName = assetStr;
						newObj->model = m_assets->getModel(assetStr);
						newObj->transform.setScale(DirectX::XMFLOAT3(scaleSize, scaleSize, scaleSize));
						newObj->transform.setPosition(finalAbsolutePos);

						if (m_scene.onObjectCreated) {
							m_scene.onObjectCreated(newObj);
						}

						DX3D_LOG_INFO("Spawned {} at world position ({:.2f}, {:.2f}, {:.2f})",
							entityName, finalAbsolutePos.x, finalAbsolutePos.y, finalAbsolutePos.z);
					}
					ImGui::EndDragDropTarget();
				}
				ImGui::End();
			}
		}
	}

	void KeplerSandbox::syncCameraOrbitTarget() {
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

	void KeplerSandbox::onDrawDebug(DeviceContext& ctx, const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& proj)
	{
		for (const auto& obj : m_scene.getAllObjects())
		{
			auto orbitComp = obj->getComponent<OrbitComponent>();
			if (orbitComp && orbitComp->isVisible)
			{
				Simulator::OrbitData& orbit = orbitComp->getOrbit();
				if (orbit.ParentOrbitIndex != -1)
				{
					const Simulator::OrbitData& parentOrbit = m_orbitSystem.GetOrbit(orbit.ParentOrbitIndex);
					dx3d::Vec3d relParentPos = parentOrbit.absoluteWorldPosition - m_camera->getPosition();
					orbitComp->visualizer.draw(ctx, view, proj, relParentPos);
				}
			}
		}
	}

	void KeplerSandbox::onMouseDown(int button)
	{
		if (button == 0)
		{
			auto mouseState = InputSystem::get()->getMouseState();

			int width = m_display->getClientWidth();
			int height = m_display->getClientHeight();

			DirectX::XMVECTOR origin, dir;
			m_camera->screenPointToRay(mouseState.coords.x, mouseState.coords.y, width, height, origin, dir);

			std::shared_ptr<GameObject> pick = m_scene.pickObject(origin, dir, m_camera->getPosition());

			if (pick) m_selectedObject = pick;
		}
	}

	void KeplerSandbox::onKeyDown(int key)
	{
		if (key == 'F')
		{
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