#pragma once
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/GraphicsDevice.h>
#include <DX3D/Graphics/DeviceContext.h>
#include <DX3D/Graphics/SwapChain.h>
#include <DX3D/Graphics/ConstantBuffer.h>

// temp 
#include <DX3D/Game/SceneManager.h>
#include <DX3D/Math/Transform.h>
#include <DX3D/Math/Vec3.h>
#include <fstream>
#include <DX3D/Core/Time.h>
#include <DX3D/Math/Matrix4x4.h>
#include <DirectXMath.h>
#include <DX3D/InputSystem/InputSystem.h>
#include <DX3D/Graphics/Mesh.h>
#include <DX3D/Graphics/ModelImporter.h>
#include <DX3D/Graphics/ModelData.h>
#include <vector>

namespace dx3d
{

    class GraphicsEngine final : public Base, public InputListener
    {
    public:
        explicit GraphicsEngine(const GraphicsEngineDesc& desc);
        virtual ~GraphicsEngine() override;

        GraphicsDevice& getGraphicsDevice() noexcept;
        void render(SwapChain& swapChain);

        void onKeyDown(int key) override;
        void onKeyUp(int key) override;
        void onKeyPress(int key) override;
        void onMouseMove(Point deltaMouse) override;
        void onMouseUp(int button) override;
        void onMouseDown(int button) override;
        void onMouseWheel(int delta) override;

    private:
        // temp
        SceneManager m_scene;
        void createCubeMesh();

    private:
        std::shared_ptr<GraphicsDevice> m_graphicsDevice{};
        DeviceContextPtr m_deviceContext{};
        GraphicsPipelineStatePtr m_pipeline{};
        ConstantBufferPtr m_cb{};

        // temp
        std::shared_ptr<ModelImporter> m_modelImporter{};

        Microsoft::WRL::ComPtr<ID3D11SamplerState> m_linearSampler;

        float m_angleX{ 0.0f };
        float m_angleY{ 0.0f };
        float m_rotationSpeed{ 0.15f };
        Matrix4x4 m_cameraPosition;
        float m_forward{ 0.0f };
        float m_right{ 0.0f };
    };

}