#pragma once
#include <DX3D/InputSystem/InputListener.h>
#include <unordered_map>
#include <vector>
#include <memory>

namespace dx3d
{
    class InputSystem
    {
    public:
        static InputSystem* get();

        InputSystem();
        ~InputSystem();

        void addListener(InputListener* listener);
        void removeListener(InputListener* listener);

        void update();

        void setKeyDown(int key);
        void setKeyUp(int key);

        void setMousePosition(int x, int y);
        void setMouseDown(int button);
        void setMouseUp(int button);
        void setMouseWheel(int delta);

        bool hasFocus() const { return m_hasFocus; }
        void setFocus(bool focus) { m_hasFocus = focus; }

        bool isKeyDown(int key) const;
        bool isKeyDown(char key) const;

        const MouseState& getMouseState() const;

    private:
        static InputSystem* m_system;

    protected:
        std::vector<InputListener*> m_listeners;

        std::unordered_map<int, bool> m_currentKeys;
        std::unordered_map<int, bool> m_previousKeys;

        MouseState m_currentMouse;
        MouseState m_previousMouse;
        int m_mouseWheelDelta = 0;

        bool m_hasFocus = true;
    };
}