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

        DirectX::XMFLOAT3 diffuseColor{ 1.0f, 1.0f, 1.0f };   // Albedo (Base Color)
        float             shininess = 0.0f;                  // Legacy

        DirectX::XMFLOAT3 ambientColor{ 0.0f, 0.0f, 0.0f };   // Legacy
        float             opacity = 1.0f;

        DirectX::XMFLOAT3 specularColor{ 0.0f, 0.0f, 0.0f };  // Legacy

        // --- NEW PBR PROPERTIES ---
        float              roughness = 0.5f;                // 0.0 (smooth) - 1.0 (rough)
        float              metallic = 0.0f;                 // 0.0 (dielectric) - 1.0 (metal)
        DirectX::XMFLOAT2  _padding;

        std::string diffuseTexturePath;
        std::shared_ptr<Texture2D> diffuseTexture;
    };
}
