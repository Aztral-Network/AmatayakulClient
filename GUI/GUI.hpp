/*
Under an4rch Development Public Source License 1.0
*/

#pragma once

#include <windows.h>
#include <map>
#include <string>
#include <vector>
#include "../ImGui/imgui.h"

// Forward declarations
struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;

/// @brief GUI class - Handles all UI logic and rendering
class GUI {
public:
    // Menu state
    static bool g_showMenu;
    static float g_menuAnim;

    // Computed ClickGUI window rect (set by RenderMenu each frame, consumed by the
    // DX11 blur-shadow pass in RenderBackdrop).
    static ImVec2 g_menuWinPos;
    static ImVec2 g_menuWinSize;
    
    // Tab state
    static int g_currentTab;
    static int g_previousTab;
    static ULONGLONG g_tabChangeTime;
    static float g_tabAnim;
    static float g_ircShiftAnim;

    // Module filter (Mods tab)
    enum ModFilter { Filter_All, Filter_Visual, Filter_Misc };
    static ModFilter g_currentFilter;
    static char g_searchBuffer[128];
    
    // Animation states for UI elements
    static std::map<std::string, float> g_elementAnims;
    // Cached natural heights for expanding panels (measured on first expand)
    static std::map<std::string, float> g_elementHeights;
    
    // Theme configurations
    enum ThemePreset {
        Theme_AmatayakulRed,
        Theme_AegleClassic,
        Theme_SakuraBlossom,
        Theme_Cyberpunk,
        Theme_EmeraldForest,
        Theme_DeepSea,
        Theme_LegacyPink,
        Theme_Max
    };
    static int g_currentTheme;
    static ImVec4 g_colorBgMain;
    static ImVec4 g_colorBgPanel;
    static ImVec4 g_colorAccent;
    static ImVec4 g_colorAccentSoft;
    static ImVec4 g_colorAccentGlow;
    // Gradient colors per theme
    static ImVec4 g_gradientColor1; // top-left
    static ImVec4 g_gradientColor2; // top-right
    static ImVec4 g_gradientColor3; // bottom-left
    static ImVec4 g_gradientColor4; // bottom-right
    // Themed logo texture
    static ImTextureID g_logoTexture;
    static int g_logoWidth;
    static int g_logoHeight;
    
    // Fonts
    static ImFont* g_fontDefault;
    static ImFont* g_fontH1;
    static ImFont* g_fontH2;
    static ImFont* g_fontH3;
    static ImFont* g_fontMono;

    struct LoadedFont {
        std::string name;
        std::string filePath;
        ImFont* fontPtr;
    };
    static std::vector<LoadedFont> g_loadedFonts;
    static void RenderFontSelect(const char* label, std::string& currentFontName);
    static ImFont* GetFontByName(const std::string& fontName);


    // Tab textures
    static void* g_tabTextures[6];
    static void* g_likeTexture;
    static void* g_downloadTexture;
    static bool InitializeTextures();
    static void ShutdownTextures();
    static void* LoadTextureFromResource(int resourceId, int* outWidth = nullptr, int* outHeight = nullptr);

    // Module card icons (LEGACY-style)
    static std::map<std::string, ImTextureID> g_moduleIcons;
    static std::map<std::string, std::pair<int,int>> g_moduleIconSizes;
    static void LoadModuleIcons();

    // Profiles tab
    static void RenderProfiles();

    // Config Market
    struct MarketConfig {
        int id;
        std::string title;
        std::string description;
        std::string author;
        int likes;
        int downloads;
        std::string downloadUrl;
        int status; // 0 = idle, 1 = downloading, 2 = downloaded, 3 = error
    };
    static std::vector<MarketConfig> g_marketConfigs;
    static bool g_fetchingMarket;
    static bool g_marketFetchDone;
    static bool g_marketFetchFailed;
    static void FetchMarketConfigs();
    static void DownloadConfig(int index);
    static void RenderConfigMarket();
    
    // Sidebar active indicator tracking
    static float g_sidebarIndicatorY;
    static float g_sidebarTargetIndicatorY;
    
    // Particle plexus background system
    struct Particle {
        ImVec2 pos;
        ImVec2 vel;
        float size;
        float alpha;
        float speedScale;
    };
    static std::vector<Particle> g_particles;
    static void InitializeParticles();
    static void RenderParticles(ImDrawList* draw, ImVec2 pos, ImVec2 size, float alpha);
    static void RenderAnimatedGradient(ImDrawList* draw, ImVec2 pos, ImVec2 size, float alpha);
    static const char* GetThemeLogoName();
    
    // Style and theme
    static void ApplyTheme();
    static void ApplyThemePreset(int presetId);
    static void LoadFont();
    
    // Menu animation update
    static void UpdateAnimation(ULONGLONG now, float dt);

    // Menu lifecycle (INS toggle + state sync when closed elsewhere)
    static void HandleMenuToggle();
    static void SyncMenuState();

    // True only while the menu is actually rendered on screen. HUDs gate their
    // drag + collision box on this so they can never stay draggable after the
    // menu closes, regardless of state desyncs.
    static bool IsHudEditable();

    // Background blur layer (full-screen Mica or region-scoped ArrayList frost)
    static void RenderBackdrop(ID3D11Device* pDevice, ID3D11DeviceContext* pContext,
                               IDXGISwapChain* pSwapChain, float screenWidth, float screenHeight);
    
    // Menu rendering
    static void RenderMenu(float screenWidth, float screenHeight);
    
    // UI Helpers
    static bool RenderSidebarButton(const char* label, int index);
    static void RenderCustomSwitch(const char* label, bool* value);
    static void RenderSectionHeader(const char* label);
    static bool BeginSection(const char* label, bool* open);
    static void EndSection();
    
    // Module card (LEGACY-style: icon + name + OPTIONS + toggle)
    static std::string g_currentSettingsModule;
    static int g_cardStaggerIndex; // for staggered entrance animation
    static void RenderModuleCard(const char* name, const char* iconName, bool* enabled, bool* showSettings = nullptr, void (*onToggle)(bool newEnabled) = nullptr);
    
    // Cascading Modules
    static bool BeginModuleSettings(const char* label, bool* open);
    static void EndModuleSettings();
    
    // Notification
    static void RenderNotification(float screenWidth, float screenHeight);
    
    // Low-level Render Helpers
    static void DrawShadow(ImDrawList* draw, ImVec2 pos, ImVec2 size, float rounding, float thickness, float opacity);
    static void AddTextGlow(ImDrawList* draw, ImFont* font, float fontSize, ImVec2 pos, ImU32 col, const char* text, float thickness = 3.0f);

    // Custom widgets (replace default ImGui look)
    static bool RenderSlider(const char* label, float* value, float min, float max, const char* format = "%.2f");
    static bool RenderSliderInt(const char* label, int* value, int min, int max, const char* format = "%d");
    static bool RenderKeybind(const char* label, int* key);
    static bool RenderCombo(const char* label, int* current_item, const char* const* items, int items_count);
    static bool RenderCheckbox(const char* label, bool* value);
    static bool RenderButton(const char* label, const ImVec2& size = ImVec2(0, 0));
};
