#include <DX3D/Game/Display.h>
#include <DX3D/Graphics/Core/GraphicsDevice.h>
#include <DX3D/Graphics/Core/SwapChain.h>

namespace dx3d
{

	Display::Display(const DisplayDesc& desc) : Window(desc.window)
	{
		m_swapChain = desc.graphicsDevice.createSwapChain({ m_handle, m_size });
	}

	void Display::onResize(int width, int height)
	{
		Window::onResize(width, height);

		if (m_isResizing)
		{
			m_wasResized = true;
			return;
		}

		if (m_swapChain) {
			m_swapChain->resize(width, height);
		}
	}

	void Display::onEnterSizeMove()
	{
		m_isResizing = true;
	}

	void Display::onExitSizeMove()
	{
		m_isResizing = false;

		if (m_wasResized)
		{
			if (m_swapChain) {
				m_swapChain->resize((int)m_size.width, (int)m_size.height);
			}
			m_wasResized = false;
		}
	}

	SwapChain& Display::getSwapChain() noexcept
	{
		return *m_swapChain;
	}
}
