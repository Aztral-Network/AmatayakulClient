#include "CPSCounter.hpp"
#include "../../../Animations/Animations.hpp"
#include "../../../Utils/HudElement.hpp"
#include "../../../ImGui/imgui.h"
#include "../../../GUI/GUI.hpp"
#include <windows.h>
#include <cmath>
#include <algorithm>

// Forward declarations for helper functions
extern bool g_showMenu;
bool CPSCounter::g_showCpsCounter = true;
std::string CPSCounter::g_cpsCounterFormat = "{CPS} CPS";
float CPSCounter::g_cpsTextScale = 1.0f;
ImVec4 CPSCounter::g_cpsTextColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);


float CPSCounter::g_cpsCounterAnim = 0.0f;
ULONGLONG CPSCounter::g_cpsCounterEnableTime = 0;
ULONGLONG CPSCounter::g_cpsCounterDisableTime = 0;

int CPSCounter::g_cpsCounterAlignment = 0;
bool CPSCounter::g_cpsCounterShadow = true;
float CPSCounter::g_cpsCounterShadowOffset = 2.0f;
ImVec4 CPSCounter::g_cpsCounterShadowColor = ImVec4(0.0f, 0.0f, 0.0f, 0.7f);
float CPSCounter::g_cpsCounterX = 0.5f;
float CPSCounter::g_cpsCounterY = 0.95f;
bool CPSCounter::g_cpsCounterFirstRender = false;

HudElement* CPSCounter::g_cpsHud = nullptr;

// CPS Click tracking
ULONGLONG CPSCounter::g_lmbClickTimes[100] = {};
ULONGLONG CPSCounter::g_rmbClickTimes[100] = {};
int CPSCounter::g_lmbClickIndex = 0;
int CPSCounter::g_rmbClickIndex = 0;
int CPSCounter::g_lmbCps = 0;
int CPSCounter::g_rmbCps = 0;
ULONGLONG CPSCounter::g_lastLmbClickTime = 0;
ULONGLONG CPSCounter::g_lastRmbClickTime = 0;
bool g_internalPrevLmb = false;
bool g_internalPrevRmb = false;

// Forward declarations for easing and HudElement (these will be linked from dllmain.cpp)
extern bool g_showMenu;

void CPSCounter::Initialize(HudElement* hudElement) {
    g_cpsHud = hudElement;
    hudElement->resizable = true;
}

void CPSCounter::UpdateCPS(ULONGLONG now, bool lmbPressed, bool rmbPressed, bool prevLmbPressed, bool prevRmbPressed) {
    static ULONGLONG lastReset = 0;
    static int currentLmb = 0;
    static int currentRmb = 0;

    // Reset every 1 second
    if (lastReset == 0) lastReset = now;
    if (now - lastReset >= 1000) {
        lastReset = now;
        currentLmb = 0;
        currentRmb = 0;
    }

    // 1 click = add 1 cps
    if (lmbPressed && !g_internalPrevLmb) {
        currentLmb++;
    }
    g_internalPrevLmb = lmbPressed;
    
    // 1 click = add 1 cps
    if (rmbPressed && !g_internalPrevRmb) {
        currentRmb++;
    }
    g_internalPrevRmb = rmbPressed;

    g_lmbCps = currentLmb;
    g_rmbCps = currentRmb;
}

void CPSCounter::UpdateAnimation(ULONGLONG now) {
    if (g_showCpsCounter && g_cpsCounterEnableTime == 0) {
        g_cpsCounterEnableTime = now;
        g_cpsCounterDisableTime = 0;
    }
    if (!g_showCpsCounter && g_cpsCounterDisableTime == 0 && g_cpsCounterEnableTime > 0) {
        g_cpsCounterDisableTime = now;
        g_cpsCounterEnableTime = 0;
    }

    if (g_cpsCounterEnableTime > 0) {
        float enableElapsed = (float)(now - g_cpsCounterEnableTime) / 1000.0f;
        g_cpsCounterAnim = fminf(1.0f, enableElapsed / 0.4f);
    }
    else if (g_cpsCounterDisableTime > 0) {
        float disableElapsed = (float)(now - g_cpsCounterDisableTime) / 1000.0f;
        float disableAnim = fminf(1.0f, disableElapsed / 0.3f);
        g_cpsCounterAnim = 1.0f - disableAnim;
        if (disableAnim >= 1.0f) {
            g_cpsCounterEnableTime = 0;
            g_cpsCounterDisableTime = 0;
            g_cpsCounterAnim = 0.0f;
        }
    }
}

