/*
Under an4rch Development Public Source License 1.0
*/

#pragma once

#include <cstdint>
#include <windows.h>
#include "../../ImGui/imgui.h"

struct ImDrawList;
struct ImVec2;
struct HudElement;

// AutoSprint Module — Toggle Sprint with sprinting text indicator
class AutoSprint {
public:
    static bool g_autoSprintEnabled;
    static uintptr_t g_autoSprintAddr;
    static void* g_autoSprintCave;
    static BYTE g_autoSprintBackup[11];
    static ULONGLONG g_autoSprintEnableTime;
    static ULONGLONG g_autoSprintDisableTime;

    // Sprint text HUD
    static bool g_showSprintText;
    static HudElement* g_sprintTextHud;
    static float g_sprintTextScale;
    static float g_sprintTextAlpha;
    static int g_sprintTextMode;      // 0 = Clean (pill), 1 = Raw (plain text)
    static bool g_sprintTextShadow;   // shadow for raw mode
    static ImVec4 g_sprintTextColor;

    // Initialize autosprint module
    static void Initialize(uintptr_t gameBase);

    // Initialize the sprint text HUD element
    static void InitializeHud(HudElement* hud);

    // Scan the game module for the autosprint patch target
    static void ScanPattern(uintptr_t gameBase, size_t imageSize);

    // Enable/Disable autosprint
    static void Enable();
    static void Disable();

    // Render autosprint in array list
    static void RenderArrayList(ImDrawList* draw, ImVec2 arrayListStart, float& yPos, ImVec2& arrayListEnd);

    // Render the sprint text HUD (movable when menu open)
    static void RenderSprintText();

    // Render autosprint UI in menu
    static void RenderMenu();

    // Check if autosprint is enabled
    static bool IsEnabled() { return g_autoSprintEnabled; }

    // Pattern scanning utility
    static uintptr_t PatternScan(uintptr_t start, size_t size, const BYTE* pattern, size_t patternSize);

private:
    // Double-tap W detection state
    static ULONGLONG g_lastWTapTime;
    static bool g_lastWState;
    static bool g_vanillaSprintActive;
};
