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

		initSandboxSimulation();
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

		for (auto& body : m_celestialBodies)
		{
			if (body.parentIndex != -1 && body.orbitIndex != -1)
			{
				Simulator::OrbitData& orbit = m_orbitSystem.GetOrbit(body.orbitIndex);
				body.worldPosition = m_celestialBodies[body.parentIndex].worldPosition + orbit.positionRelativeToAttractor;

				auto orbitComp = body.renderObject->getComponent<OrbitComponent>();
				if (orbitComp && orbitComp->isVisible)
				{
					orbitComp->visualizer.update(m_graphicsEngine->getGraphicsDevice(), orbit);
				}

				orbit.isPathDirty = false;
			}

			if (body.renderObject) {
				body.renderObject->transform.setPosition(body.worldPosition);
			}
		}

		if (m_selectedObject && m_camera->isOrbiting()) {
			for (const auto& body : m_celestialBodies) {
				if (body.renderObject == m_selectedObject) {
					m_camera->setOrbitTarget(body.worldPosition);
					break;
				}
			}
		}
	}

	void dx3d::KeplerSandbox::onGUI()
	{
		handleMouseWrapping();
		m_uiManager.update();
		handleViewportDragAndDrop();
		syncCameraOrbitTarget();
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

	void KeplerSandbox::syncCameraOrbitTarget()
	{
		if (m_selectedObject && m_camera->isOrbiting())
		{
			for (const auto& body : m_celestialBodies) {
				if (body.renderObject == m_selectedObject) {
					m_camera->setOrbitTarget(body.worldPosition);
					break;
				}
			}
		}
	}

	void KeplerSandbox::onDrawDebug(DeviceContext& ctx, const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& proj)
	{
		for (auto& body : m_celestialBodies) {
			if (body.parentIndex != -1 && body.renderObject)
			{
				auto orbitComp = body.renderObject->getComponent<OrbitComponent>();
				if (orbitComp && orbitComp->isVisible)
				{
					dx3d::Vec3d relParentPos = m_celestialBodies[body.parentIndex].worldPosition - m_camera->getPosition();
					orbitComp->visualizer.draw(ctx, view, proj, relParentPos);
				}
			}
		}
	}

	void KeplerSandbox::initSandboxSimulation()
	{
		auto planeModel = m_assets->getModel("plane.obj");
		auto plane = m_scene.createObject("plane");
		plane->model = planeModel;
		plane->transform.setPosition(0.0, -70.0, 0.0);
		plane->transform.setScale(DirectX::XMFLOAT3(50.0f, 10.0f, 50.0f));
		plane->constantBuffer = m_graphicsEngine->getGraphicsDevice().createConstantBuffer({ nullptr, sizeof(DirectX::XMFLOAT4X4) * 3 });
		plane->modelName = "plane.obj";

		auto lights = m_graphicsEngine->getLightManager();
		lights->clear();
		lights->addDirectional(DirectX::XMFLOAT3(0.f, -1.f, 0.2f), DirectX::XMFLOAT3(1.f, 1.f, 1.f), 10.f, true);

		auto bodyModel = m_assets->getModel("cube.obj");

		m_celestialBodies.clear();
		m_celestialBodies.resize(35);

		auto setupBody = [&](int index, const std::string& name, int parent, double mass, double distance, Vec3d velocity, float scale)
			{
				auto& body = m_celestialBodies[index];
				body.name = name;
				body.parentIndex = parent;

				body.renderObject = m_scene.createObject(name);
				body.renderObject->model = bodyModel;
				body.renderObject->transform.setScale(DirectX::XMFLOAT3(scale, scale, scale));
				body.renderObject->modelName = "cube.obj";
				GraphicsDevice& gd = m_graphicsEngine->getGraphicsDevice();
				body.renderObject->constantBuffer = gd.createConstantBuffer({ nullptr, sizeof(DirectX::XMFLOAT4X4) * 3 });

				body.visualizer.init(gd);

				body.renderObject->inheritPosition = false;
				body.renderObject->inheritScale = false;

				if (parent != -1)
				{
					Simulator::OrbitData initialOrbit;
					double attractorMass = m_orbitSystem.GetOrbit(m_celestialBodies[parent].orbitIndex).BodyMass;
					initialOrbit.BodyMass = mass;
					initialOrbit.AttractorMass = attractorMass;
					initialOrbit.GravConst = 1.0;
					initialOrbit.positionRelativeToAttractor = Vec3d(distance, 0.0, 0.0);
					initialOrbit.velocityRelativeToAttractor = velocity;
					initialOrbit.ParentOrbitIndex = m_celestialBodies[parent].orbitIndex;

					body.orbitIndex = m_orbitSystem.AddOrbit(initialOrbit);
					Simulator::OrbitData& activeOrbit = m_orbitSystem.GetOrbit(body.orbitIndex);

					Simulator::Kepler::CalculateOrbitStateFromOrbitalVectors(activeOrbit);
					activeOrbit.isPathDirty = true;

					auto orbitComp = body.renderObject->addComponent<OrbitComponent>(&m_orbitSystem, body.orbitIndex);
					orbitComp->visualizer.init(gd);

					m_celestialBodies[parent].renderObject->addChild(body.renderObject);
					body.worldPosition = m_celestialBodies[parent].worldPosition + activeOrbit.positionRelativeToAttractor;
				}
				else
				{
					Simulator::OrbitData sunOrbit;
					sunOrbit.BodyMass = mass;
					sunOrbit.AttractorMass = 0.0;
					sunOrbit.GravConst = 1.0;
					sunOrbit.positionRelativeToAttractor = Vec3d(0.0, 0.0, 0.0);
					sunOrbit.velocityRelativeToAttractor = Vec3d(0.0, 0.0, 0.0);
					sunOrbit.ParentOrbitIndex = -1;
					sunOrbit.isFrozen = true;

					body.orbitIndex = m_orbitSystem.AddOrbit(sunOrbit);

					auto orbitComp = body.renderObject->addComponent<OrbitComponent>(&m_orbitSystem, body.orbitIndex);
					orbitComp->visualizer.init(gd);
					orbitComp->isVisible = false;

					body.worldPosition = Vec3d(0.0, 0.0, 0.0);
					body.renderObject->transform.setPosition(DirectX::XMFLOAT3(0, 0, 0));
				}

				if (body.renderObject) {
					body.renderObject->transform.setPosition(body.worldPosition);
				}
			};

		// 0: Sun
		setupBody(0, "Sun", -1, 1000000.0, 0.0, Vec3d(), 100.0f);

		// --- First Layer: Inner Planets ---
		setupBody(1, "Planet Alpha", 0, 5000.0, 500.0, Vec3d(0.0, 5.0, 44.7), 30.0f);
		setupBody(2, "Planet Beta", 0, 8000.0, 900.0, Vec3d(0.0, 0.0, 33.3), 40.0f);
		setupBody(3, "Planet Gamma", 0, 4000.0, 1400.0, Vec3d(0.0, -8.0, 26.7), 25.0f);

		// --- First Layer: Outer Giants ---
		setupBody(15, "Planet Delta", 0, 15000.0, 4000.0, Vec3d(0.0, 0.0, 15.8), 60.0f);
		setupBody(16, "Planet Epsilon", 0, 9000.0, 6000.0, Vec3d(0.0, 0.0, 12.9), 45.0f);

		// --- Second Layer: Inner Moons ---
		setupBody(4, "Alpha Moon", 1, 100.0, 80.0, Vec3d(0.0, 0.0, 7.9), 8.0f);
		setupBody(5, "Beta Moon 1", 2, 150.0, 120.0, Vec3d(0.0, 2.0, 8.1), 10.0f);
		setupBody(6, "Beta Moon 2", 2, 80.0, 200.0, Vec3d(0.0, 0.0, 6.3), 6.0f);
		setupBody(7, "Gamma Moon", 3, 50.0, 60.0, Vec3d(0.0, 0.0, 8.1), 5.0f);

		// --- Second Layer: Outer Moons ---
		setupBody(17, "Delta Moon 1", 15, 200.0, 150.0, Vec3d(0.0, 0.0, 10.0), 12.0f);
		setupBody(18, "Delta Moon 2", 15, 180.0, 250.0, Vec3d(0.0, 5.0, 7.7), 10.0f);
		setupBody(19, "Delta Moon 3", 15, 100.0, 400.0, Vec3d(0.0, 0.0, 6.1), 8.0f);
		setupBody(20, "Delta Moon 4", 15, 50.0, 600.0, Vec3d(0.0, -2.0, 5.0), 5.0f);
		setupBody(21, "Epsilon Moon 1", 16, 100.0, 200.0, Vec3d(0.0, 0.0, 6.7), 9.0f);
		setupBody(22, "Epsilon Moon 2", 16, 80.0, 350.0, Vec3d(0.0, 0.0, 5.0), 7.0f);

		// --- Third Layer: Deep nesting satellites ---
		setupBody(8, "Alpha Station", 4, 1.0, 25.0, Vec3d(0.0, 0.0, 2.0), 12.0f);
		setupBody(9, "Beta Relay", 5, 1.0, 35.0, Vec3d(0.0, 1.0, 2.0), 12.0f);

		// --- Asteroids and Comets ---
		setupBody(10, "Asteroid 1", 0, 10.0, 2000.0, Vec3d(0.0, 15.0, 25.0), 14.0f);
		setupBody(11, "Asteroid 2", 0, 15.0, 2200.0, Vec3d(-5.0, -5.0, 18.0), 15.0f);
		setupBody(12, "Asteroid 3", 0, 12.0, 2400.0, Vec3d(0.0, 15.0, 20.0), 14.5f);
		setupBody(13, "Asteroid 4", 0, 20.0, 2700.0, Vec3d(0.0, 0.0, 15.0), 16.0f);
		setupBody(14, "Asteroid 5", 0, 5.0, 3000.0, Vec3d(10.0, 10.0, 18.0), 13.0f);
		setupBody(23, "Comet Halley", 0, 2.0, 500.0, Vec3d(0.0, 10.0, 59.0), 13.0f);
		setupBody(24, "Outer Rim Debris", 0, 1.0, 8000.0, Vec3d(0.0, 0.0, 11.1), 15.0f);

		// 25-29: The "Ghost" Belt (Extreme Inclinations)
		for (int i = 25; i < 30; ++i) {
			double dist = 4500.0 + (static_cast<double>(i) * 100.0);
			double orbitalSpeed = std::sqrt(1000000.0 / dist);
			Vec3d velocityDir(0.0, 1.0, 1.0);
			velocityDir = velocityDir.normalized();
			Vec3d stableVelocity = velocityDir * orbitalSpeed;
			setupBody(i, "Polar Debris " + std::to_string(i), 0, 5.0, dist, stableVelocity, 15.0f);
		}

		// 30-34: Nested Moon Swarm for Planet Delta
		for (int i = 30; i < 35; ++i) {
			double dist = 700.0 + (static_cast<double>(i - 30) * 150.0);
			double vMag = std::sqrt(15000.0 / dist);
			setupBody(i, "Delta Sub-Moon " + std::to_string(i - 29), 15, 20.0, dist,
				Vec3d(0.0, 0.5, vMag), 6.0f);
		}
	}

	void KeplerSandbox::rebuildSandboxState()
	{
		m_celestialBodies.clear();
		m_selectedObject = nullptr;

		const auto& objects = m_scene.getAllObjects();
		m_celestialBodies.reserve(objects.size());

		for (const auto& obj : objects)
		{
			CelestialBody body;
			body.name = obj->name;
			body.renderObject = obj;
			body.worldPosition = obj->transform.getPosition();

			auto orbitComp = obj->getComponent<OrbitComponent>();
			if (orbitComp) {
				body.orbitIndex = orbitComp->getOrbitIndex();
			}
			else {
				body.orbitIndex = -1;
			}
			body.parentIndex = -1;

			m_celestialBodies.push_back(body);
		}

		for (auto& body : m_celestialBodies)
		{
			if (body.orbitIndex != -1)
			{
				int physicsParentOrbitIdx = m_orbitSystem.GetOrbit(body.orbitIndex).ParentOrbitIndex;
				if (physicsParentOrbitIdx != -1)
				{
					auto it = std::find_if(m_celestialBodies.begin(), m_celestialBodies.end(),
						[physicsParentOrbitIdx](const CelestialBody& cb) { return cb.orbitIndex == physicsParentOrbitIdx; });

					if (it != m_celestialBodies.end()) {
						body.parentIndex = static_cast<int>(std::distance(m_celestialBodies.begin(), it));
					}
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
				bool currentState = m_camera->isOrbiting();
				for (const auto& body : m_celestialBodies) {
					if (body.renderObject == m_selectedObject) {
						m_camera->setOrbitTarget(body.worldPosition);
						break;
					}
				}
				m_camera->setOrbitMode(!currentState);
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
				rebuildSandboxState();

				DX3D_LOG_INFO("Quick loaded from quicksave.json");
			}
		}
	}
}