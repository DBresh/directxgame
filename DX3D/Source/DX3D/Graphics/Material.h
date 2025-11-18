#pragma once
#include <string>
#include <memory>
#include <DirectXMath.h>

namespace dx3d
{
    class Texture2D;

    struct Material
    {
        std::string name;

        DirectX::XMFLOAT3 diffuseColor{ 1.0f, 1.0f, 1.0f };   // Kd
        float              shininess = 0.0f;                // Ns

        DirectX::XMFLOAT3 ambientColor{ 0.0f, 0.0f, 0.0f };   // Ka
        float              opacity = 1.0f;                // d

        DirectX::XMFLOAT3 specularColor{ 0.0f, 0.0f, 0.0f };   // Ks
        float              _pad = 0.0f;                // align to 16 bytes

        std::string diffuseTexturePath;
        std::shared_ptr<Texture2D> diffuseTexture;
    };
}
