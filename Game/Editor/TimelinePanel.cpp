#include <Game/Editor/TimelinePanel.h>
#include <Game/Kepler/TimeController.h>
#include <imgui.h>
#include <string>

namespace dx3d
{
    TimelinePanel::TimelinePanel(Simulator::TimeController& timeController, Camera& camera)
        : UIPanel("Timeline Controls"), m_timeController(timeController), m_camera(camera)
    {
        this->alignment = PanelAlignment::Top;
        this->height = 100.0f;
        this->marginLeft = 250.0f;
        this->marginRight = 300.0f;
    }

    void TimelinePanel::init() {}

    void TimelinePanel::updateContent()
    {
        ImGui::TextDisabled("Simulation Epoch:");

        ImGui::SameLine(200.0f);
        ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - 350.0f);
        ImGui::SliderFloat("Cam Speed", &m_camera.m_speed, 2.0f, 500.0f);

        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%.2f Days", m_timeController.Epoch);

        ImGui::Separator();

        bool isPaused = m_timeController.IsPaused();
        bool isReversed = m_timeController.IsReversed();

        if (isReversed) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.4f, 0.2f, 1.0f)); // Orange tint for reverse
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.5f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.3f, 0.1f, 1.0f));
        }

        if (ImGui::Button(" < Rev ")) {
            m_timeController.Reverse();
            m_timeController.SetPaused(false);
        }

        if (isReversed) {
            ImGui::PopStyleColor(3);
        }
        ImGui::SameLine();

        if (ImGui::Button(isPaused ? " > Resume " : " || Pause ")) {
            m_timeController.SetPaused(!isPaused);
        }

        ImGui::SameLine();
        ImGui::TextDisabled(" | ");
        ImGui::SameLine();

        for (size_t i = 0; i < m_timeController.WarpLevels.size(); ++i) {
            std::string btnText = std::format("{:.2f}x", (m_timeController.WarpLevels[i]));

            bool isActive = (static_cast<int>(i) == m_timeController.GetTimeWarpIndex() && !isPaused);

            if (isActive) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.5f, 0.1f, 1.0f));
            }

            if (ImGui::Button(btnText.c_str())) {
                m_timeController.SetTimeWarpByIndex(static_cast<int>(i));
                m_timeController.SetPaused(false);
            }

            if (isActive) {
                ImGui::PopStyleColor(3);
            }

            ImGui::SameLine();
        }

        ImGui::TextDisabled(" | ");
        ImGui::SameLine();

        if (ImGui::Button(" << Slower ")) {
            m_timeController.DecreaseWarp();
            m_timeController.SetPaused(false);
        }
        ImGui::SameLine();

        if (ImGui::Button(" Faster >> ")) {
            m_timeController.IncreaseWarp();
            m_timeController.SetPaused(false);
        }
        ImGui::SameLine();

        if (ImGui::Button(" Reset 1x ")) {
            m_timeController.StopTimeWarp();
            m_timeController.SetPaused(false);
        }

        ImGui::SameLine();
        ImGui::TextDisabled(" | ");
        ImGui::SameLine();

        if (isPaused) {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.2f, 1.0f), "Status: PAUSED");
        }
        else {
            if (isReversed) {
                ImGui::TextColored(ImVec4(0.8f, 0.4f, 0.2f, 1.0f), "REVERSING ");
                ImGui::SameLine();
            }
            ImGui::TextDisabled("Warp Level %d:", m_timeController.GetTimeWarpIndex());
            ImGui::SameLine();
            ImGui::Text("%.1fx Realtime", std::abs(m_timeController.TimeScale));
        }
    }
}