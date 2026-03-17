#pragma once
#include <DX3D/Graphics/GraphicsResource.h>

namespace dx3d
{

	class SwapChain final: public GraphicsResource
	{
	public:

		SwapChain(const SwapChainDesc& desc, const GraphicsResourceDesc& gDesc);
		Rect getSize() const noexcept;
		void present(bool vsync = false);
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_rtv{};
		void resize(int width, int height);

	private:
		void reloadBuffers();

	private:
		Microsoft::WRL::ComPtr<IDXGISwapChain> m_swapChain{};
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_dsv{};
		Rect m_size{};
		friend class DeviceContext;
	};
}

