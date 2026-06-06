#include <Game/Editor/InspectorPanel.h>
#include <imgui.h>

namespace dx3d
{
    InspectorPanel::InspectorPanel(
        std::shared_ptr<GameObject>& selectedObject,
        OrbitSystem* orbitSystem,
        std::function<const Transform* (Entity)> resolveTransform,
        std::function<void(Entity, const Transform&)> assignTransform,
        std::function<bool(Entity, const Transform&)> applyTransform)
        : UIPanel("Inspector"),
        m_selectedObject(selectedObject),
        m_orbitSystem(orbitSystem),
        m_resolveTransform(std::move(resolveTransform)),
        m_assignTransform(std::move(assignTransform)),
        m_applyTransform(std::move(applyTransform))
    {
        this->alignment = PanelAlignment::Right;
        this->width = 300.0f;
    }

    void InspectorPanel::init() {}

    void InspectorPanel::updateContent()
    {
        if (m_selectedObject)
        {
            ImGui::Text("Name: %s", m_selectedObject->name.c_str());
            ImGui::Separator();

            if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen))
            {
                char nameBuf[128];
                strncpy_s(nameBuf, m_selectedObject->name.c_str(), sizeof(nameBuf));
                if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
                    m_selectedObject->name = nameBuf;
                }

                ImGui::Checkbox("Inherit Position", &m_selectedObject->inheritPosition);
                ImGui::Checkbox("Inherit Rotation", &m_selectedObject->inheritRotation);
                ImGui::Checkbox("Inherit Scale", &m_selectedObject->inheritScale);
            }

            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
            {
                const Transform* runtimeTransform = m_resolveTransform ? m_resolveTransform(m_selectedObject->entity) : nullptr;
                Transform workingTransform = runtimeTransform ? *runtimeTransform : m_selectedObject->cachedEditorTransform;

                bool transformChanged = false;

                dx3d::Vec3d pos = workingTransform.getPosition();
                float dragSpeed = 0.1f;
                if (ImGui::DragScalarN("Position", ImGuiDataType_Double, &pos.x, 3, dragSpeed))
                {
                    workingTransform.setPosition(pos);
                    transformChanged = true;
                }

                DirectX::XMFLOAT3 rot = workingTransform.getEuler();
                if (ImGui::DragFloat3("Rotation", &rot.x, 0.05f)) {
                    workingTransform.setEuler(rot);
                    transformChanged = true;
                }

                DirectX::XMFLOAT3 sca = workingTransform.getScale();
                if (ImGui::DragFloat3("Scale", &sca.x, 0.1f)) {
                    workingTransform.setScale(sca);
                    transformChanged = true;
                }

                if (transformChanged) {
                    if (m_applyTransform) {
                        m_applyTransform(m_selectedObject->entity, workingTransform);
                    }
                    else {
                        if (m_assignTransform) {
                            m_assignTransform(m_selectedObject->entity, workingTransform);
                        }
                        m_selectedObject->cachedEditorTransform = workingTransform;
                    }
                }
            }

            if (m_orbitSystem && m_orbitSystem->hasOrbit(m_selectedObject->entity))
            {
                ImGui::Separator();
                if (ImGui::CollapsingHeader("Orbit Dynamics", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    auto& orbit = m_orbitSystem->getOrbit(m_selectedObject->entity);
                    bool physicsChanged = false;

                    ImGui::TextDisabled("Relative State Vectors");
                    physicsChanged |= ImGui::DragScalarN("Rel Position", ImGuiDataType_Double, &orbit.positionRelativeToAttractor.x, 3, 0.5f);
                    physicsChanged |= ImGui::DragScalarN("Rel Velocity", ImGuiDataType_Double, &orbit.velocityRelativeToAttractor.x, 3, 0.05f);

                    ImGui::Separator();
                    ImGui::TextDisabled("Physics Parameters");
                    if (ImGui::Checkbox("Freeze Body (Pause Physics)", &orbit.isFrozen)) {
                        if (!orbit.isFrozen) orbit.elementsDirty = true;
                    }

                    double const minMass = 0.001;
                    physicsChanged |= ImGui::DragScalar("Body Mass", ImGuiDataType_Double, &orbit.BodyMass, 10.0f, &minMass);
                    ImGui::Text("Attractor Mass: %.2f", orbit.AttractorMass);

                    if (physicsChanged) {
                        orbit.elementsDirty = true;
                    }

                    ImGui::Separator();
                    ImGui::TextDisabled("Orbital Elements (Read-Only)");
                    ImGui::Text("Eccentricity: %.4f", orbit.Eccentricity);
                    ImGui::Text("Semi-Major Axis: %.2f", orbit.SemiMajorAxis);
                    ImGui::Text("Period: %.2f Days", orbit.Period);

                    ImGui::Separator();
                    ImGui::TextDisabled("Rendering");
                    ImGui::Checkbox("Freeze Color", &orbit.freezeColor);
                    if (orbit.freezeColor) {
                        ImGui::ColorEdit4("Orbit Color", &orbit.orbitColor.x);
                        orbit.visualDirty = true;
                    }
                }
            }
        }
        else
        {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Select an object in the Hierarchy.");
        }
    }
}