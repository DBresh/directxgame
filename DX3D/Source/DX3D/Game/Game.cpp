#include <DX3D/Game/Game.h>
#include <DX3D/Window/Window.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Core/Logger.h>
#include <DX3D/Game/Display.h>
#include <DX3D/Core/Time.h>

dx3d::Game::Game(const GameDesc& desc):
	Base({ *std::make_unique<Logger>(desc.logLevel).release()}),
	m_loggerPtr(&m_logger)
{
	m_graphicsEngine = std::make_unique<GraphicsEngine>(GraphicsEngineDesc{m_logger});
	m_display = std::make_unique<Display>(DisplayDesc{ {m_logger, desc.windowSize}, m_graphicsEngine->getGraphicsDevice() });

	dx3d::Time::Instance()->Update(0.0);

	DX3DLogInfo("Game initialized.");
}

dx3d::Game::~Game()
{
	DX3DLogInfo("Game is shutting down.")
}

void dx3d::Game::onInternalUpdate()
{
	auto dt = dx3d::Time::Instance()->deltaTime();        // for interpolation / animations
	auto fdt = dx3d::Time::Instance()->fixedDeltaTime();  // for physics steps

	m_graphicsEngine->render(m_display->getSwapChain());
}
