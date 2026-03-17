#include <DX3D/Graphics/Core/SwapChain.h>

namespace dx3d
{

	SwapChain::SwapChain(const SwapChainDesc& desc, const GraphicsResourceDesc& gDesc) :
		GraphicsResource(gDesc), m_size(desc.winSize)
	{
		if (!desc.winHandle) DX3D_LOG_THROW_ERROR("No window handle provided.");

		DXGI_SWAP_CHAIN_DESC dxgiDesc{};
		dxgiDesc.BufferDesc.Width = std::max(1, desc.winSize.width);
		dxgiDesc.BufferDesc.Height = std::max(1, desc.winSize.height);
		dxgiDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		dxgiDesc.BufferCount = 2;
		dxgiDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

		dxgiDesc.OutputWindow = static_cast<HWND>(desc.winHandle);
		dxgiDesc.SampleDesc.Count = 1;
		dxgiDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		dxgiDesc.Windowed = TRUE;
		dxgiDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

		DX3D_GRAPHICS_LOG_THROW_ON_FAIL(
			m_factory.CreateSwapChain(&m_device, &dxgiDesc, &m_swapChain)
			, "CreateSwapChain failed.");

		reloadBuffers();
	}

	Rect SwapChain::getSize() const noexcept
	{
		return m_size;
	}

	void SwapChain::present(bool vsync)
	{
		UINT syncInterval = vsync ? 1 : 0;
		UINT flags = vsync ? 0 : DXGI_PRESENT_ALLOW_TEARING;

		DX3D_GRAPHICS_LOG_THROW_ON_FAIL(
			m_swapChain->Present(syncInterval, flags),
			"Present failed.");
	}

	void SwapChain::reloadBuffers()
	{
		Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
		DX3D_GRAPHICS_LOG_THROW_ON_FAIL(
			m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))
			, "GetBuffer failed.");

		DX3D_GRAPHICS_LOG_THROW_ON_FAIL(
			m_device.CreateRenderTargetView(backBuffer.Get(), nullptr, &m_rtv)
			, "CreateRenderTargetView failed.");

		D3D11_TEXTURE2D_DESC depthDesc{};
		depthDesc.Width = m_size.width;
		depthDesc.Height = m_size.height;
		depthDesc.MipLevels = 1;
		depthDesc.ArraySize = 1;
		depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthDesc.SampleDesc.Count = 1;
		depthDesc.SampleDesc.Quality = 0;
		depthDesc.Usage = D3D11_USAGE_DEFAULT;
		depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depthDesc.CPUAccessFlags = 0;
		depthDesc.MiscFlags = 0;

		Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencilTexture;
		DX3D_GRAPHICS_LOG_THROW_ON_FAIL(
			m_device.CreateTexture2D(&depthDesc, nullptr, &depthStencilTexture)
			, "CreateTexture2D failed for depth-stencil.");

		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
		dsvDesc.Format = depthDesc.Format;
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = 0;

		DX3D_GRAPHICS_LOG_THROW_ON_FAIL(m_device.CreateDepthStencilView(depthStencilTexture.Get(), &dsvDesc, &m_dsv),
			"CreateDepthStencilView failed.");
	}

	void SwapChain::resize(int width, int height)
	{
		if (m_size.width == width && m_size.height == height) return;
		if (width <= 0 || height <= 0) return;

		m_size.width = width;
		m_size.height = height;

		Microsoft::WRL::ComPtr<ID3D11Device> d3dDevice;
		m_swapChain->GetDevice(__uuidof(ID3D11Device), (void**)&d3dDevice);

		Microsoft::WRL::ComPtr<ID3D11DeviceContext> ctx;
		d3dDevice->GetImmediateContext(&ctx);

		ctx->ClearState();

		m_rtv.Reset();
		m_dsv.Reset();

		ctx->Flush();

		DX3D_GRAPHICS_LOG_THROW_ON_FAIL(
			m_swapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING),
			"ResizeBuffers failed."
		);

		reloadBuffers();
	}
}
