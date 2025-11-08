#pragma once
#include <DX3D/Math/Vec3.h>
#include <DX3D/Math/Vec4.h>
#include <DX3D/Math/Matrix4x4.h>

namespace dx3d
{
    enum class LightType : int
    {
        Directional = 0,
        Point = 1,
        Spot = 2
    };

    struct LightShadowData
    {
        std::shared_ptr<DepthTexture2D> shadowMap;
        Matrix4x4 view;
        Matrix4x4 proj;
        Matrix4x4 viewProj;
        float bias = 0.002f;
    };

    struct Light
    {
        LightType type = LightType::Directional;

        Vec3 position{ 0, 0, 0 };     // Point/Spot
        float range = 1000.0f;        // для Point/Spot

        Vec3 direction{ 0, -1, 0 };   // Directional/Spot
        float spotAngle = 45.0f;      // Spot

        Vec3 color{ 1.0f, 1.0f, 1.0f };
        float intensity = 1.0f;

        bool castShadows = false;     // Directional/Spot
        std::shared_ptr<LightShadowData> shadow;
    };

    struct LightGPU
    {
        Vec4 posRange;   // x,y,z=position, w=range
        Vec4 dirSpot;    // x,y,z=direction, w=spotAngle
        Vec4 colInt;     // r,g,b=color, w=intensity
        int  type;       // 0=Dir,1=Point,2=Spot
        int  _pad[3];    // align to 16 bytes
    };
}
