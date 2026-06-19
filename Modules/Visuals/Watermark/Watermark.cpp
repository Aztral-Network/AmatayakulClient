#include "Watermark.hpp"
#include "../../../Animations/Animations.hpp"
#include "../../../Utils/HudElement.hpp"
#include "../../../ImGui/imgui.h"
#include "../../../GUI/GUI.hpp"
#include <windows.h>
#include <cmath>
#include <cstdio>

// Static member initialization
bool Watermark::g_showWatermark = true;
ULONGLONG Watermark::g_watermarkEnableTime = 0;
ULONGLONG Watermark::g_watermarkDisableTime = 0;
float Watermark::g_watermarkAnim = 0.0f;
HudElement* Watermark::g_watermarkHud = nullptr;

// Watermark settings
bool Watermark::g_fancyMode = true;
bool Watermark::g_chromaEnabled = true;
ImVec4 Watermark::g_textColor = ImVec4(0.68f, 0.12f, 0.18f, 1.0f);
float Watermark::g_logoScale = 1.0f;

// Local helper function for chroma color cycling
static ImVec4 GetChromaColor(float time) {
    ImVec4 colors[3] = {
        ImVec4(0x87/255.f, 0xF8/255.f, 0xFF/255.f, 1.0f),
        ImVec4(0x87/255.f, 0x97/255.f, 0xFF/255.f, 1.0f),
        ImVec4(0xE9/255.f, 0x87/255.f, 0xFF/255.f, 1.0f)
    };
    
    float cycleTime = fmodf(time * 2.0f, 3.0f);
    int colorIdx = (int)cycleTime;
    float blend = cycleTime - colorIdx;
    
    int nextIdx = (colorIdx + 1) % 3;
    ImVec4& c1 = colors[colorIdx];
    ImVec4& c2 = colors[nextIdx];
    
    return ImVec4(
        c1.x + (c2.x - c1.x) * blend,
        c1.y + (c2.y - c1.y) * blend,
        c1.z + (c2.z - c1.z) * blend,
        1.0f
    );
}

void Watermark::Initialize(HudElement* hud) {
    g_watermarkHud = hud;
    hud->resizable = true;
}

void Watermark::UpdateAnimation(ULONGLONG now) {
    if (g_showWatermark && g_watermarkEnableTime == 0) {
        g_watermarkEnableTime = now;
        g_watermarkDisableTime = 0;
    }
    if (!g_showWatermark && g_watermarkDisableTime == 0 && g_watermarkEnableTime > 0) {
        g_watermarkDisableTime = now;
        g_watermarkEnableTime = 0;
    }
    
    if (g_watermarkEnableTime > 0) {
        float enableElapsed = (float)(now - g_watermarkEnableTime) / 1000.0f;
        g_watermarkAnim = fminf(1.0f, enableElapsed / 0.4f);
    }
    else if (g_watermarkDisableTime > 0) {
        float disableElapsed = (float)(now - g_watermarkDisableTime) / 1000.0f;
        float disableAnim = fminf(1.0f, disableElapsed / 0.3f);
        g_watermarkAnim = 1.0f - disableAnim;
        if (disableAnim >= 1.0f) {
            g_watermarkEnableTime = 0;
            g_watermarkDisableTime = 0;
        }
    }
}

void Watermark::RenderArrayList(ImDrawList* draw, ImVec2 arrayListStart, float& yPos, ImVec2& arrayListEnd) {
    if (g_showWatermark || (g_watermarkDisableTime > 0 && g_watermarkAnim > 0.01f)) {
        float watermarkAlpha = g_watermarkAnim * 255.0f;
        float slideOffset = -60.0f + (Animations::SmoothInertia(g_watermarkAnim) * 60.0f);
        
        if (watermarkAlpha > 1.0f) {
            char wBuf[64];
            sprintf_s(wBuf, "Watermark");
            float wW = ImGui::CalcTextSize(wBuf).x;
            float xPosW = arrayListStart.x + 290.0f - wW - 10;
            draw->AddText(ImVec2(xPosW + slideOffset - 1, yPos + 1), IM_COL32(0, 0, 0, 220), wBuf);
            draw->AddText(ImVec2(xPosW + slideOffset, yPos), IM_COL32(100, 255, 200, (int)watermarkAlpha), wBuf);
            yPos += 18.0f;
            arrayListEnd.y = yPos;
        }
    }
}

