#pragma once
#include <DX3D/Core/Base.h>
#include <DX3D/Core/Core.h>
#include <DX3D/InputSystem/InputListener.h>
#include <DX3D/Game/GameObject.h>

namespace dx3d
{

	class Game: public Base, public InputListener
	{
	public:
		explicit Game(const GameDesc& desc);
		virtual ~Game() override;
		virtual void run() final;

		virtual void onMouseDown(int button) override;
		virtual void onKeyDown(int) override {}
		virtual void onKeyUp(int) override {}
		virtual void onKeyPress(int) override {}
		virtual void onMouseMove(Point) override {}
		virtual void onMouseUp(int) override {}
		virtual void onMouseWheel(int) override {}

	protected:
		virtual void onGUI();

	private:
		void onInternalUpdate();

	private:
		std::unique_ptr<GraphicsEngine> m_graphicsEngine{};
		std::unique_ptr<Display> m_display{};
		bool m_isRunning{ true };

		std::shared_ptr<GameObject> m_selectedObject{ nullptr };

		// temp
		double m_currentFPS{ 0.0 };
	};
}

