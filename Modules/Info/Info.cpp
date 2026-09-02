/*
Under an4rch Development Public Source License 1.0
*/

#include "Info.hpp"
#include "../../GUI/GUI.hpp"
#include "../../ImGui/imgui.h"
#include "../../ImGui/imgui-markdown/imgui-markdown.h"
#include "../Globals.hpp"
#include "../Misc/UnlockFPS/UnlockFPS.hpp"
#include "../Visuals/RenderInfo/RenderInfo.hpp"
#include "../../Assets/resource.h"
#include "../../Assets/stb/stb_image.h"
#include "../../nlohmann/json.hpp"
#include <d3d11.h>
#include <windows.h>
#include <winhttp.h>
#include <cmath>
#include <cfloat>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <shellapi.h>
#include <ctime>
#include <string>
#include <mutex>

#pragma comment(lib, "winhttp.lib")

extern "C" {
#define MINIAUDIO_IMPLEMENTATION
#pragma warning(push)
#pragma warning(disable: 4244)
#include "../../miniaudio/miniaudio.h"
#pragma warning(pop)
}

// External globals from dllmain.cpp
extern ID3D11Device* pDevice;
extern ID3D11DeviceContext* pContext;
extern HMODULE g_hModule;

// External globals from GUI.cpp
extern char g_notifTitle[64];
extern char g_notifMessage[128];

// Static member initialization
ImTextureID Info::g_logoTexture = 0;
int Info::g_logoWidth = 0;
int Info::g_logoHeight = 0;
ImTextureID Info::g_discordIcon = 0;
int Info::g_discordIconW = 0;
int Info::g_discordIconH = 0;
ImTextureID Info::g_githubIcon = 0;
int Info::g_githubIconW = 0;
int Info::g_githubIconH = 0;
uint8_t* Info::g_audioData = nullptr;
uint32_t Info::g_audioDataSize = 0;

// GitHub release statics
std::string Info::g_releaseBody = "";
std::string Info::g_releaseTag = "";
std::string Info::g_releaseName = "";
std::string Info::g_releaseDate = "";
bool Info::g_fetchInProgress = false;
bool Info::g_fetchDone = false;
bool Info::g_fetchFailed = false;
bool Info::g_showReleaseModal = false;
float Info::g_releaseModalAnim = 0.0f;
static std::mutex g_releaseMutex;

// Audio context global
static const int NUM_CLICK_SOUNDS = 3;
static ma_engine g_audioEngine;
static bool g_audioEngineInitialized = false;

// Arrays for multiple sounds
static ma_decoder g_audioDecoders[NUM_CLICK_SOUNDS];
static ma_sound g_clickSounds[NUM_CLICK_SOUNDS];
static uint8_t* g_audioDatas[NUM_CLICK_SOUNDS] = {nullptr};
static uint32_t g_audioSizes[NUM_CLICK_SOUNDS] = {0};
static bool g_soundsLoaded = false;
static unsigned int g_soundCount = 0;

// Forward declaration
static void LoadIconFromResource(int resourceID, ImTextureID* outTex, int* outW = nullptr, int* outH = nullptr);

void Info::Initialize() {
    if (g_audioEngineInitialized) return;
    
    LoadLogoFromResource();
    LoadAudioFromResource();
    LoadIconFromResource(IDR_ICON_DISCORD, &g_discordIcon, &g_discordIconW, &g_discordIconH);
    LoadIconFromResource(IDR_ICON_GITHUB, &g_githubIcon, &g_githubIconW, &g_githubIconH);
    
    // Initialize miniaudio engine
    if (ma_engine_init(NULL, &g_audioEngine) == MA_SUCCESS) {
        g_audioEngineInitialized = true;
        // Once engine is ready, initialize sound from memory
        InitSoundFromMemory();
    }
}

