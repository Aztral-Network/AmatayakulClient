#include "MotionBlur.hpp"
#include "../../../ImGui/imgui.h"
#include "../../../GUI/GUI.hpp"
#include "../../../Animations/Animations.hpp"
#include "../../../Animations/Animations.hpp"
#include <cstdio>
#include <cmath>

// Static member initialization
bool MotionBlur::g_motionBlurEnabled = false;
float MotionBlur::g_motionBlurAnim = 0.0f;
ULONGLONG MotionBlur::g_motionBlurEnableTime = 0;
ULONGLONG MotionBlur::g_motionBlurDisableTime = 0;
float MotionBlur::g_blurIntensity = 3.0f;
float MotionBlur::g_maxHistoryFrames = 8.0f;
float MotionBlur::g_blurTimeConstant = 0.0667f;
bool MotionBlur::g_blurDynamicMode = false;
std::string MotionBlur::g_blurType = "Average Pixel Blur";
std::vector<ID3D11ShaderResourceView*> MotionBlur::g_previousFrames;
std::vector<float> MotionBlur::g_frameTimestamps;

void MotionBlur::Initialize() {
}

void MotionBlur::UpdateAnimation(ULONGLONG now) {
    if (g_motionBlurEnabled && g_motionBlurEnableTime == 0) {
        g_motionBlurEnableTime = now;
        g_motionBlurDisableTime = 0;
    }
    if (!g_motionBlurEnabled && g_motionBlurDisableTime == 0 && g_motionBlurEnableTime > 0) {
        g_motionBlurDisableTime = now;
        g_motionBlurEnableTime = 0;
    }

    if (g_motionBlurEnableTime > 0) {
        float enableElapsed = (float)(now - g_motionBlurEnableTime) / 1000.0f;
        g_motionBlurAnim = fminf(1.0f, enableElapsed / 0.3f);
    }
    else if (g_motionBlurDisableTime > 0) {
        float disableElapsed = (float)(now - g_motionBlurDisableTime) / 1000.0f;
        float disableAnim = fminf(1.0f, disableElapsed / 0.2f);
        g_motionBlurAnim = 1.0f - disableAnim;
        if (disableAnim >= 1.0f) {
            g_motionBlurEnableTime = 0;
            g_motionBlurDisableTime = 0;
        }
    }
}

void MotionBlur::InitializeBackbufferStorage(int maxFrames) {
    // Initialize storage for motion blur frames
    // Clean up old frames if needed
    while ((int)g_previousFrames.size() >= maxFrames) {
        if (g_previousFrames[0]) {
            g_previousFrames[0]->Release();
        }
        g_previousFrames.erase(g_previousFrames.begin());
        if (!g_frameTimestamps.empty()) {
            g_frameTimestamps.erase(g_frameTimestamps.begin());
        }
    }
}

ID3D11ShaderResourceView* MotionBlur::CopyBackbufferToSRV(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, IDXGISwapChain* pSwapChain) {
    if (!pSwapChain || !pDevice || !pContext) return nullptr;

    ID3D11Texture2D* pBackBuffer = nullptr;
    if (FAILED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer))) {
        return nullptr;
    }

    D3D11_TEXTURE2D_DESC desc;
    pBackBuffer->GetDesc(&desc);

    D3D11_TEXTURE2D_DESC texDesc = desc;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    texDesc.CPUAccessFlags = 0;

    ID3D11Texture2D* pTexture = nullptr;
    if (FAILED(pDevice->CreateTexture2D(&texDesc, nullptr, &pTexture))) {
        pBackBuffer->Release();
        return nullptr;
    }

    pContext->CopyResource(pTexture, pBackBuffer);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;

    ID3D11ShaderResourceView* pSRV = nullptr;
    if (FAILED(pDevice->CreateShaderResourceView(pTexture, &srvDesc, &pSRV))) {
        pTexture->Release();
        pBackBuffer->Release();
        return nullptr;
    }

    pTexture->Release();
    pBackBuffer->Release();

    return pSRV;
}

void MotionBlur::CleanupBackbufferStorage() {
    for (auto frame : g_previousFrames) {
        if (frame) frame->Release();
    }
    g_previousFrames.clear();
    g_frameTimestamps.clear();
}

