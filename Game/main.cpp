#include <DX3D/All.h>
#include <Windows.h>

int main()
{
	try
	{
		SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

		dx3d::Game game({ {1024, 640} });
		game.run();
	}
	catch (const std::runtime_error&)
	{
		return EXIT_FAILURE;
	}
	catch (const std::invalid_argument&)
	{
		return EXIT_FAILURE;
	}
	catch (const std::exception&)
	{
		return EXIT_FAILURE;
	}
	catch (...)
	{
		return EXIT_FAILURE;
	}


	return EXIT_SUCCESS;
}