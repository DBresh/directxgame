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

    private:
        static InputSystem* m_system;

        std::vector<InputListener*> m_listeners;

        std::unordered_map<int, bool> m_currentKeys;
        std::unordered_map<int, bool> m_previousKeys;
    };
}