void CPSCounter::RenderArrayList(ImDrawList* draw, ImVec2 arrayListStart, float& yPos, ImVec2& arrayListEnd) {
    if (g_showCpsCounter || (g_cpsCounterDisableTime > 0 && g_cpsCounterAnim > 0.01f)) {
        float cpsAlpha = g_cpsCounterAnim * 255.0f;
        float slideOffset = -60.0f + (Animations::EaseOutExpo(g_cpsCounterAnim) * 60.0f);
        
        if (cpsAlpha > 1.0f && draw) {
            char cBuf[64];
            sprintf_s(cBuf, sizeof(cBuf), "CPS Counter");
            ImVec2 textSize = ImGui::CalcTextSize(cBuf);
            float xPosC = arrayListStart.x + 300.0f - textSize.x - 10.0f;
            
            draw->AddText(ImVec2(xPosC + slideOffset - 1, yPos + 1), IM_COL32(0, 0, 0, 220), cBuf);
            draw->AddText(ImVec2(xPosC + slideOffset, yPos), IM_COL32(255, 200, 100, (int)cpsAlpha), cBuf);
            yPos += 18.0f;
            arrayListEnd.y = yPos;
        }
    }
}

void CPSCounter::RenderDisplay(int screenWidth, int screenHeight) {
    if (!g_cpsHud) return;
    
    if (g_showCpsCounter || g_cpsCounterAnim > 0.01f) {
        // Initialize position on first draw
        if (g_cpsHud->pos.x == 500 && g_cpsHud->pos.y == 400) {
            g_cpsHud->pos = ImVec2(screenWidth / 2.0f - 50, screenHeight - 100);
        }
        
        // Calculate text size for collision box
        std::string cpsText = ProcessCPSCounterFormat(g_cpsCounterFormat, g_lmbCps, g_rmbCps);
        ImFont* cpsFont = ImGui::GetFont();
        if (cpsFont) {
            float fontSize = 18.0f * g_cpsTextScale * g_cpsHud->scale;
            ImVec2 textSize = cpsFont->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, cpsText.c_str());
            // Update collision size from text
            g_cpsHud->size = ImVec2(
                textSize.x + 8.0f * g_cpsHud->scale,  // Small padding
                textSize.y + 4.0f * g_cpsHud->scale
            );
        }
        
        // Handle CPS counter drag when menu is open
        if (g_showMenu) {
            g_cpsHud->HandleDrag(true);
            g_cpsHud->ClampToScreen();
            
            ImDrawList* hudDraw = ImGui::GetForegroundDrawList();
            if (hudDraw) {
                g_cpsHud->RenderHudEditor(hudDraw);
            }
        }
        
        float easedAnim = Animations::EaseOutExpo(g_cpsCounterAnim);
        float cpsAlpha = easedAnim * 255.0f;
        
        ImDrawList* cpsDraw = ImGui::GetForegroundDrawList();
        if (cpsDraw && cpsAlpha > 1.0f) {
            if (cpsFont) {
                float fontSize = 18.0f * g_cpsTextScale * g_cpsHud->scale;
                ImVec2 textSize = ImGui::CalcTextSize(cpsText.c_str());
                
                // Use HUD position
                float posX = g_cpsHud->pos.x;
                float posY = g_cpsHud->pos.y;
                
                switch (g_cpsCounterAlignment) {
                    case 0: // Left
                        break;
                    case 1: // Center
                        posX += (g_cpsHud->size.x - (textSize.x * g_cpsTextScale * g_cpsHud->scale)) / 2.0f;
                        break;
                    case 2: // Right
                        posX += g_cpsHud->size.x - (textSize.x * g_cpsTextScale * g_cpsHud->scale);
                        break;
                }
                
                posY += (g_cpsHud->size.y - fontSize) / 2.0f;  // Vertical centering
                ImVec2 finalPos = ImVec2(posX, posY);
                
                // Shadow if enabled
                if (g_cpsCounterShadow) {
                    ImVec4 shadowCol = g_cpsCounterShadowColor;
                    shadowCol.w *= easedAnim;
                    cpsDraw->AddText(cpsFont, fontSize, 
                        ImVec2(finalPos.x + g_cpsCounterShadowOffset, finalPos.y + g_cpsCounterShadowOffset),
                        ImGui::GetColorU32(shadowCol),
                        cpsText.c_str()
                    );
                }
                
                // Main text
                ImVec4 textCol = g_cpsTextColor;
                textCol.w *= easedAnim;
                cpsDraw->AddText(cpsFont, fontSize, finalPos, ImGui::GetColorU32(textCol), cpsText.c_str());
            }
        }
    }
}

