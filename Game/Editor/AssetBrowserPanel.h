#pragma once
#include <Game/Editor/UIPanel.h>
#include <vector>
#include <string>

namespace dx3d {

    class AssetBrowserPanel : public UIPanel
    {
    public:
        AssetBrowserPanel();
        ~AssetBrowserPanel() override = default;

        void init() override;
        void updateContent() override;

    private:
        std::vector<std::string> m_assets;

        void refreshAssets();
    };

}