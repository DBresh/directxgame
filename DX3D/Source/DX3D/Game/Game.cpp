#include <DX3D/Game/Game.h>
#include <DX3D/Window/Window.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Core/Logger.h>
#include <DX3D/Game/Display.h>
#include <DX3D/Core/Time.h>

namespace dx3d
{

	Game::Game(const GameDesc& desc) :
		Base(BaseDesc{})
	{
		m_graphicsEngine = std::make_unique<GraphicsEngine>(GraphicsEngineDesc{});
		m_display = std::make_unique<Display>(DisplayDesc{ WindowDesc{desc.windowSize}, m_graphicsEngine->getGraphicsDevice() });

		Time::Instance()->Update(0.0);
		
		DX3D_LOG_INFO("Game initialized.");
	}

	Game::~Game()
	{
		DX3D_LOG_INFO("Game is shutting down.");
	}

	void Game::onInternalUpdate()
	{
		auto dt = Time::Instance()->deltaTime();        // for interpolation / animations
		auto fdt = Time::Instance()->fixedDeltaTime();  // for physics steps

		m_graphicsEngine->render(m_display->getSwapChain());
	}
}
