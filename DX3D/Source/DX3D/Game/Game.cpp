#include <DX3D/Game/Game.h>
#include <DX3D/Window/Window.h>
#include <DX3D/Graphics/Rendering/GraphicsEngine.h>
#include <DX3D/Core/Logger.h>
#include <DX3D/Graphics/GraphicsLogUtils.h>
#include <DX3D/Game/Display.h>
#include <DX3D/Core/Time.h>
#include <DX3D/Core/JobSystem.h>

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
			m_graphicsEngine->onWindowResized(w, h);
			};

		m_graphicsEngine->initUI(m_display->getHWND());
		Time::Instance()->Update(0.0);
		InputSystem::get()->addListener(this);


		DX3D_LOG_INFO("Game initialized.");
	}

	Game::~Game()
	{
		JobSystem::Shutdown();
		InputSystem::get()->removeListener(this);
		DX3D_LOG_INFO("Game is shutting down.");
	}

	void Game::onMouseDown(int button)
	{
		if (button == 0)
		{
			auto mouseState = InputSystem::get()->getMouseState();

			RECT clientRect;
			HWND hwnd = static_cast<HWND>(m_display->getHWND());
			GetClientRect(hwnd, &clientRect);
			int width = clientRect.right - clientRect.left;
			int height = clientRect.bottom - clientRect.top;

			m_selectedObject = m_graphicsEngine->pickObject(mouseState.coords.x, mouseState.coords.y, width, height);

			if (m_selectedObject)
			{
				DX3D_LOG_INFO("Picked object: {}", m_selectedObject->name);
			}
			else
			{
				DX3D_LOG_INFO("Clicked empty space.");
			}
		}
	}

	void Game::onKeyDown(int key)
	{
		if (key == 'F')
		{
			auto& camera = m_graphicsEngine->getCamera();

			if (m_selectedObject)
			{
				bool currentState = camera.isOrbiting();
				camera.setOrbitTarget(m_selectedObject->transform.getPosition());
				camera.setOrbitMode(!currentState);
			}
			else
			{
				camera.setOrbitMode(false);
			}
		}
	}

	void Game::onInternalUpdate()
	{
		auto dt = Time::Instance()->deltaTime();        // for interpolation / animations
		auto fdt = Time::Instance()->fixedDeltaTime();  // for physics steps

		m_graphicsEngine->render(m_display->getSwapChain(), [&]() {
			this->onGUI();
			});
	}
}
