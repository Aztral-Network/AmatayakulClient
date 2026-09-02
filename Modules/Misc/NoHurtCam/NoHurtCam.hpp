/*
Under an4rch Development Public Source License 1.0
*/

#pragma once

#include <cstdint>
#include <windows.h>

struct ImDrawList;
struct ImVec2;

/// @brief NoHurtCam module - Removes the hurt-camera shake effect when the player takes damage.
///
/// Patches Minecraft.Win10.DX11.exe+0x4D9618 so the value written to [rcx+0x224]
/// is 0 instead of 0x0A, disabling the hurt camera animation.
class NoHurtCam {
public:
    static bool     g_enabled;
    static uintptr_t g_patchAddr;
    static BYTE      g_originalBytes[14];
    static bool      g_bytesValidated;
    static ULONGLONG g_enableTime;
    static ULONGLONG g_disableTime;

    // Validate the target AOB at the offset and save original bytes
    static void ScanPattern(uintptr_t gameBase, size_t imageSize);

    // Enable/Disable NoHurtCam
    static void Enable();
    static void Disable();

    // Render NoHurtCam in array list
    static void RenderArrayList(ImDrawList* draw, ImVec2 arrayListStart, float& yPos, ImVec2& arrayListEnd);

    // Render NoHurtCam UI in menu
    static void RenderMenu();

    // Check if enabled
    static bool IsEnabled() { return g_enabled; }
};
