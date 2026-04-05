#include <DX3D/Editor/TimelinePanel.h>
#include <imgui.h>

namespace dx3d
{
    TimelinePanel::TimelinePanel(float& timeWarp, Camera& camera)
        : UIPanel("Timeline Controls"), m_timeWarp(timeWarp), m_camera(camera)
    {
        this->alignment = PanelAlignment::Bottom;
        this->height = 80.0f;

        this->marginLeft = 250.0f;
        this->marginRight = 300.0f;
    }

    void TimelinePanel::updateContent()
    {
        ImGui::SliderFloat("Time Warp", &m_timeWarp, 0.0f, 5000.0f);
        ImGui::SliderFloat("Player Speed", &m_camera.m_speed, 2.0f, 500.0f);
    }
}