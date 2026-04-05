#include <DX3D/Game/Game.h>
#include <DX3D/Window/Window.h>
#include <DX3D/Graphics/Rendering/GraphicsEngine.h>
#include <DX3D/Core/Logger.h>
#include <DX3D/Graphics/GraphicsLogUtils.h>
#include <DX3D/Game/Display.h>
#include <DX3D/Core/Time.h>
#include <DX3D/Core/JobSystem.h>
#include <imgui.h>

namespace dx3d
{

	Game::Game(const GameDesc& desc) :
		Base(BaseDesc{})
	{
		HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
		{
			DX3D_LOG_THROW_ERROR("Failed to initialize COM library. HRESULT: {:#010x}", static_cast<unsigned int>(hr));
		}

		m_graphicsEngine = std::make_unique<GraphicsEngine>(GraphicsEngineDesc{});
		m_display = std::make_unique<Display>(DisplayDesc{ WindowDesc{desc.windowSize}, m_graphicsEngine->getGraphicsDevice() });
		m_display->onWindowResized = [this](int w, int h) {
			this->onWindowResized(w, h);
			};

		m_graphicsEngine->initUI(m_display->getHWND());

		AssetManagerDesc aDesc{};
		aDesc.graphicsDevice = m_graphicsEngine->getGraphicsDevicePtr();
		aDesc.assetsRoot = std::filesystem::path("DX3D/Assets/Models");
		m_assets = std::make_shared<AssetManager>(aDesc);

		m_camera = std::make_unique<Camera>();
		InputSystem::get()->addListener(m_camera.get());

		Time::Instance()->Update(0.0);
		InputSystem::get()->addListener(this);

		DX3D_LOG_INFO("Game initialized.");
	}

	void Game::onWindowResized(int width, int height)
	{
		if (m_camera)
		{
			m_camera->setScreenSize((float)width, (float)height);
		}
	}

	Game::~Game()
	{
		JobSystem::Shutdown();
		InputSystem::get()->removeListener(this);
		DX3D_LOG_INFO("Game is shutting down.");
	}

	void Game::onInternalUpdate()
	{
		auto dt = Time::Instance()->deltaTime();
		auto fdt = Time::Instance()->fixedDeltaTime();

		onUpdate(dt, fdt);

		m_graphicsEngine->render(m_scene, *m_camera, m_display->getSwapChain(),
			[&]() { this->onGUI(); },
			[&](DeviceContext& ctx, const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& proj) {
				this->onDrawDebug(ctx, view, proj);
			});
	}
}
