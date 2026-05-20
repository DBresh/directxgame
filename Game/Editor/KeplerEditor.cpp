#include <Game/Editor/KeplerEditor.h>
#include <Game/Editor/HierarchyPanel.h>
#include <Game/Editor/InspectorPanel.h>
#include <Game/Editor/TimelinePanel.h>
#include <Game/Editor/AssetBrowserPanel.h>
#include <DX3D/Core/Logger.h>
#include <Game/Components/OrbitComponent.h>

#include <imgui.h>

namespace dx3d
{
    KeplerEditor::KeplerEditor(SceneManager& scene, Simulator::OrbitSystem& orbitSystem,
        Simulator::TimeController& timeController, Camera& camera,
        AssetManager& assets, Display* display, GraphicsEngine& graphicsEngine)
        : m_scene(scene), m_orbitSystem(orbitSystem), m_timeController(timeController),
        m_camera(camera), m_assets(assets), m_display(display), m_graphicsEngine(graphicsEngine)
    {
    }

    KeplerEditor::~KeplerEditor() = default;

    void KeplerEditor::init()
    {
        m_uiManager.addPanel(std::make_shared<HierarchyPanel>(m_scene, m_selectedObject, m_camera));
        m_uiManager.addPanel(std::make_shared<InspectorPanel>(m_selectedObject));
        m_uiManager.addPanel(std::make_shared<TimelinePanel>(m_timeController, m_camera));
        m_uiManager.addPanel(std::make_shared<AssetBrowserPanel>());
    }

    void KeplerEditor::onUpdate(double dt)
    {
    }

    void KeplerEditor::onWindowResized(int width, int height)
    {
        if (ImGui::GetCurrentContext())
        {
            float scaleFactor = (static_cast<float>(height) / 1080.0f) * 1.25f;
            if (scaleFactor < 1.0f) scaleFactor = 1.0f;

            ImGui::GetIO().FontGlobalScale = scaleFactor;

            ImGuiStyle& style = ImGui::GetStyle();
            style = ImGuiStyle();
            ImGui::StyleColorsDark();
            style.ScaleAllSizes(scaleFactor);

            m_uiManager.setScale(scaleFactor);
        }
    }

    void KeplerEditor::onGUI()
    {
        handleMouseWrapping();
        m_uiManager.update();
        handleViewportDragAndDrop();
    }

    void KeplerEditor::handleMouseWrapping()
    {
        ImGuiIO& io = ImGui::GetIO();
        if (ImGui::IsAnyItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            ImVec2 pos = io.MousePos;
            bool wrapped = false;

            if (pos.x <= 0.0f) { pos.x = io.DisplaySize.x - 2.0f; wrapped = true; }
            else if (pos.x >= io.DisplaySize.x - 1.0f) { pos.x = 1.0f; wrapped = true; }

            if (wrapped)
            {
                io.WantSetMousePos = true;
                io.MousePos = pos;
                io.MouseDelta = ImVec2(0.0f, 0.0f);
                io.MousePosPrev = pos;
            }
        }
    }

