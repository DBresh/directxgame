#pragma once
#include <stdexcept>
#include <memory>


namespace dx3d
{
	class Base;
	class Window;
	class Game;

	class GraphicsEngine;
	class RenderSystem;
	class SwapChain;
	class Display;

	class Logger;

	using i32 = int;
	using ui32 = unsigned int;
	using f32 = float;
	using d64 = double;

	using SwapChainPtr = std::shared_ptr<SwapChain>;
}