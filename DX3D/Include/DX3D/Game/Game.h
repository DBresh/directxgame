#pragma once
#include <DX3D/Core/Base.h>
#include <DX3D/Core/Core.h>
#include <DX3D/InputSystem/InputListener.h>
#include <DX3D/InputSystem/Camera.h>
#include <DX3D/Game/SceneManager.h>
#include <DX3D/Game/GameObject.h>
#include <DX3D/Graphics/Importers/AssetManager.h>

namespace dx3d
{
    class Game : public Base, public InputListener
    {
    public:
        explicit Game(const GameDesc& desc);
        virtual ~Game() override;
        virtual void run() final;

        virtual void onMouseDown(int button) override {}
        virtual void onKeyDown(int key) override {}
        virtual void onKeyUp(int) override {}
        virtual void onKeyPress(int) override {}
        virtual void onMouseMove(Point) override {}
        virtual void onMouseUp(int) override {}
        virtual void onMouseWheel(int) override {}

    protected:
        virtual void onGUI() {}
        virtual void onUpdate(double dt, double fdt) {}
        virtual void onDrawDebug(DeviceContext& ctx, const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& proj) {}
        virtual void onWindowResized(int width, int height);

    protected:
        std::unique_ptr<GraphicsEngine> m_graphicsEngine{};
        std::unique_ptr<Display> m_display{};

        SceneManager m_scene;
        std::unique_ptr<Camera> m_camera;
        std::shared_ptr<AssetManager> m_assets;

        bool m_isRunning{ true };

    private:
        void onInternalUpdate();
    };
}