#include "GUI.hpp"
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_internal.h"
#include "../Modules/ModuleHeader.hpp"
#include "../Assets/resource.h"
#include <windows.h>
#include <algorithm>
#include <d3d11.h>
#include "../Assets/stb/stb_image.h"
#include "../Animations/Animations.hpp"
#include "../Modules/Terminal/Terminal.hpp"
#include "../Config/ConfigManager.hpp"

extern HMODULE g_hModule;
extern ID3D11Device* pDevice;
extern bool g_showMenu;
extern ULONGLONG g_lastToggle;

bool GUI::g_showMenu = false;
float GUI::g_menuAnim = 0.0f;

GUI::ThemePreset GUI::g_currentTheme = Theme_AmatayakulRed;
ImVec4 GUI::g_colorBgMain = ImVec4(0.05f, 0.05f, 0.05f, 1.0f);
ImVec4 GUI::g_colorBgPanel = ImVec4(0.08f, 0.08f, 0.08f, 0.0f);
ImVec4 GUI::g_colorAccent = ImVec4(0.85f, 0.05f, 0.10f, 1.0f);
ImVec4 GUI::g_colorAccentSoft = ImVec4(0.65f, 0.03f, 0.08f, 0.4f);
ImVec4 GUI::g_colorAccentGlow = ImVec4(0.85f, 0.05f, 0.10f, 0.4f);

GUI::ModFilter GUI::g_currentFilter = Filter_All;
GUI::ModCategory GUI::g_currentCategory = Cat_Mods;
char GUI::g_searchBuffer[128] = "";
float GUI::g_filterHoverAnim[3] = {1.0f, 0.0f, 0.0f};
float GUI::g_categoryHoverAnim[4] = {1.0f, 0.0f, 0.0f};

std::vector<std::string> GUI::g_profiles = {"Default"};
int GUI::g_selectedProfile = 0;
char GUI::g_newProfileBuf[64] = "";
bool GUI::g_showNewProfileInput = false;

std::map<std::string, ImTextureID> GUI::g_icons;
std::map<std::string, float> GUI::g_elementAnims;
std::string GUI::g_currentSettingsModule = "";

int GUI::g_editingProfileIndex = -1;
char GUI::g_renameBuf[64] = "";

GUI::ModCategory GUI::g_lastCategory = Cat_Mods;
float GUI::g_tabTransitionAnim = 1.0f;
float GUI::g_settingsTransitionAnim = 0.0f;

static HMODULE GetCurrentModule() {
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery((LPCVOID)&GetCurrentModule, &mbi, sizeof(mbi)) != 0) {
        return (HMODULE)mbi.AllocationBase;
    }
    return GetModuleHandleA("amatayakul.dll");
}

static ID3D11ShaderResourceView* LoadTextureFromResource(ID3D11Device* device, int resourceId) {
    HMODULE hMod = GetCurrentModule();
    if (!hMod) return NULL;
    HRSRC hRes = FindResourceA(hMod, MAKEINTRESOURCEA(resourceId), (LPCSTR)10);
    if (!hRes) return NULL;
    DWORD imageSize = SizeofResource(hMod, hRes);
    HGLOBAL hData = LoadResource(hMod, hRes);
    if (!hData) return NULL;
    LPVOID pData = LockResource(hData);
    int width, height, channels;
    unsigned char* image = stbi_load_from_memory((unsigned char*)pData, imageSize, &width, &height, &channels, 4);
    if (!image) return NULL;
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    ID3D11Texture2D* pTexture = NULL;
    D3D11_SUBRESOURCE_DATA subResource = {};
    subResource.pSysMem = image;
    subResource.SysMemPitch = desc.Width * 4;
    device->CreateTexture2D(&desc, &subResource, &pTexture);
    ID3D11ShaderResourceView* out_srv = NULL;
    if (pTexture) {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        device->CreateShaderResourceView(pTexture, &srvDesc, &out_srv);
        pTexture->Release();
    }
    stbi_image_free(image);
    return out_srv;
}

void GUI::ApplyThemePreset(ThemePreset preset) {
    g_currentTheme = preset;
    switch (preset) {
        case Theme_AmatayakulRed:
            g_colorBgMain = ImVec4(0.00f, 0.00f, 0.00f, 0.40f);
            g_colorBgPanel = ImVec4(0.09f, 0.03f, 0.04f, 0.00f);
            g_colorAccent = ImVec4(0.85f, 0.05f, 0.10f, 1.0f);
            g_colorAccentSoft = ImVec4(0.65f, 0.03f, 0.08f, 0.4f);
            g_colorAccentGlow = ImVec4(0.85f, 0.05f, 0.10f, 0.4f);
            break;
        case Theme_SakuraBlossom:
            g_colorBgMain = ImVec4(0.00f, 0.00f, 0.00f, 0.40f);
            g_colorBgPanel = ImVec4(0.12f, 0.06f, 0.08f, 0.00f);
            g_colorAccent = ImVec4(1.00f, 0.60f, 0.75f, 1.0f);
            g_colorAccentSoft = ImVec4(1.00f, 0.78f, 0.83f, 0.4f);
            g_colorAccentGlow = ImVec4(1.00f, 0.60f, 0.75f, 0.45f);
            break;
        case Theme_Cyberpunk:
            g_colorBgMain = ImVec4(0.00f, 0.00f, 0.00f, 0.40f);
            g_colorBgPanel = ImVec4(0.04f, 0.04f, 0.10f, 0.00f);
            g_colorAccent = ImVec4(0.00f, 0.95f, 1.00f, 1.0f);
            g_colorAccentSoft = ImVec4(0.95f, 0.90f, 0.00f, 0.4f);
            g_colorAccentGlow = ImVec4(0.00f, 0.95f, 1.00f, 0.4f);
            break;
        case Theme_EmeraldForest:
            g_colorBgMain = ImVec4(0.00f, 0.00f, 0.00f, 0.40f);
            g_colorBgPanel = ImVec4(0.04f, 0.09f, 0.06f, 0.00f);
            g_colorAccent = ImVec4(0.20f, 0.85f, 0.55f, 1.0f);
            g_colorAccentSoft = ImVec4(0.15f, 0.60f, 0.40f, 0.4f);
            g_colorAccentGlow = ImVec4(0.20f, 0.85f, 0.55f, 0.4f);
            break;
        case Theme_DeepSea:
            g_colorBgMain = ImVec4(0.00f, 0.00f, 0.00f, 0.40f);
            g_colorBgPanel = ImVec4(0.03f, 0.06f, 0.12f, 0.00f);
            g_colorAccent = ImVec4(0.20f, 0.60f, 1.00f, 1.0f);
            g_colorAccentSoft = ImVec4(0.40f, 0.85f, 1.00f, 0.4f);
            g_colorAccentGlow = ImVec4(0.20f, 0.60f, 1.00f, 0.4f);
            break;
        default: break;
    }
    ApplyTheme();
}

