#include <DX3D/InputSystem/InputSystem.h>
#include <algorithm>

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

	void InputSystem::update()
	{
		for (auto& [key, isDown] : m_currentKeys)
		{
			bool wasDown = false;

			if (m_previousKeys.find(key) != m_previousKeys.end())
				wasDown = m_previousKeys[key];

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
		}

		m_previousKeys = m_currentKeys;
	}
}