void Info::Shutdown() {
    if (g_soundsLoaded) {
        // Uninitialize all sounds and decoders
        for (int i = 0; i < NUM_CLICK_SOUNDS; i++) {
            ma_sound_uninit(&g_clickSounds[i]);
            ma_decoder_uninit(&g_audioDecoders[i]);
        }
        g_soundsLoaded = false;
        g_soundCount = 0;
    }
    
    if (g_audioEngineInitialized) {
        ma_engine_uninit(&g_audioEngine);
        g_audioEngineInitialized = false;
    }
    
    // Free all audio data
    for (int i = 0; i < NUM_CLICK_SOUNDS; i++) {
        if (g_audioDatas[i]) {
            delete[] g_audioDatas[i];
            g_audioDatas[i] = nullptr;
            g_audioSizes[i] = 0;
        }
    }
    
    auto ReleaseTex = [](ImTextureID& tex) {
        if (tex != 0) { reinterpret_cast<ID3D11ShaderResourceView*>(tex)->Release(); tex = 0; }
    };
    ReleaseTex(g_logoTexture);
    ReleaseTex(g_discordIcon);
    ReleaseTex(g_githubIcon);
}

void Info::OnFocusGained() {
    if (!g_audioEngineInitialized) return;
    ma_engine_start(&g_audioEngine);
    ma_device* pDev = ma_engine_get_device(&g_audioEngine);
    if (pDev) ma_device_start(pDev);
}

bool Info::LoadLogoFromResource() {
    // Find the resource using the module handle
    HRSRC hRes = FindResource(g_hModule, MAKEINTRESOURCE(IDR_ICON_LOGO), RT_RCDATA);
    if (!hRes) {
        OutputDebugStringA("INFO: FindResource failed for IDR_ICON_LOGO\n");
        return false;
    }
    
    // Load the resource
    HGLOBAL hGlobal = LoadResource(g_hModule, hRes);
    if (!hGlobal) {
        OutputDebugStringA("INFO: LoadResource failed\n");
        return false;
    }
    
    // Get resource data
    DWORD dwSize = SizeofResource(g_hModule, hRes);
    if (dwSize == 0) {
        OutputDebugStringA("INFO: SizeofResource returned 0\n");
        return false;
    }
    
    void* pData = LockResource(hGlobal);
    if (!pData) {
        OutputDebugStringA("INFO: LockResource failed\n");
        return false;
    }
    
    // Load texture from memory
    g_logoTexture = LoadTextureFromMemory((const unsigned char*)pData, (int)dwSize);
    
    return g_logoTexture != 0;
}

static void LoadIconFromResource(int resourceID, ImTextureID* outTex, int* outW, int* outH) {
    if (!pDevice || !outTex) return;
    HRSRC hRes = FindResource(g_hModule, MAKEINTRESOURCE(resourceID), RT_RCDATA);
    if (!hRes) return;
    HGLOBAL hGlobal = LoadResource(g_hModule, hRes);
    if (!hGlobal) return;
    DWORD dwSize = SizeofResource(g_hModule, hRes);
    if (dwSize == 0) return;
    void* pData = LockResource(hGlobal);
    if (!pData) return;

    int w, h, ch;
    unsigned char* img = stbi_load_from_memory((const unsigned char*)pData, (int)dwSize, &w, &h, &ch, 4);
    if (!img) return;

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
    sub.pSysMem = img;
    sub.SysMemPitch = w * 4;

    ID3D11Texture2D* tex = nullptr;
    if (FAILED(pDevice->CreateTexture2D(&desc, &sub, &tex))) { stbi_image_free(img); return; }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    ID3D11ShaderResourceView* srv = nullptr;
    HRESULT hr = pDevice->CreateShaderResourceView(tex, &srvDesc, &srv);
    tex->Release();
    stbi_image_free(img);
    if (FAILED(hr) || !srv) return;
    *outTex = reinterpret_cast<ImTextureID>(srv);
    if (outW) *outW = w;
    if (outH) *outH = h;
}
ImTextureID Info::LoadTextureFromMemory(const unsigned char* data, int size) {
    // Load image from memory using stb_image
    int width, height, channels;
    unsigned char* img_data = stbi_load_from_memory(data, size, &width, &height, &channels, 4);
    if (!img_data) {
        const char* err = stbi_failure_reason();
        OutputDebugStringA("INFO: stbi_load_from_memory failed: ");
        OutputDebugStringA(err ? err : "unknown error\n");
        return 0;
    }
    
    g_logoWidth = width;
    g_logoHeight = height;
    
    // Check if device is available
    if (!pDevice) {
        stbi_image_free(img_data);
        return 0;
    }
    
    // Create texture description
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    
    D3D11_SUBRESOURCE_DATA subResource = {};
    subResource.pSysMem = img_data;
    subResource.SysMemPitch = width * 4;
    
    ID3D11Texture2D* pTexture = nullptr;
    HRESULT hr = pDevice->CreateTexture2D(&desc, &subResource, &pTexture);
    
    if (FAILED(hr) || !pTexture) {
        stbi_image_free(img_data);
        return 0;
    }
    
    // Create shader resource view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;
    
    ID3D11ShaderResourceView* pSRV = nullptr;
    hr = pDevice->CreateShaderResourceView(pTexture, &srvDesc, &pSRV);
    
    pTexture->Release();
    stbi_image_free(img_data);
    
    if (FAILED(hr) || !pSRV) {
        return 0;
    }
    
    return reinterpret_cast<ImTextureID>(pSRV);
}

