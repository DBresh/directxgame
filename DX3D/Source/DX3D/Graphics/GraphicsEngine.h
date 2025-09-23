#pragma once
#include <DX3D/Core/Core.h>
#include <DX3D/Core/Base.h>
#include <DX3D/Math/Vec3.h>
#include <DX3D/Math/Vec4.h>
#include <DX3D/InputSystem/InputListener.h> // temp
#include <unordered_map> // temp

namespace dx3d
{

	class GraphicsEngine final: public Base, public InputListener // InputListener is temp
	{
	public:
		explicit GraphicsEngine(const GraphicsEngineDesc& desc);
		virtual ~GraphicsEngine() override;

		GraphicsDevice& getGraphicsDevice() noexcept;
		void render(SwapChain& swapChain);

		void onKeyDown(int key) override
		{
			m_keysPressed[key] = true;
		}

		void onKeyUp(int key) override
		{
			m_keysPressed[key] = false;
		}

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

	private:	// temp
		float m_angleX{ 0.0f };
		float m_angleY{ 0.0f };
		float m_rotationSpeed{ 1.5f }; // radians per second
		std::unordered_map<int, bool> m_keysPressed;
	};
}