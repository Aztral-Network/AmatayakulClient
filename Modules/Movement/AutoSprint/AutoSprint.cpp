/*
Under an4rch Development Public Source License 1.0
*/

#include "AutoSprint.hpp"
#include "../../Alloc/AllocateNear.hpp"
#include "../../../Animations/Animations.hpp"
#include "../../../ImGui/imgui.h"
#include "../../../GUI/GUI.hpp"
#include "../../../Input/Input.hpp"
#include "../../Globals.hpp"
#include <windows.h>
#include <cstring>
#include <cstdio>
#include <cmath>

// Static member initialization
bool AutoSprint::g_autoSprintEnabled = false;
uintptr_t AutoSprint::g_autoSprintAddr = 0;
void* AutoSprint::g_autoSprintCave = nullptr;
BYTE AutoSprint::g_autoSprintBackup[11] = { 0 };
ULONGLONG AutoSprint::g_autoSprintEnableTime = 0;
ULONGLONG AutoSprint::g_autoSprintDisableTime = 0;

// Sprint text HUD
bool AutoSprint::g_showSprintText = false;
HudElement* AutoSprint::g_sprintTextHud = nullptr;
float AutoSprint::g_sprintTextScale = 1.0f;
float AutoSprint::g_sprintTextAlpha = 0.0f;
int AutoSprint::g_sprintTextMode = 0;
bool AutoSprint::g_sprintTextShadow = true;
ImVec4 AutoSprint::g_sprintTextColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

// Double-tap W detection
ULONGLONG AutoSprint::g_lastWTapTime = 0;
bool AutoSprint::g_lastWState = false;
bool AutoSprint::g_vanillaSprintActive = false;

// Pattern scanner - finds byte patterns in memory
uintptr_t AutoSprint::PatternScan(uintptr_t start, size_t size, const BYTE* pattern, size_t patternSize) {
    for (size_t i = 0; i <= size - patternSize; ++i) {
        if (memcmp((void*)(start + i), pattern, patternSize) == 0) {
            return start + i;
        }
    }
    return 0;
}

void AutoSprint::ScanPattern(uintptr_t gameBase, size_t imageSize) {
    if (g_autoSprintAddr) return;
    BYTE pattern[] = {0x0F, 0xB6, 0x41, 0x63, 0x48, 0x8D, 0x2D, 0x39, 0xE0, 0xC3, 0x00};
    g_autoSprintAddr = PatternScan(gameBase, imageSize, pattern, sizeof(pattern));
}

void AutoSprint::Initialize(uintptr_t gameBase) {
    (void)gameBase;
}

void AutoSprint::InitializeHud(HudElement* hud) {
    g_sprintTextHud = hud;
}

void AutoSprint::Disable() {
    if (!g_autoSprintAddr || g_autoSprintBackup[0] == 0) return;
    g_autoSprintDisableTime = GetTickCount64();
    DWORD old;
    VirtualProtect((void*)g_autoSprintAddr, 11, PAGE_EXECUTE_READWRITE, &old);
    memcpy((void*)g_autoSprintAddr, g_autoSprintBackup, 11);
    VirtualProtect((void*)g_autoSprintAddr, 11, old, &old);
}