void Watermark::RenderDisplay() {
    if (g_showWatermark || g_watermarkAnim > 0.01f) {
        if (!g_watermarkHud) return;
        
        extern bool g_showMenu;
        g_watermarkHud->HandleDrag(g_showMenu);
        g_watermarkHud->ClampToScreen();

        float easedWatermarkAnim = Animations::EaseOutExpo(g_watermarkAnim);
        float watermarkAlpha = easedWatermarkAnim;
        
        ImDrawList* watermarkDraw = ImGui::GetForegroundDrawList();
        ImVec2 textPos = g_watermarkHud->pos;

        if (g_fancyMode) {
            // ========== FANCY MODE: Render logo image (theme-aware) ==========
            const char* logoKey = "logo";
            switch (GUI::g_currentTheme) {
                case GUI::Theme_SakuraBlossom: logoKey = "logo_pink"; break;
                case GUI::Theme_Cyberpunk:     logoKey = "logo_cyan"; break;
                case GUI::Theme_EmeraldForest: logoKey = "logo_green"; break;
                case GUI::Theme_DeepSea:       logoKey = "logo_blue"; break;
                default: break;
            }
            ImTextureID logoTex = GUI::g_icons.count(logoKey) ? GUI::g_icons[logoKey] : (ImTextureID)0;
            if (logoTex) {
                float logoW = 200.0f * g_logoScale * g_watermarkHud->scale;
                float logoH = logoW * 301.0f / 1127.0f;
                
                ImVec4 tintColor = ImVec4(1.0f, 1.0f, 1.0f, watermarkAlpha);
                watermarkDraw->AddImage(logoTex, textPos, 
                    ImVec2(textPos.x + logoW, textPos.y + logoH),
                    ImVec2(0, 0), ImVec2(1, 1),
                    ImGui::GetColorU32(tintColor));
                
                g_watermarkHud->size = ImVec2(logoW, logoH);
            } else {
                // Fallback to text if logo failed to load
                ImFont* font = ImGui::GetFont();
                float fontSize = 32.0f * g_watermarkHud->scale;
                watermarkDraw->AddText(font, fontSize, textPos, IM_COL32(180, 30, 45, (int)(watermarkAlpha * 255)), "Amatayakul");
                ImVec2 ts = ImGui::CalcTextSize("Amatayakul");
                float fontScale = fontSize / 16.0f;
                g_watermarkHud->size = ImVec2(ts.x * fontScale + 20, ts.y * fontScale + 10);
            }
        } else {
            // ========== LEGACY MODE: Render text ==========
            ImFont* font = ImGui::GetFont();
            float fontSize = 32.0f * g_watermarkHud->scale;
            
            ImVec4 renderColor;
            if (g_chromaEnabled) {
                renderColor = GetChromaColor((float)GetTickCount64() / 1000.0f);
                renderColor.w = watermarkAlpha;
            } else {
                renderColor = g_textColor;
                renderColor.w = g_textColor.w * watermarkAlpha;
            }
            
            // Glow blur (multiple layers with decreasing alpha)
            ImVec4 glowColor = renderColor;
            for (int i = 3; i >= 1; --i) {
                glowColor.w = (renderColor.w * 0.4f) / i;
                watermarkDraw->AddText(font, fontSize, ImVec2(textPos.x + i, textPos.y), ImGui::GetColorU32(glowColor), "Amatayakul");
                watermarkDraw->AddText(font, fontSize, ImVec2(textPos.x - i, textPos.y), ImGui::GetColorU32(glowColor), "Amatayakul");
                watermarkDraw->AddText(font, fontSize, ImVec2(textPos.x, textPos.y + i), ImGui::GetColorU32(glowColor), "Amatayakul");
                watermarkDraw->AddText(font, fontSize, ImVec2(textPos.x, textPos.y - i), ImGui::GetColorU32(glowColor), "Amatayakul");
            }
            
            // Main text
            watermarkDraw->AddText(font, fontSize, textPos, ImGui::GetColorU32(renderColor), "Amatayakul");

            // Update hitbox
            float fontScale = fontSize / 16.0f;
            ImVec2 textSize = ImGui::CalcTextSize("Amatayakul");
            g_watermarkHud->size = ImVec2(textSize.x * fontScale + 20, textSize.y * fontScale + 10);
        }

        // HUD editor overlay when menu is open
        if (g_showMenu) {
            g_watermarkHud->HandleDrag(true);
            g_watermarkHud->ClampToScreen();
            g_watermarkHud->RenderHudEditor(watermarkDraw);
        }
    }
}

void Watermark::RenderMenu() {
    GUI::RenderCustomSwitch("Watermark", &g_showWatermark);

    if (GUI::BeginModuleSettings("Watermark", &g_showWatermark)) {
        const char* modes[] = { "Text", "Image" };
        int currentMode = g_fancyMode ? 1 : 0;
        if (ImGui::Combo("Display Mode", &currentMode, modes, IM_ARRAYSIZE(modes))) {
            g_fancyMode = (currentMode == 1);
        }

        if (g_fancyMode) {
            ImGui::Separator();
            ImGui::Text("Image Settings");
            ImGui::SliderFloat("Logo Scale", &g_logoScale, 0.3f, 3.0f, "%.2f");
        } else {
            ImGui::Separator();
            ImGui::Text("Text Settings");
            GUI::RenderCustomSwitch("Chroma##WM", &g_chromaEnabled);
            if (!g_chromaEnabled) {
                ImGui::ColorEdit4("Text Color##WM", (float*)&g_textColor, ImGuiColorEditFlags_NoInputs);
                ImGui::SliderFloat("Text Opacity##WM", &g_textColor.w, 0.0f, 1.0f, "%.2f");
            }
        }

        GUI::EndModuleSettings();
    }
}