void GUI::ApplyTheme() {
    ImGuiStyle* style = &ImGui::GetStyle();
    style->WindowRounding = 14.0f;
    style->ChildRounding = 10.0f;
    style->FrameRounding = 7.0f;
    style->GrabRounding = 6.0f;
    style->PopupRounding = 8.0f;
    style->ScrollbarRounding = 12.0f;
    style->TabRounding = 7.0f;
    style->WindowBorderSize = 0.0f;
    style->ChildBorderSize = 0.0f;
    style->PopupBorderSize = 1.0f;
    style->FrameBorderSize = 0.0f;
    style->ItemSpacing = ImVec2(12, 10);
    style->WindowPadding = ImVec2(0, 0);

    ImVec4* colors = style->Colors;
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = g_colorBgMain;
    colors[ImGuiCol_ChildBg] = ImVec4(0, 0, 0, 0);
    colors[ImGuiCol_PopupBg] = g_colorBgPanel;
    colors[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.22f, 0.50f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark] = g_colorAccent;
    colors[ImGuiCol_SliderGrab] = g_colorAccent;
    colors[ImGuiCol_SliderGrabActive] = ImVec4(g_colorAccent.x * 1.2f, g_colorAccent.y * 1.2f, g_colorAccent.z * 1.2f, 1.0f);
    colors[ImGuiCol_Button] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = g_colorAccent;
    colors[ImGuiCol_ButtonActive] = ImVec4(g_colorAccent.x * 0.7f, g_colorAccent.y * 0.7f, g_colorAccent.z * 0.7f, 1.0f);
    colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = g_colorAccent;
    colors[ImGuiCol_HeaderActive] = g_colorAccent;
    colors[ImGuiCol_Separator] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = g_colorAccent;
    colors[ImGuiCol_SeparatorActive] = g_colorAccent;
}

void GUI::LoadFont() {
    ImGuiIO& io = ImGui::GetIO();
    HMODULE hMod = GetCurrentModule();
    if (!hMod) return;
    HRSRC hRes = FindResourceA(hMod, MAKEINTRESOURCEA(IDR_FONT_ROBOTO), (LPCSTR)10);
    if (hRes) {
        DWORD fontSize = SizeofResource(hMod, hRes);
        HGLOBAL hData = LoadResource(hMod, hRes);
        if (hData) {
            LPVOID pFontData = LockResource(hData);
            void* pFontCopy = ImGui::MemAlloc(fontSize);
            if (pFontCopy) {
                memcpy(pFontCopy, pFontData, fontSize);
                io.Fonts->AddFontFromMemoryTTF(pFontCopy, (int)fontSize, 18.0f);
            }
        }
    }
}

void GUI::LoadIcons(void* pDevice) {
    ID3D11Device* device = (ID3D11Device*)pDevice;
    struct IconRes { const char* name; int id; };
    IconRes icons[] = {
        {"cpscounter", IDR_ICON_CPS}, {"fpscounter", IDR_ICON_FPS},
        {"gear", IDR_ICON_GEAR}, {"keystrokes", IDR_ICON_KEYSTROKES},
        {"renderinfo", IDR_ICON_RENDERINFO}, {"unlockfps", IDR_ICON_UNLOCKFPS},
        {"watermark", IDR_ICON_WATERMARK}, {"arraylist", IDR_ICON_ARRAYLIST},
        {"back", IDR_ICON_BACK}, {"logo", IDR_ICON_LOGO},
        {"dashboard", IDR_ICON_DASHBOARD}, {"visuals", IDR_ICON_VISUALS},
        {"misc", IDR_ICON_MISC},
        {"motionblur", IDR_ICON_MOTIONBLUR}, {"autosprint", IDR_ICON_AUTOSPRINT},
        {"edit", IDR_ICON_EDIT},
        {"closeX", IDR_ICON_CLOSEX},
        {"logo_pink", IDR_ICON_LOGO_PINK}, {"logo_cyan", IDR_ICON_LOGO_CYAN},
        {"logo_green", IDR_ICON_LOGO_GREEN}, {"logo_blue", IDR_ICON_LOGO_BLUE}
    };
    for (auto& ic : icons) {
        ID3D11ShaderResourceView* srv = LoadTextureFromResource(device, ic.id);
        g_icons[ic.name] = (ImTextureID)srv;
    }
}

void GUI::UpdateAnimation(ULONGLONG now, float dt) {
    float target = g_showMenu ? 1.0f : 0.0f;
    float speed = 8.0f * dt;
    g_menuAnim += (target - g_menuAnim) * speed;
    if (fabsf(g_menuAnim - target) < 0.001f) g_menuAnim = target;

    // Tab transition animation
    if (g_currentCategory != g_lastCategory) {
        g_tabTransitionAnim = 0.0f;
        g_lastCategory = g_currentCategory;
    }
    g_tabTransitionAnim += (1.0f - g_tabTransitionAnim) * 10.0f * dt;
    if (g_tabTransitionAnim > 0.999f) g_tabTransitionAnim = 1.0f;

    // Settings transition animation
    float settingsTarget = g_currentSettingsModule.empty() ? 0.0f : 1.0f;
    g_settingsTransitionAnim += (settingsTarget - g_settingsTransitionAnim) * 10.0f * dt;
    if (fabsf(g_settingsTransitionAnim - settingsTarget) < 0.001f) g_settingsTransitionAnim = settingsTarget;
}

bool GUI::PassesFilter(const char* moduleName, const char* category) {
    if (g_searchBuffer[0] != '\0') {
        std::string search = g_searchBuffer;
        std::string name = moduleName;
        std::transform(search.begin(), search.end(), search.begin(), ::tolower);
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        if (name.find(search) == std::string::npos) return false;
    }
    if (g_currentFilter == Filter_Visual && std::string(category) != "visual") return false;
    if (g_currentFilter == Filter_Misc && std::string(category) != "misc") return false;
    return true;
}

