#pragma once
#include <Game/Editor/UIPanel.h>
#include <DX3D/InputSystem/Camera.h>
#include <Game/Editor/HierarchyPanel.h>
#include <Game/Kepler/TimeController.h>

namespace dx3d
{
    class TimelinePanel : public UIPanel
    {
    public:
        TimelinePanel(Simulator::TimeController& timeController, Camera& camera);

    protected:
        void updateContent() override;

    private:
        Simulator::TimeController& m_timeController;
        Camera& m_camera;

        bool m_isPaused = false;
        double m_savedTimeScale = 1.0;
    };
}