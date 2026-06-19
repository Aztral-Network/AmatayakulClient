#include "ArrayList.hpp"
#include "../../../Animations/Animations.hpp"
#include "../../../GUI/GUI.hpp"
#include "../../ModuleHeader.hpp"
#include <algorithm>
#include <cmath>

extern bool g_showMenu;

bool ArrayList::g_showArrayList = true;
bool ArrayList::g_followTheme = false;
ImVec4 ArrayList::g_bgColor = ImVec4(0.02f, 0.02f, 0.06f, 1.0f);
float ArrayList::g_bgOpacity = 0.75f;
bool ArrayList::g_showSideBar = true;
ImVec4 ArrayList::g_sideBarColor = ImVec4(1.00f, 0.40f, 0.80f, 1.00f);
bool ArrayList::g_chromaSideBar = true;
bool ArrayList::g_roundedBorders = false;
float ArrayList::g_borderRadius = 4.0f;
bool ArrayList::g_showSuffix = true;

static std::vector<ArrayList::ModuleInfo> g_modules;
static bool g_initialized = false;
static HudElement* g_hud = nullptr;

static ImVec4 GetArrayListChroma(float index, float total) {
    float time = (float)GetTickCount64() / 1000.0f;
    ImVec4 col1 = ArrayList::g_chromaSideBar ? ImVec4(1.00f, 0.40f, 0.80f, 1.00f) : ArrayList::g_sideBarColor;
    ImVec4 col2(0.60f, 0.50f, 1.00f, 1.00f);
    float blend = (sinf(time * 2.0f + index * 0.5f) + 1.0f) * 0.5f;
    return ImVec4(
        col1.x + (col2.x - col1.x) * blend,
        col1.y + (col2.y - col1.y) * blend,
        col1.z + (col2.z - col1.z) * blend,
        1.0f
    );
}

void ArrayList::Initialize() {
}

