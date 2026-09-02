/*
Under an4rch Development Public Source License 1.0
*/

#include "NoHurtCam.hpp"
#include "../../../Animations/Animations.hpp"
#include "../../../ImGui/imgui.h"
#include "../../../GUI/GUI.hpp"
#include "../../Globals.hpp"
#include <windows.h>
#include <cstring>
#include <cstdio>
#include <cmath>

// Static member initialization
bool      NoHurtCam::g_enabled       = false;
uintptr_t NoHurtCam::g_patchAddr     = 0;
BYTE      NoHurtCam::g_originalBytes[14] = { 0 };
bool      NoHurtCam::g_bytesValidated = false;
ULONGLONG NoHurtCam::g_enableTime    = 0;
ULONGLONG NoHurtCam::g_disableTime   = 0;

// Expected original AOB at Minecraft.Win10.DX11.exe + 0x4D9618
//   66 0F 6E BB 24 02 00 00 48 8B 03 0F 5B FF
// The first 8 bytes (movd xmm7, dword ptr [rbx+0x224]) are patched
// with xorps xmm7,xmm7 + 5x NOP to zero the hurt-camera timer.
static const BYTE kExpectedOriginal[14] = {
    0x66, 0x0F, 0x6E, 0xBB, 0x24, 0x02, 0x00, 0x00,
    0x48, 0x8B, 0x03, 0x0F, 0x5B, 0xFF
};

void NoHurtCam::ScanPattern(uintptr_t gameBase, size_t imageSize) {
    if (g_patchAddr) return;

    // Fixed offset from the game module base
    uintptr_t target = gameBase + 0x4D9618;

    // Bounds check: ensure the patch region is within the module
    if (target + sizeof(kExpectedOriginal) > gameBase + imageSize) {
        return;
    }

    // Validate that the original bytes match what we expect
    if (memcmp((void*)target, kExpectedOriginal, sizeof(kExpectedOriginal)) != 0) {
        return; // Bytes don't match — wrong game version or already patched
    }

    // Save original bytes and store the validated address
    memcpy(g_originalBytes, kExpectedOriginal, sizeof(kExpectedOriginal));
    g_patchAddr = target;
    g_bytesValidated = true;
}

void NoHurtCam::Enable() {
    if (!g_patchAddr || !g_bytesValidated) return;

    g_enableTime = GetTickCount64();

    DWORD old;
    VirtualProtect((void*)g_patchAddr, sizeof(kExpectedOriginal), PAGE_EXECUTE_READWRITE, &old);

    // Patch first 8 bytes: xorps xmm7,xmm7 (0F 57 FF) + 5x NOP (90)
    // This zeros the xmm7 register before the dword-to-float conversion,
    // so the hurt-camera timer value is always 0.
    BYTE patched[14];
    memcpy(patched, g_originalBytes, sizeof(patched));
    patched[0] = 0x0F; patched[1] = 0x57; patched[2] = 0xFF;  // xorps xmm7, xmm7
    patched[3] = 0x90; patched[4] = 0x90; patched[5] = 0x90;  // nop
    patched[6] = 0x90; patched[7] = 0x90;                      // nop
    memcpy((void*)g_patchAddr, patched, sizeof(patched));

    VirtualProtect((void*)g_patchAddr, sizeof(kExpectedOriginal), old, &old);
}

void NoHurtCam::Disable() {
    if (!g_patchAddr || !g_bytesValidated) return;

    g_disableTime = GetTickCount64();

    DWORD old;
    VirtualProtect((void*)g_patchAddr, sizeof(kExpectedOriginal), PAGE_EXECUTE_READWRITE, &old);
    memcpy((void*)g_patchAddr, g_originalBytes, sizeof(kExpectedOriginal));
    VirtualProtect((void*)g_patchAddr, sizeof(kExpectedOriginal), old, &old);
}

void NoHurtCam::RenderArrayList(ImDrawList* draw, ImVec2 arrayListStart, float& yPos, ImVec2& arrayListEnd) {
    if (!g_enabled && g_disableTime == 0) return;

    ULONGLONG now = GetTickCount64();
    float timeSinceEnable  = (float)(now - g_enableTime)  / 1000.0f;
    float timeSinceDisable = (float)(now - g_disableTime) / 1000.0f;
    const float FADE = 0.3f;

    float alpha = 255.0f;
    float slide = 0.0f;

    if (g_enabled) {
        alpha = Animations::SmoothInertia(fminf(1.0f, timeSinceEnable / FADE)) * 255.0f;
        float slideProgress = fminf(1.0f, timeSinceEnable / 0.4f);
        slide = Animations::SmoothInertia(slideProgress) * 60.0f - 60.0f;
    } else if (timeSinceDisable < FADE) {
        alpha = Animations::SmoothInertia(1.0f - timeSinceDisable / FADE) * 255.0f;
    } else {
        g_disableTime = 0;
        return;
    }

    if (alpha > 1.0f && draw) {
        const char* label = "NoHurtCam";
        ImVec2 textSize = ImGui::CalcTextSize(label);
        float x = arrayListStart.x + 300.0f - textSize.x - 10.0f;
        draw->AddText(ImVec2(x + slide - 1, yPos + 1), IM_COL32(0, 0, 0, 200), label);
        draw->AddText(ImVec2(x + slide, yPos), IM_COL32(255, 120, 80, (int)alpha), label);
        yPos += 18.0f;
        arrayListEnd.y = yPos;
    }
}

void NoHurtCam::RenderMenu() {
    bool prev = g_enabled;

    if (!g_bytesValidated) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
            "NoHurtCam could not validate the memory patch target.\n"
            "The game version may be unsupported or the address has changed.");
    }

    GUI::RenderCustomSwitch("NoHurtCam", &g_enabled);
    if (prev != g_enabled) {
        if (g_enabled) {
            Enable();
        } else {
            Disable();
        }
    }
}
