#pragma once
#include <DX3D/Graphics/GraphicsResource.h>
#include <wrl.h>
#include <d3d11.h>
#include <string>

namespace dx3d
{
    class Texture2D final : public GraphicsResource
    {
    public:
        Texture2D(const std::string& filePath, const GraphicsResourceDesc& gDesc);

        ID3D11ShaderResourceView* getSRV() const noexcept { return m_srv.Get(); }

    private:
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_srv;
    };
}