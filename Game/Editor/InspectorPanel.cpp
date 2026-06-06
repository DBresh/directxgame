#include <Game/Editor/InspectorPanel.h>
#include <imgui.h>
#include <cstring>

namespace dx3d
{
    InspectorPanel::InspectorPanel(
        Entity& selectedEntity,
        SceneManager& scene,
        TransformSystem& transforms,
        RenderComponentSystem& renderables,
        OrbitSystem* orbitSystem)
        : UIPanel("Inspector"),
        m_selectedEntity(selectedEntity),
        m_scene(scene),
        m_transforms(transforms),
        m_renderables(renderables),
        m_orbitSystem(orbitSystem)
    {
        this->alignment = PanelAlignment::Right;
        this->width = 300.0f;
    }

    void InspectorPanel::init() {}

    void InspectorPanel::updateContent()
    {
        if (!m_selectedEntity.isNull() && m_transforms.hasTransform(m_selectedEntity))
        {
            auto metadataObject = m_scene.findObjectByEntity(m_selectedEntity);
            const char* selectedName = metadataObject ? metadataObject->name.c_str() : "Unnamed Entity";

            ImGui::Text("Name: %s", selectedName);
            ImGui::Separator();

            if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (metadataObject) {
                    char nameBuf[128];
                    strncpy_s(nameBuf, metadataObject->name.c_str(), sizeof(nameBuf));
                    if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
                        metadataObject->name = nameBuf;
                    }
                }
                else {
                    ImGui::TextDisabled("No editor metadata is bound for this entity.");
                }

                auto& hierarchy = m_transforms.getHierarchy(m_selectedEntity);
                ImGui::Checkbox("Inherit Position", &hierarchy.inheritPosition);
                ImGui::Checkbox("Inherit Rotation", &hierarchy.inheritRotation);
                ImGui::Checkbox("Inherit Scale", &hierarchy.inheritScale);
            }

            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
            {
                Transform workingTransform = m_transforms.getTransform(m_selectedEntity);
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
                    m_transforms.setTransform(m_selectedEntity, workingTransform);
                    if (metadataObject) {
                        metadataObject->cachedEditorTransform = workingTransform;
                    }
                }
            }

            if (m_renderables.has(m_selectedEntity))
            {
                ImGui::Separator();
                if (ImGui::CollapsingHeader("Render Component", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    auto& renderable = m_renderables.get(m_selectedEntity);
                    ImGui::Checkbox("Visible", &renderable.visible);
                    ImGui::Checkbox("Casts Shadow", &renderable.castsShadow);
                    ImGui::Text("Model: %s", renderable.model ? "Assigned" : "None");
                }
            }

            if (m_orbitSystem && m_orbitSystem->hasOrbit(m_selectedEntity))
            {
                ImGui::Separator();
                if (ImGui::CollapsingHeader("Orbit Dynamics", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    auto& orbit = m_orbitSystem->getOrbit(m_selectedEntity);
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
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Select an entity in the Hierarchy.");
        }
    }
}