void GUI::RenderMenu(float screenWidth, float screenHeight) {
    if (!g_showMenu) return;

    // Darken background behind menu
    ImDrawList* bgDrawList = ImGui::GetBackgroundDrawList();
    bgDrawList->AddRectFilled(ImVec2(0, 0), ImVec2(screenWidth, screenHeight),
        IM_COL32(0, 0, 0, (int)(80 * Animations::EaseOutExpo(g_menuAnim))));

    static bool profilesLoaded = false;
    if (!profilesLoaded) {
        ConfigManager::Initialize();
        LoadProfiles();
        profilesLoaded = true;
    }

    // 25% larger main window dimensions (1125x750)
    float menuW = 1125.0f;
    float menuH = 750.0f;
    float menuX = (screenWidth - menuW) * 0.5f;
    float menuY = (screenHeight - menuH) * 0.5f;

    ImGui::SetNextWindowPos(ImVec2(menuX, menuY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(menuW, menuH), ImGuiCond_Always);

    float windowAlpha = Animations::EaseOutExpo(g_menuAnim);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, windowAlpha);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;
    if (ImGui::Begin("##AmatayakulMenu", &g_showMenu, flags)) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 wp = ImGui::GetWindowPos();
        ImVec2 ws = ImGui::GetWindowSize();

        // 25% larger sidebar and top bar sizes
        float sidebarW = 250.0f;
        float topBarH = 75.0f;

        // --- UNIFIED TOP BAR BACKGROUND ---
        dl->AddRectFilled(wp, ImVec2(wp.x + ws.x, wp.y + topBarH), ImGui::GetColorU32(ImVec4(0.00f, 0.00f, 0.00f, 0.40f)), 14.0f, ImDrawFlags_RoundCornersTop);

        // --- UNIFIED SIDEBAR BACKGROUND (Below Top Bar) ---
        dl->AddRectFilled(ImVec2(wp.x, wp.y + topBarH), ImVec2(wp.x + sidebarW, wp.y + ws.y), ImGui::GetColorU32(ImVec4(0.00f, 0.00f, 0.00f, 0.40f)), 14.0f, ImDrawFlags_RoundCornersBottomLeft);

        // --- TOP BAR CONTENTS ---
        // 1. Logo/Client Name on the left (theme-aware, bigger)
        const char* logoKey = "logo";
        switch (g_currentTheme) {
            case Theme_SakuraBlossom: logoKey = "logo_pink"; break;
            case Theme_Cyberpunk:     logoKey = "logo_cyan"; break;
            case Theme_EmeraldForest: logoKey = "logo_green"; break;
            case Theme_DeepSea:       logoKey = "logo_blue"; break;
            default: break;
        }
        ImTextureID logoTex = g_icons.count(logoKey) ? g_icons[logoKey] : (ImTextureID)0;
        if (logoTex) {
            float logoW = 170.0f, logoH = 45.0f;
            float logoX = wp.x + 24, logoY = wp.y + (topBarH - logoH) * 0.5f;
            dl->AddImage(logoTex, ImVec2(logoX, logoY), ImVec2(logoX + logoW, logoY + logoH), ImVec2(0,0), ImVec2(1,1), IM_COL32_WHITE);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, g_colorAccent);
            ImGui::SetWindowFontScale(1.15f);
            ImVec2 ts = ImGui::CalcTextSize("AMATAYAKUL");
            dl->AddText(ImVec2(wp.x + 24, wp.y + (topBarH - ts.y) * 0.5f), ImGui::GetColorU32(ImGuiCol_Text), "AMATAYAKUL");
            ImGui::SetWindowFontScale(1.0f);
            ImGui::PopStyleColor();
        }

        // 2. Horizontal Tab Buttons (Mods / Terminal / Info / IRC Chat) centered horizontally
        float tabX = wp.x + 220.0f;
        const char* catLabels[] = { "Mods", "Terminal", "Info", "IRC (Soon)" };
        for (int c = 0; c < 4; c++) {
            bool isActive = (g_currentCategory == (ModCategory)c);
            bool disabled = (c == 3);
            
            // Clean pill outlines or soft fill like Lunar Client top tabs
            ImVec4 tabBg = disabled ? ImVec4(0.08f, 0.08f, 0.08f, 0.2f)
                : (isActive ? ImVec4(g_colorAccent.x, g_colorAccent.y, g_colorAccent.z, 0.85f) : ImVec4(0, 0, 0, 0));
            ImVec4 borderCol = disabled ? ImVec4(0.15f, 0.15f, 0.15f, 0.4f)
                : (isActive ? g_colorAccent : ImVec4(0.25f, 0.25f, 0.28f, 0.6f));
            
            ImGui::SetCursorScreenPos(ImVec2(tabX, wp.y + 13));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(16, 7));
            ImGui::PushStyleColor(ImGuiCol_Button, tabBg);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, disabled ? tabBg : ImVec4(g_colorAccent.x, g_colorAccent.y, g_colorAccent.z, 0.4f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, tabBg);
            ImGui::PushStyleColor(ImGuiCol_Text, disabled ? ImVec4(0.4f, 0.4f, 0.4f, 1.0f) : (isActive ? ImVec4(1, 1, 1, 1) : ImVec4(0.7f, 0.7f, 0.7f, 1.0f)));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Border, borderCol);

        char cId[32];
        snprintf(cId, sizeof(cId), "%s##cat%d", catLabels[c], c);
        if (ImGui::Button(cId, ImVec2(0, 34)) && !disabled) {
            g_currentCategory = (ModCategory)c;
            g_currentSettingsModule = "";
            memset(g_searchBuffer, 0, sizeof(g_searchBuffer));
        }
            ImGui::PopStyleColor(5);
            ImGui::PopStyleVar(3);
            
            tabX += ImGui::GetItemRectSize().x + 8.0f;
        }

        // 3. Close Button on the far top-right
        {
            ImVec2 closePos = ImVec2(wp.x + ws.x - 45, wp.y + 13);
            ImVec2 closeMin = closePos;
            ImVec2 closeMax = ImVec2(closePos.x + 30, closePos.y + 30);
            bool closeHovered = ImGui::IsMouseHoveringRect(closeMin, closeMax);
            ImU32 closeBg = closeHovered ? IM_COL32(200, 50, 50, 255) : IM_COL32(60, 60, 65, 120);
            dl->AddRectFilled(closeMin, closeMax, closeBg, 6.0f);
            ImTextureID closeTex = g_icons.count("closeX") ? g_icons["closeX"] : (ImTextureID)0;
            if (closeTex) {
                float iconS = 20.0f;
                float iconX = closeMin.x + (30.0f - iconS) * 0.5f;
                float iconY = closeMin.y + (30.0f - iconS) * 0.5f;
                dl->AddImage(closeTex, ImVec2(iconX, iconY), ImVec2(iconX + iconS, iconY + iconS), ImVec2(0,0), ImVec2(1,1), IM_COL32_WHITE);
            }
            if (closeHovered && ImGui::IsMouseClicked(0)) {
                ::g_showMenu = false;
                ::g_lastToggle = GetTickCount64();
                g_showMenu = false;
            }
        }

        // --- LEFT SIDEBAR (Profiles) ---
        float sidebarTopY = topBarH + 20.0f;
        float sidebarContentH = ws.y - sidebarTopY;
        ImGui::SetCursorPos(ImVec2(0, sidebarTopY));
        ImGui::BeginChild("Sidebar", ImVec2(sidebarW, sidebarContentH), false, ImGuiWindowFlags_NoScrollbar);

        // Profiles header
        ImGui::SetCursorPosX(12);
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.55f, 1.0f), "PROFILES");
        ImGui::Spacing();

        // Profiles list (scrollable, leaves room for add + delete buttons below)
        float profileListH = sidebarContentH - 150.0f;
        if (profileListH < 80.0f) profileListH = 80.0f;
        ImGui::BeginChild("ProfileListScroll", ImVec2(sidebarW, profileListH), false);
        for (int i = 0; i < (int)g_profiles.size(); i++) {
            bool selected = (i == g_selectedProfile);
            bool editing = (i == g_editingProfileIndex);
            ImGui::SetCursorPosX(12);

            if (editing) {
                // Inline edit: text input replaces profile button
                ImGui::PushItemWidth(sidebarW - 48);
                bool done = ImGui::InputTextWithHint((std::string("##edit") + std::to_string(i)).c_str(),
                    g_profiles[i].c_str(), g_renameBuf, sizeof(g_renameBuf),
                    ImGuiInputTextFlags_EnterReturnsTrue);
                if (done) {
                    if (g_renameBuf[0] != '\0') {
                        RenameProfile(i, g_renameBuf);
                    }
                    g_editingProfileIndex = -1;
                }
                if (ImGui::IsItemDeactivatedAfterEdit()) {
                    if (g_renameBuf[0] != '\0') {
                        RenameProfile(i, g_renameBuf);
                    }
                    g_editingProfileIndex = -1;
                }
                ImGui::PopItemWidth();
                ImGui::SameLine(0, 2);
                // Disabled edit icon while editing
                ImTextureID editTex = g_icons.count("edit") ? g_icons["edit"] : (ImTextureID)0;
                if (editTex) {
                    ImVec2 iconPos = ImGui::GetCursorScreenPos();
                    ImGui::GetWindowDrawList()->AddImage(editTex, iconPos,
                        ImVec2(iconPos.x + 24, iconPos.y + 30),
                        ImVec2(0,0), ImVec2(1,1), IM_COL32(100, 100, 110, 120));
                }
            } else {
                float buttonW = sidebarW - 48.0f; // room for edit icon

                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
                ImGui::PushStyleColor(ImGuiCol_Button, selected ? ImVec4(g_colorAccent.x, g_colorAccent.y, g_colorAccent.z, 0.35f) : ImVec4(0, 0, 0, 0));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.05f));
                ImGui::PushStyleColor(ImGuiCol_Border, selected ? g_colorAccent : ImVec4(0,0,0,0));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, selected ? 1.0f : 0.0f);

                char btnLabel[128];
                snprintf(btnLabel, sizeof(btnLabel), "%s##prof%d", g_profiles[i].c_str(), i);
                if (ImGui::Button(btnLabel, ImVec2(buttonW, 30))) {
                    SwitchProfile(i);
                }

                ImGui::PopStyleColor(3);
                ImGui::PopStyleVar(2);

                // Edit icon (only for non-Default)
                if (i > 0) {
                    ImGui::SameLine(0, 2);
                    ImTextureID editTex = g_icons.count("edit") ? g_icons["edit"] : (ImTextureID)0;
                    ImVec2 btnMin = ImGui::GetCursorScreenPos();
                    ImVec2 btnSize(24, 30);
                    ImGui::InvisibleButton((std::string("##ren") + std::to_string(i)).c_str(), btnSize);
                    bool hovered = ImGui::IsItemHovered();
                    if (ImGui::IsItemClicked()) {
                        g_editingProfileIndex = i;
                        strncpy(g_renameBuf, g_profiles[i].c_str(), sizeof(g_renameBuf) - 1);
                        g_renameBuf[sizeof(g_renameBuf) - 1] = '\0';
                    }
                    if (editTex) {
                        ImU32 iconCol = hovered ? IM_COL32(255, 255, 255, 255) : IM_COL32(180, 180, 190, 200);
                        ImVec2 imgSize(18, 18);
                        ImVec2 imgPos(btnMin.x + (btnSize.x - imgSize.x) * 0.5f,
                                      btnMin.y + (btnSize.y - imgSize.y) * 0.5f);
                        ImGui::GetWindowDrawList()->AddImage(editTex, imgPos,
                            ImVec2(imgPos.x + imgSize.x, imgPos.y + imgSize.y),
                            ImVec2(0,0), ImVec2(1,1), iconCol);
                    }
                }
            }
        }

        // Inline input inside the scroll area, below the last profile (only when toggled)
        if (g_showNewProfileInput) {
            ImGui::Spacing();
            ImGui::SetCursorPosX(12);
            ImGui::PushItemWidth(sidebarW - 24);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.08f, 0.08f, 0.10f, 0.7f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
            if (ImGui::InputTextWithHint("##newprof", "Profile name + Enter", g_newProfileBuf, sizeof(g_newProfileBuf), ImGuiInputTextFlags_EnterReturnsTrue)) {
                if (g_newProfileBuf[0] != '\0') {
                    AddProfile(g_newProfileBuf);
                    g_newProfileBuf[0] = '\0';
                    g_showNewProfileInput = false;
                }
            }
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(1);
            ImGui::PopItemWidth();
        }

        ImGui::EndChild(); // ProfileListScroll

        ImGui::Spacing();

        // Add Profile button below the scroll area
        ImGui::SetCursorPosX(12);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.12f, 0.14f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.18f, 0.20f, 1.0f));
        if (ImGui::Button("+ Add Profile", ImVec2(sidebarW - 24, 30.0f))) {
            g_showNewProfileInput = !g_showNewProfileInput;
            if (g_showNewProfileInput) g_newProfileBuf[0] = '\0';
        }
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar();

        // Delete Profile button (below Add Profile)
        ImGui::Spacing();
        ImGui::SetCursorPosX(12);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.08f, 0.08f, 0.4f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.10f, 0.10f, 0.7f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
        bool canDelete = (g_selectedProfile > 0);
        if (!canDelete) ImGui::BeginDisabled();
        if (ImGui::Button("Delete Profile", ImVec2(sidebarW - 24, 30.0f))) {
            int idx = g_selectedProfile;
            DeleteProfile(idx);
            g_showNewProfileInput = false;
            g_editingProfileIndex = -1;
        }
        if (!canDelete) ImGui::EndDisabled();
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();

        ImGui::EndChild(); // Sidebar

        // --- RIGHT CONTENT AREA ---
        float contentX = sidebarW;
        float contentTop = topBarH;
        ImGui::SetCursorPos(ImVec2(contentX, contentTop));
        ImGui::BeginChild("ContentArea", ImVec2(ws.x - contentX, ws.y - contentTop), false, ImGuiWindowFlags_NoScrollbar);

        if (g_currentSettingsModule.empty()) {
            // Transition alpha for tab switching
            float contentAlpha = Animations::EaseOutExpo(g_tabTransitionAnim);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, contentAlpha);

            // === FILTER ROW (only shown on Mods tab) ===
            if (g_currentCategory == Cat_Mods) {
                ImGui::SetCursorPos(ImVec2(20, 16));
                
                const char* filterLabels[] = { "ALL", "VISUAL", "MISC" };
                float filterX = 20;
                float filterBtnH = 28.0f;
                for (int f = 0; f < 3; f++) {
                    ImGui::SetCursorPos(ImVec2(filterX, 14));
                    bool isActive = (g_currentFilter == (ModFilter)f);
                    
                    ImVec4 bg = isActive ? ImVec4(g_colorAccent.x, g_colorAccent.y, g_colorAccent.z, 0.85f) : ImVec4(0.08f, 0.08f, 0.10f, 0.5f);
                    ImVec4 textCol = isActive ? ImVec4(1, 1, 1, 1) : ImVec4(0.6f, 0.6f, 0.65f, 1.0f);

                    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(14, 5));
                    ImGui::PushStyleColor(ImGuiCol_Button, bg);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, isActive ? bg : ImVec4(g_colorAccent.x, g_colorAccent.y, g_colorAccent.z, 0.4f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, bg);
                    ImGui::PushStyleColor(ImGuiCol_Text, textCol);

                    char fId[32];
                    snprintf(fId, sizeof(fId), "%s##filt%d", filterLabels[f], f);
                    if (ImGui::Button(fId, ImVec2(0, filterBtnH))) {
                        g_currentFilter = (ModFilter)f;
                    }
                    ImGui::PopStyleColor(4);
                    ImGui::PopStyleVar(2);
                    filterX += ImGui::GetItemRectSize().x + 8;
                }

                // Right-aligned Search box on the same row
                float searchW = 180.0f;
                float searchX = ImGui::GetWindowWidth() - searchW - 20.0f;
                ImGui::SetCursorPos(ImVec2(searchX, 14));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 5));
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.08f, 0.08f, 0.10f, 0.8f));
                ImGui::PushItemWidth(searchW);
                ImGui::InputTextWithHint("##search", "Search mods...", g_searchBuffer, sizeof(g_searchBuffer));
                ImGui::PopItemWidth();
                ImGui::PopStyleColor();
                ImGui::PopStyleVar(2);

                // Nice horizontal divider line below filters
                ImGui::SetCursorPos(ImVec2(20, 48));
                ImGui::Separator();
            }

            // Content based on category
            if (g_currentCategory == Cat_Terminal) {
                ImGui::SetCursorPos(ImVec2(20, 20));
                float termH = ws.y - topBarH - 70.0f;
                ImGui::BeginChild("TerminalContent", ImVec2(ImGui::GetWindowWidth() - 32, termH), false, ImGuiWindowFlags_NoScrollbar);
                Terminal::RenderConsole();
                ImGui::EndChild();
            } else if (g_currentCategory == Cat_Info) {
                ImGui::SetCursorPos(ImVec2(20, 60));
                ImGui::TextColored(g_colorAccent, "INFO");
                ImGui::SetCursorPos(ImVec2(20, 90));
                ImGui::Text("Amatayakul Client v1.0.3");
                ImGui::SetCursorPos(ImVec2(20, 115));
                ImGui::Text("Minecraft Beta 0.15.10");
                ImGui::SetCursorPos(ImVec2(20, 145));
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.55f, 1.0f), "THEME");
                ImGui::SetCursorPos(ImVec2(20, 170));
                ImGui::PushItemWidth(250.0f);
                const char* infoThemeNames[] = { "Amatayakul Red", "Sakura Blossom", "Cyberpunk", "Emerald Forest", "Deep Sea" };
                int themeInt = (int)g_currentTheme;
                if (ImGui::Combo("##infotheme", &themeInt, infoThemeNames, Theme_Max)) {
                    ApplyThemePreset((ThemePreset)themeInt);
                }
                ImGui::PopItemWidth();
            } else if (g_currentCategory == Cat_IRC) {
                ImGui::SetCursorPos(ImVec2(20, 60));
                ImGui::TextColored(g_colorAccent, "IRC CHAT");
                ImGui::SetCursorPos(ImVec2(20, 90));
                ImGui::TextDisabled("IRC chat coming soon.");
            } else {
                // Mods Grid
                float modsTop = (g_currentCategory == Cat_Mods) ? 56.0f : 20.0f;
                ImGui::SetCursorPos(ImVec2(20, modsTop));

                ImGui::BeginChild("ModsScroll", ImVec2(ImGui::GetWindowWidth() - 32, ws.y - topBarH - (modsTop + 10)), false);
                float contentWidth = ImGui::GetWindowWidth();
                float cardSpacing = 16.0f;
                int columns = 3;
                float cardW = (contentWidth - (cardSpacing * (columns + 1))) / columns;
                int col = 0;

                auto ModCard = [&](const char* name, const char* iconName, bool* enabled, const char* category) {
                    if (!PassesFilter(name, category)) return;
                    if (col > 0) {
                        ImGui::SameLine(0, cardSpacing);
                    }

                    ImGui::PushID(name);
                    RenderModuleCard(name, iconName, enabled, nullptr);
                    ImGui::PopID();
                    col++;
                    if (col >= columns) col = 0;
                };

                ModCard("Render Info", "renderinfo", &RenderInfo::g_showRenderInfo, "visual");
                ModCard("Watermark", "watermark", &Watermark::g_showWatermark, "visual");
                ModCard("ArrayList", "arraylist", &ArrayList::g_showArrayList, "visual");
                ModCard("Keystrokes", "keystrokes", &Keystrokes::g_showKeystrokes, "visual");
                ModCard("CPS Counter", "cpscounter", &CPSCounter::g_showCpsCounter, "visual");
                ModCard("FPS Counter", "fpscounter", &FPSCounter::g_showFpsCounter, "visual");
                ModCard("Motion Blur", "motionblur", &MotionBlur::g_motionBlurEnabled, "visual");
                ModCard("Unlock FPS", "unlockfps", &UnlockFPS::g_unlockFpsEnabled, "misc");
                ModCard("Auto Sprint", "autosprint", &AutoSprint::g_autoSprintEnabled, "misc");

                ImGui::EndChild(); // ModsScroll
            }

            ImGui::PopStyleVar(); // contentAlpha

        } else {
            // Settings view with transition animation
            float settingsAlpha = Animations::EaseOutExpo(g_settingsTransitionAnim);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, settingsAlpha);

            ImGui::SetCursorPos(ImVec2(20, 16));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(g_colorAccent.x, g_colorAccent.y, g_colorAccent.z, 0.85f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(g_colorAccent.x * 1.1f, g_colorAccent.y * 1.1f, g_colorAccent.z * 1.1f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(g_colorAccent.x * 0.7f, g_colorAccent.y * 0.7f, g_colorAccent.z * 0.7f, 1.0f));

            ImTextureID backIcon = g_icons.count("back") ? g_icons["back"] : (ImTextureID)0;
            if (backIcon) {
                if (ImGui::ImageButton("##Back", backIcon, ImVec2(20, 20))) {
                    g_currentSettingsModule = "";
                }
            } else {
                if (ImGui::Button("< Back", ImVec2(70, 26))) {
                    g_currentSettingsModule = "";
                }
            }
            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar();

            ImGui::SameLine();
            std::string modName = g_currentSettingsModule;
            for (char& c : modName) c = std::toupper(c);
            ImGui::TextColored(g_colorAccent, "%s SETTINGS", modName.c_str());

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::SetCursorPosX(20);
            ImGui::BeginChild("SettingsScroll", ImVec2(ImGui::GetWindowWidth() - 32, ws.y - topBarH - 66), false);

            if (g_currentSettingsModule == "renderinfo") RenderInfo::RenderMenu();
            else if (g_currentSettingsModule == "watermark") Watermark::RenderMenu();
            else if (g_currentSettingsModule == "arraylist") ArrayList::RenderMenu();
            else if (g_currentSettingsModule == "keystrokes") Keystrokes::RenderMenu();
            else if (g_currentSettingsModule == "cpscounter") CPSCounter::RenderMenu();
            else if (g_currentSettingsModule == "fpscounter") FPSCounter::RenderMenu();
            else if (g_currentSettingsModule == "motionblur") MotionBlur::RenderMenu();
            else if (g_currentSettingsModule == "unlockfps") UnlockFPS::RenderMenu();
            else if (g_currentSettingsModule == "autosprint") AutoSprint::RenderMenu();

            ImGui::EndChild();

            ImGui::PopStyleVar(); // settingsAlpha
        }

        ImGui::EndChild(); // ContentArea

        // Detect AutoSprint toggle from ModCard and apply Enable/Disable
        static bool s_prevAutoSprint = AutoSprint::g_autoSprintEnabled;
        if (s_prevAutoSprint != AutoSprint::g_autoSprintEnabled) {
            if (AutoSprint::g_autoSprintEnabled) AutoSprint::Enable();
            else AutoSprint::Disable();
            s_prevAutoSprint = AutoSprint::g_autoSprintEnabled;
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

void GUI::DrawShadow(ImDrawList* draw, ImVec2 pos, ImVec2 size, float rounding, float thickness, float opacity) {
    if (opacity <= 0.01f || thickness <= 0.0f) return;
    float offset = thickness * 0.3f;
    draw->AddRectFilled(
        ImVec2(pos.x + offset, pos.y + offset),
        ImVec2(pos.x + size.x + offset, pos.y + size.y + offset),
        ImColor(0, 0, 0, (int)(opacity * 180)), rounding
    );
}

void GUI::AddTextGlow(ImDrawList* draw, ImFont* font, float fontSize, ImVec2 pos, ImU32 col, const char* text, float thickness) {
    ImVec4 colV = ImGui::ColorConvertU32ToFloat4(col);
    for (int i = 1; i <= (int)thickness; i++) {
        float alpha = (0.25f / i);
        draw->AddText(font, fontSize, ImVec2(pos.x, pos.y), ImColor(colV.x, colV.y, colV.z, alpha), text);
        draw->AddText(font, fontSize, ImVec2(pos.x - i * 0.5f, pos.y), ImColor(colV.x, colV.y, colV.z, alpha * 0.5f), text);
        draw->AddText(font, fontSize, ImVec2(pos.x + i * 0.5f, pos.y), ImColor(colV.x, colV.y, colV.z, alpha * 0.5f), text);
    }
    draw->AddText(font, fontSize, pos, col, text);
}

void GUI::RenderCustomSwitch(const char* label, bool* value) {
    ImGui::PushID(label);
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    float height = 18.0f;
    float width = 36.0f;
    float radius = height * 0.5f;

    static std::map<std::string, float> switchAnims;
    std::string key = "switch_" + std::string(label);
    if (switchAnims.find(key) == switchAnims.end()) switchAnims[key] = *value ? 1.0f : 0.0f;

    float target = *value ? 1.0f : 0.0f;
    switchAnims[key] += (target - switchAnims[key]) * 0.22f;
    float anim = switchAnims[key];

    ImGui::InvisibleButton("##switch", ImVec2(width + ImGui::CalcTextSize(label).x + 15, height));
    if (ImGui::IsItemClicked()) *value = !*value;

    ImVec4 bgEmpty = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
    ImU32 col_bg = ImColor(
        bgEmpty.x + (g_colorAccent.x - bgEmpty.x) * anim,
        bgEmpty.y + (g_colorAccent.y - bgEmpty.y) * anim,
        bgEmpty.z + (g_colorAccent.z - bgEmpty.z) * anim, 1.0f
    );

    if (anim > 0.01f) {
        draw->AddRectFilled(ImVec2(p.x - 1, p.y - 1), ImVec2(p.x + width + 1, p.y + height + 1),
            ImColor(g_colorAccent.x, g_colorAccent.y, g_colorAccent.z, anim * 0.12f), radius + 1.0f);
    }
    draw->AddRectFilled(p, ImVec2(p.x + width, p.y + height), col_bg, radius);
    float circle_pos = p.x + radius + anim * (width - radius * 2.0f);
    draw->AddCircleFilled(ImVec2(circle_pos, p.y + radius), radius - 3.0f, IM_COL32_WHITE);

    ImU32 textCol = ImColor(210 + (int)((255 - 210) * anim), 210 + (int)((255 - 210) * anim), 220 + (int)((255 - 220) * anim), 255);
    draw->AddText(ImVec2(p.x + width + 12, p.y + (height - ImGui::GetFontSize()) * 0.5f), textCol, label);
    ImGui::PopID();
}

bool GUI::BeginModuleSettings(const char* label, bool* open) {
    if (!*open) return false;
    ImGui::Spacing();
    ImGui::Indent(15.0f);
    ImGui::BeginGroup();
    ImGui::PushID(label);
    return true;
}

void GUI::EndModuleSettings() {
    ImGui::PopID();
    ImGui::EndGroup();
    ImGui::Unindent(15.0f);
    ImGui::Spacing();
}

bool GUI::HoverButton(const char* label, ImVec2 size, ImVec4 normalCol, ImVec4 hoverCol, ImVec4 activeCol) {
    ImGui::PushStyleColor(ImGuiCol_Button, normalCol);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverCol);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeCol);
    bool pressed = ImGui::Button(label, size);
    ImGui::PopStyleColor(3);
    return pressed;
}

