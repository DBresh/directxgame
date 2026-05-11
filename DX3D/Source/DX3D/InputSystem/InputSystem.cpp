#include <DX3D/InputSystem/InputSystem.h>
#include <algorithm>
#include <cctype>

namespace dx3d
{
	InputSystem* InputSystem::m_system = nullptr;

	InputSystem* InputSystem::get()
	{
		if (!m_system)
			m_system = new InputSystem();

		return m_system;
	}

	InputSystem::InputSystem()
	{
	}

	InputSystem::~InputSystem()
	{
		if (m_system)
		{
			delete m_system;
			m_system = nullptr;
		}
	}

	void InputSystem::addListener(InputListener* listener)
	{
		m_listeners.push_back(listener);
	}

	void InputSystem::removeListener(InputListener* listener)
	{
		m_listeners.erase(
			std::remove(m_listeners.begin(), m_listeners.end(), listener),
			m_listeners.end()
		);
	}

	void InputSystem::setKeyDown(int key)
	{
		m_currentKeys[key] = true;
	}

	void InputSystem::setKeyUp(int key)
	{
		m_currentKeys[key] = false;
	}

	void InputSystem::setMousePosition(int x, int y)
	{
		m_currentMouse.coords.x = x;
		m_currentMouse.coords.y = y;
	}

	void InputSystem::setMouseDown(int button)
	{
		if (button == 0) m_currentMouse.leftButton = true;
		else if (button == 1) m_currentMouse.rightButton = true;
		else if (button == 2) m_currentMouse.middleButton = true;
	}

	void InputSystem::setMouseUp(int button)
	{
		if (button == 0) m_currentMouse.leftButton = false;
		else if (button == 1) m_currentMouse.rightButton = false;
		else if (button == 2) m_currentMouse.middleButton = false;
	}

	void InputSystem::setMouseWheel(int delta)
	{
		m_mouseWheelDelta = delta;
	}

	bool InputSystem::isKeyDown(int key) const
	{
		auto it = m_currentKeys.find(key);
		return it != m_currentKeys.end() && it->second;
	}

	bool InputSystem::isKeyDown(char key) const
	{
		return isKeyDown(static_cast<int>(toupper(key)));
	}

	bool InputSystem::isMouseKeyDown(int button) const
	{
		if (m_currentMouse.leftButton && button == 0) return true;
		if (m_currentMouse.rightButton && button == 1) return true;
		return false;
	}

	const MouseState& InputSystem::getMouseState() const
	{
		return m_currentMouse;
	}

	void InputSystem::update()
	{
		if (hasFocus())
		{
			// Keyboard
			for (auto& [key, isDown] : m_currentKeys)
			{
				bool wasDown = m_previousKeys[key];
				if (isDown && !wasDown)
				{
					for (auto listener : m_listeners)
						listener->onKeyDown(key);
				}

				if (!isDown && wasDown)
				{
					for (auto listener : m_listeners)
						listener->onKeyUp(key);
				}

				if (isDown) // && wasDown ?
				{
					for (auto listener : m_listeners)
						listener->onKeyPress(key);
				}
			}
			m_previousKeys = m_currentKeys;

			// Mouse movement
			if (m_currentMouse.coords.x != m_previousMouse.coords.x || m_currentMouse.coords.y != m_previousMouse.coords.y)
			{
				for (auto listener : m_listeners)
					listener->onMouseMove(Point(m_currentMouse.coords.x - m_previousMouse.coords.x, m_currentMouse.coords.y - m_previousMouse.coords.y));
			}

			// Mouse buttons
			if (m_currentMouse.leftButton != m_previousMouse.leftButton)
			{
				for (auto listener : m_listeners)
					if (m_currentMouse.leftButton)
						listener->onMouseDown(0);
					else
						listener->onMouseUp(0);
			}
			if (m_currentMouse.rightButton != m_previousMouse.rightButton)
			{
				for (auto listener : m_listeners)
					if (m_currentMouse.rightButton)
						listener->onMouseDown(1);
					else
						listener->onMouseUp(1);
			}
			if (m_currentMouse.middleButton != m_previousMouse.middleButton)
			{
				for (auto listener : m_listeners)
					if (m_currentMouse.middleButton)
						listener->onMouseDown(2);
					else
						listener->onMouseUp(2);
			}

			// Mouse wheel
			if (m_mouseWheelDelta != 0)
			{
				for (auto listener : m_listeners)
					listener->onMouseWheel(m_mouseWheelDelta);
				m_mouseWheelDelta = 0;
			}

			m_previousMouse = m_currentMouse;
		}
	}
}
