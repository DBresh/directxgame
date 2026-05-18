#pragma once
#include <vector>
#include <memory>
#include <Game/Editor/UIPanel.h>

namespace dx3d
{
    class UIManager
    {
    public:
        void addPanel(std::shared_ptr<UIPanel> panel)
        {
            panel->init();
            m_panels.push_back(panel);
        }

        void update()
        {
            for (auto& panel : m_panels)
            {
                panel->render(m_uiScale);
            }
        }

        void setScale(float scale) { m_uiScale = scale; }

    private:
        std::vector<std::shared_ptr<UIPanel>> m_panels;
        float m_uiScale = 1.0f;
    };
}