void GUI::RenderModuleCard(const char* name, const char* iconName, bool* enabled, bool* showSettings) {
    float windowWidth = ImGui::GetWindowWidth();
    float spacing = 20.0f; // Spacious gaps matching 25% larger UI
    int columns = 3;
    float cardWidth = (windowWidth - (spacing * (columns + 1))) / columns;
    ImVec2 size(cardWidth, 225.0f); // 25% bigger card height (from 180 to 225)

    // Subtle dark semi-transparent card background matching Lunar Client
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.04f, 0.04f, 0.05f, 0.65f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    std::string childId = std::string("##Card_") + name;
    ImGui::BeginChild(childId.c_str(), size, true, ImGuiWindowFlags_NoScrollbar);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 cardPos = ImGui::GetCursorScreenPos(); // Absolute screen top-left of card (taking child scroll/pos into account)

    // Center icon in the upper portion
    ImTextureID iconTexture = (iconName && g_icons.count(iconName)) ? g_icons[iconName] : (ImTextureID)0;
    if (iconTexture) {
        ImVec2 iconSize(55, 55); // 25% bigger icon (from 44 to 55)
        drawList->AddImage(iconTexture, 
            ImVec2(cardPos.x + (cardWidth - iconSize.x) * 0.5f, cardPos.y + 26),
            ImVec2(cardPos.x + (cardWidth + iconSize.x) * 0.5f, cardPos.y + 26 + iconSize.y),
            ImVec2(0,0), ImVec2(1,1), IM_COL32_WHITE);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.5f, 0.6f, 0.8f));
        ImGui::SetWindowFontScale(1.8f);
        ImVec2 ts = ImGui::CalcTextSize("[O]");
        drawList->AddText(ImVec2(cardPos.x + (cardWidth - ts.x) * 0.5f, cardPos.y + 28), ImGui::GetColorU32(ImGuiCol_Text), "[O]");
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleColor();
    }

    // Centered module name
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.92f, 0.9f));
    ImGui::SetWindowFontScale(1.2f); // Make module names bigger
    ImVec2 textSize = ImGui::CalcTextSize(name);
    drawList->AddText(ImVec2(cardPos.x + (cardWidth - textSize.x) * 0.5f, cardPos.y + 95), ImGui::GetColorU32(ImGuiCol_Text), name);
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    // --- OPTIONS BAR (Flush horizontal segment) ---
    float barY = 135.0f;
    float barHeight = 40.0f; // 25% bigger height (from 34 to 40)
    ImVec2 barMin = ImVec2(cardPos.x, cardPos.y + barY);
    ImVec2 barMax = ImVec2(cardPos.x + cardWidth, cardPos.y + barY + barHeight);
    
    // Animation for OPTIONS Hover
    std::string optKey = "opts_" + std::string(name);
    if (g_elementAnims.find(optKey) == g_elementAnims.end()) g_elementAnims[optKey] = 0.0f;
    bool optHovered = ImGui::IsMouseHoveringRect(barMin, barMax);
    g_elementAnims[optKey] += ((optHovered ? 1.0f : 0.0f) - g_elementAnims[optKey]) * 0.16f;
    float optAnim = g_elementAnims[optKey];

    // Background color based on hover animation
    ImVec4 optBg = ImVec4(0.08f + optAnim * 0.04f, 0.08f + optAnim * 0.04f, 0.10f + optAnim * 0.05f, 0.4f + optAnim * 0.25f);
    drawList->AddRectFilled(barMin, barMax, ImGui::GetColorU32(optBg));
    drawList->AddLine(barMin, ImVec2(barMax.x, barMin.y), ImGui::GetColorU32(ImGuiCol_Border), 1.0f);
    drawList->AddLine(barMax, ImVec2(barMin.x, barMax.y), ImGui::GetColorU32(ImGuiCol_Border), 1.0f);

    // Centered Options Text
    ImVec2 optTextSize = ImGui::CalcTextSize("OPTIONS");
    drawList->AddText(ImVec2(cardPos.x + (cardWidth - optTextSize.x) * 0.5f, cardPos.y + barY + (barHeight - optTextSize.y) * 0.5f), 
        IM_COL32(180, 180, 185, (int)(200 + optAnim * 55)), "OPTIONS");

    // Gear Icon on the right (with smooth rotation/fade)
    ImTextureID gearIcon = g_icons.count("gear") ? g_icons["gear"] : (ImTextureID)0;
    if (gearIcon) {
        ImVec2 gearSize(16, 16);
        drawList->AddImage(gearIcon,
            ImVec2(cardPos.x + cardWidth - 28, cardPos.y + barY + (barHeight - gearSize.y) * 0.5f),
            ImVec2(cardPos.x + cardWidth - 28 + gearSize.x, cardPos.y + barY + (barHeight + gearSize.y) * 0.5f),
            ImVec2(0,0), ImVec2(1,1), IM_COL32(180, 180, 185, (int)(180 + optAnim * 75)));
    }

    // Handle click on Options Bar
    ImGui::SetCursorPos(ImVec2(0, barY));
    if (ImGui::InvisibleButton((std::string("##OptBtn_") + name).c_str(), ImVec2(cardWidth, barHeight))) {
        g_currentSettingsModule = iconName;
    }

    // --- ENABLED / DISABLED BUTTON (Rounded bottom segment) ---
    float btnY = barY + barHeight;
    float btnHeight = 225.0f - btnY; // Covers the exact rest of the card up to bottom
    ImVec2 btnMin = ImVec2(cardPos.x, cardPos.y + btnY);
    ImVec2 btnMax = ImVec2(cardPos.x + cardWidth, cardPos.y + 225.0f);

    // Animation for Toggle Button Hover
    std::string tglKey = "tgl_" + std::string(name);
    if (g_elementAnims.find(tglKey) == g_elementAnims.end()) g_elementAnims[tglKey] = 0.0f;
    bool tglHovered = ImGui::IsMouseHoveringRect(btnMin, btnMax);
    g_elementAnims[tglKey] += ((tglHovered ? 1.0f : 0.0f) - g_elementAnims[tglKey]) * 0.16f;
    float tglAnim = g_elementAnims[tglKey];

    // Soft color shift on hover
    ImVec4 baseColor = *enabled ? ImVec4(0.0f, 0.65f, 0.42f, 1.0f) : ImVec4(0.72f, 0.10f, 0.25f, 1.0f);
    ImVec4 btnColor = baseColor;
    if (tglHovered) {
        btnColor.x = baseColor.x * 1.1f;
        btnColor.y = baseColor.y * 1.1f;
        btnColor.z = baseColor.z * 1.1f;
    }
    
    // Draw beautiful rounded bottom rect flush with card corner radius
    drawList->AddRectFilled(btnMin, btnMax, ImGui::GetColorU32(btnColor), 12.0f, ImDrawFlags_RoundCornersBottom);

    // Draw centered text inside button
    const char* btnText = *enabled ? "E N A B L E D" : "D I S A B L E D";
    ImVec2 btnTextSize = ImGui::CalcTextSize(btnText);
    drawList->AddText(ImVec2(cardPos.x + (cardWidth - btnTextSize.x) * 0.5f, cardPos.y + btnY + (btnHeight - btnTextSize.y) * 0.5f),
        IM_COL32_WHITE, btnText);

    // Handle click on toggle button
    ImGui::SetCursorPos(ImVec2(0, btnY));
    if (ImGui::InvisibleButton((std::string("##TglBtn_") + name).c_str(), ImVec2(cardWidth, btnHeight))) {
        *enabled = !(*enabled);
        SaveCurrentProfile();
    }

    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}

