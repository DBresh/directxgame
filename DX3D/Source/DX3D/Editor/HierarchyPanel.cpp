#include <DX3D/Editor/HierarchyPanel.h>
#include <imgui.h>

namespace dx3d
{
    HierarchyPanel::HierarchyPanel(SceneManager& scene, std::shared_ptr<GameObject>& selectedObject)
        : UIPanel("Scene Hierarchy"), m_scene(scene), m_selectedObject(selectedObject)
    {
        this->alignment = PanelAlignment::Left;
        this->width = 250.0f;
    }

    void HierarchyPanel::updateContent()
    {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::Separator();

        auto& objects = m_scene.getAllObjects();
        for (auto& obj : objects)
        {
            bool isSelected = (m_selectedObject == obj);
            if (ImGui::Selectable(obj->name.c_str(), isSelected))
            {
                m_selectedObject = obj;
            }
        }
    }
}