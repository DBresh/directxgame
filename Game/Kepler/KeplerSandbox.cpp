#include <Game/Kepler/KeplerSandbox.h>
#include <DX3D/InputSystem/InputSystem.h>
#include <DX3D/Graphics/Rendering/GraphicsEngine.h>
#include <DX3D/Game/Display.h>
#include <DX3D/Core/Logger.h>
#include <Game/Editor/HierarchyPanel.h>
#include <Game/Editor/InspectorPanel.h>
#include <Game/Editor/TimelinePanel.h>
#include <Game/Components/OrbitComponent.h>
#include <Game/Components/OrbitVisualizerComponent.h>
#include <imgui.h>

namespace dx3d
{
	KeplerSandbox::KeplerSandbox(const GameDesc& desc) : Game(desc)
	{
		initSandboxSimulation();
		initUI();

	}

	KeplerSandbox::~KeplerSandbox() = default;

	void KeplerSandbox::initUI()
	{
		m_uiManager.addPanel(std::make_shared<HierarchyPanel>(m_scene, m_selectedObject));
		m_uiManager.addPanel(std::make_shared<InspectorPanel>(m_selectedObject));
		m_uiManager.addPanel(std::make_shared<TimelinePanel>(m_timeController, *m_camera));
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

		for (auto& body : m_celestialBodies)
		{
			if (body.orbit.isPathDirty)
			{
				Simulator::Kepler::CalculateOrbitStateFromOrbitalVectors(body.orbit);

				if (!body.orbit.freezeColor)
				{
					double currentSpeed = body.orbit.velocityRelativeToAttractor.magnitude();
					double referenceSpeed = sqrt(body.orbit.GravConst * body.orbit.AttractorMass / body.orbit.SemiMajorAxis);
					float speedRatio = static_cast<float>(currentSpeed / referenceSpeed);

					body.orbit.orbitColor.x = speedRatio - 0.5f; // Red increases as we go fast
					body.orbit.orbitColor.y = 1.0f - abs(speedRatio - 1.0f); // Green peaks at circular speed
					body.orbit.orbitColor.z = 1.5f - speedRatio; // Blue for slow speeds
					body.orbit.orbitColor.w = 1.0f;
				}
			}

			if (body.parentIndex != -1) {
				Simulator::Kepler::UpdateOrbitAnomaliesByTime(body.orbit, scaledDt);

				body.worldPosition = m_celestialBodies[body.parentIndex].worldPosition + body.orbit.positionRelativeToAttractor;
				auto visComp = body.renderObject->getComponent<OrbitVisualizerComponent>();
				bool shouldDraw = visComp ? visComp->isVisible : true;
				if (shouldDraw)
				{
					body.visualizer.update(m_graphicsEngine->getGraphicsDevice(), body.orbit);
				}

				body.orbit.isPathDirty = false;

				if (body.renderObject) {
					body.renderObject->transform.setPosition(DirectX::XMFLOAT3(
						static_cast<float>(body.worldPosition.x),
						static_cast<float>(body.worldPosition.y),
						static_cast<float>(body.worldPosition.z)
					));
				}
			}
			else {
				if (body.renderObject)
				{
					auto pos = body.renderObject->transform.getPosition();
					body.worldPosition = Simulator::Vec3d(pos.x, pos.y, pos.z);
				}
				else
				{
					body.worldPosition = Simulator::Vec3d(0.0, 0.0, 0.0);
				}
			}
		}

		if (m_selectedObject && m_camera->isOrbiting()) {
			m_camera->setOrbitTarget(m_selectedObject->getWorldTransform().getPosition());
		}
	}

	void dx3d::KeplerSandbox::onGUI()
	{
		ImGuiIO& io = ImGui::GetIO();

		if (ImGui::IsAnyItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
		{
			ImVec2 pos = io.MousePos;
			bool wrapped = false;

			if (pos.x <= 0.0f)
			{
				pos.x = io.DisplaySize.x - 2.0f;
				wrapped = true;
			}
			else if (pos.x >= io.DisplaySize.x - 1.0f)
			{
				pos.x = 1.0f;
				wrapped = true;
			}

			if (wrapped)
			{
				io.WantSetMousePos = true;
				io.MousePos = pos;

				io.MouseDelta = ImVec2(0.0f, 0.0f);
				io.MousePosPrev = pos;
			}
		}

		m_uiManager.update();

		if (m_selectedObject && m_camera->isOrbiting())
		{
			m_camera->setOrbitTarget(m_selectedObject->getWorldTransform().getPosition());
		}
	}

	void KeplerSandbox::onDrawDebug(DeviceContext& ctx, const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& proj)
	{
		for (auto& body : m_celestialBodies) {
			if (body.parentIndex != -1 && body.renderObject)
			{
				auto visComp = body.renderObject->getComponent<OrbitVisualizerComponent>();
				if (visComp && visComp->isVisible)
				{
					body.visualizer.draw(ctx, view, proj, m_celestialBodies[body.parentIndex].worldPosition);
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
			std::shared_ptr<GameObject> pick = m_scene.pickObject(origin, dir);
			if (pick)
				m_selectedObject = pick;
		}
	}

	void KeplerSandbox::initSandboxSimulation()
	{
		auto planeModel = m_assets->getModel("plane.obj");
		auto plane = m_scene.createObject("plane");
		plane->model = planeModel;
		plane->transform.setPosition(DirectX::XMFLOAT3(0.0f, -70.0f, 0.0f));
		plane->transform.setScale(DirectX::XMFLOAT3(50.0f, 10.0f, 50.0f));
		plane->constantBuffer = m_graphicsEngine->getGraphicsDevice().createConstantBuffer({ nullptr, sizeof(DirectX::XMFLOAT4X4) * 3 });

		auto lights = m_graphicsEngine->getLightManager();
		lights->clear();
		lights->addDirectional(DirectX::XMFLOAT3(0.f, -1.f, 0.2f), DirectX::XMFLOAT3(1.f, 1.f, 1.f), 10.f, true);

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
				GraphicsDevice& gd = m_graphicsEngine->getGraphicsDevice();
				body.renderObject->constantBuffer = gd.createConstantBuffer({ nullptr, sizeof(DirectX::XMFLOAT4X4) * 3 });

				body.visualizer.init(gd);

				body.renderObject->addComponent<OrbitComponent>(&body.orbit);
				body.renderObject->addComponent<OrbitVisualizerComponent>(&body.visualizer, &body.orbit);

				body.renderObject->inheritPosition = false;
				body.renderObject->inheritScale = false;

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

					m_celestialBodies[parent].renderObject->addChild(body.renderObject);
					body.worldPosition = m_celestialBodies[parent].worldPosition + body.orbit.positionRelativeToAttractor;
					body.renderObject->transform.setPosition(DirectX::XMFLOAT3(
						static_cast<float>(body.worldPosition.x),
						static_cast<float>(body.worldPosition.y),
						static_cast<float>(body.worldPosition.z)
					));
				}
				else
				{
					body.orbit.BodyMass = mass;
					body.worldPosition = Simulator::Vec3d(0.0, 0.0, 0.0);
					body.renderObject->transform.setPosition(DirectX::XMFLOAT3(0, 0, 0));
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

	void KeplerSandbox::onKeyDown(int key)
	{
		if (key == 'F')
		{
			if (m_selectedObject)
			{
				bool currentState = m_camera->isOrbiting();
				m_camera->setOrbitTarget(m_selectedObject->transform.getPosition());
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
	}
}