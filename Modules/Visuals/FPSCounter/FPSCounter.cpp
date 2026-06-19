#include "FPSCounter.hpp"
#include "../../../Animations/Animations.hpp"
#include "../../../Utils/HudElement.hpp"
#include "../../../ImGui/imgui.h"
#include "../../../GUI/GUI.hpp"
#include <windows.h>
#include <cmath>

// Static member initialization
bool FPSCounter::g_showFpsCounter = true;
float FPSCounter::g_fpsTextScale = 1.0f;
ImVec4 FPSCounter::g_fpsTextColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
float FPSCounter::g_fpsCounterAnim = 0.0f;

ULONGLONG FPSCounter::g_fpsCounterEnableTime = 0;
ULONGLONG FPSCounter::g_fpsCounterDisableTime = 0;
int FPSCounter::g_fpsCounterAlignment = 0;
bool FPSCounter::g_fpsCounterShadow = true;
float FPSCounter::g_fpsCounterShadowOffset = 2.0f;
ImVec4 FPSCounter::g_fpsCounterShadowColor = ImVec4(0.0f, 0.0f, 0.0f, 0.7f);
float FPSCounter::g_fpsX = 0.5f;
float FPSCounter::g_fpsY = 0.05f;
bool FPSCounter::g_fpsFirstRender = false;
HudElement* FPSCounter::g_fpsHud = nullptr;

bool FPSCounter::g_showBackground = false;
float FPSCounter::g_bgOpacity = 0.5f;
bool FPSCounter::g_showShadow = false;
float FPSCounter::g_shadowSpread = 10.0f;
float FPSCounter::g_shadowBlur = 10.0f;
bool FPSCounter::g_showTextShadow = true;
float FPSCounter::g_textShadowOffset = 1.0f;
ImVec4 FPSCounter::g_accentColor = ImVec4(1.0f, 0.4f, 0.8f, 1.0f);

// Forward declarations
extern bool g_showMenu;

void FPSCounter::Initialize(HudElement* hudElement) {
    g_fpsHud = hudElement;
    hudElement->resizable = true;
}

float FPSCounter::GetFPS() {
    ImGuiIO& io = ImGui::GetIO();
    if (io.DeltaTime > 0.0f) {
        return 1.0f / io.DeltaTime;
    }
    return 0.0f;
}

void FPSCounter::UpdateAnimation(ULONGLONG now) {
    if (g_showFpsCounter && g_fpsCounterEnableTime == 0) {
        g_fpsCounterEnableTime = now;
        g_fpsCounterDisableTime = 0;
    }
    if (!g_showFpsCounter && g_fpsCounterDisableTime == 0 && g_fpsCounterEnableTime > 0) {
        g_fpsCounterDisableTime = now;
        g_fpsCounterEnableTime = 0;
    }

    if (g_fpsCounterEnableTime > 0) {
        float enableElapsed = (float)(now - g_fpsCounterEnableTime) / 1000.0f;
        g_fpsCounterAnim = fminf(1.0f, enableElapsed / 0.4f);
    }
    else if (g_fpsCounterDisableTime > 0) {
        float disableElapsed = (float)(now - g_fpsCounterDisableTime) / 1000.0f;
        float disableAnim = fminf(1.0f, disableElapsed / 0.3f);
        g_fpsCounterAnim = 1.0f - disableAnim;
        if (disableAnim >= 1.0f) {
            g_fpsCounterEnableTime = 0;
            g_fpsCounterDisableTime = 0;
            g_fpsCounterAnim = 0.0f;
        }
    }
}

void FPSCounter::RenderArrayList(ImDrawList* draw, ImVec2 arrayListStart, float& yPos, ImVec2& arrayListEnd) {
    if (g_showFpsCounter || (g_fpsCounterDisableTime > 0 && g_fpsCounterAnim > 0.01f)) {
        float fpsAlpha = g_fpsCounterAnim * 255.0f;
        float slideOffset = -60.0f + (Animations::EaseOutExpo(g_fpsCounterAnim) * 60.0f);

        if (fpsAlpha > 1.0f && draw) {
            char fBuf[64];
            sprintf_s(fBuf, sizeof(fBuf), "FPS Counter");
            ImVec2 textSize = ImGui::CalcTextSize(fBuf);
            float xPosF = arrayListStart.x + 300.0f - textSize.x - 10.0f;

            draw->AddText(ImVec2(xPosF + slideOffset - 1, yPos + 1), IM_COL32(0, 0, 0, 220), fBuf);
            draw->AddText(ImVec2(xPosF + slideOffset, yPos), IM_COL32(100, 200, 255, (int)fpsAlpha), fBuf);
            yPos += 18.0f;
            arrayListEnd.y = yPos;
        }
    }
}

