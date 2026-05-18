#include <Game/Editor/AssetBrowserPanel.h>
#include <ThirdParty/ImGui/imgui.h>
#include <filesystem>
#include <DX3D/Core/Logger.h>

namespace fs = std::filesystem;

namespace dx3d {
    AssetBrowserPanel::AssetBrowserPanel() : UIPanel("Asset Browser")
    {
        this->alignment = PanelAlignment::Bottom;
        this->height = 200.f;
        this->marginLeft = 250.f;
        this->marginRight = 300.f;
    }

    void AssetBrowserPanel::init()
    {
        refreshAssets();
    }

    void AssetBrowserPanel::refreshAssets()
    {
        m_assets.clear();
        std::string path = "DX3D/Assets/Models/";

        try {
            if (fs::exists(path)) {
                for (const auto& entry : fs::directory_iterator(path)) {
                    if (entry.is_regular_file()) {
                        std::string ext = entry.path().extension().string();
                        std::string filename = entry.path().filename().string();

                        if (ext == ".obj" && filename.find(".bin") == std::string::npos) {
                            m_assets.push_back(filename);
                        }
                    }
                }
            }
            else {
                DX3D_LOG_ERROR("Asset path does not exist: {}", path);
            }
        }
        catch (const fs::filesystem_error& e) {
            DX3D_LOG_ERROR("Filesystem error scanning assets: {}", e.what());
        }
    }

    void AssetBrowserPanel::updateContent()
    {
        if (ImGui::Button("Refresh")) {
            refreshAssets();
        }
        ImGui::Separator();

        float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
        ImGuiStyle& style = ImGui::GetStyle();

        float buttonSize = 90.0f;

        for (size_t i = 0; i < m_assets.size(); ++i)
        {
            ImGui::PushID(static_cast<int>(i));

            ImGui::Button(m_assets[i].c_str(), ImVec2(buttonSize, buttonSize));

            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                const std::string& payloadStr = m_assets[i];
                ImGui::SetDragDropPayload("DND_ASSET_MODEL", payloadStr.c_str(), payloadStr.size() + 1);

                ImGui::Text("Drop to spawn: %s", payloadStr.c_str());

                ImGui::EndDragDropSource();
            }

            ImGui::PopID();

            float last_button_x2 = ImGui::GetItemRectMax().x;
            float next_button_x2 = last_button_x2 + style.ItemSpacing.x + buttonSize;

            if (i + 1 < m_assets.size() && next_button_x2 < window_visible_x2) {
                ImGui::SameLine();
            }
        }
    }

}