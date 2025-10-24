#pragma once
#include <DX3D/Core/Common.h>


namespace dx3d
{
	class Base
	{
	public:
		explicit Base(const BaseDesc& desc);
		virtual ~Base();

	protected:
		Base(const Base&) = delete;
		Base(Base&&) = delete;
		Base& operator = (const Base&) = delete;
		Base& operator = (Base&&) = delete;
	};
}