// Helper: Load single audio from resource by ID
static bool LoadAudioByID(int resourceID, uint8_t*& outData, uint32_t& outSize) {
    HRSRC hRes = FindResource(g_hModule, MAKEINTRESOURCE(resourceID), RT_RCDATA);
    if (!hRes) {
        OutputDebugStringA("INFO: FindResource failed for audio resource\n");
        return false;
    }
    
    HGLOBAL hGlobal = LoadResource(g_hModule, hRes);
    if (!hGlobal) {
        OutputDebugStringA("INFO: LoadResource failed\n");
        return false;
    }
    
    DWORD dwSize = SizeofResource(g_hModule, hRes);
    if (dwSize == 0) {
        OutputDebugStringA("INFO: SizeofResource returned 0\n");
        return false;
    }
    
    void* pData = LockResource(hGlobal);
    if (!pData) {
        OutputDebugStringA("INFO: LockResource failed\n");
        return false;
    }
    
    outData = new uint8_t[dwSize];
    if (!outData) {
        OutputDebugStringA("INFO: Failed to allocate audio memory\n");
        return false;
    }
    
    std::memcpy(outData, pData, dwSize);
    outSize = dwSize;
    
    return true;
}

bool Info::LoadAudioFromResource() {
    // Load all 3 click sounds from resources
    int resourceIDs[NUM_CLICK_SOUNDS] = {IDR_CLICK_SOUND, IDR_CLICK_SOUND_1, IDR_CLICK_SOUND_2};
    int successCount = 0;
    
    for (int i = 0; i < NUM_CLICK_SOUNDS; i++) {
        char debugMsg[256];
        sprintf(debugMsg, "INFO: Attempting to load audio resource %d (ID: %d)\n", i, resourceIDs[i]);
        OutputDebugStringA(debugMsg);
        
        if (LoadAudioByID(resourceIDs[i], g_audioDatas[i], g_audioSizes[i])) {
            sprintf(debugMsg, "INFO: Audio %d loaded successfully, size: %d bytes\n", i, g_audioSizes[i]);
            OutputDebugStringA(debugMsg);
            successCount++;
        } else {
            sprintf(debugMsg, "INFO: Failed to load audio resource %d\n", i);
            OutputDebugStringA(debugMsg);
        }
    }
    
    char finalMsg[256];
    sprintf(finalMsg, "INFO: Loaded %d/%d click sounds\n", successCount, NUM_CLICK_SOUNDS);
    OutputDebugStringA(finalMsg);
    
    return successCount > 0;
}

