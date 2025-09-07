#pragma once
#include <DX3D/Core/Core.h>
#include <DX3D/Core/Base.h>

namespace dx3d
{

	class GraphicsEngine final: public Base
	{
	public:
		GraphicsEngine();
		virtual ~GraphicsEngine() override;

	private:
		std::unique_ptr<RenderSystem> m_renderSystem{};

	};

}

