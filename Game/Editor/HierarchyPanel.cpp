#include <Game/Editor/HierarchyPanel.h>
#include <DX3D/InputSystem/Camera.h>
#include <DX3D/Math/Frustrum.h>
#include <imgui.h>
#include <functional>
#include <unordered_set>

namespace dx3d
{
    HierarchyPanel::HierarchyPanel(SceneManager& scene, std::shared_ptr<GameObject>& selectedObject, Camera& camera)
        : UIPanel("Scene Hierarchy"), m_scene(scene), m_selectedObject(selectedObject), m_camera(camera)
    {
        this->alignment = PanelAlignment::Left;
        this->width = 250.0f;
    }

    void HierarchyPanel::updateContent()
    {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::SameLine();
        ImGui::TextDisabled(" | ");
        ImGui::SameLine();

        Frustum frustum;
        frustum.constructFromViewProj(m_camera.getViewMatrix(), m_camera.getProjectionMatrix());

        size_t visibleCount = 0;
        const auto& objects = m_scene.getAllObjects();
        for (const auto& obj : objects)
        {
            if (obj->hasMesh()) {
                AABB bounds = obj->getWorldAABB();
                if (frustum.checkAABB(bounds)) {
                    visibleCount++;
                }
            }
        }

        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Visible: %zu", visibleCount);
        ImGui::Separator();

        bool selectionChanged = (m_lastSelectedObject != m_selectedObject);
        m_lastSelectedObject = m_selectedObject;

        std::unordered_set<GameObject*> ancestorsToExpand;
        if (selectionChanged && m_selectedObject)
        {
            auto curr = m_selectedObject->getParent();
            while (curr)
            {
                ancestorsToExpand.insert(curr.get());
                curr = curr->getParent();
            }
        }

        std::function<void(const std::shared_ptr<GameObject>&)> drawNode;
        drawNode = [&](const std::shared_ptr<GameObject>& obj)
            {
                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;

                // Expand all by default at startup
                flags |= ImGuiTreeNodeFlags_DefaultOpen;

                if (m_selectedObject == obj) {
                    flags |= ImGuiTreeNodeFlags_Selected;
                }

                if (obj->children.empty()) {
                    flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                }

                if (ancestorsToExpand.count(obj.get())) {
                    ImGui::SetNextItemOpen(true, ImGuiCond_Always);
                }

                bool nodeOpen = ImGui::TreeNodeEx((void*)obj.get(), flags, "%s", obj->name.c_str());

                if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                    m_selectedObject = obj;
                    m_lastSelectedObject = obj;
                }

                if (nodeOpen && !obj->children.empty()) {
                    for (auto& child : obj->children) {
                        drawNode(child);
                    }
                    ImGui::TreePop();
                }
            };

        for (auto& obj : objects)
        {
            if (!obj->hasParent())
            {
                drawNode(obj);
            }
        }
    }
}