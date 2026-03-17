#include <DX3D/Window/Window.h>
#include <DX3D/InputSystem/InputSystem.h>
#include <DX3D/Graphics/GraphicsLogUtils.h>
#include <imgui.h>
#include <Windows.h>
#include <windowsx.h>
#include <stdexcept>


static LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	dx3d::Window* window = nullptr;
	if (msg == WM_NCCREATE) {
		CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lparam);
		window = reinterpret_cast<dx3d::Window*>(pCreate->lpCreateParams);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
	}
	else {
		window = reinterpret_cast<dx3d::Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	}

	extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
		return true;

	auto input = dx3d::InputSystem::get();

	ImGuiIO* io = nullptr;
	if (ImGui::GetCurrentContext()) {
		io = &ImGui::GetIO();
	}
	
	switch (msg)
	{
	case WM_CLOSE:
		PostQuitMessage(0);
		break;

	case WM_SIZE:
		if (window && wparam != SIZE_MINIMIZED)
		{
			int width = LOWORD(lparam);
			int height = HIWORD(lparam);
			window->onResize(width, height);
		}
		break;

	case WM_ENTERSIZEMOVE:
		if (window) window->onEnterSizeMove();
		break;

	case WM_EXITSIZEMOVE:
		if (window) window->onExitSizeMove();
		break;

	case WM_SETFOCUS:
		input->setFocus(true);
		break;

	case WM_KILLFOCUS:
		input->setFocus(false);
		break;

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		if (input->hasFocus() && (io == nullptr || !io->WantCaptureKeyboard))
			input->setKeyDown((int)wparam);
		break;

	case WM_KEYUP:
		if (input->hasFocus())
			input->setKeyUp((int)wparam);
		break;

	case WM_MOUSEMOVE:
		if (input->hasFocus())
			input->setMousePosition(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
		break;

	case WM_LBUTTONDOWN:
		if (input->hasFocus() && (io == nullptr || !io->WantCaptureMouse))
			input->setMouseDown(0);
		break;

	case WM_LBUTTONUP:
		if (input->hasFocus())
			input->setMouseUp(0);
		break;

	case WM_RBUTTONDOWN:
		if (input->hasFocus() && (io == nullptr || !io->WantCaptureMouse))
			input->setMouseDown(1);
		break;
	case WM_RBUTTONUP:
		if (input->hasFocus())
			input->setMouseUp(1);
		break;

	case WM_MBUTTONDOWN:
		if (input->hasFocus() && (io == nullptr || !io->WantCaptureMouse))
			input->setMouseDown(2);
		break;
	case WM_MBUTTONUP:
		if (input->hasFocus())
			input->setMouseUp(2);
		break;

	case WM_MOUSEWHEEL:
		if (input->hasFocus())
			input->setMouseWheel(GET_WHEEL_DELTA_WPARAM(wparam));
		break;

	default:
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}
	return 0;
}


dx3d::Window::Window(const WindowDesc& desc) : Base(BaseDesc{}), m_size(desc.size)
{
	auto registerWindowClassFunction = []() {
		WNDCLASSEX wc{};
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.lpszClassName = L"DX3DWindow";
		wc.lpfnWndProc = &WindowProcedure;
		return RegisterClassEx(&wc);
		};

	static const auto windowClassId = std::invoke(registerWindowClassFunction);
	if (!windowClassId) DX3D_LOG_THROW_ERROR("RegisterClassEx failed.");

	RECT rc{ 0,0, m_size.width, m_size.height };

	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, false);

	m_handle = CreateWindowEx(NULL, MAKEINTATOM(windowClassId), L"DX3D Engine Tests",
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		rc.right - rc.left, rc.bottom - rc.top,
		NULL, NULL, NULL, this);

	if (!m_handle) DX3D_LOG_THROW_ERROR("CreateWindowEx failed.");
	ShowWindow(static_cast<HWND>(m_handle), SW_SHOW);
}

void dx3d::Window::onResize(int width, int height)
{
	m_size.width = width;
	m_size.height = height;
}

dx3d::Window::~Window()
{
	DestroyWindow(static_cast<HWND>(m_handle));
}