    void KeplerEditor::handleViewportDragAndDrop()
    {
        bool isHoveringDropTarget = false;

        if (const ImGuiPayload* activePayload = ImGui::GetDragDropPayload())
        {
            if (activePayload->IsDataType("DND_ASSET_MODEL"))
            {
                ImGuiViewport* viewport = ImGui::GetMainViewport();
                ImGui::SetNextWindowPos(viewport->WorkPos);
                ImGui::SetNextWindowSize(viewport->WorkSize);
                ImGui::SetNextWindowBgAlpha(0.0f);

                ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
                    ImGuiWindowFlags_NoBackground;

                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
                ImGui::Begin("ViewportDropTarget", nullptr, window_flags);
                ImGui::PopStyleVar();

                ImGui::Dummy(ImGui::GetContentRegionAvail());

                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_ASSET_MODEL", ImGuiDragDropFlags_AcceptBeforeDelivery))
                    {
                        isHoveringDropTarget = true;
                        m_isDraggingAsset = true;

                        const char* modelName = (const char*)payload->Data;
                        std::string assetStr(modelName);

                        ImVec2 mousePos = ImGui::GetMousePos();
                        int width = m_display->getClientWidth();
                        int height = m_display->getClientHeight();

                        DirectX::XMVECTOR rayOrigin, rayDir;
                        m_camera.screenPointToRay(mousePos.x, mousePos.y, width, height, rayOrigin, rayDir);

                        float hitDistance = -1.0f;
                        auto hitObj = m_scene.pickObject(rayOrigin, rayDir, m_camera.getPosition(), &hitDistance);

                        float scaleSize = 30.0f;
                        DirectX::XMVECTOR relativeOffset;

                        if (hitObj && hitDistance > 0.0f) {
                            float backStepOffset = scaleSize * 0.5f;
                            relativeOffset = DirectX::XMVectorScale(rayDir, hitDistance - backStepOffset);
                        }
                        else {
                            relativeOffset = DirectX::XMVectorScale(rayDir, 400.0f);
                        }

                        dx3d::Vec3d camPos = m_camera.getPosition();
                        dx3d::Vec3d finalAbsolutePos(
                            camPos.x + DirectX::XMVectorGetX(relativeOffset),
                            camPos.y + DirectX::XMVectorGetY(relativeOffset),
                            camPos.z + DirectX::XMVectorGetZ(relativeOffset)
                        );

                        if (!m_dragPreviewObject) {
                            m_dragPreviewObject = std::make_shared<GameObject>("DragPreview");
                            m_dragPreviewObject->model = m_assets.getModel(assetStr);
                            m_dragPreviewObject->transform.setScale(DirectX::XMFLOAT3(scaleSize, scaleSize, scaleSize));

                            GraphicsDevice& gd = m_graphicsEngine.getGraphicsDevice();
                            m_dragPreviewObject->constantBuffer = gd.createConstantBuffer({ nullptr, sizeof(DirectX::XMFLOAT4X4) * 3 });
                        }
                        m_dragPreviewObject->transform.setPosition(finalAbsolutePos);

                        if (payload->IsDelivery())
                        {
                            std::string entityName = assetStr.substr(0, assetStr.find_last_of('.'));
                            auto newObj = m_scene.createObject(entityName);
                            newObj->modelName = assetStr;
                            newObj->model = m_dragPreviewObject->model;
                            newObj->transform.setScale(DirectX::XMFLOAT3(scaleSize, scaleSize, scaleSize));
                            newObj->transform.setPosition(finalAbsolutePos);

                            if (m_scene.onObjectCreated) {
                                m_scene.onObjectCreated(newObj);
                            }

                            DX3D_LOG_INFO("Spawned {} at ({:.2f}, {:.2f}, {:.2f})", entityName, finalAbsolutePos.x, finalAbsolutePos.y, finalAbsolutePos.z);

                            m_dragPreviewObject = nullptr;
                            m_isDraggingAsset = false;
                        }
                    }
                    ImGui::EndDragDropTarget();
                }
                ImGui::End();
            }
        }

        if (!isHoveringDropTarget && m_isDraggingAsset) {
            m_dragPreviewObject = nullptr;
            m_isDraggingAsset = false;
        }
    }

    void KeplerEditor::onDrawDebug(DeviceContext& ctx, const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& proj)
    {
        for (const auto& obj : m_scene.getAllObjects())
        {
            auto orbitComp = obj->getComponent<OrbitComponent>();
            if (orbitComp && orbitComp->isVisible)
            {
                Simulator::OrbitData& orbit = orbitComp->getOrbit();
                if (orbit.ParentOrbitIndex != -1)
                {
                    const Simulator::OrbitData& parentOrbit = m_orbitSystem.GetOrbit(orbit.ParentOrbitIndex);
                    dx3d::Vec3d relParentPos = parentOrbit.absoluteWorldPosition - m_camera.getPosition();
                    orbitComp->visualizer.draw(ctx, view, proj, relParentPos);
                }
            }
        }

        if (m_dragPreviewObject && m_isDraggingAsset && m_dragPreviewObject->model)
        {
            DirectX::XMFLOAT4X4 worldMatrix = m_dragPreviewObject->transform.getWorldMatrixRelative(m_camera.getPosition());
            m_graphicsEngine.getRenderSystem().drawModel(
                ctx,
                *m_dragPreviewObject->model,
                *m_dragPreviewObject->constantBuffer,
                worldMatrix
            );
        }
    }
}