#pragma once

#include "../../../ImGui/imgui.h"
#include <string>
#include <vector>
#include <map>
#include "../../../Utils/HudElement.hpp"

class ArrayList {
public:
    struct ModuleInfo {
        std::string name;
        std::string suffix;
        bool enabled;
        float animation;
        float width;
    };

    static bool g_showArrayList;
    static bool g_followTheme;
    static ImVec4 g_bgColor;
    static float g_bgOpacity;
    static bool g_showSideBar;
    static ImVec4 g_sideBarColor;
    static bool g_chromaSideBar;
    static bool g_roundedBorders;
    static float g_borderRadius;
    static bool g_showSuffix;

    static void Initialize();
    static void Render();
    static void RenderMenu();
};
