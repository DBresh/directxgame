#include <DX3D/Graphics/Resources/Texture2D.h>
#include <DX3D/Graphics/Core/GraphicsDevice.h>
#include <DX3D/Core/Logger.h>
#include <wincodec.h>
#include <wrl.h>
#include <filesystem>

using namespace Microsoft::WRL;

namespace dx3d
{
    Texture2D::Texture2D(const std::string& filePath, const GraphicsResourceDesc& gDesc)
        : GraphicsResource(gDesc)
    {
        std::filesystem::path path(filePath);
        ComPtr<IWICImagingFactory> factory;
        HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&factory));
        DX3D_GRAPHICS_LOG_THROW_ON_FAIL(hr, "Failed to create WIC factory");

        ComPtr<IWICBitmapDecoder> decoder;
        hr = factory->CreateDecoderFromFilename(std::wstring(path.wstring()).c_str(), nullptr,
            GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);
        DX3D_GRAPHICS_LOG_THROW_ON_FAIL(hr, "Failed to open texture: {}", filePath);

        ComPtr<IWICBitmapFrameDecode> frame;
        decoder->GetFrame(0, &frame);

        ComPtr<IWICFormatConverter> converter;
        factory->CreateFormatConverter(&converter);
        converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA,
            WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);

        UINT width = 0, height = 0;
        converter->GetSize(&width, &height);
        std::vector<unsigned char> pixels(width * height * 4);
        converter->CopyPixels(nullptr, width * 4, static_cast<UINT>(pixels.size()), pixels.data());

        D3D11_TEXTURE2D_DESC texDesc{};
        texDesc.Width = width;
        texDesc.Height = height;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        texDesc.SampleDesc.Count = 1;

        D3D11_SUBRESOURCE_DATA initData{};
        initData.pSysMem = pixels.data();
        initData.SysMemPitch = width * 4;

        ComPtr<ID3D11Texture2D> texture;
        DX3D_GRAPHICS_LOG_THROW_ON_FAIL(
            m_device.CreateTexture2D(&texDesc, &initData, &texture),
            "Failed to create Texture2D from file: {}", filePath);

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = texDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        DX3D_GRAPHICS_LOG_THROW_ON_FAIL(
            m_device.CreateShaderResourceView(texture.Get(), &srvDesc, &m_srv),
            "Failed to create SRV for texture: {}", filePath);

        if (!m_srv)
            DX3D_LOG_ERROR("Texture SRV creation failed for '{}'", filePath);
        else
            DX3D_LOG_INFO("Loaded texture '{}' ({}x{})", filePath, width, height);
    }
}
