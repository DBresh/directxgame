#include <Game/Editor/HierarchyPanel.h>
#include <DX3D/InputSystem/Camera.h>
#include <DX3D/Math/Frustrum.h>
#include <imgui.h>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_set>

namespace dx3d
{
    HierarchyPanel::HierarchyPanel(SceneManager& scene, TransformSystem& transforms, RenderComponentSystem& renderables, Entity& selectedEntity, Camera& camera)
        : UIPanel("Scene Hierarchy"), m_scene(scene), m_transforms(transforms), m_renderables(renderables), m_selectedEntity(selectedEntity), m_camera(camera)
    {
        this->alignment = PanelAlignment::Left;
        this->width = 250.0f;
    }

    void HierarchyPanel::init() {}

    void HierarchyPanel::updateContent()
    {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::SameLine();
        ImGui::TextDisabled(" | ");
        ImGui::SameLine();

        Frustum frustum;
        frustum.constructFromViewProj(m_camera.getViewMatrix(), m_camera.getProjectionMatrix());

        size_t visibleCount = 0;
        const auto& renderEntities = m_renderables.getRawEntities();
        for (Entity entity : renderEntities)
        {
            if (!m_renderables.has(entity) || !m_transforms.hasTransform(entity)) {
                continue;
            }

            const auto& renderable = static_cast<const RenderComponentSystem&>(m_renderables).get(entity);
            if (!renderable.model) {
                continue;
            }

            const auto& world = m_transforms.getWorld(entity);
            Transform worldTransform;
            worldTransform.setPosition(world.position);
            worldTransform.setQuaternion(world.rotation);
            worldTransform.setScale(world.scale);

            AABB bounds = renderable.model->boundingBox.transform(worldTransform.getWorldMatrixRelative(m_camera.getPosition()));
            if (frustum.checkAABB(bounds)) {
                visibleCount++;
            }
        }

        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Visible: %zu", visibleCount);
        ImGui::Separator();

        bool selectionChanged = (m_lastSelectedEntity != m_selectedEntity);
        m_lastSelectedEntity = m_selectedEntity;

        std::unordered_set<uint32_t> ancestorsToExpand;
        if (selectionChanged && !m_selectedEntity.isNull() && m_transforms.hasTransform(m_selectedEntity))
        {
            Entity curr = m_transforms.getHierarchy(m_selectedEntity).parent;
            while (!curr.isNull() && m_transforms.hasTransform(curr))
            {
                ancestorsToExpand.insert(curr.id);
                curr = m_transforms.getHierarchy(curr).parent;
            }
        }

        std::function<void(Entity)> drawNode;
        drawNode = [&](Entity entity)
            {
                if (!m_transforms.hasTransform(entity)) {
                    return;
                }

                const auto& hierarchy = m_transforms.getHierarchy(entity);
                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;

                flags |= ImGuiTreeNodeFlags_DefaultOpen;

                if (m_selectedEntity == entity) {
                    flags |= ImGuiTreeNodeFlags_Selected;
                }

                if (hierarchy.firstChild.isNull()) {
                    flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                }

                if (ancestorsToExpand.count(entity.id)) {
                    ImGui::SetNextItemOpen(true, ImGuiCond_Always);
                }

                std::string displayName = "Entity " + std::to_string(entity.id);
                if (auto obj = m_scene.findObjectByEntity(entity)) {
                    displayName = obj->name;
                }

                bool nodeOpen = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<uintptr_t>(entity.id) + 1u), flags, "%s", displayName.c_str());

                if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                    m_selectedEntity = entity;
                    m_lastSelectedEntity = entity;
                }

                if (nodeOpen && !hierarchy.firstChild.isNull()) {
                    Entity child = hierarchy.firstChild;
                    while (!child.isNull() && m_transforms.hasTransform(child)) {
                        Entity nextSibling = m_transforms.getHierarchy(child).nextSibling;
                        drawNode(child);
                        child = nextSibling;
                    }
                    ImGui::TreePop();
                }
            };

        const auto& entities = m_transforms.getRawEntities();
        for (Entity entity : entities)
        {
            if (!m_transforms.hasTransform(entity)) {
                continue;
            }

            if (m_transforms.getHierarchy(entity).parent.isNull())
            {
                drawNode(entity);
            }
        }
    }
}
