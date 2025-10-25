#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/GraphicsDevice.h>
#include <DX3D/Graphics/DeviceContext.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/GraphicsLogUtils.h>
#include <DX3D/Graphics/Texture2D.h>
#include <DX3D/Math/Vec3.h>
#include <DX3D/Math/Matrix4x4.h>
#include <DX3D/Core/Time.h>
#include <DX3D/InputSystem/InputSystem.h>
#include <fstream>

namespace dx3d
{
    namespace
    {
        static std::string loadFileText(const std::string& path)
        {
            std::ifstream file(path);
            if (!file.is_open())
                DX3D_LOG_THROW_ERROR("Failed to open file: {}", path);
            return { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
        }
    }

    GraphicsEngine::GraphicsEngine(const GraphicsEngineDesc& desc) : Base(desc.base)
    {
        m_graphicsDevice = std::make_shared<GraphicsDevice>(GraphicsDeviceDesc{});
        auto& device = *m_graphicsDevice;
        m_deviceContext = device.createDeviceContext();

        const std::string shaderFileData = loadFileText("DX3D/Assets/Shaders/Basic.hlsl");
        const char* shaderSourceCode = shaderFileData.c_str();
        const size_t shaderSize = shaderFileData.size();

        auto vs = device.compileShader({ "Basic.hlsl", shaderSourceCode, shaderSize, "VSMain", ShaderType::VertexShader });
        auto ps = device.compileShader({ "Basic.hlsl", shaderSourceCode, shaderSize, "PSMain", ShaderType::PixelShader });
        auto vsSig = device.createVertexShaderSignature({ vs });

        m_pipeline = device.createGraphicsPipelineState({ *vsSig, *ps });

        m_modelImporter = std::make_shared<ModelImporter>(m_graphicsDevice);
        m_renderSystem = std::make_unique<RenderSystem>(m_graphicsDevice, m_deviceContext);
        m_renderSystem->setPipeline(m_pipeline);

        createCubeMesh();

        m_camera = std::make_unique<Camera>();
        InputSystem::get()->addListener(m_camera.get());
    }

    GraphicsEngine::~GraphicsEngine()
    {
    }

    GraphicsDevice& GraphicsEngine::getGraphicsDevice() noexcept
    {
        return *m_graphicsDevice;
    }

    void GraphicsEngine::createCubeMesh()
    {
        ConstantBufferDesc cbDesc{};
        cbDesc.data = nullptr;
        cbDesc.size = sizeof(DirectX::XMFLOAT4X4) * 3;

        auto cubeModel = m_modelImporter->loadOBJ("Models\\cube.obj");
        auto cubeMesh = m_graphicsDevice->createMesh(cubeModel.vertices, cubeModel.indices);

        auto cube = m_scene.createObject("Cube1");
        cube->mesh = cubeMesh;
        cube->materials = cubeModel.materials;
        cube->materialGroups = cubeModel.materialGroups;
        cube->transform.setPosition(Vec3(0.0f, 0.0f, 0.0f));
        cube->transform.setScale(Vec3(1, 1, 1));
        cube->constantBuffer = m_graphicsDevice->createConstantBuffer(cbDesc);
    }

    void GraphicsEngine::render(SwapChain& swapChain)
    {
        m_camera->update();

        Matrix4x4 viewT = m_camera->getViewMatrix().transpose();
        Matrix4x4 projT = m_camera->getProjectionMatrix().transpose();

        m_renderSystem->beginFrame(swapChain, { 0.2f, 0.2f, 0.2f, 1.0f });

        for (const auto& object : m_scene.getAllObjects())
        {
            if (!object->mesh) continue;
            Matrix4x4 worldT = object->getWorldTransform().getWorldMatrix().transpose();

            m_renderSystem->drawMesh(
                *object->mesh,
                *object->constantBuffer,
                worldT,
                viewT,
                projT,
                object->materialGroups,
                object->materials
            );
        }

        m_renderSystem->endFrame(*m_graphicsDevice, swapChain, true);
    }
}