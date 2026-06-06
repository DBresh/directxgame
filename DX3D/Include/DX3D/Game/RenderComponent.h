#pragma once
#include <DX3D/Graphics/Resources/ModelGPU.h>
#include <DX3D/Graphics/Buffers/ConstantBuffer.h>

namespace dx3d {
    struct RenderComponent {
        ModelGPU* model = nullptr;
        const ConstantBuffer* objectCB = nullptr;

        bool castsShadow = true;
        bool visible = true;
    };
}