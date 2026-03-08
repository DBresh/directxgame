#include <DX3D/Game/Game.h>
#include <DX3D/Core/Time.h>
#include <DX3D/InputSystem/InputSystem.h>
#include <DX3D/Core/Logger.h>
#include <DX3D/Graphics/GraphicsLogUtils.h>

#include <imgui.h>
#include <Windows.h>
#include <string>

namespace dx3d
{
	void Game::run()
	{
		MSG msg{};

		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);

		LARGE_INTEGER lastTime;
		QueryPerformanceCounter(&lastTime);

		int frameCount = 0;
		double elapsed = 0.0;

		while (m_isRunning)
		{
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				if (msg.message == WM_QUIT)
				{
					m_isRunning = false;
					break;
				}
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			dx3d::InputSystem::get()->update();

			// Compute delta time
			LARGE_INTEGER now;
			QueryPerformanceCounter(&now);
			double realDelta = double(now.QuadPart - lastTime.QuadPart) / double(frequency.QuadPart);
			lastTime = now;

			// Update Time singleton
			dx3d::Time::Instance()->Update(realDelta);

			// Count frames
			frameCount++;
			elapsed += realDelta;

			// Every second, print FPS
			if (elapsed >= 1.0)
			{
				m_currentFPS = frameCount / elapsed;
				// DX3D_LOG_INFO("FPS: {:.2f}", m_currentFPS);

				frameCount = 0;
				elapsed = 0.0;
			}

			onInternalUpdate();
		}
	}

	void dx3d::Game::onGUI()
	{
		ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Always);
		ImGui::SetNextWindowBgAlpha(0.0f);

		ImGuiWindowFlags overlayFlags = ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_AlwaysAutoResize |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoFocusOnAppearing |
			ImGuiWindowFlags_NoNav |
			ImGuiWindowFlags_NoMove;

		if (ImGui::Begin("FPS_Overlay", nullptr, overlayFlags))
		{
			ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "FPS: %.0f", m_currentFPS);
			// ImGui::Text("ImGui FPS: %.1f", ImGui::GetIO().Framerate);
		}
		ImGui::End();
	}
}