/*
Under an4rch Development Public Source License 1.0
*/

#include "Splash.hpp"
#include "../Globals.hpp"
#include "../../GUI/GUI.hpp"
#include "../../ImGui/imgui.h"
#include "../../Animations/Animations.hpp"
#include "../../Assets/resource.h"
#include "../../Assets/stb/stb_image.h"

#include <d3d11.h>
#include <windows.h>
#include <cmath>

extern ID3D11Device* pDevice;
extern HMODULE g_hModule;

namespace {
    ID3D11ShaderResourceView* s_bgSrv = nullptr;
    ID3D11ShaderResourceView* s_srv = nullptr;
    int   s_bgW = 0, s_bgH = 0;
    int   s_texW = 0, s_texH = 0;

    unsigned long long s_start = 0;
    bool  s_active = false;
    float s_alpha = 0.0f;

    const float kTotalDur = 2.5f;  // 1.2 + 0.3 + 0.6 + 0.4 (fade)

    void LoadTex(int resId, ID3D11ShaderResourceView** outSrv, int* outW, int* outH) {
        if (*outSrv) return;
        HRSRC hRes = FindResource(g_hModule, MAKEINTRESOURCE(resId), RT_RCDATA);
        if (!hRes) return;
        HGLOBAL hGlobal = LoadResource(g_hModule, hRes);
        if (!hGlobal) return;
        void* pData = LockResource(hGlobal);
        DWORD size  = SizeofResource(g_hModule, hRes);
        int w = 0, h = 0, channels = 0;
        unsigned char* px = stbi_load_from_memory((unsigned char*)pData, (int)size, &w, &h, &channels, 4);
        if (!px) return;

        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = w;
        desc.Height = h;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA sub = {};
        sub.pSysMem = px;
        sub.SysMemPitch = w * 4;

        ID3D11Texture2D* pTex = nullptr;
        if (SUCCEEDED(pDevice->CreateTexture2D(&desc, &sub, &pTex))) {
            if (SUCCEEDED(pDevice->CreateShaderResourceView(pTex, nullptr, outSrv))) {
                *outW = w; *outH = h;
            }
            pTex->Release();
        }
        stbi_image_free(px);
    }
}

void Splash::Initialize() {
    if (!pDevice) return;
    LoadTex(IDR_SPLASH_BG, &s_bgSrv, &s_bgW, &s_bgH);
    LoadTex(IDR_SPLASH_LOGO, &s_srv, &s_texW, &s_texH);
}

void Splash::Begin() {
    Initialize();
    s_start = GetTickCount64();
    s_alpha = 0.0f;
    s_active = true;
}

bool Splash::IsActive() {
    return s_active;
}

void Splash::Update(unsigned long long now) {
    if (!s_active) return;

    float t = (float)(now - s_start) / 1000.0f;

    // Fade in over 0.3s
    if (t < 0.3f)
        s_alpha = Animations::Clamp01(t / 0.3f);
    else if (t > kTotalDur - 0.4f)
        s_alpha = Animations::Clamp01(1.0f - ((t - (kTotalDur - 0.4f)) / 0.4f));
    else
        s_alpha = 1.0f;

    if (t >= kTotalDur) {
        s_active = false;
        s_alpha = 0.0f;
        // Fire the welcome notification now that the splash is done
        extern ULONGLONG g_notifStart;
        g_notifStart = GetTickCount64();
    }
}

void Splash::Render(float sw, float sh) {
    if (!s_active || s_alpha <= 0.01f) return;

    float t = (float)(GetTickCount64() - s_start) / 1000.0f;
    ImDrawList* draw = ImGui::GetForegroundDrawList();

    // ── Fullscreen Background ──
    if (s_bgSrv) {
        draw->AddImage((ImTextureID)s_bgSrv, ImVec2(0, 0), ImVec2(sw, sh), ImVec2(0,0), ImVec2(1,1), IM_COL32(255, 255, 255, (int)(255 * s_alpha)));
    } else {
        draw->AddRectFilled(ImVec2(0, 0), ImVec2(sw, sh), IM_COL32(10, 10, 15, (int)(255 * s_alpha)));
    }

    // ── Centered logo ──
    if (s_srv) {
        float logoH = 300.0f;
        float logoW = logoH * (float)s_texW / (float)s_texH;
        if (logoW > sw * 0.45f) {
            logoW = sw * 0.45f;
            logoH = logoW * (float)s_texH / (float)s_texW;
        }

        float lx = (sw - logoW) * 0.5f;
        float ly = (sh - logoH) * 0.5f - 40.0f;

        draw->AddImage((ImTextureID)s_srv,
            ImVec2(lx, ly), ImVec2(lx + logoW, ly + logoH),
            ImVec2(0, 0), ImVec2(1, 1),
            IM_COL32(255, 255, 255, (int)(255 * s_alpha)));
    }

    // ── Text below logo ──
    {
        ImFont* font = ImGui::GetIO().Fonts->Fonts[0];
        float fontSize = ImGui::GetFontSize() * 1.1f;
        float textY = (sh * 0.5f) + 165.0f;

        const char* activeLine = nullptr;

        if (t < 1.2f) {
            activeLine = "Loading KittyDLL assets...";
        } else if (t < 1.5f) {
            activeLine = "Finished.";
        } else if (t < 2.1f) {
            activeLine = "Welcome to Amatayakul Client!";
        }

        if (activeLine) {
            ImVec2 ts = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, activeLine);
            ImVec2 pos((sw - ts.x) * 0.5f, textY);

            // Shadow
            draw->AddText(font, fontSize, ImVec2(pos.x + 2.0f, pos.y + 2.0f),
                IM_COL32(0, 0, 0, (int)(150 * s_alpha)), activeLine);
            // Main text
            draw->AddText(font, fontSize, pos,
                IM_COL32(230, 230, 235, (int)(255 * s_alpha)), activeLine);
        }
    }
}

void Splash::Shutdown() {
    if (s_bgSrv) {
        s_bgSrv->Release();
        s_bgSrv = nullptr;
    }
    if (s_srv) {
        s_srv->Release();
        s_srv = nullptr;
    }
    s_active = false;
    s_alpha = 0.0f;
}
