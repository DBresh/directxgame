#pragma once
#include <DX3D/Game/Registry.h>
#include <DX3D/Game/TransformSystem.h>
#include <DX3D/Game/RenderComponentSystem.h>
#include <DX3D/Game/VisibilitySystem.h>
#include <Game/Kepler/OrbitSystem.h>

namespace dx3d {
    struct RuntimeWorld {
        Registry registry;
        TransformSystem transforms;
        RenderComponentSystem renderables;
        VisibilitySystem visibility;
        OrbitSystem orbitSystem;
    };
}