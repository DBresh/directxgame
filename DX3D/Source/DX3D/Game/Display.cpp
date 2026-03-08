#include <DX3D/Game/Display.h>
#include <DX3D/Graphics/Core/GraphicsDevice.h>

namespace dx3d
{

	Display::Display(const DisplayDesc& desc) : Window(desc.window)
	{
		m_swapChain = desc.graphicsDevice.createSwapChain({ m_handle, m_size });
	}

	SwapChain& Display::getSwapChain() noexcept
	{
		return *m_swapChain;
	}
}