void GUI::ToggleButton(const char* label, bool* v) {
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (window->SkipItems) return;
    ImGuiID id = window->GetID(label);
    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size(60.0f, 28.0f);
    const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
    ImGui::ItemSize(size, 0.0f);
    if (!ImGui::ItemAdd(bb, id)) return;
    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
    if (pressed) *v = !(*v);
    float t = *v ? 1.0f : 0.0f;
    static std::map<ImGuiID, float> anims;
    float& anim = anims[id];
    anim = Animations::Lerp(anim, t, 0.15f);
    ImU32 col_bg = ImGui::GetColorU32(Animations::Lerp(ImVec4(0.15f, 0.15f, 0.15f, 1.0f), ImVec4(g_colorAccent.x, g_colorAccent.y, g_colorAccent.z, 1.0f), anim));
    window->DrawList->AddRectFilled(bb.Min, bb.Max, col_bg, size.y / 2.0f);
    window->DrawList->AddCircleFilled(ImVec2(bb.Min.x + size.y / 2.0f + anim * (size.x - size.y), bb.Min.y + size.y / 2.0f), size.y / 2.0f - 3.0f, IM_COL32_WHITE);
    const char* label_end = ImGui::FindRenderedTextEnd(label);
    if (label != label_end) {
        ImGui::SameLine(0.0f, 10.0f);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (size.y - ImGui::GetTextLineHeight()) / 2.0f);
        ImGui::TextUnformatted(label, label_end);
    }
}

