#pragma once

#include <cstdint>
#include <windows.h>

class ImDrawList;
struct ImVec2;

class AutoSprint {
public:
    static bool g_autoSprintEnabled;
    static uintptr_t g_autoSprintAddr;
    static void* g_autoSprintCave;
    static BYTE g_autoSprintBackup[11];
    static ULONGLONG g_autoSprintEnableTime;
    static ULONGLONG g_autoSprintDisableTime;

    static void Initialize(uintptr_t gameBase);

    static void Enable();
    static void Disable();

    static void RenderArrayList(ImDrawList* draw, ImVec2 arrayListStart, float& yPos, ImVec2& arrayListEnd);

    static void RenderMenu();

    static bool IsEnabled() { return g_autoSprintEnabled; }

    static uintptr_t PatternScan(uintptr_t start, size_t size, const BYTE* pattern, size_t patternSize);
};
