#pragma once
#include <DX3D/Editor/UIPanel.h>
#include <DX3D/InputSystem/Camera.h>

namespace dx3d
{
    class TimelinePanel : public UIPanel
    {
    public:
        TimelinePanel(float& timeWarp, Camera& camera);

    protected:
        void updateContent() override;

    private:
        float& m_timeWarp;
        Camera& m_camera;
    };
}