void Info::InitSoundFromMemory() {
    if (!g_audioEngineInitialized) {
        OutputDebugStringA("INFO: Cannot init sounds - engine not initialized\n");
        return;
    }
    
    char debugMsg[256];
    sprintf(debugMsg, "INFO: InitSoundFromMemory started, checking %d audio slots\n", NUM_CLICK_SOUNDS);
    OutputDebugStringA(debugMsg);
    
    // Initialize decoder and sound for each audio
    for (int i = 0; i < NUM_CLICK_SOUNDS; i++) {
        if (!g_audioDatas[i] || g_audioSizes[i] == 0) {
            sprintf(debugMsg, "INFO: Slot %d - NO DATA (data=%p, size=%d)\n", i, g_audioDatas[i], g_audioSizes[i]);
            OutputDebugStringA(debugMsg);
            continue;
        }
        
        sprintf(debugMsg, "INFO: Slot %d - data=%p, size=%d\n", i, g_audioDatas[i], g_audioSizes[i]);
        OutputDebugStringA(debugMsg);
        
        // Initialize decoder from memory
        ma_result result = ma_decoder_init_memory(
            g_audioDatas[i],
            g_audioSizes[i],
            NULL,
            &g_audioDecoders[i]
        );
        
        if (result != MA_SUCCESS) {
            sprintf(debugMsg, "INFO: Slot %d - Decoder init FAILED (result: %d)\n", i, result);
            OutputDebugStringA(debugMsg);
            continue;
        }
        
        sprintf(debugMsg, "INFO: Slot %d - Decoder init SUCCESS\n", i);
        OutputDebugStringA(debugMsg);
        
        // Initialize sound from decoder
        result = ma_sound_init_from_data_source(
            &g_audioEngine,
            &g_audioDecoders[i],
            0,
            NULL,
            &g_clickSounds[i]
        );
        
        if (result != MA_SUCCESS) {
            sprintf(debugMsg, "INFO: Slot %d - Sound init FAILED (result: %d)\n", i, result);
            OutputDebugStringA(debugMsg);
            ma_decoder_uninit(&g_audioDecoders[i]);
            continue;
        }
        
        sprintf(debugMsg, "INFO: Slot %d - Sound init SUCCESS, count++\n", i);
        OutputDebugStringA(debugMsg);
        g_soundCount++;
    }
    
    sprintf(debugMsg, "INFO: InitSoundFromMemory finished - g_soundCount = %d\n", g_soundCount);
    OutputDebugStringA(debugMsg);
    
    if (g_soundCount > 0) {
        g_soundsLoaded = true;
        OutputDebugStringA("INFO: Click sounds loaded from memory successfully\n");
    } else {
        OutputDebugStringA("INFO: WARNING - No sounds loaded!\n");
    }
}

void Info::PlayClickSound() {
    if (!g_soundsLoaded || g_soundCount == 0) {
        return;
    }

    // Select a random sound
    int randomIndex = rand() % g_soundCount;

    // Check engine state
    if (ma_engine_start(&g_audioEngine) != MA_SUCCESS) {
        // If starting fails, try to start the device directly
        ma_device* pAudioDevice = ma_engine_get_device(&g_audioEngine);
        if (pAudioDevice) {
            ma_device_start(pAudioDevice);
        }
    }
    
    // Stop and Reset
    ma_sound_stop(&g_clickSounds[randomIndex]);
    ma_sound_seek_to_pcm_frame(&g_clickSounds[randomIndex], 0);
    
    // Attempt play
    ma_result result = ma_sound_start(&g_clickSounds[randomIndex]);
    
    // If still failing (common after Fullscreen Alt-Tab device loss), 
    // try one last hardware-level kick
    if (result != MA_SUCCESS) {
        ma_device* pAudioDevice = ma_engine_get_device(&g_audioEngine);
        if (pAudioDevice) {
            ma_device_stop(pAudioDevice);
            ma_device_start(pAudioDevice);
            ma_sound_start(&g_clickSounds[randomIndex]);
        }
    }
}

static std::string GetGitHubTokenFromResource() {
    HRSRC hRes = FindResource(g_hModule, MAKEINTRESOURCE(IDR_GITHUB_INI), RT_RCDATA);
    if (!hRes) return "";
    
    HGLOBAL hGlobal = LoadResource(g_hModule, hRes);
    if (!hGlobal) return "";
    
    DWORD dwSize = SizeofResource(g_hModule, hRes);
    if (dwSize == 0) return "";
    
    const char* pData = (const char*)LockResource(hGlobal);
    if (!pData) return "";
    
    std::string content(pData, dwSize);
    size_t tokenPos = content.find("token=");
    if (tokenPos == std::string::npos) return "";
    
    tokenPos += 6; // skip "token="
    size_t endPos = content.find_first_of("\r\n\t;", tokenPos);
    if (endPos == std::string::npos) {
        return content.substr(tokenPos);
    }
    return content.substr(tokenPos, endPos - tokenPos);
}

