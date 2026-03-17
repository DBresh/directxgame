#pragma once
#include <DX3D/Window/Window.h>

namespace dx3d
{

	class Display final: public Window
	{
	public:

		explicit Display(const DisplayDesc& desc);
		SwapChain& getSwapChain() noexcept;
		virtual void onResize(int width, int height) override;

		virtual void onEnterSizeMove() override;
		virtual void onExitSizeMove() override;

	private:

		SwapChainPtr m_swapChain{};

		bool m_isResizing = false;
		bool m_wasResized = false;

	};
}