void AutoSprint::Enable() {
    if (!g_autoSprintAddr) return;
    g_autoSprintEnableTime = GetTickCount64();
    if (!g_autoSprintCave) g_autoSprintCave = AllocateNear::Allocate(g_autoSprintAddr, 1024);
    if (!g_autoSprintCave) return;

    memcpy(g_autoSprintBackup, (void*)g_autoSprintAddr, 11);

    BYTE shellcode[64] = { 0 };
    int p = 0;
    // mov eax, 6
    shellcode[p++] = 0xB8; int sprintValue = 6; memcpy(&shellcode[p], &sprintValue, 4); p += 4;
    // jmp back
    shellcode[p++] = 0xE9;
    uintptr_t retAddr = g_autoSprintAddr + 11;
    uintptr_t cur = (uintptr_t)g_autoSprintCave + p;
    int32_t rel = (int32_t)(retAddr - (cur + 4));
    memcpy(&shellcode[p], &rel, 4); p += 4;

    memcpy(g_autoSprintCave, shellcode, p);

    DWORD old;
    VirtualProtect((void*)g_autoSprintAddr, 11, PAGE_EXECUTE_READWRITE, &old);
    BYTE patch[11] = { 0xE9, 0, 0, 0, 0, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
    int32_t jmpToCave = (int32_t)((uintptr_t)g_autoSprintCave - (g_autoSprintAddr + 5));
    memcpy(&patch[1], &jmpToCave, 4);
    memcpy((void*)g_autoSprintAddr, patch, 11);
    VirtualProtect((void*)g_autoSprintAddr, 11, old, &old);
}

void AutoSprint::RenderArrayList(ImDrawList* draw, ImVec2 arrayListStart, float& yPos, ImVec2& arrayListEnd) {
    const float FADE_OUT_TIME = 0.15f;
    const float FADE_IN_TIME = 0.12f;
    const float SLIDE_TIME = 0.25f;

    // AutoSprint module
    if (g_autoSprintEnabled || g_autoSprintDisableTime > 0) {
        float timeSinceEnable = (float)(GetTickCount64() - g_autoSprintEnableTime) / 1000.0f;
        float timeSinceDisable = (float)(GetTickCount64() - g_autoSprintDisableTime) / 1000.0f;

        float autoSprintAlpha = 255.0f;
        float slideOffset = 0.0f;

        if (g_autoSprintEnabled) {
            autoSprintAlpha = Animations::SmoothInertia(fminf(1.0f, timeSinceEnable / FADE_IN_TIME)) * 255.0f;
            float slideProgress = fminf(1.0f, timeSinceEnable / SLIDE_TIME);
            slideOffset = Animations::SmoothInertia(slideProgress) * 60.0f - 60.0f;
        } else if (timeSinceDisable < FADE_OUT_TIME) {
            autoSprintAlpha = Animations::SmoothInertia(1.0f - (timeSinceDisable / FADE_OUT_TIME)) * 255.0f;
        } else {
            g_autoSprintDisableTime = 0;
        }

        if (autoSprintAlpha > 1.0f) {
            char aBuf[64];
            sprintf_s(aBuf, "Toggle Sprint");
            float wA = ImGui::CalcTextSize(aBuf).x;
            float xPosA = arrayListStart.x + 290.0f - wA - 10;
            draw->AddText(ImVec2(xPosA + slideOffset - 1, yPos + 1), IM_COL32(0, 0, 0, 220), aBuf); // Shadow
            draw->AddText(ImVec2(xPosA + slideOffset, yPos), IM_COL32(60, 255, 60, (int)autoSprintAlpha), aBuf);
            yPos += 18.0f;
            arrayListEnd.y = yPos;
        }
    }
}

// ── Sprint Text HUD ────────────────────────────────────────────────────────
// Detects what type of sprinting is happening and renders the text.

void AutoSprint::RenderSprintText() {
    if (!g_showSprintText || !g_sprintTextHud) return;

    float dt = ImGui::GetIO().DeltaTime;

    // Detect key states (W = 0x57, VK_CONTROL = 0x11)
    bool wHeld = Input::g_keys[0x57];
    bool ctrlHeld = Input::g_keys[VK_CONTROL];

    // ── Double-tap W detection for vanilla sprint ──
    if (wHeld && !g_lastWState) {
        ULONGLONG now = GetTickCount64();
        if (now - g_lastWTapTime < 300) {
            g_vanillaSprintActive = true;
        }
        g_lastWTapTime = now;
    }
    g_lastWState = wHeld;

    // Vanilla sprint stops when W is released
    if (!wHeld) {
        g_vanillaSprintActive = false;
    }

    // ── Determine sprint mode text ──
    const char* sprintText = nullptr;

    if (wHeld) {
        if (g_autoSprintEnabled) {
            // Toggle sprint is ON — memory patch handles it, just show toggled mode
            sprintText = "Sprinting: Toggled";
        } else if (ctrlHeld) {
            // W + CTRL = key-held sprinting
            sprintText = "Sprinting: Key Held";
        } else if (g_vanillaSprintActive) {
            // Double-tap W = vanilla sprinting
            sprintText = "Sprinting: Vanilla";
        }
    }

    // ── Animate alpha (fade in when sprinting, fade out when not) ──
    float targetAlpha = sprintText ? 1.0f : 0.0f;
    float approachSpeed = sprintText ? 10.0f : 6.0f;
    g_sprintTextAlpha = Animations::Approach(g_sprintTextAlpha, targetAlpha, dt, approachSpeed);

    if (g_sprintTextAlpha < 0.01f) return;

    // ── Render the HUD ──
    float alpha = g_sprintTextAlpha;
    const char* displayText = sprintText ? sprintText : "Sprinting: Toggled";

    ImDrawList* draw = ImGui::GetForegroundDrawList();
    if (!draw) return;

    const float baseFontSize = 16.0f;
    float fontSize = baseFontSize * g_sprintTextHud->scale * g_sprintTextScale;
    ImVec2 textSize = ImGui::CalcTextSize(displayText);
    textSize.x *= g_sprintTextHud->scale * g_sprintTextScale;
    textSize.y *= g_sprintTextHud->scale * g_sprintTextScale;

    if (g_sprintTextMode == 0) {
        // ── Clean mode: accent pill ──
        float padX = 12.0f * g_sprintTextHud->scale;
        float padY = 6.0f * g_sprintTextHud->scale;
        float boxW = textSize.x + padX * 2.0f;
        float boxH = textSize.y + padY * 2.0f;
        g_sprintTextHud->size = ImVec2(boxW, boxH);

        if (g_sprintTextHud->pos.x == 0 && g_sprintTextHud->pos.y == 0) {
            g_sprintTextHud->pos = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f - boxW * 0.5f, ImGui::GetIO().DisplaySize.y * 0.75f);
        }
        g_sprintTextHud->HandleDrag(GUI::IsHudEditable());
        g_sprintTextHud->ClampToScreen();

        ImVec2 pos = g_sprintTextHud->pos;
        const ImVec4& accent = GUI::g_colorAccent;

        draw->AddRectFilled(ImVec2(pos.x, pos.y), ImVec2(pos.x + boxW, pos.y + boxH),
            IM_COL32(20, 20, 26, (int)(200 * alpha)), boxH * 0.5f);
        draw->AddRect(ImVec2(pos.x, pos.y), ImVec2(pos.x + boxW, pos.y + boxH),
            ImColor(accent.x, accent.y, accent.z, 0.5f * alpha), boxH * 0.5f, 0, 1.5f);

        float textX = pos.x + (boxW - textSize.x) * 0.5f;
        float textY = pos.y + (boxH - textSize.y) * 0.5f;
        draw->AddText(nullptr, fontSize, ImVec2(textX + 1, textY + 1), IM_COL32(0, 0, 0, (int)(180 * alpha)), displayText);
        draw->AddText(nullptr, fontSize, ImVec2(textX, textY),
            ImColor(accent.x, accent.y, accent.z, alpha), displayText);

        if (GUI::IsHudEditable()) {
            g_sprintTextHud->RenderHudEditor(draw);
        }
    } else {
        // ── Raw mode: plain text like CPS/FPS counter ──
        float rawPad = 4.0f * g_sprintTextHud->scale;
        g_sprintTextHud->size = ImVec2(
            (textSize.x + rawPad) * g_sprintTextHud->scale,
            (fontSize + rawPad) * g_sprintTextHud->scale);

        if (g_sprintTextHud->pos.x == 0 && g_sprintTextHud->pos.y == 0) {
            g_sprintTextHud->pos = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f - g_sprintTextHud->size.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.75f);
        }
        g_sprintTextHud->HandleDrag(GUI::IsHudEditable());
        g_sprintTextHud->ClampToScreen();

        ImVec2 pos = g_sprintTextHud->pos;

        // Shadow
        if (g_sprintTextShadow) {
            draw->AddText(nullptr, fontSize,
                ImVec2(pos.x + 1.5f, pos.y + 1.5f),
                IM_COL32(0, 0, 0, (int)(160 * alpha)), displayText);
        }

        // Main text (uses custom color)
        ImVec4 col = g_sprintTextColor;
        col.w *= alpha;
        draw->AddText(nullptr, fontSize, pos,
            ImColor(col.x, col.y, col.z, col.w), displayText);

        if (GUI::IsHudEditable()) {
            g_sprintTextHud->RenderHudEditor(draw);
        }
    }
}

void AutoSprint::RenderMenu() {
    bool prev = g_autoSprintEnabled;
    GUI::RenderCustomSwitch("Toggle Sprint", &g_autoSprintEnabled);
    if (prev != g_autoSprintEnabled) {
        if (g_autoSprintEnabled) Enable(); else Disable();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    GUI::RenderCustomSwitch("Show Sprinting Text", &g_showSprintText);
    if (g_showSprintText) {
        const char* modes[] = { "Clean", "Raw" };
        GUI::RenderCombo("Display Mode##Sprint", &g_sprintTextMode, modes, IM_ARRAYSIZE(modes));
        GUI::RenderSlider("Text Scale##Sprint", &g_sprintTextScale, 0.5f, 3.0f, "%.2fx");
        if (g_sprintTextMode == 1) {
            GUI::RenderCustomSwitch("Shadow##SprintRaw", &g_sprintTextShadow);
            ImGui::ColorEdit4("Text Color##SprintRaw", &g_sprintTextColor.x);
        }
    }
}
