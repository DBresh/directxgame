# 🛰️ DX3D Engine — Development Roadmap

## 🧱 Core Systems

- [✅] **Logger** — formatted runtime logging used across the engine  
  *(will gain colored console output and file logging later)*  
- [✅] **Base / Common Utilities** — shared engine types, macros, and initialization helpers  
- [✅] **Math Library** — `Vec2`, `Vec3`, `Vec4`, `Matrix4x4`, `Transform` for all math operations  
  *(will expand with quaternions, AABB, frustum, and tangent utilities)*  
- [✅] **Core Framework** — time system and base lifecycle management  
  *(will expose frame stats and fixed-update sync)*  
- [✅] **Input System** — keyboard & mouse via `InputListener`/`InputSystem`  
  *(controller support and key mapping planned)*  

## 🧩 Graphics Backend

- [✅] **Graphics Device** — D3D11 device, swap chain, shader compilation, resource creation  
  *(D3D12 abstraction planned later)*  
- [✅] **Device Context** — rendering commands via deferred contexts  
  *(multi-thread recording later)*  
- [✅] **Swap Chain** — back buffer management and presentation  
- [✅] **Constant Buffers & Pipeline States** — encapsulated shader/pipeline setup  

## 🎨 Rendering Layer

- [✅] **Model Importer (OBJ + MTL)** — parses geometry & materials, generates normals, bounding boxes, and material groups  
  *(later: tangent generation, import settings, binary cache, GLTF/FBX)*  
- [✅] **Material System** — diffuse color + texture loading  
  *(normal/specular/opacity maps planned)*  
- [✅] **Mesh System** — GPU buffers for vertices and indices  
  *(will expand for per-material meshes & instancing)*  
- [✅] **Render System** — frame begin/end, draw submission, per-object rendering  
  *(later: RenderQueue, batching, and post-process passes)*  
- [✅] **Graphics Engine** — orchestrates device, scene, and render system  
  *(multi-camera and settings later)*  
- [✅] **Camera** — free-fly camera handling input & FOV  
  *(zoom, orbit, and cinematic modes planned)*  

## 🌍 Scene & Game Layer

- [✅] **Scene Manager** — stores objects, transforms, meshes  
  *(will gain hierarchy & spatial partitioning)*  
- [✅] **Game Loop** — frame timing and system update orchestration  

## 🧠 Asset Management & Caching (next stage)

- [⏳] **Model Cache** — prevent re-parsing duplicate `.obj` files  
- [⏳] **GPU Mesh Creation** — build vertex/index buffers per material group  
- [⏳] **Asset Manager** — centralized resource registry (links CPU/GPU data)  
- [⏳] **Binary Cache** — store parsed model data as binary blobs for faster reloads  

## ⚙️ Rendering Improvements

- [⏳] **Render Queue / Sorting** — organize draw calls by material & transparency  
- [⏳] **Lighting System** — directional, point, and spot lights with Blinn-Phong shading  
- [⏳] **Frustum Culling** — use AABBs to skip off-screen objects  
- [⏳] **Texture & Sampler Manager** — central texture reuse and sampler caching  
- [⏳] **Deferred / Forward+ Renderer** — G-Buffer, lighting pass, tone mapping  

## 🧱 Importer / Asset Pipeline Extensions

- [⏳] **Tangent Generation** — required for normal maps  
- [⏳] **Import Settings Struct** — per-asset flags (flip UVs, generate normals, etc.)  
- [⏳] **GLTF / FBX Importers** — modern animated & PBR model support  
- [⏳] **Binary Scene Format** — serialized layout for levels and editor use  

## 🧰 Tools & Debugging

- [⏳] **Debug Draw** — wireframe AABB & transform visualization  
- [⏳] **Profiler Overlay** — GPU/CPU timing display & logging  

## 🎮 Game Integration

- [⏳] **Component System (ECS)** — entities with Transform, MeshRenderer, Light, Camera components  
- [⏳] **Physics Integration** — basic colliders and rigidbodies  
- [⏳] **UI Layer / ImGui** — console, FPS counter, material inspector  
- [⏳] **Game Logic Layer** — scene loading, prefabs, and gameplay scripting  
