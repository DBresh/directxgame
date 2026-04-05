#pragma once
#include <string>
#include <imgui.h>

namespace dx3d
{
    enum class PanelAlignment { None, Left, Right, Bottom, Top };

    class UIPanel
    {
    public:
        explicit UIPanel(const std::string& name) : m_name(name) {}
        virtual ~UIPanel() = default;

        void render(float scale = 1.0f)
        {
            if (!enabled) return;

            ImGuiIO& io = ImGui::GetIO();

            // Multiply all our hardcoded base sizes by the scale factor
            float scaledWidth = width * scale;
            float scaledHeight = height * scale;
            float sMarginLeft = marginLeft * scale;
            float sMarginRight = marginRight * scale;
            float sMarginTop = marginTop * scale;
            float sMarginBottom = marginBottom * scale;

            if (alignment == PanelAlignment::Left) {
                ImGui::SetNextWindowPos(ImVec2(0.0f, sMarginTop), ImGuiCond_Always);
                ImGui::SetNextWindowSize(ImVec2(scaledWidth, io.DisplaySize.y - sMarginTop - sMarginBottom), ImGuiCond_Always);
            }
            else if (alignment == PanelAlignment::Right) {
                ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - scaledWidth, sMarginTop), ImGuiCond_Always);
                ImGui::SetNextWindowSize(ImVec2(scaledWidth, io.DisplaySize.y - sMarginTop - sMarginBottom), ImGuiCond_Always);
            }
            else if (alignment == PanelAlignment::Bottom) {
                ImGui::SetNextWindowPos(ImVec2(sMarginLeft, io.DisplaySize.y - scaledHeight), ImGuiCond_Always);
                ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x - sMarginLeft - sMarginRight, scaledHeight), ImGuiCond_Always);
            }

            ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

            ImGui::Begin(m_name.c_str(), &enabled, flags);
            updateContent();
            ImGui::End();
        }

        bool enabled = true;
        PanelAlignment alignment = PanelAlignment::None;
        float width = 300.0f;
        float height = 200.0f;

        float marginLeft = 0.0f;
        float marginRight = 0.0f;
        float marginTop = 0.0f;
        float marginBottom = 0.0f;

    protected:
        virtual void updateContent() = 0;

    private:
        std::string m_name;
    };
}