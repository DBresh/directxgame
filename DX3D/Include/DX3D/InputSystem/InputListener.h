#pragma once
namespace dx3d
{
	class InputListener
	{
	public:
		virtual ~InputListener() = default;

		virtual void onKeyDown(int key) = 0;
		virtual void onKeyUp(int key) = 0;
	};
}