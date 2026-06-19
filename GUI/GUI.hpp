#pragma once

#include <windows.h>
#include "../ImGui/imgui.h"
#include <map>
#include <string>
#include <vector>

class GUI {
public:
    enum ThemePreset {
        Theme_AmatayakulRed,
        Theme_SakuraBlossom,
        Theme_Cyberpunk,
        Theme_EmeraldForest,
        Theme_DeepSea,
        Theme_Max
    };

    enum ModFilter {
        Filter_All,
        Filter_Visual,
        Filter_Misc
    };

    enum ModCategory {
        Cat_Mods,
        Cat_Terminal,
        Cat_Info,
        Cat_IRC
    };

    // Menu state
    static bool g_showMenu;
    static float g_menuAnim;

    // Theme
    static ThemePreset g_currentTheme;
    static ImVec4 g_colorBgMain;
    static ImVec4 g_colorBgPanel;
    static ImVec4 g_colorAccent;
    static ImVec4 g_colorAccentSoft;
    static ImVec4 g_colorAccentGlow;

    // Mod filters & search
    static ModFilter g_currentFilter;
    static ModCategory g_currentCategory;
    static char g_searchBuffer[128];
    static float g_filterHoverAnim[3];
    static float g_categoryHoverAnim[4];

    // Profiles
    static std::vector<std::string> g_profiles;
    static int g_selectedProfile;
    static char g_newProfileBuf[64];
    static bool g_showNewProfileInput;
    static int g_editingProfileIndex;
    static char g_renameBuf[64];

    // Icons
    static std::map<std::string, ImTextureID> g_icons;
    static std::map<std::string, float> g_elementAnims;
    static std::string g_currentSettingsModule;

    // Transition animations
    static ModCategory g_lastCategory;
    static float g_tabTransitionAnim;
    static float g_settingsTransitionAnim;

    // Theme & rendering
    static void ApplyTheme();
    static void ApplyThemePreset(ThemePreset preset);
    static void LoadFont();
    static void LoadIcons(void* pDevice);

    // Animation
    static void UpdateAnimation(ULONGLONG now, float dt);

    // Rendering
    static void RenderMenu(float screenWidth, float screenHeight);
    static void RenderNotification(float screenWidth, float screenHeight);

    // UI widgets
    static void ToggleButton(const char* label, bool* v);
    static void RenderCustomSwitch(const char* label, bool* value);
    static void RenderModuleCard(const char* name, const char* iconName, bool* enabled, bool* showSettings);
    static bool BeginModuleSettings(const char* label, bool* open);
    static void EndModuleSettings();
    static bool HoverButton(const char* label, ImVec2 size, ImVec4 normalCol, ImVec4 hoverCol, ImVec4 activeCol);

    // Render helpers
    static void DrawShadow(ImDrawList* draw, ImVec2 pos, ImVec2 size, float rounding, float thickness, float opacity);
    static void AddTextGlow(ImDrawList* draw, ImFont* font, float fontSize, ImVec2 pos, ImU32 col, const char* text, float thickness = 3.0f);

    // Profiles
    static void LoadProfiles();
    static void SaveCurrentProfile();
    static void SwitchProfile(int index);
    static void AddProfile(const char* name);
    static void DeleteProfile(int index);
    static void RenameProfile(int index, const char* newName);
    static std::string GetProfilePath(const std::string& name);

    // Search/filter
    static bool PassesFilter(const char* moduleName, const char* category);

};