void MotionBlur::RenderMotionBlur(ImDrawList* draw, ImVec2 screenSize) {
    if (!g_motionBlurEnabled || g_motionBlurAnim <= 0.01f || g_previousFrames.empty()) return;

    float currentTime = (float)GetTickCount64() / 1000.0f;
    bool isMenuOpen = false;
    extern bool g_showMenu;
    isMenuOpen = g_showMenu;

    if (g_blurType == "Average Pixel Blur") {
        float alpha = 0.25f;
        float bleedFactor = 0.95f;
        for (const auto& frame : g_previousFrames) {
            if (frame) {
                ImU32 col = IM_COL32(255, 255, 255, (int)(alpha * g_motionBlurAnim * 255.0f));
                draw->AddImage((ImTextureID)frame, ImVec2(0, 0), screenSize, ImVec2(0, 0), ImVec2(1, 1), col);
                alpha *= bleedFactor;
            }
        }
    }
    else if (g_blurType == "Ghost Frames") {
        float alpha = 0.30f;
        float bleedFactor = 0.80f;
        for (const auto& frame : g_previousFrames) {
            if (frame) {
                ImU32 col = IM_COL32(255, 255, 255, (int)(alpha * g_motionBlurAnim * 255.0f));
                draw->AddImage((ImTextureID)frame, ImVec2(0, 0), screenSize, ImVec2(0, 0), ImVec2(1, 1), col);
                alpha *= bleedFactor;
            }
        }
    }
    else if (g_blurType == "Time Aware Blur") {
        float T = g_blurTimeConstant;
        std::vector<float> weights;
        float totalWeight = 0.0f;

        for (size_t i = 0; i < g_previousFrames.size(); i++) {
            float age = currentTime - g_frameTimestamps[i];
            float weight = expf(-age / T);
            weights.push_back(weight);
            totalWeight += weight;
        }

        if (totalWeight > 0.0f) {
            for (float& w : weights) {
                w /= totalWeight;
            }
        }

        for (size_t i = 0; i < g_previousFrames.size(); i++) {
            if (g_previousFrames[i] && weights[i] > 0.001f) {
                ImU32 col = IM_COL32(255, 255, 255, (int)(weights[i] * g_motionBlurAnim * 255.0f));
                draw->AddImage((ImTextureID)g_previousFrames[i], ImVec2(0, 0), screenSize, ImVec2(0, 0), ImVec2(1, 1), col);
            }
        }
    }
    else if (g_blurType == "Real Motion Blur") {
        float alpha = 0.35f;
        float bleedFactor = 0.85f;
        for (const auto& frame : g_previousFrames) {
            if (frame) {
                ImU32 col = IM_COL32(255, 255, 255, (int)(alpha * g_motionBlurAnim * 255.0f));
                draw->AddImage((ImTextureID)frame, ImVec2(0, 0), screenSize, ImVec2(0, 0), ImVec2(1, 1), col);
                alpha *= bleedFactor;
            }
        }
    }
    else {
        float alpha = 0.35f;
        float bleedFactor = 0.85f;
        for (const auto& frame : g_previousFrames) {
            if (frame) {
                ImU32 col = IM_COL32(255, 255, 255, (int)(alpha * g_motionBlurAnim * 255.0f));
                draw->AddImage((ImTextureID)frame, ImVec2(0, 0), screenSize, ImVec2(0, 0), ImVec2(1, 1), col);
                alpha *= bleedFactor;
            }
        }
    }
}

void MotionBlur::RenderArrayList(ImDrawList* draw, ImVec2 arrayListStart, float& yPos, ImVec2& arrayListEnd) {
    if (g_motionBlurEnabled || (g_motionBlurDisableTime > 0 && g_motionBlurAnim > 0.01f)) {
        float motionBlurAlpha = g_motionBlurAnim * 255.0f;
        float slideOffset = -60.0f + (Animations::SmoothInertia(g_motionBlurAnim) * 60.0f);

        if (motionBlurAlpha > 1.0f) {
            char mbBuf[64];
            sprintf_s(mbBuf, "Motion Blur");
            float wMB = ImGui::CalcTextSize(mbBuf).x;
            float xPosMB = arrayListStart.x + 290.0f - wMB - 10;
            draw->AddText(ImVec2(xPosMB + slideOffset - 1, yPos + 1), IM_COL32(0, 0, 0, 220), mbBuf);
            draw->AddText(ImVec2(xPosMB + slideOffset, yPos), IM_COL32(100, 255, 150, (int)motionBlurAlpha), mbBuf);
            yPos += 18.0f;
            arrayListEnd.y = yPos;
        }
    }
}

void MotionBlur::RenderMenu() {
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.2f, 1.0f), "Unstable Feature - May Cause Performance Issues\nMotion Blur may cause performance issues on lower-end systems");
    GUI::RenderCustomSwitch("Motion Blur", &g_motionBlurEnabled);

    if (GUI::BeginModuleSettings("Motion Blur", &g_motionBlurEnabled)) {
        static int blurTypeIndex = 0;
        static const char* blurTypes[] = {
            "Average Pixel Blur",
            "Ghost Frames",
            "Time Aware Blur",
            "Real Motion Blur",
            "V4"
        };
        if (ImGui::Combo("Blur Type##MB", &blurTypeIndex, blurTypes, IM_ARRAYSIZE(blurTypes))) {
            g_blurType = blurTypes[blurTypeIndex];
        }

        if (g_blurType == "Time Aware Blur") {
            ImGui::SliderFloat("Blur Time Constant##MB", &g_blurTimeConstant, 0.01f, 0.2f, "%.4f");
            ImGui::SliderFloat("Max History Frames##MB", &g_maxHistoryFrames, 4.0f, 16.0f, "%.0f");
        } else {
            ImGui::SliderFloat("Intensity##MB", &g_blurIntensity, 1.0f, 30.0f, "%.0f");
        }

        if (g_blurType == "Average Pixel Blur") {
            GUI::RenderCustomSwitch("Dynamic Mode##MB", &g_blurDynamicMode);
            if (g_blurDynamicMode) {
                ImGui::TextDisabled("Adjusts intensity based on FPS");
            }
        }

        GUI::EndModuleSettings();
    }
}