void FPSCounter::RenderDisplay(float screenWidth, float screenHeight) {
    if (g_fpsCounterAnim <= 0.01f) return;
    if (!g_fpsHud) return;

    float fps = GetFPS();
    char fpsText[64];
    sprintf_s(fpsText, "FPS: %.0f", fps);

    float fontSize = ImGui::GetFontSize() * g_fpsTextScale * g_fpsHud->scale;
    ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, fpsText);
    float padding = 20.0f * g_fpsTextScale * g_fpsHud->scale;
    g_fpsHud->size = ImVec2(textSize.x + padding, textSize.y + padding);

    if (g_showMenu) {
        g_fpsHud->HandleDrag(true);
        g_fpsHud->ClampToScreen();

        ImDrawList* hudDraw = ImGui::GetForegroundDrawList();
        if (hudDraw) {
            g_fpsHud->RenderHudEditor(hudDraw);
        }
    }

    float easedAnim = Animations::EaseOutExpo(g_fpsCounterAnim);
    ImGui::SetNextWindowPos(g_fpsHud->pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(g_fpsHud->size, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.0f);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

    if (ImGui::Begin("##FPSOverlay", nullptr, flags)) {
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetWindowPos();
        ImVec2 s = ImGui::GetWindowSize();

        if (g_showShadow) {
            GUI::DrawShadow(draw, p, s, g_shadowSpread, g_shadowBlur * easedAnim, 0.4f * easedAnim);
        }

        if (g_showBackground) {
            draw->AddRectFilled(p, ImVec2(p.x + s.x, p.y + s.y), ImColor(15, 15, 25, (int)(g_bgOpacity * 255 * easedAnim)), 10.0f);
        }

        ImVec2 textPos = ImVec2(p.x + (s.x - textSize.x) * 0.5f, p.y + (s.y - textSize.y) * 0.5f);

        ImU32 textCol = ImGui::ColorConvertFloat4ToU32(ImVec4(g_fpsTextColor.x, g_fpsTextColor.y, g_fpsTextColor.z, g_fpsTextColor.w * easedAnim * g_bgOpacity));

        if (g_showTextShadow) {
            ImU32 shadowCol = ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 0.8f * easedAnim * g_bgOpacity));
            draw->AddText(ImGui::GetFont(), fontSize, ImVec2(textPos.x + g_textShadowOffset, textPos.y + g_textShadowOffset), shadowCol, fpsText);
        }

        GUI::AddTextGlow(draw, ImGui::GetFont(), fontSize, textPos, textCol, fpsText, 2.0f);

        ImGui::End();
    }
}

void FPSCounter::RenderMenu() {
    bool prev = g_showFpsCounter;
    GUI::RenderCustomSwitch("FPS Overlay", &g_showFpsCounter);
    if (prev != g_showFpsCounter) {
        if (g_showFpsCounter) {
            g_fpsCounterEnableTime = GetTickCount64();
            g_fpsCounterDisableTime = 0;
        } else {
            g_fpsCounterDisableTime = GetTickCount64();
            g_fpsCounterEnableTime = 0;
        }
    }

    if (GUI::BeginModuleSettings("FPS Overlay", &g_showFpsCounter)) {
        ImGui::SliderFloat("Scale", &g_fpsTextScale, 0.5f, 2.0f, "%.1f");
        ImGui::ColorEdit4("Text Color", (float*)&g_fpsTextColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
        GUI::RenderCustomSwitch("Show Background", &g_showBackground);
        if (g_showBackground) {
            ImGui::SliderFloat("Opacity", &g_bgOpacity, 0.0f, 1.0f, "%.2f");
        }

        GUI::RenderCustomSwitch("Show Shadow", &g_showShadow);
        if (g_showShadow) {
            ImGui::SliderFloat("Shadow Offset", &g_shadowSpread, 0.0f, 20.0f, "%.1f");
        }

        GUI::RenderCustomSwitch("Text Shadow", &g_showTextShadow);
        if (g_showTextShadow) {
            ImGui::SliderFloat("Text Shadow Offset", &g_textShadowOffset, 0.0f, 5.0f, "%.1f");
        }

        ImGui::ColorEdit4("Accent Color", (float*)&g_accentColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
        GUI::EndModuleSettings();
    }
}
