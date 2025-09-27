#include <DX3D/Window/Window.h>
#include <DX3D/InputSystem/InputSystem.h>
#include <Windows.h>
#include <windowsx.h>
#include <stdexcept>


static LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	auto input = dx3d::InputSystem::get();

	switch (msg)
	{
	case WM_CLOSE:
		PostQuitMessage(0);
		break;

	case WM_SETFOCUS:
		input->setFocus(true);
		break;

	case WM_KILLFOCUS:
		input->setFocus(false);
		break;

	case WM_KEYDOWN:
		if (input->hasFocus())
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
		if (input->hasFocus())
			input->setMouseDown(0);
		break;
	case WM_LBUTTONUP:
		if (input->hasFocus())
			input->setMouseUp(0);
		break;

	case WM_RBUTTONDOWN:
		if (input->hasFocus())
			input->setMouseDown(1);
		break;
	case WM_RBUTTONUP:
		if (input->hasFocus())
			input->setMouseUp(1);
		break;

	case WM_MBUTTONDOWN:
		if (input->hasFocus())
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


dx3d::Window::Window(const WindowDesc& desc) : Base(desc.base), m_size(desc.size)
{

	auto registerWindowClassFunction = []()
		{
			WNDCLASSEX wc{};
			wc.cbSize = sizeof(WNDCLASSEX);
			wc.lpszClassName = L"DX3DWindow";
			wc.lpfnWndProc = &WindowProcedure;
			return RegisterClassEx(&wc);
		};

	static const auto windowClassId = std::invoke(registerWindowClassFunction);

	if (!windowClassId)
		DX3DLogThrowError("RegisterClassEx failed.");

	RECT rc{ 0,0, m_size.width, m_size.height };
	AdjustWindowRect(&rc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, false);

	m_handle = CreateWindowEx(NULL, MAKEINTATOM(windowClassId), L"DX3D Engine Tests",
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT,
		rc.right - rc.left, rc.bottom - rc.top,
		NULL, NULL, NULL, NULL);

	if (!m_handle)
		DX3DLogThrowError("CreateWindowEx failed.");

	ShowWindow(static_cast<HWND>(m_handle), SW_SHOW);
}

dx3d::Window::~Window()
{
	DestroyWindow(static_cast<HWND>(m_handle));
}