void GUI::RenderNotification(float screenWidth, float screenHeight) {
    static ULONGLONG startTime = GetTickCount64();
    ULONGLONG now = GetTickCount64();
    float elapsed = (float)(now - startTime) / 1000.0f;
    if (elapsed > 8.0f) return;

    float alpha = 1.0f;
    if (elapsed < 0.5f) alpha = elapsed / 0.5f;
    else if (elapsed > 7.5f) alpha = 1.0f - (elapsed - 7.5f) / 0.5f;

    ImGui::SetNextWindowPos(ImVec2(screenWidth / 2.0f, 40.0f), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 15));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.06f, 0.85f * alpha));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(g_colorAccent.x, g_colorAccent.y, g_colorAccent.z, 0.4f * alpha));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);

    if (ImGui::Begin("##WelcomeNotif", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoFocusOnAppearing)) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, alpha));
        ImGui::Text("Welcome to Amatayakul Client");
        ImGui::SetWindowFontScale(0.9f);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2);
        ImGui::TextColored(ImVec4(g_colorAccent.x, g_colorAccent.y, g_colorAccent.z, alpha), "Press RIGHT SHIFT to open mod menu");
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleColor();
        ImVec2 pMin = ImGui::GetWindowPos();
        ImVec2 pMax = ImVec2(pMin.x + ImGui::GetWindowWidth(), pMin.y + ImGui::GetWindowHeight());
        ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(pMin.x, pMax.y - 3), pMax, ImGui::GetColorU32(ImVec4(g_colorAccent.x, g_colorAccent.y, g_colorAccent.z, alpha)), 10.0f, ImDrawFlags_RoundCornersBottom);
    }
    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
}

