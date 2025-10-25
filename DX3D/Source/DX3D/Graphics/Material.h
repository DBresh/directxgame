#pragma once
#include <string>
#include <memory>
#include <DX3D/Math/Vec3.h>

namespace dx3d
{
    class Texture2D;

    struct Material
    {
        std::string name;

        Vec3 diffuseColor{ 1.0f, 1.0f, 1.0f };   // Kd
        Vec3 ambientColor{ 0.0f, 0.0f, 0.0f };   // Ka
        Vec3 specularColor{ 0.0f, 0.0f, 0.0f };  // Ks
        float shininess{ 0.0f };                 // Ns
        float opacity{ 1.0f };                   // d

        std::string diffuseTexturePath;          // file 
        std::shared_ptr<Texture2D> diffuseTexture; // GPU texture resource
    };
}
