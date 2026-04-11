#include <DX3D/Editor/InspectorPanel.h>
#include <imgui.h>

namespace dx3d
{
    InspectorPanel::InspectorPanel(std::shared_ptr<GameObject>& selectedObject)
        : UIPanel("Inspector"), m_selectedObject(selectedObject)
    {
        this->alignment = PanelAlignment::Right;
        this->width = 300.0f;
    }

    void InspectorPanel::updateContent()
    {
        if (m_selectedObject)
        {
            ImGui::Text("Name: %s", m_selectedObject->name.c_str());
            ImGui::Separator();

            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
            {
                DirectX::XMFLOAT3 pos = m_selectedObject->transform.getPosition();
                if (ImGui::DragFloat3("Position", &pos.x, 0.1f)) m_selectedObject->transform.setPosition(pos);

                DirectX::XMFLOAT3 rot = m_selectedObject->transform.getEuler();
                if (ImGui::DragFloat3("Rotation", &rot.x, 0.05f)) m_selectedObject->transform.setEuler(rot);

                DirectX::XMFLOAT3 sca = m_selectedObject->transform.getScale();
                if (ImGui::DragFloat3("Scale", &sca.x, 0.1f)) m_selectedObject->transform.setScale(sca);
            }

            for (auto& comp : m_selectedObject->components)
            {
                ImGui::Separator();
                comp->onInspectorGUI();
            }
        }
        else
        {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Select an object in the Hierarchy.");
        }
    }
}