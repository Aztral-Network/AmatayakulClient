#include "ModuleManager.hpp"
#include "ModuleHeader.hpp"
#include "Terminal/Terminal.hpp"

void Module::Initialize(uintptr_t gameBase, HudElement* renderInfoHud, HudElement* watermarkHud, HudElement* keystrokesHud, HudElement* cpsHud, HudElement* fpsHud) {
    AutoSprint::Initialize(gameBase);
    ArrayList::Initialize();
    RenderInfo::Initialize(renderInfoHud);
    Watermark::Initialize(watermarkHud);
    Keystrokes::Initialize(keystrokesHud);
    CPSCounter::Initialize(cpsHud);
    FPSCounter::Initialize(fpsHud);
    MotionBlur::Initialize();
    Info::Initialize();
    UnlockFPS::Initialize();
    Terminal::Initialize();
}

void Module::UpdateAnimation(unsigned long long now) {
    RenderInfo::UpdateFPS();
    RenderInfo::UpdateAnimation(now);
    MotionBlur::UpdateAnimation(now);
    Keystrokes::UpdateAnimation(now);
    Watermark::UpdateAnimation(now);
    FPSCounter::UpdateAnimation(now);
}

void Module::RenderDisplay(float sw, float sh) {
    Watermark::RenderDisplay();
    Keystrokes::RenderDisplay(sw, sh);
    RenderInfo::RenderWindow();
    CPSCounter::RenderDisplay(sw, sh);
    FPSCounter::RenderDisplay(sw, sh);

    ArrayList::Render();
}

void Module::RenderArrayList(ImDrawList* draw, ImVec2 arrayListStart, float& yPos, ImVec2& arrayListEnd) {
}