// Profile management
std::string GUI::GetProfilePath(const std::string& name) {
    ConfigManager::Initialize();
    return ConfigManager::GetConfigDir() + name + ".json";
}

void GUI::LoadProfiles() {
    g_profiles.clear();
    g_profiles.push_back("Default");
    g_selectedProfile = 0;

    auto configs = ConfigManager::ListConfigs();
    for (auto& c : configs) {
        if (c != "default" && c != "Default") {
            g_profiles.push_back(c);
        }
    }
}

void GUI::SaveCurrentProfile() {
    if (g_selectedProfile < 0 || g_selectedProfile >= (int)g_profiles.size()) return;
    ConfigManager::SaveConfig(g_profiles[g_selectedProfile]);
}

void GUI::SwitchProfile(int index) {
    if (index < 0 || index >= (int)g_profiles.size()) return;
    g_selectedProfile = index;
    ConfigManager::LoadConfig(g_profiles[index]);
}

void GUI::AddProfile(const char* name) {
    if (!name || name[0] == '\0') return;
    g_profiles.push_back(name);
    g_selectedProfile = (int)g_profiles.size() - 1;
    SaveCurrentProfile();
}

void GUI::DeleteProfile(int index) {
    if (index <= 0 || index >= (int)g_profiles.size()) return;
    ConfigManager::DeleteConfig(g_profiles[index]);
    g_profiles.erase(g_profiles.begin() + index);
    if (g_selectedProfile >= (int)g_profiles.size()) {
        g_selectedProfile = (int)g_profiles.size() - 1;
    }
}

void GUI::RenameProfile(int index, const char* newName) {
    if (index < 0 || index >= (int)g_profiles.size()) return;
    if (!newName || newName[0] == '\0') return;
    std::string oldName = g_profiles[index];
    // Delete old config file, save new one
    ConfigManager::DeleteConfig(oldName);
    g_profiles[index] = newName;
    ConfigManager::SaveConfig(newName);
    if (g_selectedProfile == index) {
        g_selectedProfile = index;
    }
}
