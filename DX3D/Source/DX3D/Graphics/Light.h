#pragma once
#include <DirectXMath.h>
#include <DX3D/Graphics/DepthTexture2D.h>
#include <memory>

namespace dx3d
{
    using namespace DirectX;

    enum class LightType : int
    {
        Directional = 0,
        Point = 1,
        Spot = 2
    };

    struct LightShadowData
    {
        std::shared_ptr<DepthTexture2D> shadowMap;

        XMFLOAT4X4 view;     // transposed for HLSL
        XMFLOAT4X4 proj;     // transposed for HLSL
        XMFLOAT4X4 viewProj; // transposed for HLSL

        float bias = 0.005f;
        float pad[3] = {}; // 16-byte align
    };

    struct Light
    {
        LightType type = LightType::Directional;

        XMFLOAT3 position = XMFLOAT3(0, 0, 0); // Point / Spot
        float    range = 1000.0f;

        XMFLOAT3 direction = XMFLOAT3(0, -1, 0); // Directional / Spot
        float    spotAngle = XMConvertToRadians(45.0f); // store radians

        XMFLOAT3 color = XMFLOAT3(1.0f, 1.0f, 1.0f);
        float    intensity = 1.0f;

        bool castShadows = false;
        std::shared_ptr<LightShadowData> shadow;
    };

    struct LightGPU
    {
        XMFLOAT4 posRange; // xyz pos,  w = range
        XMFLOAT4 dirSpot;  // xyz dir,  w = spotAngleRadians
        XMFLOAT4 colInt;   // rgb col,  w = intensity
        int type;
        int _pad[3]; // 16-byte alignment
    };
}
