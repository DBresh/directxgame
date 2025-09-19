#include <DX3D/Game/Game.h>
#include <DX3D/Core/Time.h>
#include <Windows.h>
#include <string>

void dx3d::Game::run()
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
			double fps = frameCount / elapsed;
			char buffer[64];
			snprintf(buffer, sizeof(buffer), "FPS: %.2f", fps);
			DX3DLogInfo(buffer);

			frameCount = 0;
			elapsed = 0.0;
		}

		onInternalUpdate();
	}
}