void ArrayList::Render() {
    if (!g_showArrayList) return;

    struct ModEntry { std::string name; std::string suffix; bool enabled; };

    std::vector<ModEntry> currentStates;
    currentStates.push_back({"Watermark", "", Watermark::g_showWatermark});
    currentStates.push_back({"ArrayList", "", g_showArrayList});
    currentStates.push_back({"Render Info", "", RenderInfo::g_showRenderInfo});
    currentStates.push_back({"Keystrokes", "", Keystrokes::g_showKeystrokes});
    currentStates.push_back({"CPSCounter", "", CPSCounter::g_showCpsCounter});
    currentStates.push_back({"FPS Counter", "", FPSCounter::g_showFpsCounter});
    currentStates.push_back({"UnlockFPS", std::to_string((int)UnlockFPS::g_fpsLimit) + "fps", UnlockFPS::g_unlockFpsEnabled});
    currentStates.push_back({"MotionBlur", "", MotionBlur::g_motionBlurEnabled});
    currentStates.push_back({"AutoSprint", "", AutoSprint::g_autoSprintEnabled});

    if (!g_initialized) {
        for (const auto& m : currentStates) {
            g_modules.push_back({m.name, m.suffix, m.enabled, m.enabled ? 1.0f : 0.0f, 0.0f});
        }
        g_initialized = true;
    }

    for (size_t i = 0; i < currentStates.size(); i++) {
        g_modules[i].enabled = currentStates[i].enabled;
        g_modules[i].suffix = currentStates[i].suffix;

        float target = g_modules[i].enabled ? 1.0f : 0.0f;
        g_modules[i].animation += (target - g_modules[i].animation) * 0.12f;
    }

    extern HudElement g_arrayListHud;
    g_hud = &g_arrayListHud;
    g_hud->resizable = true;

    if (!g_hud) return;

    std::vector<ModuleInfo*> activeMods;
    for (auto& m : g_modules) {
        if (m.animation > 0.001f) {
            activeMods.push_back(&m);
        }
    }

    ImDrawList* draw = ImGui::GetForegroundDrawList();

    if (activeMods.empty()) {
        if (g_showMenu) {
            g_hud->RenderHudEditor(draw);
            draw->AddText(ImVec2(g_hud->pos.x + 5, g_hud->pos.y + 2), IM_COL32(255, 255, 255, 150), "ArrayList (Empty)");
        }
        return;
    }

    ImVec2 screenSize = ImGui::GetIO().DisplaySize;
    bool rightAligned = (g_hud->pos.x > screenSize.x / 2.0f);

    std::sort(activeMods.begin(), activeMods.end(), [](ModuleInfo* a, ModuleInfo* b) {
        float wa = ImGui::CalcTextSize(a->name.c_str()).x + (a->suffix.empty() ? 0 : ImGui::CalcTextSize(a->suffix.c_str()).x + 10);
        float wb = ImGui::CalcTextSize(b->name.c_str()).x + (b->suffix.empty() ? 0 : ImGui::CalcTextSize(b->suffix.c_str()).x + 10);
        return wa > wb;
    });

    float yOffset = g_hud->pos.y;
    float baseSpacing = 22.0f * g_hud->scale;
    float maxW = 0.0f;

    for (size_t i = 0; i < activeMods.size(); i++) {
        ModuleInfo* m = activeMods[i];

        float anim = Animations::EaseOutExpo(m->animation);
        float currentSpacing = baseSpacing * anim;

        std::string fullText = m->name + (m->suffix.empty() ? "" : " [" + m->suffix + "]");
        ImVec2 textSize = ImGui::CalcTextSize(fullText.c_str());
        if (textSize.x > maxW) maxW = textSize.x;

        float xPos;
        if (rightAligned) {
            xPos = g_hud->pos.x + g_hud->size.x - (textSize.x + 10) * anim;
        } else {
            xPos = g_hud->pos.x + 10 * anim;
        }

        float yPos = yOffset;

        ImU32 bgColU32 = ImGui::GetColorU32(ImVec4(g_bgColor.x, g_bgColor.y, g_bgColor.z, g_bgOpacity * anim));
        float rounding = g_roundedBorders ? g_borderRadius : 0.0f;

        if (rightAligned) {
            draw->AddRectFilled(ImVec2(xPos - 8, yPos), ImVec2(g_hud->pos.x + g_hud->size.x, yPos + currentSpacing), bgColU32, rounding);
        } else {
            draw->AddRectFilled(ImVec2(g_hud->pos.x, yPos), ImVec2(xPos + textSize.x + 8, yPos + currentSpacing), bgColU32, rounding);
        }

        if (g_showSideBar) {
            bool useChroma = g_followTheme ? false : g_chromaSideBar;
            ImVec4 sbColor = g_followTheme ? GUI::g_colorAccent : g_sideBarColor;
            ImVec4 chromaCol = GetArrayListChroma((float)i, (float)activeMods.size());
            if (!useChroma) chromaCol = sbColor;
            chromaCol.w = anim * g_bgOpacity;

            if (rightAligned) {
                draw->AddRectFilled(ImVec2(g_hud->pos.x + g_hud->size.x - 3, yPos), ImVec2(g_hud->pos.x + g_hud->size.x, yPos + currentSpacing), ImGui::GetColorU32(chromaCol), rounding);
            } else {
                draw->AddRectFilled(ImVec2(g_hud->pos.x, yPos), ImVec2(g_hud->pos.x + 3, yPos + currentSpacing), ImGui::GetColorU32(chromaCol), rounding);
            }
        }

        if (anim > 0.4f) {
            ImU32 textCol;
            if (g_followTheme ? false : g_chromaSideBar) {
                ImVec4 chromaText = GetArrayListChroma((float)i, (float)activeMods.size());
                chromaText.w = anim * g_bgOpacity;
                textCol = ImGui::GetColorU32(chromaText);
            } else {
                textCol = IM_COL32(255, 255, 255, (int)(anim * 255.0f * g_bgOpacity));
            }
            float textY = yPos + (currentSpacing - textSize.y) * 0.5f;

            draw->AddText(ImVec2(xPos, textY), textCol, m->name.c_str());

            if (!m->suffix.empty() && g_showSuffix) {
                float nameWidth = ImGui::CalcTextSize(m->name.c_str()).x;
                std::string sText = " [" + m->suffix + "]";
                draw->AddText(ImVec2(xPos + nameWidth, textY), IM_COL32(160, 160, 180, (int)(anim * 220.0f * g_bgOpacity)), sText.c_str());
            }
        }

        yOffset += currentSpacing;
    }

    g_hud->size.x = (maxW + 20) * g_hud->scale;
    g_hud->size.y = (yOffset - g_hud->pos.y) * g_hud->scale;

    if (g_showMenu) {
        g_hud->HandleDrag(g_showMenu);
        g_hud->ClampToScreen();
        g_hud->RenderHudEditor(draw);
    }
}

void ArrayList::RenderMenu() {
    GUI::RenderCustomSwitch("ArrayList", &g_showArrayList);
    if (GUI::BeginModuleSettings("ArrayList", &g_showArrayList)) {
        GUI::RenderCustomSwitch("Follow Theme##AL", &g_followTheme);

        ImGui::ColorEdit4("Background Color", (float*)&g_bgColor, ImGuiColorEditFlags_NoInputs);
        ImGui::SliderFloat("Opacity", &g_bgOpacity, 0.0f, 1.0f, "%.2f");

        ImGui::Separator();

        GUI::RenderCustomSwitch("Show Side Bar", &g_showSideBar);
        if (g_showSideBar) {
            GUI::RenderCustomSwitch("Chroma Side Bar", &g_chromaSideBar);
            if (!g_chromaSideBar) {
                ImGui::ColorEdit4("Side Bar Color", (float*)&g_sideBarColor, ImGuiColorEditFlags_NoInputs);
            }
        }

        ImGui::Separator();

        GUI::RenderCustomSwitch("Rounded Borders", &g_roundedBorders);
        if (g_roundedBorders) {
            ImGui::SliderFloat("Radius", &g_borderRadius, 0.0f, 12.0f, "%.0f px");
        }

        GUI::RenderCustomSwitch("Show Suffixes", &g_showSuffix);

        GUI::EndModuleSettings();
    }
}
