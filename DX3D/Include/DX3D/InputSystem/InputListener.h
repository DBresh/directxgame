#pragma once
#include <DX3D/Math/Point.h>

namespace dx3d
{
    struct MouseState
    {
        Point coords{0};
        int wheelDelta = 0;
        bool leftButton = false;
        bool rightButton = false;
        bool middleButton = false;
    };

    class InputListener
    {
    public:
        virtual ~InputListener() = default;

        virtual void onKeyDown(int key) = 0;
        virtual void onKeyUp(int key) = 0;
        virtual void onKeyPress(int key) = 0;

        virtual void onMouseMove(Point deltaMouse) = 0;
        virtual void onMouseDown(int button) = 0; // button: 0=left,1=right,2=middle
        virtual void onMouseUp(int button) = 0;
        virtual void onMouseWheel(int delta) = 0;
    };
}