void Info::FetchLatestRelease() {
    if (g_fetchInProgress) return;
    g_fetchInProgress = true;
    g_fetchDone = false;
    g_fetchFailed = false;

    // Fire off a background thread — WinHTTP is safe off the render thread
    CreateThread(nullptr, 0, [](LPVOID) -> DWORD {
        std::string result;
        std::string tag, name, date;
        bool failed = false;

        HINTERNET hSession = WinHttpOpen(
            L"AegleDLL/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0);

        if (hSession) {
            HINTERNET hConnect = WinHttpConnect(
                hSession,
                L"api.github.com",
                INTERNET_DEFAULT_HTTPS_PORT,
                0);

            if (hConnect) {
                HINTERNET hRequest = WinHttpOpenRequest(
                    hConnect,
                    L"GET",
                    L"/repos/AnarchDevelopment/aegledll/releases/latest",
                    nullptr,
                    WINHTTP_NO_REFERER,
                    WINHTTP_DEFAULT_ACCEPT_TYPES,
                    WINHTTP_FLAG_SECURE);

                if (hRequest) {
                    // Get token and add Authorization header if present
                    std::string token = GetGitHubTokenFromResource();
                    if (!token.empty() && token != "gph_") { // It works the same with github_pat_xxxxxxxxxx
                        std::wstring authHeader = L"Authorization: token " + std::wstring(token.begin(), token.end());
                        WinHttpAddRequestHeaders(hRequest,
                            authHeader.c_str(),
                            (DWORD)-1L,
                            WINHTTP_ADDREQ_FLAG_ADD);
                    }

                    // GitHub API requires a User-Agent header
                    WinHttpAddRequestHeaders(hRequest,
                        L"User-Agent: AegleDLL/1.0",
                        (DWORD)-1L,
                        WINHTTP_ADDREQ_FLAG_ADD);

                    WinHttpAddRequestHeaders(hRequest,
                        L"Accept: application/vnd.github+json",
                        (DWORD)-1L,
                        WINHTTP_ADDREQ_FLAG_ADD);

                    if (WinHttpSendRequest(hRequest,
                            WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
                        WinHttpReceiveResponse(hRequest, nullptr)) {

                        DWORD dwSize = 0;
                        std::string raw;
                        do {
                            dwSize = 0;
                            WinHttpQueryDataAvailable(hRequest, &dwSize);
                            if (dwSize == 0) break;
                            std::string chunk(dwSize, '\0');
                            DWORD dwDownloaded = 0;
                            WinHttpReadData(hRequest, &chunk[0], dwSize, &dwDownloaded);
                            raw.append(chunk, 0, dwDownloaded);
                        } while (dwSize > 0);

                        // Parse JSON and extract release metadata + "body"
                        try {
                            auto j = nlohmann::json::parse(raw);
                            if (j.contains("body") && j["body"].is_string()) {
                                result = j["body"].get<std::string>();
                            } else if (j.contains("message") && j["message"].is_string()) {
                                result = "**GitHub API Error:** " + j["message"].get<std::string>() + "\n\nRaw response:\n```json\n" + raw + "\n```";
                            } else {
                                result = "_No release notes found._\n\nRaw response:\n```\n" + raw + "\n```";
                            }
                            if (j.contains("tag_name") && j["tag_name"].is_string()) {
                                tag = j["tag_name"].get<std::string>();
                            }
                            if (j.contains("name") && j["name"].is_string()) {
                                name = j["name"].get<std::string>();
                            }
                            if (j.contains("published_at") && j["published_at"].is_string()) {
                                std::string full = j["published_at"].get<std::string>();
                                size_t tPos = full.find_first_of("Tt");
                                date = (tPos == std::string::npos) ? full : full.substr(0, tPos);
                            }
                        } catch (...) {
                            failed = true;
                            result = "**Error:** Could not parse GitHub API response. Raw response:\n```\n" + raw + "\n```";
                        }
                    } else {
                        failed = true;
                        result = "**Error:** HTTP request failed.";
                    }
                    WinHttpCloseHandle(hRequest);
                } else {
                    failed = true;
                    result = "**Error:** Could not open HTTP request.";
                }
                WinHttpCloseHandle(hConnect);
            } else {
                failed = true;
                result = "**Error:** Could not connect to api.github.com.";
            }
            WinHttpCloseHandle(hSession);
        } else {
            failed = true;
            result = "**Error:** WinHTTP session could not be created.";
        }

        {
            std::lock_guard<std::mutex> lock(g_releaseMutex);
            Info::g_releaseBody = result;
            Info::g_releaseTag = tag;
            Info::g_releaseName = name;
            Info::g_releaseDate = date;
            Info::g_fetchFailed = failed;
            Info::g_fetchDone = true;
            Info::g_fetchInProgress = false;
        }
        return 0;
    }, nullptr, 0, nullptr);
}
void Info::RenderMenu() {
    ImVec2 avail = ImGui::GetContentRegionAvail();
    const ImVec4 accent = GUI::g_colorAccent;

    // Center everything vertically
    float totalH = 200.0f; // logo + title + icons approx height
    float startY = (avail.y - totalH) * 0.5f;
    if (startY < 0) startY = 0;
    ImGui::SetCursorPosY(startY);

    // ── Centered logo ──
    if (g_logoTexture != 0 && g_logoWidth > 0 && g_logoHeight > 0) {
        float maxLogoH = 120.0f;
        float scale = (float)maxLogoH / (float)g_logoHeight;
        float logoW = (float)g_logoWidth * scale;
        float logoH = maxLogoH;
        ImGui::SetCursorPosX((avail.x - logoW) * 0.5f);
        ImGui::Image((ImTextureID)g_logoTexture, ImVec2(logoW, logoH));
        ImGui::Spacing();
        ImGui::Spacing();
    }

    // ── Title text ──
    const char* title = "KittyDLL 2.0.0";
    const char* subtitle = "Amatayakul Client";
    ImFont* fontH2 = GUI::g_fontH2 ? GUI::g_fontH2 : ImGui::GetIO().Fonts->Fonts[0];
    ImFont* fontH3 = GUI::g_fontH3 ? GUI::g_fontH3 : ImGui::GetIO().Fonts->Fonts[0];

    {
        ImVec2 ts = fontH2->CalcTextSizeA(22.0f, FLT_MAX, 0.0f, title);
        ImGui::SetCursorPosX((avail.x - ts.x) * 0.5f);
        ImGui::PushFont(fontH2);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.92f));
        ImGui::Text("%s", title);
        ImGui::PopStyleColor();
        ImGui::PopFont();
    }
    {
        ImVec2 ts = fontH3->CalcTextSizeA(14.0f, FLT_MAX, 0.0f, subtitle);
        ImGui::SetCursorPosX((avail.x - ts.x) * 0.5f);
        ImGui::PushFont(fontH3);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.45f));
        ImGui::Text("%s", subtitle);
        ImGui::PopStyleColor();
        ImGui::PopFont();
    }

    ImGui::Spacing();
    ImGui::Spacing();

    // ── Social icon buttons ──
    float iconMaxH = 36.0f;
    float gap = 20.0f;
    // Compute each icon size maintaining its native aspect ratio
    float discordW = 0, discordH = 0, githubW = 0, githubH = 0;
    if (g_discordIconW > 0 && g_discordIconH > 0) {
        float s = iconMaxH / (float)g_discordIconH;
        discordW = (float)g_discordIconW * s;
        discordH = iconMaxH;
    }
    if (g_githubIconW > 0 && g_githubIconH > 0) {
        float s = iconMaxH / (float)g_githubIconH;
        githubW = (float)g_githubIconW * s;
        githubH = iconMaxH;
    }
    float totalW = discordW + githubW + gap;
    float startX = (avail.x - totalW) * 0.5f;
    ImGui::SetCursorPosX(startX);

    // Discord button
    if (g_discordIcon != 0 && discordW > 0) {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(accent.x, accent.y, accent.z, 0.18f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(accent.x, accent.y, accent.z, 0.30f));
        if (ImGui::ImageButton("##DiscordBtn", g_discordIcon, ImVec2(discordW, discordH), ImVec2(0,0), ImVec2(1,1), ImVec4(0,0,0,0))) {
            ShellExecuteA(0, "open", "https://dsc.gg/anarchdevelopment", 0, 0, SW_SHOWNORMAL);
            strcpy(g_notifTitle, "Discord");
            strcpy(g_notifMessage, "Opening Discord...");
            g_notifStart = GetTickCount64();
            PlayClickSound();
        }
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);
        if (ImGui::IsItemHovered()) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            ImGui::SetTooltip("Join our Discord server");
        }
    }
    ImGui::SameLine(0, gap);

    // GitHub button
    if (g_githubIcon != 0 && githubW > 0) {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(accent.x, accent.y, accent.z, 0.18f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(accent.x, accent.y, accent.z, 0.30f));
        if (ImGui::ImageButton("##GithubBtn", g_githubIcon, ImVec2(githubW, githubH), ImVec2(0,0), ImVec2(1,1), ImVec4(0,0,0,0))) {
            ShellExecuteA(0, "open", "https://github.com/AnarchDevelopment/KittyDLL", 0, 0, SW_SHOWNORMAL);
            strcpy(g_notifTitle, "GitHub");
            strcpy(g_notifMessage, "Opening GitHub...");
            g_notifStart = GetTickCount64();
            PlayClickSound();
        }
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);
        if (ImGui::IsItemHovered()) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            ImGui::SetTooltip("View source on GitHub");
        }
    }

    // ── Release Notes Modal ──────────────────────────────────────────────────
    float targetAnim = g_showReleaseModal ? 1.0f : 0.0f;
    g_releaseModalAnim += (targetAnim - g_releaseModalAnim) * 0.12f;

    if (g_releaseModalAnim > 0.001f) {
        float scale = 0.85f + 0.15f * g_releaseModalAnim;
        float alpha = g_releaseModalAnim;

        ImVec2 baseSize = ImVec2(600, 440);
        ImVec2 size = ImVec2(baseSize.x * scale, baseSize.y * scale);

        ImGui::SetNextWindowSize(size, ImGuiCond_Always);
        ImGui::SetNextWindowPos(
            ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f),
            ImGuiCond_Always, ImVec2(0.5f, 0.5f));

        // Suppress default border — we draw a custom animated one
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(18.0f, 14.0f));

        // Capture window rect inside Begin (valid even if Begin returns true)
        ImVec2 wMin, wMax;

        if (ImGui::Begin("##ReleaseNotes", nullptr,
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoResize   |
                ImGuiWindowFlags_NoMove     |
                ImGuiWindowFlags_NoCollapse)) {

            wMin = ImGui::GetWindowPos();
            wMax = ImVec2(wMin.x + ImGui::GetWindowWidth(), wMin.y + ImGui::GetWindowHeight());

                // ── Title bar (accent title + right-aligned close button) ──
                float titlePadX = ImGui::GetStyle().WindowPadding.x;
                ImGui::SetCursorPosX(titlePadX);
                ImGui::PushStyleColor(ImGuiCol_Text, GUI::g_colorAccent);
                ImGui::Text("Current Version Info");
                ImGui::PopStyleColor();

                float closeSize = 24.0f;
                float closeX = ImGui::GetWindowWidth() - titlePadX - closeSize;
                float titleY = ImGui::GetCursorPosY();
                ImGui::SetCursorPos(ImVec2(closeX, titleY));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
                if (GUI::RenderButton("X##CloseRelease", ImVec2(closeSize, closeSize))) {
                    g_showReleaseModal = false;
                }
                ImGui::PopStyleVar();

                ImGui::SetCursorPosY(titleY + closeSize + 8.0f);
                ImGui::Separator();
                ImGui::Spacing();

                if (g_fetchInProgress) {
                    float t = (float)(GetTickCount64() % 900) / 300.0f;
                    const char* dots[] = { "Loading .  ", "Loading .. ", "Loading ..." };
                    ImVec2 dotSize = ImGui::CalcTextSize(dots[0]);
                    ImVec2 avail = ImGui::GetContentRegionAvail();
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail.x - dotSize.x) * 0.5f);
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (avail.y - dotSize.y) * 0.5f);
                    ImGui::TextDisabled("%s", dots[(int)t % 3]);
                } else if (!g_releaseBody.empty()) {
                    // Inner padding so the release notes don't touch the modal edges
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 14.0f));
                    ImGui::BeginChild("##MarkdownScroll", ImVec2(0, 0), false,
                        ImGuiWindowFlags_AlwaysVerticalScrollbar);

                ImGui::MarkdownConfig mdConfig;
                mdConfig.linkCallback    = [](ImGui::MarkdownLinkCallbackData data) {
                    std::string url(data.link, data.linkLength);
                    ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                };
                mdConfig.tooltipCallback = nullptr;
                mdConfig.imageCallback   = nullptr;
                mdConfig.linkIcon        = "";
                mdConfig.headingFormats[0] = { GUI::g_fontH1 ? GUI::g_fontH1 : ImGui::GetIO().Fonts->Fonts[0], true };
                mdConfig.headingFormats[1] = { GUI::g_fontH2 ? GUI::g_fontH2 : ImGui::GetIO().Fonts->Fonts[0], true };
                mdConfig.headingFormats[2] = { GUI::g_fontH3 ? GUI::g_fontH3 : ImGui::GetIO().Fonts->Fonts[0], false };
                mdConfig.formatCallback = [](const ImGui::MarkdownFormatInfo& markdownFormatInfo_, bool start_) {
                    ImGui::defaultMarkdownFormatCallback(markdownFormatInfo_, start_);
                    if (markdownFormatInfo_.type == ImGui::MarkdownFormatType::HEADING) {
                        if (start_) ImGui::PushStyleColor(ImGuiCol_Text, GUI::g_colorAccent);
                        else        ImGui::PopStyleColor();
                    }
                };

                std::lock_guard<std::mutex> lock(g_releaseMutex);
                ImGui::Markdown(g_releaseBody.c_str(), g_releaseBody.size(), mdConfig);

                ImGui::EndChild();
                ImGui::PopStyleVar();
            } else {
                ImVec2 ts = ImGui::CalcTextSize("No data available.");
                ImVec2 avail = ImGui::GetContentRegionAvail();
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail.x - ts.x) * 0.5f);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (avail.y - ts.y) * 0.5f);
                ImGui::TextDisabled("No data available.");
            }
        }
        ImGui::End();
        ImGui::PopStyleVar(4); // Alpha, WindowRounding, WindowBorderSize, WindowPadding

        // ── Animated glowing border drawn on the foreground draw list ─────────
        if (wMax.x > wMin.x && wMax.y > wMin.y) {
            ImDrawList* fg = ImGui::GetForegroundDrawList();
            const float rounding = 12.0f;
            const ImVec4& ac = GUI::g_colorAccent;

            // 1. Base thick border (slightly transparent accent color)
            fg->AddRect(wMin, wMax,
                ImColor(ac.x, ac.y, ac.z, alpha * 0.55f),
                rounding, 0, 3.5f);

            // 2. Outer soft glow layer (thicker, very transparent)
            fg->AddRect(
                ImVec2(wMin.x - 1, wMin.y - 1),
                ImVec2(wMax.x + 1, wMax.y + 1),
                ImColor(ac.x, ac.y, ac.z, alpha * 0.18f),
                rounding + 1.0f, 0, 8.0f);

            // 3. Moving spotlight along the perimeter (clockwise, 3-second loop)
            float W = wMax.x - wMin.x;
            float H = wMax.y - wMin.y;
            float perimeter = 2.0f * (W + H);
            float tLight = fmodf((float)(GetTickCount64()) / 3000.0f, 1.0f);

            // Compute position(s) for the spotlight (draw primary + a 2nd halfway around for symmetry)
            for (int pass = 0; pass < 2; pass++) {
                float tPass = fmodf(tLight + pass * 0.5f, 1.0f);
                float dist = tPass * perimeter;

                ImVec2 lightPos;
                if (dist < W) {                     // top edge →
                    lightPos = ImVec2(wMin.x + dist, wMin.y);
                } else if (dist < W + H) {          // right edge ↓
                    lightPos = ImVec2(wMax.x, wMin.y + (dist - W));
                } else if (dist < 2.0f*W + H) {    // bottom edge ←
                    lightPos = ImVec2(wMax.x - (dist - W - H), wMax.y);
                } else {                            // left edge ↑
                    lightPos = ImVec2(wMin.x, wMax.y - (dist - 2.0f*W - H));
                }

                // Draw soft halo layers (outermost → innermost)
                fg->AddCircleFilled(lightPos, 28.0f, ImColor(ac.x, ac.y, ac.z, alpha * 0.07f));
                fg->AddCircleFilled(lightPos, 18.0f, ImColor(ac.x, ac.y, ac.z, alpha * 0.15f));
                fg->AddCircleFilled(lightPos, 10.0f, ImColor(ac.x, ac.y, ac.z, alpha * 0.35f));
                fg->AddCircleFilled(lightPos,  5.0f, ImColor(ac.x, ac.y, ac.z, alpha * 0.70f));
                // Bright white core
                fg->AddCircleFilled(lightPos,  2.5f, ImColor(1.0f, 1.0f, 1.0f, alpha * 0.85f));
            }
        }
    }
}


