#pragma once
#include <DX3D/Core/Common.h>
#include <d3d11.h>
#include <bit>
#include <string>

namespace dx3d
{
	inline std::string getHResultMessage(HRESULT hr)
	{
		switch (hr)
		{
		case D3D11_ERROR_FILE_NOT_FOUND: return "D3D11_ERROR_FILE_NOT_FOUND";
		case D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS: return "D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS";
		case D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS: return "D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS";
		case D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD:
			return "D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD";
		case E_INVALIDARG:   return "E_INVALIDARG";
		case E_OUTOFMEMORY:  return "E_OUTOFMEMORY";
		case E_NOTIMPL:      return "E_NOTIMPL";
		case S_FALSE:        return "S_FALSE";
		case S_OK:           return "S_OK";
		default:             return "Unknown HRESULT";
		}
	}

	inline const char* getShaderModelTarget(ShaderType type)
	{
		switch (type)
		{
		case ShaderType::VertexShader: return "vs_5_0";
		case ShaderType::PixelShader:  return "ps_5_0";
		default: return "";
		}
	}

	inline DXGI_FORMAT getDXGIFormatFromMask(D3D_REGISTER_COMPONENT_TYPE type, UINT mask)
	{
		auto componentCount = std::popcount(mask);
		if (componentCount < 1) return DXGI_FORMAT_UNKNOWN;

		constexpr DXGI_FORMAT formatTable[1][4] = { {
			DXGI_FORMAT_R32_FLOAT,
			DXGI_FORMAT_R32G32_FLOAT,
			DXGI_FORMAT_R32G32B32_FLOAT,
			DXGI_FORMAT_R32G32B32A32_FLOAT
		} };

		unsigned int typeIndex = 0;
		switch (type)
		{
		case D3D_REGISTER_COMPONENT_FLOAT32: typeIndex = 0; break;
		default: return DXGI_FORMAT_UNKNOWN;
		}

		return formatTable[typeIndex][componentCount - 1];
	}
}