#pragma once
#include <DX3D/Core/Logger.h>
#include <DX3D/Graphics/GraphicsUtils.h>
#include <d3d11.h>
#include <stdexcept>

namespace dx3d
{
	// Logs and throws a runtime error if the HRESULT indicates failure.
#define DX3D_GRAPHICS_LOG_THROW_ON_FAIL(hr, fmt, ...) \
        do { \
            HRESULT __hr = (hr); \
            if (FAILED(__hr)) { \
                std::string hrMessage = dx3d::getHResultMessage(__hr); \
                std::string fullMessage = std::format("{} - HRESULT: {} ({:#010x})", \
                    dx3d::formatLog(fmt, ##__VA_ARGS__), \
                    hrMessage, static_cast<unsigned int>(__hr)); \
                DX3D_LOG_ERROR("{}", fullMessage); \
                throw std::runtime_error(fullMessage); \
            } \
        } while(0)

	// Logs a warning if the HRESULT indicates failure.
#define DX3D_GRAPHICS_LOG_ON_FAIL(hr, fmt, ...) \
        do { \
            HRESULT __hr = (hr); \
            if (FAILED(__hr)) { \
                std::string hrMessage = dx3d::getHResultMessage(__hr); \
                std::string fullMessage = std::format("{} - HRESULT: {} ({:#010x})", \
                    dx3d::formatLog(fmt, ##__VA_ARGS__), \
                    hrMessage, static_cast<unsigned int>(__hr)); \
                DX3D_LOG_WARNING("{}", fullMessage); \
            } \
        } while(0)
}