void CPSCounter::RenderMenu() {
    static char cpsFormatBuf[256] = "{CPS} CPS";

    GUI::RenderCustomSwitch("CPS Counter", &g_showCpsCounter);

    if (GUI::BeginModuleSettings("CPSCounter", &g_showCpsCounter)) {
        ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.12f, 0.12f, 0.14f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(GUI::g_colorAccent.x * 0.8f, GUI::g_colorAccent.y * 0.8f, GUI::g_colorAccent.z * 0.8f, 0.7f));
        ImGui::PushStyleColor(ImGuiCol_TabSelected, GUI::g_colorAccent);
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(GUI::g_colorAccent.x * 0.5f, GUI::g_colorAccent.y * 0.5f, GUI::g_colorAccent.z * 0.5f, 0.4f));
        if (ImGui::BeginTabBar("CPSTabs")) {
            if (ImGui::BeginTabItem("Text")) {
                if (!g_cpsCounterFirstRender) {
                    strncpy_s(cpsFormatBuf, sizeof(cpsFormatBuf), g_cpsCounterFormat.c_str(), _TRUNCATE);
                    g_cpsCounterFirstRender = true;
                }
                if (ImGui::InputText("Format String##CPS", cpsFormatBuf, sizeof(cpsFormatBuf))) {
                    g_cpsCounterFormat = std::string(cpsFormatBuf);
                }
                ImGui::TextDisabled("Use {CPS}, {LMB}, {RMB} as placeholders");
                ImGui::SliderFloat("Text Scale##CPS", &g_cpsTextScale, 0.5f, 2.0f, "%.2f");
                ImGui::ColorEdit4("Text Color##CPS", (float*)&g_cpsTextColor, ImGuiColorEditFlags_NoInputs);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Visual")) {
                const char* alignmentItems[] = { "Left", "Center", "Right" };
                ImGui::Combo("Alignment##CPS", &g_cpsCounterAlignment, alignmentItems, IM_ARRAYSIZE(alignmentItems));
                GUI::RenderCustomSwitch("Text Shadow##CPS", &g_cpsCounterShadow);
                if (g_cpsCounterShadow) {
                    ImGui::SliderFloat("Shadow Offset##CPS", &g_cpsCounterShadowOffset, 0.0f, 10.0f, "%.1f");
                    ImGui::ColorEdit4("Shadow Color##CPS", (float*)&g_cpsCounterShadowColor, ImGuiColorEditFlags_NoInputs);
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::PopStyleColor(4);
        GUI::EndModuleSettings();
    }
}

std::string CPSCounter::ProcessCPSCounterFormat(const std::string& format, int lmb, int rmb) {
    std::string result = format;
    
    // Replace placeholders (case-insensitive)
    std::string formatUpper = format;
    for (char& c : formatUpper) c = std::toupper(c);
    
    // Replace CPS placeholder (uses LMB CPS)
    size_t pos = formatUpper.find("{CPS}");
    if (pos != std::string::npos) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%d", lmb);
        result.replace(pos, 5, buffer);
    }
    
    // Replace LMB placeholder
    formatUpper = result;
    for (char& c : formatUpper) c = std::toupper(c);
    pos = formatUpper.find("{LMB}");
    if (pos != std::string::npos) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%d", lmb);
        result.replace(pos, 5, buffer);
    }
    
    // Replace RMB placeholder
    formatUpper = result;
    for (char& c : formatUpper) c = std::toupper(c);
    pos = formatUpper.find("{RMB}");
    if (pos != std::string::npos) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%d", rmb);
        result.replace(pos, 5, buffer);
    }
    
    return result;
}
