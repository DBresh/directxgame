#pragma once
#include <DX3D/Core/Logger.h>
#include <d3d11.h>


namespace dx3d
{
	namespace GraphicsLogsUtils
	{
		inline void CheckShaderCompile(Logger& logger, HRESULT hr, ID3DBlob* errorBlob)
		{
			auto errorMsg = errorBlob ? static_cast<const char*>(errorBlob->GetBufferPointer()) : nullptr;

			if (FAILED(hr))
			{
				if (errorMsg)
				{
					DX3DLogThrow(logger, std::runtime_error, Logger::LogLevel::Error, errorMsg);
				}
				else
				{
					DX3DLogThrow(logger, std::runtime_error, Logger::LogLevel::Error, "Shader compilation failed.");
				}
			}

			if (errorMsg)
			{
				DX3DLog(logger, Logger::LogLevel::Warning, errorMsg);
			}
		}
	}


#define DX3DGraphicsLogThrowOnFail(hr, message)\
	{\
	auto res = (hr);\
	if (FAILED(res))\
		DX3DLogThrowError("Direct3D11 initialization failed.");\
	}
}


#define DX3DGraphicsCheckShaderCompile(hr, errorBlob)\
{\
auto res = (hr);\
dx3d::GraphicsLogsUtils::CheckShaderCompile(getLogger(), res, errorBlob);\
}