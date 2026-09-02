/*
Under an4rch Development Public Source License 1.0
*/

#include "ModuleManager.hpp"
#include "../ArrayList/ArrayList.hpp"
#include "Globals.hpp"

void Module::Initialize(uintptr_t gameBase, size_t imageSize, HudElement* renderInfoHud, HudElement* watermarkHud, HudElement* keystrokesHud, HudElement* cpsHud, HudElement* fpsOverlayHud, HudElement* pingHud, HudElement* playerInfoHud) {
    FullBright::Initialize(gameBase);
    RenderInfo::Initialize(renderInfoHud);
    Watermark::Initialize(watermarkHud);
    Keystrokes::Initialize(keystrokesHud);
    CPSCounter::Initialize(cpsHud);
    FPSOverlay::Initialize(fpsOverlayHud);
    PingCounter::Initialize(pingHud);
    PlayerInfo::Initialize(playerInfoHud);
    Terminal::Initialize();
    Info::Initialize();
    UnlockFPS::Initialize();
    ClickGUI::Initialize();
    
    AntiAFK::Initialize();
    NoHurtCam::ScanPattern(gameBase, imageSize);
    Screenshot::Initialize();

    // Resolve the patch targets once (game module memory)
    AutoSprint::ScanPattern(gameBase, imageSize);
    AutoSprint::InitializeHud(&g_sprintTextHud);
    FullBright::ScanPattern(gameBase, imageSize);

    // Enable resize handles on all HUD elements
    renderInfoHud->resizable = true;
    watermarkHud->resizable = true;
    keystrokesHud->resizable = true;
    cpsHud->resizable = true;
    fpsOverlayHud->resizable = true;
    pingHud->resizable = true;
    playerInfoHud->resizable = true;

    // Register all HUD elements for snap-to-other alignment
    HudElement::s_snapCount = 0;
    HudElement::RegisterSnapTarget(renderInfoHud);
    HudElement::RegisterSnapTarget(watermarkHud);
    HudElement::RegisterSnapTarget(keystrokesHud);
    HudElement::RegisterSnapTarget(cpsHud);
    HudElement::RegisterSnapTarget(fpsOverlayHud);
    HudElement::RegisterSnapTarget(pingHud);
    HudElement::RegisterSnapTarget(playerInfoHud);
    HudElement::RegisterSnapTarget(&g_arrayListHud);
    HudElement::RegisterSnapTarget(&g_sprintTextHud);
}

void Module::UpdateAnimation(unsigned long long now) {
    RenderInfo::UpdateFPS();
    RenderInfo::UpdateAnimation(now);
    MotionBlur::UpdateAnimation(now);
    Keystrokes::UpdateAnimation(now);
    Watermark::UpdateAnimation(now);
    FPSOverlay::UpdateAnimation(now);
    PingCounter::UpdateAnimation(now);
    PingCounter::UpdatePing(now);
    PlayerInfo::UpdateAnimation(now);
    
    AntiAFK::Tick();
}

void Module::RenderDisplay(float sw, float sh) {
    Watermark::RenderDisplay();
    Keystrokes::RenderDisplay(sw, sh);
    RenderInfo::RenderWindow();
    CPSCounter::RenderDisplay((int)sw, (int)sh);
    FPSOverlay::RenderDisplay((int)sw, (int)sh);
    PingCounter::RenderDisplay(sw, sh);
    PlayerInfo::RenderDisplay();
    
    // Sprint text HUD
    AutoSprint::RenderSprintText();

    // Call new centralized ArrayList
    ArrayList::Render();
}

void Module::RenderArrayList(ImDrawList* draw, ImVec2 arrayListStart, float& yPos, ImVec2& arrayListEnd) {
    // Legacy function, now handled inside RenderDisplay -> ArrayList::Render()
}
