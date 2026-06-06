#pragma once
#include <string>

namespace dx3d {

    struct EditorMetadata {
        std::string name;
        std::string tag;
        bool expanded = false;
    };
}