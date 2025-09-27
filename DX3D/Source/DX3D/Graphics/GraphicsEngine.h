#pragma once
#include <DX3D/Core/Core.h>
#include <DX3D/Core/Base.h>
#include <DX3D/Math/Vec3.h>
#include <DX3D/Math/Vec4.h>
#include <DX3D/InputSystem/InputListener.h> //temp

namespace dx3d
{

	class GraphicsEngine final : public Base, public InputListener
	{
	public:
		explicit GraphicsEngine(const GraphicsEngineDesc& desc);
		virtual ~GraphicsEngine() override;

		GraphicsDevice& getGraphicsDevice() noexcept;
		void render(SwapChain& swapChain);

		//temp
		void onKeyDown(int key) override;
		void onKeyUp(int key) override;
		void onKeyPress(int key) override;

		void onMouseMove(Point deltaMouse) override;
		void onMouseUp(int button) override;
		void onMouseDown(int button) override;
		void onMouseWheel(int delta) override;


	private:
		struct Vertex
		{
			Vec3 position;
			Vec4 color;
		};
	private:
		std::shared_ptr<GraphicsDevice> m_graphicsDevice{};
		DeviceContextPtr m_deviceContext{};
		GraphicsPipelineStatePtr m_pipeline{};
		VertexBufferPtr m_vb{};
		IndexBufferPtr m_ib{};
		ConstantBufferPtr m_cb{};

	private: // temp
		float m_angleX{ 0.0f };
		float m_angleY{ 0.0f };
		float m_rotationSpeed{ 1.5f }; // radians per second
		float m_eyePosition{ -3.0f };
	};
}