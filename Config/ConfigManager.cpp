/*
Under an4rch Development Public Source License 1.0
*/

#include "ConfigManager.hpp"
#include "Modules/ModuleHeader.hpp"
#include "Modules/Terminal/Terminal.hpp"
#include "Modules/Globals.hpp"
#include "ArrayList/ArrayList.hpp"
#include "GUI/GUI.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <windows.h>
#include <shlobj.h>
#define INITGUID
#include <knownfolders.h>
#undef INITGUID
#ifndef KF_FLAG_NO_PACKAGE_REDIRECTION
#define KF_FLAG_NO_PACKAGE_REDIRECTION 0x00002000
#endif
#include <shellapi.h>
#include <ctime>

std::string ConfigManager::configDir;
std::string ConfigManager::currentConfig;

extern char g_notifMessage[128];
extern char g_notifTitle[64];

// Minimal key-name helper used for the startup notification.
static std::string VkToNotifName(int vk) {
    if (vk == 0) return "None";
    bool extended = (vk == VK_RSHIFT || vk == VK_RCONTROL || vk == VK_RMENU ||
                     vk == VK_INSERT || vk == VK_HOME || vk == VK_END ||
                     vk == VK_PRIOR || vk == VK_NEXT || vk == VK_DELETE ||
                     vk == VK_UP || vk == VK_DOWN || vk == VK_LEFT || vk == VK_RIGHT);
    UINT sc = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
    char name[64] = {};
    if (sc) {
        LONG lp = (sc << 16) | (extended ? (1 << 24) : 0);
        if (GetKeyNameTextA(lp, name, sizeof(name)) > 0) return name;
    }
    switch (vk) {
        case VK_LSHIFT:   return "LSHIFT";
        case VK_RSHIFT:   return "RSHIFT";
        case VK_LCONTROL: return "LCTRL";
        case VK_RCONTROL: return "RCTRL";
        case VK_LMENU:    return "LALT";
        case VK_RMENU:    return "RALT";
    }
    char buf[16]; sprintf_s(buf, "[0x%X]", vk);
    return buf;
}

static bool EnsureDirectoryExists(const std::string& directoryPath) {
    try {
        std::filesystem::path path(directoryPath);
        if (std::filesystem::exists(path)) {
            return std::filesystem::is_directory(path);
        }
        return std::filesystem::create_directories(path);
    } catch (...) {
        return false;
    }
}

void ConfigManager::Initialize() {
    try {
        std::filesystem::path baseDir;

        // NOTE: Inside the packaged (UWP) app, FOLDERID_LocalAppData and the
        // LOCALAPPDATA env var are redirected to the package data folder
        // (e.g. ...\Packages\<family>\AC), duplicating the path. USERPROFILE is
        // NOT redirected, so resolve the real profile and build the path from it.
        char* userProfile = std::getenv("USERPROFILE");
        if (userProfile && strlen(userProfile) > 0) {
            baseDir = std::filesystem::path(userProfile) /
                "AppData" /
                "Local" /
                "Packages" /
                "Microsoft.MinecraftUWP_8wekyb3d8bbwe" /
                "LocalState" /
                "KittyClient";
        } else {
            baseDir = std::filesystem::current_path() / "KittyClient";
        }

        if (!EnsureDirectoryExists(baseDir.string())) {
            baseDir = std::filesystem::current_path() / "KittyClient";
            EnsureDirectoryExists(baseDir.string());
        }

        configDir = baseDir.string();
        if (!configDir.empty() && configDir.back() != '\\' && configDir.back() != '/') {
            configDir += "\\";
        }
    } catch (...) {
        configDir = (std::filesystem::current_path() / "KittyClient").string();
        EnsureDirectoryExists(configDir);
        if (!configDir.empty() && configDir.back() != '\\' && configDir.back() != '/') {
            configDir += "\\";
        }
    }

    // Always ensure a "Default" config exists
    {
        std::filesystem::path defaultPath = std::filesystem::path(configDir) / "Default.json";
        if (!std::filesystem::exists(defaultPath)) {
            // Seed the default config with the built-in preset
            static const char* kDefaultConfig = R"json({
    "Misc": {
        "AntiAFK": {
            "enabled": false,
            "intervalSecs": 30.0,
            "jump": false,
            "pressDurationMs": 150.0,
            "randomizeKeys": true
        },
        "Screenshot": {
            "enabled": false,
            "hotkey": 123,
            "notifyOnCapture": true,
            "showHud": true
        },
        "UnlockFPS": {
            "enabled": false,
            "fpsLimit": 0.0,
            "lowLatency": true
        }
    },
    "Movement": {
        "AutoSprint": {
            "enabled": true,
            "position": {
                "x": 1696.123779296875,
                "y": 453.0
            },
            "scale": 1.0,
            "showSprintText": true,
            "sprintTextColor": [1.0, 1.0, 1.0, 1.0],
            "sprintTextMode": 1,
            "sprintTextScale": 1.673097014427185,
            "sprintTextShadow": true
        }
    },
    "Visuals": {
        "ArrayList": {
            "animationSpeed": 1.0,
            "animationStyle": 0,
            "backgroundMode": 0,
            "bgOpacity": 0.75,
            "blurOpacity": 0.30000001192092896,
            "blurRadius": 6.0,
            "borderRadius": 4.0,
            "borderWidth": 1.0,
            "chromaSideBar": true,
            "chromaSpeed": 2.0,
            "chromaText": false,
            "colors": {
                "bgColor": [0.019999999552965164, 0.019999999552965164, 0.05999999865889549, 1.0],
                "borderColor": [1.0, 1.0, 1.0, 0.3499999940395355],
                "sideBarColor": [1.0, 0.4000000059604645, 0.800000011920929, 1.0],
                "suffixColor": [0.550000011920929, 0.550000011920929, 0.6800000071525574, 1.0],
                "textColor": [1.0, 1.0, 1.0, 1.0]
            },
            "enabled": false,
            "fontName": "Default",
            "glowEnabled": false,
            "glowStrength": 3.0,
            "hudScale": 1.0,
            "position": {"x": 1620.0, "y": 192.6924285888672},
            "roundedBorders": false,
            "rowSpacing": 2.0,
            "showBorder": false,
            "showSideBar": true,
            "showSuffix": true,
            "sideBarWidth": 3.0,
            "sideMode": 0,
            "size": 1.0,
            "textShadow": true,
            "textShadowOffset": 1.5
        },
        "CPSCounter": {
            "colors": {
                "shadowColor": [0.0, 0.0, 0.0, 0.699999988079071],
                "textColor": [1.0, 1.0, 1.0, 1.0]
            },
            "enabled": true,
            "fontName": "Default",
            "hudScale": 1.2870315313339233,
            "position": {"x": 1696.123779296875, "y": 424.685302734375}
        },
        "ClickGUI": {
            "bgOpacity": 0.699999988079071,
            "bgStyle": 1,
            "bindKey": 161,
            "blurOpacity": 0.25,
            "blurRadius": 2.5,
            "enabled": false,
            "guiStyle": 0,
            "showParticles": true,
            "showRiseBackground": true,
            "theme": 0
        },
        "FPSOverlay": {
            "bgOpacity": 0.5,
            "colors": {
                "accentColor": [1.0, 0.4000000059604645, 0.800000011920929, 1.0],
                "textColor": [1.0, 1.0, 1.0, 1.0]
            },
            "enabled": false,
            "fontName": "Default",
            "hudScale": 1.0,
            "position": {"x": 10.0, "y": 250.0},
            "scale": 1.0,
            "showBackground": false
        },
        "FullBright": {
            "enabled": false,
            "value": 100.0
        },
        "Keystrokes": {
            "blurEffect": false,
            "border": false,
            "colors": {
                "bgColor": [0.0, 0.0, 0.0, 1.0],
                "borderColor": [0.7799999713897705, 0.7799999713897705, 0.7799999713897705, 1.0],
                "disabledShadowColor": [0.0, 0.0, 0.0, 0.550000011920929],
                "enabledColor": [1.0, 1.0, 1.0, 1.0],
                "enabledShadowColor": [0.0, 0.0, 0.0, 0.800000011920929],
                "glowColor": [1.0, 1.0, 1.0, 1.0],
                "glowEnabledColor": [0.9409999847412109, 0.9409999847412109, 1.0, 1.0],
                "rectShadowColor": [0.0, 0.0, 0.0, 0.550000011920929],
                "textColor": [1.0, 1.0, 1.0, 1.0],
                "textEnabledColor": [0.0, 0.0, 0.0, 1.0],
                "textShadowColor": [0.0, 0.0, 0.0, 0.550000011920929]
            },
            "edSpeed": 1.0,
            "enabled": true,
            "fontName": "Default",
            "glow": false,
            "glowEnabled": false,
            "glowSpeed": 1.0,
            "hudScale": 1.4384615421295166,
            "keySpacing": 1.6299999952316284,
            "position": {"x": 1707.0, "y": 537.0},
            "rectShadow": false,
            "rounding": 0.0,
            "scale": 1.0,
            "showBg": false,
            "showMouseButtons": true,
            "showSpacebar": true,
            "textShadow": false
        },
        "MotionBlur": {
            "enabled": false
        },
        "PingCounter": {
            "bgOpacity": 0.5,
            "colors": {
                "shadowColor": [0.0, 0.0, 0.0, 0.550000011920929],
                "textColor": [1.0, 1.0, 1.0, 1.0]
            },
            "enabled": false,
            "fontName": "Default",
            "hudScale": 1.0,
            "position": {"x": 1848.0, "y": 10.0},
            "showBackground": true,
            "textScale": 1.0,
            "textShadow": true,
            "updateInterval": 1000
        },
        "PlayerInfo": {
            "bgOpacity": 0.8999999761581421,
            "bgRadius": 6.0,
            "colors": {
                "borderColor": [1.0, 1.0, 1.0, 0.20000000298023224],
                "nameColor": [0.9200000166893005, 0.9200000166893005, 0.9599999785423279, 1.0]
            },
            "enabled": false,
            "headRadius": 10.0,
            "headRounded": true,
            "headSize": 34.0,
            "hudScale": 1.6603772640228271,
            "position": {"x": 1735.2459716796875, "y": 90.673583984375},
            "showBackground": true,
            "showBorder": true,
            "showHatLayer": false,
            "textScale": 1.0
        },
        "RenderInfo": {
            "bgOpacity": 0.6000000238418579,
            "colors": {
                "themeColor": [1.0, 0.4000000059604645, 0.800000011920929, 1.0]
            },
            "enabled": false,
            "fontName": "Default",
            "hudScale": 1.0,
            "position": {"x": 10.0, "y": 100.0},
            "scale": 1.0,
            "showBackground": true
        },
        "Watermark": {
            "animStyle": 0,
            "bgOpacity": 0.5,
            "bgPadX": 10.0,
            "bgPadY": 5.0,
            "bgRadius": 5.0,
            "chromaDirection": true,
            "chromaSpeed": 1.0,
            "chromaText": true,
            "colors": {
                "bgColor": [0.019999999552965164, 0.019999999552965164, 0.03999999910593033, 1.0],
                "chromaColors": [
                    [1.0, 0.4000000059604645, 0.800000011920929, 1.0],
                    [0.6000000238418579, 0.5, 1.0, 1.0],
                    [0.4000000059604645, 0.800000011920929, 1.0, 1.0]
                ],
                "outlineColor": [0.0, 0.0, 0.0, 1.0],
                "staticColor": [1.0, 0.4000000059604645, 0.800000011920929, 1.0]
            },
            "customText": "Amatayakul",
            "edgeFade": false,
            "enabled": true,
            "fontName": "Default",
            "fontSize": 32.0,
            "hudScale": 1.992478370666504,
            "imageOpacity": 1.0,
            "imageSize": 28.979808807373047,
            "mirroredGradient": true,
            "outlineWidth": 1.5,
            "position": {"x": 1693.416015625, "y": 0.0},
            "showBackground": false,
            "showGlow": true,
            "showOutline": false,
            "showShimmer": false,
            "slideOffset": 40.0,
            "snapCorner": 0,
            "snapPadding": 10.0,
            "useImage": true
        }
    },
    "version": "1.0"
})json";
            try {
                nlohmann::json def = nlohmann::json::parse(kDefaultConfig);
                std::ofstream f(defaultPath, std::ios::out | std::ios::trunc);
                if (f.is_open()) f << def.dump(4);
            } catch (...) {
                // Fallback: write current state if parse somehow fails
                nlohmann::json def = CollectCurrentConfig();
                std::ofstream f(defaultPath, std::ios::out | std::ios::trunc);
                if (f.is_open()) f << def.dump(4);
            }
        }
    }

    // Restore the last-used config, falling back to "Default"
    {
        std::string last = LoadCurrentFile();
        if (last.empty()) last = "Default";
        std::filesystem::path lastPath = std::filesystem::path(configDir) / (last + ".json");
        if (std::filesystem::exists(lastPath)) {
            currentConfig = last;
            std::ifstream f(lastPath);
            if (f.is_open()) {
                try {
                    nlohmann::json cfg;
                    f >> cfg;
                    ApplyConfig(cfg);
                    ReloadModulesAfterConfig();
                } catch (...) {}
            }
        } else {
            currentConfig = "Default";
        }
    }
}

bool ConfigManager::SaveConfig(const std::string& name) {
    try {
        if (configDir.empty()) {
            Terminal::AddOutput("ConfigDir is empty! Cannot save config.");
            return false;
        }

        if (name.empty()) {
            Terminal::AddOutput("Config name cannot be empty");
            return false;
        }

        nlohmann::json config = CollectCurrentConfig();
        std::filesystem::path dirPath = std::filesystem::path(configDir);
        std::filesystem::path filepath = dirPath / (name + ".json");
        Terminal::AddOutput("Saving config to: " + filepath.string());

        if (!EnsureDirectoryExists(dirPath.string())) {
            Terminal::AddOutput("Failed to create config directory: " + dirPath.string());
            return false;
        }

        std::ofstream file(filepath, std::ios::out | std::ios::trunc);
        if (file.is_open()) {
            file << config.dump(4);
            Terminal::AddOutput("Config saved successfully");
            return true;
        }

        Terminal::AddOutput("Failed to open file for writing: " + filepath.string());
    } catch (const std::exception& e) {
        Terminal::AddOutput("Exception in SaveConfig: " + std::string(e.what()));
    } catch (...) {
        Terminal::AddOutput("Unknown exception in SaveConfig");
    }
    return false;
}

bool ConfigManager::LoadConfig(const std::string& name) {
    try {
        if (configDir.empty()) {
            Terminal::AddOutput("ConfigDir is empty! Cannot load config.");
            return false;
        }

        std::filesystem::path filepath = std::filesystem::path(configDir) / (name + ".json");
        Terminal::AddOutput("Loading config from: " + filepath.string());
        std::ifstream file(filepath);
        if (file.is_open()) {
            nlohmann::json config;
            file >> config;
            ApplyConfig(config);
            ReloadModulesAfterConfig();
            currentConfig = name;
            SaveCurrentFile();
            Terminal::AddOutput("Config loaded successfully");
            return true;
        } else {
            Terminal::AddOutput("Failed to open file for reading: " + filepath.string());
        }
    } catch (const std::exception& e) {
        Terminal::AddOutput("Exception in LoadConfig: " + std::string(e.what()));
    } catch (...) {
        Terminal::AddOutput("Unknown exception in LoadConfig");
    }
    return false;
}

bool ConfigManager::DeleteConfig(const std::string& name) {
    try {
        if (configDir.empty()) {
            Terminal::AddOutput("ConfigDir is empty! Cannot delete config.");
            return false;
        }

        std::string filepath = configDir + name + ".json";
        Terminal::AddOutput("Deleting config: " + filepath);
        std::filesystem::path path = filepath;
        if (std::filesystem::remove(path)) {
            Terminal::AddOutput("Config deleted successfully");
            return true;
        } else {
            Terminal::AddOutput("Config file not found");
        }
    } catch (const std::exception& e) {
        Terminal::AddOutput("Exception in DeleteConfig: " + std::string(e.what()));
    } catch (...) {
        Terminal::AddOutput("Unknown exception in DeleteConfig");
    }
    return false;
}

std::vector<std::string> ConfigManager::ListConfigs() {
    std::vector<std::string> configs;
    try {
        if (configDir.empty()) {
            Terminal::AddOutput("ConfigDir is empty! Cannot list configs.");
            return configs;
        }

        for (const auto& entry : std::filesystem::directory_iterator(configDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                configs.push_back(entry.path().stem().string());
            }
        }
    } catch (...) {}
    return configs;
}

bool ConfigManager::OpenConfigDirectory() {
    try {
        if (configDir.empty()) {
            return false;
        }

        std::filesystem::path dirPath = std::filesystem::path(configDir);
        if (!std::filesystem::exists(dirPath)) {
            if (!EnsureDirectoryExists(dirPath.string())) {
                return false;
            }
        }

        HINSTANCE result = ShellExecuteA(NULL, "open", configDir.c_str(), NULL, NULL, SW_SHOWDEFAULT);
        return reinterpret_cast<intptr_t>(result) > 32;
    } catch (...) {
        return false;
    }
}

nlohmann::json ConfigManager::CollectCurrentConfig() {
    nlohmann::json config;

    // Movement modules
    config["Movement"]["AutoSprint"]["enabled"] = AutoSprint::g_autoSprintEnabled;
    config["Movement"]["AutoSprint"]["showSprintText"] = AutoSprint::g_showSprintText;
    config["Movement"]["AutoSprint"]["sprintTextScale"] = AutoSprint::g_sprintTextScale;
    config["Movement"]["AutoSprint"]["sprintTextMode"] = AutoSprint::g_sprintTextMode;
    config["Movement"]["AutoSprint"]["sprintTextShadow"] = AutoSprint::g_sprintTextShadow;
    config["Movement"]["AutoSprint"]["sprintTextColor"] = nlohmann::json::array({AutoSprint::g_sprintTextColor.x, AutoSprint::g_sprintTextColor.y, AutoSprint::g_sprintTextColor.z, AutoSprint::g_sprintTextColor.w});
    if (AutoSprint::g_sprintTextHud) {
        config["Movement"]["AutoSprint"]["position"]["x"] = AutoSprint::g_sprintTextHud->pos.x;
        config["Movement"]["AutoSprint"]["position"]["y"] = AutoSprint::g_sprintTextHud->pos.y;
        config["Movement"]["AutoSprint"]["scale"] = AutoSprint::g_sprintTextHud->scale;
    }

    // Visuals modules
    config["Visuals"]["FullBright"]["enabled"] = FullBright::g_fullBrightEnabled;
    config["Visuals"]["FullBright"]["value"] = FullBright::g_fullBrightValue;

    config["Visuals"]["RenderInfo"]["enabled"] = RenderInfo::g_showRenderInfo;
    config["Visuals"]["RenderInfo"]["showBackground"] = RenderInfo::g_showBackground;
    config["Visuals"]["RenderInfo"]["bgOpacity"] = RenderInfo::g_bgOpacity;
    config["Visuals"]["RenderInfo"]["scale"] = RenderInfo::g_scale;
    config["Visuals"]["RenderInfo"]["fontName"] = RenderInfo::g_fontName;
    config["Visuals"]["RenderInfo"]["colors"]["themeColor"] = 
        nlohmann::json::array({RenderInfo::g_staticColor.x, RenderInfo::g_staticColor.y, 
                               RenderInfo::g_staticColor.z, RenderInfo::g_staticColor.w});

    if (RenderInfo::g_renderInfoHud) {
        config["Visuals"]["RenderInfo"]["position"]["x"] = RenderInfo::g_renderInfoHud->pos.x;
        config["Visuals"]["RenderInfo"]["position"]["y"] = RenderInfo::g_renderInfoHud->pos.y;
        config["Visuals"]["RenderInfo"]["hudScale"] = RenderInfo::g_renderInfoHud->scale;
    }

    config["Visuals"]["Watermark"]["enabled"] = Watermark::g_showWatermark;
    config["Visuals"]["Watermark"]["useImage"] = Watermark::g_useImage;
    config["Visuals"]["Watermark"]["customText"] = Watermark::g_customText;
    config["Visuals"]["Watermark"]["fontSize"] = Watermark::g_fontSize;
    config["Visuals"]["Watermark"]["bgOpacity"] = Watermark::g_bgOpacity;
    config["Visuals"]["Watermark"]["showBackground"] = Watermark::g_showBackground;
    config["Visuals"]["Watermark"]["showShimmer"] = Watermark::g_showShimmer;
    config["Visuals"]["Watermark"]["showGlow"] = Watermark::g_showGlow;
    config["Visuals"]["Watermark"]["chromaText"] = Watermark::g_chromaText;
    config["Visuals"]["Watermark"]["chromaSpeed"] = Watermark::g_chromaSpeed;
    config["Visuals"]["Watermark"]["chromaDirection"] = Watermark::g_chromaDirection;
    config["Visuals"]["Watermark"]["mirroredGradient"] = Watermark::g_mirroredGradient;
    config["Visuals"]["Watermark"]["edgeFade"] = Watermark::g_edgeFade;
    config["Visuals"]["Watermark"]["imageOpacity"] = Watermark::g_imageOpacity;
    config["Visuals"]["Watermark"]["imageSize"] = Watermark::g_imageSize;
    config["Visuals"]["Watermark"]["fontName"] = Watermark::g_fontName;
    config["Visuals"]["Watermark"]["animStyle"] = Watermark::g_animStyle;
    config["Visuals"]["Watermark"]["slideOffset"] = Watermark::g_slideOffset;
    config["Visuals"]["Watermark"]["snapCorner"] = Watermark::g_snapCorner;
    config["Visuals"]["Watermark"]["snapPadding"] = Watermark::g_snapPadding;
    config["Visuals"]["Watermark"]["showOutline"] = Watermark::g_showOutline;
    config["Visuals"]["Watermark"]["outlineWidth"] = Watermark::g_outlineWidth;
    config["Visuals"]["Watermark"]["bgRadius"] = Watermark::g_bgRadius;
    config["Visuals"]["Watermark"]["bgPadX"] = Watermark::g_bgPadX;
    config["Visuals"]["Watermark"]["bgPadY"] = Watermark::g_bgPadY;

    config["Visuals"]["Watermark"]["colors"]["staticColor"] = 
        nlohmann::json::array({Watermark::g_staticColor.x, Watermark::g_staticColor.y, 
                               Watermark::g_staticColor.z, Watermark::g_staticColor.w});
    config["Visuals"]["Watermark"]["colors"]["outlineColor"] = 
        nlohmann::json::array({Watermark::g_outlineColor.x, Watermark::g_outlineColor.y, 
                               Watermark::g_outlineColor.z, Watermark::g_outlineColor.w});
    config["Visuals"]["Watermark"]["colors"]["bgColor"] = 
        nlohmann::json::array({Watermark::g_bgColor.x, Watermark::g_bgColor.y, 
                               Watermark::g_bgColor.z, Watermark::g_bgColor.w});
    
    for (size_t i = 0; i < Watermark::g_chromaColors.size(); i++) {
        config["Visuals"]["Watermark"]["colors"]["chromaColors"][i] = 
            nlohmann::json::array({Watermark::g_chromaColors[i].x, Watermark::g_chromaColors[i].y, 
                                   Watermark::g_chromaColors[i].z, Watermark::g_chromaColors[i].w});
    }

    if (Watermark::g_watermarkHud) {
        config["Visuals"]["Watermark"]["position"]["x"] = Watermark::g_watermarkHud->pos.x;
        config["Visuals"]["Watermark"]["position"]["y"] = Watermark::g_watermarkHud->pos.y;
        config["Visuals"]["Watermark"]["hudScale"] = Watermark::g_watermarkHud->scale;
    }

    // ArrayList
    config["Visuals"]["ArrayList"]["enabled"] = ArrayList::g_enabled;
    config["Visuals"]["ArrayList"]["bgOpacity"] = ArrayList::g_bgOpacity;
    config["Visuals"]["ArrayList"]["showSideBar"] = ArrayList::g_showSideBar;
    config["Visuals"]["ArrayList"]["chromaSideBar"] = ArrayList::g_chromaSideBar;
    config["Visuals"]["ArrayList"]["roundedBorders"] = ArrayList::g_roundedBorders;
    config["Visuals"]["ArrayList"]["borderRadius"] = ArrayList::g_borderRadius;
    config["Visuals"]["ArrayList"]["showSuffix"] = ArrayList::g_showSuffix;
    config["Visuals"]["ArrayList"]["fontName"] = ArrayList::g_fontName;
    config["Visuals"]["ArrayList"]["size"] = ArrayList::g_size;
    config["Visuals"]["ArrayList"]["chromaText"] = ArrayList::g_chromaText;
    config["Visuals"]["ArrayList"]["chromaSpeed"] = ArrayList::g_chromaSpeed;
    config["Visuals"]["ArrayList"]["glowEnabled"] = ArrayList::g_glowEnabled;
    config["Visuals"]["ArrayList"]["glowStrength"] = ArrayList::g_glowStrength;
    config["Visuals"]["ArrayList"]["animationStyle"] = ArrayList::g_animationStyle;
    config["Visuals"]["ArrayList"]["sideMode"] = ArrayList::g_sideMode;
    config["Visuals"]["ArrayList"]["backgroundMode"] = ArrayList::g_backgroundMode;
    config["Visuals"]["ArrayList"]["blurRadius"] = ArrayList::g_blurRadius;
    config["Visuals"]["ArrayList"]["blurOpacity"] = ArrayList::g_blurOpacity;
    config["Visuals"]["ArrayList"]["rowSpacing"] = ArrayList::g_rowSpacing;
    config["Visuals"]["ArrayList"]["animationSpeed"] = ArrayList::g_animationSpeed;
    config["Visuals"]["ArrayList"]["sideBarWidth"] = ArrayList::g_sideBarWidth;
    config["Visuals"]["ArrayList"]["showBorder"] = ArrayList::g_showBorder;
    config["Visuals"]["ArrayList"]["borderWidth"] = ArrayList::g_borderWidth;
    config["Visuals"]["ArrayList"]["textShadow"] = ArrayList::g_textShadow;
    config["Visuals"]["ArrayList"]["textShadowOffset"] = ArrayList::g_textShadowOffset;
    
    config["Visuals"]["ArrayList"]["colors"]["bgColor"] = 
        nlohmann::json::array({ArrayList::g_bgColor.x, ArrayList::g_bgColor.y, 
                               ArrayList::g_bgColor.z, ArrayList::g_bgColor.w});
    config["Visuals"]["ArrayList"]["colors"]["sideBarColor"] = 
        nlohmann::json::array({ArrayList::g_sideBarColor.x, ArrayList::g_sideBarColor.y, 
                               ArrayList::g_sideBarColor.z, ArrayList::g_sideBarColor.w});
    config["Visuals"]["ArrayList"]["colors"]["borderColor"] = 
        nlohmann::json::array({ArrayList::g_borderColor.x, ArrayList::g_borderColor.y, 
                               ArrayList::g_borderColor.z, ArrayList::g_borderColor.w});
    config["Visuals"]["ArrayList"]["colors"]["textColor"] = 
        nlohmann::json::array({ArrayList::g_textColor.x, ArrayList::g_textColor.y, 
                               ArrayList::g_textColor.z, ArrayList::g_textColor.w});
    config["Visuals"]["ArrayList"]["colors"]["suffixColor"] = 
        nlohmann::json::array({ArrayList::g_suffixColor.x, ArrayList::g_suffixColor.y, 
                               ArrayList::g_suffixColor.z, ArrayList::g_suffixColor.w});

    if (ArrayList::g_hud) {
        config["Visuals"]["ArrayList"]["position"]["x"] = ArrayList::g_hud->pos.x;
        config["Visuals"]["ArrayList"]["position"]["y"] = ArrayList::g_hud->pos.y;
        config["Visuals"]["ArrayList"]["hudScale"] = ArrayList::g_hud->scale;
    }

    config["Visuals"]["MotionBlur"]["enabled"] = MotionBlur::g_motionBlurEnabled;
    
    config["Visuals"]["Keystrokes"]["enabled"] = Keystrokes::g_showKeystrokes;
    config["Visuals"]["Keystrokes"]["scale"] = Keystrokes::g_keystrokesUIScale;
    config["Visuals"]["Keystrokes"]["blurEffect"] = Keystrokes::g_keystrokesBlurEffect;
    config["Visuals"]["Keystrokes"]["rounding"] = Keystrokes::g_keystrokesRounding;
    config["Visuals"]["Keystrokes"]["showBg"] = Keystrokes::g_keystrokesShowBg;
    config["Visuals"]["Keystrokes"]["rectShadow"] = Keystrokes::g_keystrokesRectShadow;
    config["Visuals"]["Keystrokes"]["border"] = Keystrokes::g_keystrokesBorder;
    config["Visuals"]["Keystrokes"]["glow"] = Keystrokes::g_keystrokesGlow;
    config["Visuals"]["Keystrokes"]["glowEnabled"] = Keystrokes::g_keystrokesGlowEnabled;
    config["Visuals"]["Keystrokes"]["glowSpeed"] = Keystrokes::g_keystrokesGlowSpeed;
    config["Visuals"]["Keystrokes"]["keySpacing"] = Keystrokes::g_keystrokesKeySpacing;
    config["Visuals"]["Keystrokes"]["edSpeed"] = Keystrokes::g_keystrokesEdSpeed;
    config["Visuals"]["Keystrokes"]["textShadow"] = Keystrokes::g_keystrokesTextShadow;
    config["Visuals"]["Keystrokes"]["showMouseButtons"] = Keystrokes::g_keystrokesShowMouseButtons;
    config["Visuals"]["Keystrokes"]["showSpacebar"] = Keystrokes::g_keystrokesShowSpacebar;
    config["Visuals"]["Keystrokes"]["fontName"] = Keystrokes::g_fontName;

    if (Keystrokes::g_keystrokesHud) {
        config["Visuals"]["Keystrokes"]["position"]["x"] = Keystrokes::g_keystrokesHud->pos.x;
        config["Visuals"]["Keystrokes"]["position"]["y"] = Keystrokes::g_keystrokesHud->pos.y;
        config["Visuals"]["Keystrokes"]["hudScale"] = Keystrokes::g_keystrokesHud->scale;
    }
    // Save Keystrokes colors
    config["Visuals"]["Keystrokes"]["colors"]["bgColor"] = 
        nlohmann::json::array({Keystrokes::g_keystrokesBgColor.x, Keystrokes::g_keystrokesBgColor.y, 
                               Keystrokes::g_keystrokesBgColor.z, Keystrokes::g_keystrokesBgColor.w});
    config["Visuals"]["Keystrokes"]["colors"]["enabledColor"] = 
        nlohmann::json::array({Keystrokes::g_keystrokesEnabledColor.x, Keystrokes::g_keystrokesEnabledColor.y, 
                               Keystrokes::g_keystrokesEnabledColor.z, Keystrokes::g_keystrokesEnabledColor.w});
    config["Visuals"]["Keystrokes"]["colors"]["textColor"] = 
        nlohmann::json::array({Keystrokes::g_keystrokesTextColor.x, Keystrokes::g_keystrokesTextColor.y, 
                               Keystrokes::g_keystrokesTextColor.z, Keystrokes::g_keystrokesTextColor.w});
    config["Visuals"]["Keystrokes"]["colors"]["textEnabledColor"] = 
        nlohmann::json::array({Keystrokes::g_keystrokesTextEnabledColor.x, Keystrokes::g_keystrokesTextEnabledColor.y, 
                               Keystrokes::g_keystrokesTextEnabledColor.z, Keystrokes::g_keystrokesTextEnabledColor.w});
    config["Visuals"]["Keystrokes"]["colors"]["borderColor"] = 
        nlohmann::json::array({Keystrokes::g_keystrokesBorderColor.x, Keystrokes::g_keystrokesBorderColor.y, 
                               Keystrokes::g_keystrokesBorderColor.z, Keystrokes::g_keystrokesBorderColor.w});
    config["Visuals"]["Keystrokes"]["colors"]["glowColor"] = 
        nlohmann::json::array({Keystrokes::g_keystrokesGlowColor.x, Keystrokes::g_keystrokesGlowColor.y, 
                               Keystrokes::g_keystrokesGlowColor.z, Keystrokes::g_keystrokesGlowColor.w});
    config["Visuals"]["Keystrokes"]["colors"]["glowEnabledColor"] = 
        nlohmann::json::array({Keystrokes::g_keystrokesGlowEnabledColor.x, Keystrokes::g_keystrokesGlowEnabledColor.y, 
                               Keystrokes::g_keystrokesGlowEnabledColor.z, Keystrokes::g_keystrokesGlowEnabledColor.w});
    config["Visuals"]["Keystrokes"]["colors"]["rectShadowColor"] = 
        nlohmann::json::array({Keystrokes::g_keystrokesRectShadowColor.x, Keystrokes::g_keystrokesRectShadowColor.y, 
                               Keystrokes::g_keystrokesRectShadowColor.z, Keystrokes::g_keystrokesRectShadowColor.w});
    config["Visuals"]["Keystrokes"]["colors"]["textShadowColor"] = 
        nlohmann::json::array({Keystrokes::g_keystrokesTextShadowColor.x, Keystrokes::g_keystrokesTextShadowColor.y, 
                               Keystrokes::g_keystrokesTextShadowColor.z, Keystrokes::g_keystrokesTextShadowColor.w});
    config["Visuals"]["Keystrokes"]["colors"]["enabledShadowColor"] = 
        nlohmann::json::array({Keystrokes::g_keystrokesEnabledShadowColor.x, Keystrokes::g_keystrokesEnabledShadowColor.y, 
                               Keystrokes::g_keystrokesEnabledShadowColor.z, Keystrokes::g_keystrokesEnabledShadowColor.w});
    config["Visuals"]["Keystrokes"]["colors"]["disabledShadowColor"] = 
        nlohmann::json::array({Keystrokes::g_keystrokesDisabledShadowColor.x, Keystrokes::g_keystrokesDisabledShadowColor.y, 
                               Keystrokes::g_keystrokesDisabledShadowColor.z, Keystrokes::g_keystrokesDisabledShadowColor.w});

    config["Visuals"]["CPSCounter"]["enabled"] = CPSCounter::g_showCpsCounter;
    config["Visuals"]["CPSCounter"]["fontName"] = CPSCounter::g_fontName;
    if (CPSCounter::g_cpsHud) {
        config["Visuals"]["CPSCounter"]["position"]["x"] = CPSCounter::g_cpsHud->pos.x;
        config["Visuals"]["CPSCounter"]["position"]["y"] = CPSCounter::g_cpsHud->pos.y;
        config["Visuals"]["CPSCounter"]["hudScale"] = CPSCounter::g_cpsHud->scale;
    }
    // Save CPSCounter colors
    config["Visuals"]["CPSCounter"]["colors"]["textColor"] = 
        nlohmann::json::array({CPSCounter::g_cpsTextColor.x, CPSCounter::g_cpsTextColor.y, 
                               CPSCounter::g_cpsTextColor.z, CPSCounter::g_cpsTextColor.w});
    config["Visuals"]["CPSCounter"]["colors"]["shadowColor"] = 
        nlohmann::json::array({CPSCounter::g_cpsCounterShadowColor.x, CPSCounter::g_cpsCounterShadowColor.y, 
                               CPSCounter::g_cpsCounterShadowColor.z, CPSCounter::g_cpsCounterShadowColor.w});

    // FPSOverlay
    config["Visuals"]["FPSOverlay"]["enabled"] = FPSOverlay::g_showFpsOverlay;
    config["Visuals"]["FPSOverlay"]["scale"] = FPSOverlay::g_fpsTextScale;
    config["Visuals"]["FPSOverlay"]["showBackground"] = FPSOverlay::g_showBackground;
    config["Visuals"]["FPSOverlay"]["bgOpacity"] = FPSOverlay::g_bgOpacity;
    config["Visuals"]["FPSOverlay"]["fontName"] = FPSOverlay::g_fontName;
    config["Visuals"]["FPSOverlay"]["colors"]["textColor"] = 
        nlohmann::json::array({FPSOverlay::g_fpsTextColor.x, FPSOverlay::g_fpsTextColor.y, 
                               FPSOverlay::g_fpsTextColor.z, FPSOverlay::g_fpsTextColor.w});
    config["Visuals"]["FPSOverlay"]["colors"]["accentColor"] = 
        nlohmann::json::array({FPSOverlay::g_accentColor.x, FPSOverlay::g_accentColor.y, 
                               FPSOverlay::g_accentColor.z, FPSOverlay::g_accentColor.w});

    if (FPSOverlay::g_fpsHud) {
        config["Visuals"]["FPSOverlay"]["position"]["x"] = FPSOverlay::g_fpsHud->pos.x;
        config["Visuals"]["FPSOverlay"]["position"]["y"] = FPSOverlay::g_fpsHud->pos.y;
        config["Visuals"]["FPSOverlay"]["hudScale"] = FPSOverlay::g_fpsHud->scale;
    }

    // PingCounter
    config["Visuals"]["PingCounter"]["enabled"] = PingCounter::g_showPingCounter;
    config["Visuals"]["PingCounter"]["textScale"] = PingCounter::g_pingTextScale;
    config["Visuals"]["PingCounter"]["showBackground"] = PingCounter::g_showBackground;
    config["Visuals"]["PingCounter"]["bgOpacity"] = PingCounter::g_bgOpacity;
    config["Visuals"]["PingCounter"]["textShadow"] = PingCounter::g_pingTextShadow;
    config["Visuals"]["PingCounter"]["updateInterval"] = PingCounter::g_pingUpdateInterval;
    config["Visuals"]["PingCounter"]["fontName"] = PingCounter::g_fontName;
    config["Visuals"]["PingCounter"]["colors"]["textColor"] =
        nlohmann::json::array({PingCounter::g_pingTextColor.x, PingCounter::g_pingTextColor.y,
                               PingCounter::g_pingTextColor.z, PingCounter::g_pingTextColor.w});
    config["Visuals"]["PingCounter"]["colors"]["shadowColor"] =
        nlohmann::json::array({PingCounter::g_pingCounterShadowColor.x, PingCounter::g_pingCounterShadowColor.y,
                               PingCounter::g_pingCounterShadowColor.z, PingCounter::g_pingCounterShadowColor.w});
    if (PingCounter::g_pingHud) {
        config["Visuals"]["PingCounter"]["position"]["x"] = PingCounter::g_pingHud->pos.x;
        config["Visuals"]["PingCounter"]["position"]["y"] = PingCounter::g_pingHud->pos.y;
        config["Visuals"]["PingCounter"]["hudScale"] = PingCounter::g_pingHud->scale;
    }

    // PlayerInfo
    config["Visuals"]["PlayerInfo"]["enabled"] = PlayerInfo::g_showPlayerInfo;
    config["Visuals"]["PlayerInfo"]["showHatLayer"] = PlayerInfo::g_showHatLayer;
    config["Visuals"]["PlayerInfo"]["headSize"] = PlayerInfo::g_headSize;
    config["Visuals"]["PlayerInfo"]["textScale"] = PlayerInfo::g_textScale;
    config["Visuals"]["PlayerInfo"]["headRounded"] = PlayerInfo::g_headRounded;
    config["Visuals"]["PlayerInfo"]["headRadius"] = PlayerInfo::g_headRadius;
    config["Visuals"]["PlayerInfo"]["showBackground"] = PlayerInfo::g_showBackground;
    config["Visuals"]["PlayerInfo"]["bgOpacity"] = PlayerInfo::g_bgOpacity;
    config["Visuals"]["PlayerInfo"]["bgRadius"] = PlayerInfo::g_bgRadius;
    config["Visuals"]["PlayerInfo"]["showBorder"] = PlayerInfo::g_showBorder;
    config["Visuals"]["PlayerInfo"]["colors"]["borderColor"] =
        nlohmann::json::array({PlayerInfo::g_borderColor.x, PlayerInfo::g_borderColor.y,
                               PlayerInfo::g_borderColor.z, PlayerInfo::g_borderColor.w});
    config["Visuals"]["PlayerInfo"]["colors"]["nameColor"] =
        nlohmann::json::array({PlayerInfo::g_nameColor.x, PlayerInfo::g_nameColor.y,
                               PlayerInfo::g_nameColor.z, PlayerInfo::g_nameColor.w});
    if (PlayerInfo::g_playerInfoHud) {
        config["Visuals"]["PlayerInfo"]["position"]["x"] = PlayerInfo::g_playerInfoHud->pos.x;
        config["Visuals"]["PlayerInfo"]["position"]["y"] = PlayerInfo::g_playerInfoHud->pos.y;
        config["Visuals"]["PlayerInfo"]["hudScale"] = PlayerInfo::g_playerInfoHud->scale;
    }

    config["Misc"]["UnlockFPS"]["enabled"] = UnlockFPS::g_unlockFpsEnabled;
    config["Misc"]["UnlockFPS"]["fpsLimit"] = UnlockFPS::g_fpsLimit;
    config["Misc"]["UnlockFPS"]["lowLatency"] = UnlockFPS::g_lowLatency;

    config["Misc"]["AntiAFK"]["enabled"] = AntiAFK::g_enabled;
    config["Misc"]["AntiAFK"]["intervalSecs"] = AntiAFK::g_intervalSecs;
    config["Misc"]["AntiAFK"]["pressDurationMs"] = AntiAFK::g_pressDurationMs;
    config["Misc"]["AntiAFK"]["randomizeKeys"] = AntiAFK::g_randomizeKeys;
    config["Misc"]["AntiAFK"]["jump"] = AntiAFK::g_jump;

    config["Misc"]["Screenshot"]["enabled"] = Screenshot::g_enabled;
    config["Misc"]["Screenshot"]["hotkey"] = Screenshot::g_hotkey;
    config["Misc"]["Screenshot"]["showHud"] = Screenshot::g_showHud;
    config["Misc"]["Screenshot"]["notifyOnCapture"] = Screenshot::g_notifyOnCapture;

    // ClickGUI settings
    config["Visuals"]["ClickGUI"]["enabled"] = ClickGUI::g_enabled;
    config["Visuals"]["ClickGUI"]["bindKey"] = ClickGUI::g_bindKey;
    config["Visuals"]["ClickGUI"]["guiStyle"] = ClickGUI::g_guiStyle;
    config["Visuals"]["ClickGUI"]["showParticles"] = ClickGUI::g_showParticles;
    config["Visuals"]["ClickGUI"]["showRiseBackground"] = ClickGUI::g_showRiseBackground;
    config["Visuals"]["ClickGUI"]["bgOpacity"] = ClickGUI::g_bgOpacity;
    config["Visuals"]["ClickGUI"]["bgStyle"] = ClickGUI::g_bgStyle;
    config["Visuals"]["ClickGUI"]["blurRadius"] = ClickGUI::g_blurRadius;
    config["Visuals"]["ClickGUI"]["blurOpacity"] = ClickGUI::g_blurOpacity;
    config["Visuals"]["ClickGUI"]["theme"] = GUI::g_currentTheme;

    config["version"] = "1.0";
    config["timestamp"] = std::time(nullptr);

    return config;
}

void ConfigManager::ApplyConfig(const nlohmann::json& config) {
    // Movement modules
    if (config.contains("Movement")) {
        if (config["Movement"].contains("AutoSprint")) {
            if (config["Movement"]["AutoSprint"].contains("enabled")) {
                AutoSprint::g_autoSprintEnabled = config["Movement"]["AutoSprint"]["enabled"];
            }
            if (config["Movement"]["AutoSprint"].contains("showSprintText")) {
                AutoSprint::g_showSprintText = config["Movement"]["AutoSprint"]["showSprintText"];
            }
            if (config["Movement"]["AutoSprint"].contains("sprintTextScale")) {
                AutoSprint::g_sprintTextScale = config["Movement"]["AutoSprint"]["sprintTextScale"];
            }
            if (config["Movement"]["AutoSprint"].contains("sprintTextMode")) {
                AutoSprint::g_sprintTextMode = config["Movement"]["AutoSprint"]["sprintTextMode"];
            }
            if (config["Movement"]["AutoSprint"].contains("sprintTextShadow")) {
                AutoSprint::g_sprintTextShadow = config["Movement"]["AutoSprint"]["sprintTextShadow"];
            }
            if (config["Movement"]["AutoSprint"].contains("sprintTextColor") && config["Movement"]["AutoSprint"]["sprintTextColor"].size() == 4) {
                auto& c = config["Movement"]["AutoSprint"]["sprintTextColor"];
                AutoSprint::g_sprintTextColor = ImVec4(c[0], c[1], c[2], c[3]);
            }
            if (config["Movement"]["AutoSprint"].contains("position") && AutoSprint::g_sprintTextHud) {
                AutoSprint::g_sprintTextHud->pos.x = config["Movement"]["AutoSprint"]["position"]["x"];
                AutoSprint::g_sprintTextHud->pos.y = config["Movement"]["AutoSprint"]["position"]["y"];
                AutoSprint::g_sprintTextHud->hasConfigPos = true;
            }
            if (config["Movement"]["AutoSprint"].contains("scale") && AutoSprint::g_sprintTextHud) {
                AutoSprint::g_sprintTextHud->scale = config["Movement"]["AutoSprint"]["scale"];
            }
        }
    }

    // Visuals modules
    if (config.contains("Visuals")) {
        auto& visuals = config["Visuals"];
        if (visuals.contains("FullBright")) {
            if (visuals["FullBright"].contains("enabled")) {
                FullBright::g_fullBrightEnabled = visuals["FullBright"]["enabled"];
            }
            if (visuals["FullBright"].contains("value")) {
                FullBright::g_fullBrightValue = visuals["FullBright"]["value"];
            }
        }
        if (visuals.contains("RenderInfo")) {
            if (visuals["RenderInfo"].contains("enabled")) {
                RenderInfo::g_showRenderInfo = visuals["RenderInfo"]["enabled"];
            }
            if (visuals["RenderInfo"].contains("showBackground")) RenderInfo::g_showBackground = visuals["RenderInfo"]["showBackground"];
            if (visuals["RenderInfo"].contains("bgOpacity")) RenderInfo::g_bgOpacity = visuals["RenderInfo"]["bgOpacity"];
            if (visuals["RenderInfo"].contains("scale")) RenderInfo::g_scale = visuals["RenderInfo"]["scale"];
            if (visuals["RenderInfo"].contains("fontName") && visuals["RenderInfo"]["fontName"].is_string()) {
                RenderInfo::g_fontName = visuals["RenderInfo"]["fontName"];
            }
            
            if (visuals["RenderInfo"].contains("colors")) {
                auto& c = visuals["RenderInfo"]["colors"]["themeColor"];
                if (c.is_array() && c.size() == 4) {
                    RenderInfo::g_staticColor = ImVec4(c[0], c[1], c[2], c[3]);
                }
            }

            if (visuals["RenderInfo"].contains("position") && RenderInfo::g_renderInfoHud) {
                RenderInfo::g_renderInfoHud->pos.x = visuals["RenderInfo"]["position"]["x"];
                RenderInfo::g_renderInfoHud->pos.y = visuals["RenderInfo"]["position"]["y"];
                RenderInfo::g_renderInfoHud->hasConfigPos = true;
            }
            if (visuals["RenderInfo"].contains("hudScale") && RenderInfo::g_renderInfoHud) {
                RenderInfo::g_renderInfoHud->scale = visuals["RenderInfo"]["hudScale"];
            }
        }
        if (visuals.contains("Watermark")) {
            auto& wm = visuals["Watermark"];
            if (wm.contains("enabled")) Watermark::g_showWatermark = wm["enabled"];
            if (wm.contains("useImage")) Watermark::g_useImage = wm["useImage"];
            if (wm.contains("customText")) strcpy_s(Watermark::g_customText, std::string(wm["customText"]).c_str());
            if (wm.contains("fontSize")) Watermark::g_fontSize = wm["fontSize"];
            if (wm.contains("bgOpacity")) Watermark::g_bgOpacity = wm["bgOpacity"];
            if (wm.contains("showBackground")) Watermark::g_showBackground = wm["showBackground"];
            if (wm.contains("showShimmer")) Watermark::g_showShimmer = wm["showShimmer"];
            if (wm.contains("showGlow")) Watermark::g_showGlow = wm["showGlow"];
            if (wm.contains("chromaText")) Watermark::g_chromaText = wm["chromaText"];
            if (wm.contains("chromaSpeed")) Watermark::g_chromaSpeed = wm["chromaSpeed"];
            if (wm.contains("chromaDirection")) Watermark::g_chromaDirection = wm["chromaDirection"];
            if (wm.contains("mirroredGradient")) Watermark::g_mirroredGradient = wm["mirroredGradient"];
            if (wm.contains("edgeFade")) Watermark::g_edgeFade = wm["edgeFade"];
            if (wm.contains("imageOpacity")) Watermark::g_imageOpacity = wm["imageOpacity"];
            if (wm.contains("imageSize")) Watermark::g_imageSize = wm["imageSize"];
            if (wm.contains("animStyle")) Watermark::g_animStyle = wm["animStyle"];
            if (wm.contains("slideOffset")) Watermark::g_slideOffset = wm["slideOffset"];
            if (wm.contains("snapCorner")) Watermark::g_snapCorner = wm["snapCorner"];
            if (wm.contains("snapPadding")) Watermark::g_snapPadding = wm["snapPadding"];
            if (wm.contains("showOutline")) Watermark::g_showOutline = wm["showOutline"];
            if (wm.contains("outlineWidth")) Watermark::g_outlineWidth = wm["outlineWidth"];
            if (wm.contains("bgRadius")) Watermark::g_bgRadius = wm["bgRadius"];
            if (wm.contains("bgPadX")) Watermark::g_bgPadX = wm["bgPadX"];
            if (wm.contains("bgPadY")) Watermark::g_bgPadY = wm["bgPadY"];
            if (wm.contains("fontName") && wm["fontName"].is_string()) {
                Watermark::g_fontName = wm["fontName"];
            }
            
            if (wm.contains("colors")) {
                if (wm["colors"].contains("staticColor") && wm["colors"]["staticColor"].size() == 4) {
                    Watermark::g_staticColor = ImVec4(wm["colors"]["staticColor"][0], wm["colors"]["staticColor"][1], 
                                                      wm["colors"]["staticColor"][2], wm["colors"]["staticColor"][3]);
                }
                if (wm["colors"].contains("outlineColor") && wm["colors"]["outlineColor"].size() == 4) {
                    Watermark::g_outlineColor = ImVec4(wm["colors"]["outlineColor"][0], wm["colors"]["outlineColor"][1], 
                                                       wm["colors"]["outlineColor"][2], wm["colors"]["outlineColor"][3]);
                }
                if (wm["colors"].contains("bgColor") && wm["colors"]["bgColor"].size() == 4) {
                    Watermark::g_bgColor = ImVec4(wm["colors"]["bgColor"][0], wm["colors"]["bgColor"][1], 
                                                  wm["colors"]["bgColor"][2], wm["colors"]["bgColor"][3]);
                }
                if (wm["colors"].contains("chromaColors") && wm["colors"]["chromaColors"].is_array()) {
                    Watermark::g_chromaColors.clear();
                    for (size_t i = 0; i < wm["colors"]["chromaColors"].size(); i++) {
                        if (wm["colors"]["chromaColors"][i].size() == 4) {
                            Watermark::g_chromaColors.push_back(ImVec4(
                                wm["colors"]["chromaColors"][i][0], 
                                wm["colors"]["chromaColors"][i][1], 
                                wm["colors"]["chromaColors"][i][2], 
                                wm["colors"]["chromaColors"][i][3]
                            ));
                        }
                    }
                }
            }

            if (wm.contains("position") && Watermark::g_watermarkHud) {
                Watermark::g_watermarkHud->pos.x = wm["position"]["x"];
                Watermark::g_watermarkHud->pos.y = wm["position"]["y"];
                Watermark::g_watermarkHud->hasConfigPos = true;
            }
            if (wm.contains("hudScale") && Watermark::g_watermarkHud) {
                Watermark::g_watermarkHud->scale = wm["hudScale"];
            }
        }
        
        if (visuals.contains("ArrayList")) {
            auto& al = visuals["ArrayList"];
            if (al.contains("enabled")) ArrayList::g_enabled = al["enabled"];
            if (al.contains("bgOpacity")) ArrayList::g_bgOpacity = al["bgOpacity"];
            if (al.contains("showSideBar")) ArrayList::g_showSideBar = al["showSideBar"];
            if (al.contains("chromaSideBar")) ArrayList::g_chromaSideBar = al["chromaSideBar"];
            if (al.contains("roundedBorders")) ArrayList::g_roundedBorders = al["roundedBorders"];
            if (al.contains("borderRadius")) ArrayList::g_borderRadius = al["borderRadius"];
            if (al.contains("showSuffix")) ArrayList::g_showSuffix = al["showSuffix"];
            if (al.contains("fontName") && al["fontName"].is_string()) {
                ArrayList::g_fontName = al["fontName"];
            }
            if (al.contains("size")) ArrayList::g_size = al["size"];
            if (al.contains("chromaText")) ArrayList::g_chromaText = al["chromaText"];
            if (al.contains("chromaSpeed")) ArrayList::g_chromaSpeed = al["chromaSpeed"];
            if (al.contains("glowEnabled")) ArrayList::g_glowEnabled = al["glowEnabled"];
            if (al.contains("glowStrength")) ArrayList::g_glowStrength = al["glowStrength"];
            if (al.contains("animationStyle")) ArrayList::g_animationStyle = al["animationStyle"];
            if (al.contains("sideMode")) ArrayList::g_sideMode = al["sideMode"];
            if (al.contains("backgroundMode")) ArrayList::g_backgroundMode = al["backgroundMode"];
            if (al.contains("blurRadius")) ArrayList::g_blurRadius = al["blurRadius"];
            if (al.contains("blurOpacity")) ArrayList::g_blurOpacity = al["blurOpacity"];
            if (al.contains("rowSpacing")) ArrayList::g_rowSpacing = al["rowSpacing"];
            if (al.contains("animationSpeed")) ArrayList::g_animationSpeed = al["animationSpeed"];
            if (al.contains("sideBarWidth")) ArrayList::g_sideBarWidth = al["sideBarWidth"];
            if (al.contains("showBorder")) ArrayList::g_showBorder = al["showBorder"];
            if (al.contains("borderWidth")) ArrayList::g_borderWidth = al["borderWidth"];
            if (al.contains("textShadow")) ArrayList::g_textShadow = al["textShadow"];
            if (al.contains("textShadowOffset")) ArrayList::g_textShadowOffset = al["textShadowOffset"];
            
            if (al.contains("colors")) {
                if (al["colors"].contains("bgColor") && al["colors"]["bgColor"].size() == 4) {
                    ArrayList::g_bgColor = ImVec4(al["colors"]["bgColor"][0], al["colors"]["bgColor"][1], 
                                                  al["colors"]["bgColor"][2], al["colors"]["bgColor"][3]);
                }
                if (al["colors"].contains("sideBarColor") && al["colors"]["sideBarColor"].size() == 4) {
                    ArrayList::g_sideBarColor = ImVec4(al["colors"]["sideBarColor"][0], al["colors"]["sideBarColor"][1], 
                                                       al["colors"]["sideBarColor"][2], al["colors"]["sideBarColor"][3]);
                }
                if (al["colors"].contains("borderColor") && al["colors"]["borderColor"].size() == 4) {
                    ArrayList::g_borderColor = ImVec4(al["colors"]["borderColor"][0], al["colors"]["borderColor"][1], 
                                                      al["colors"]["borderColor"][2], al["colors"]["borderColor"][3]);
                }
                if (al["colors"].contains("textColor") && al["colors"]["textColor"].size() == 4) {
                    ArrayList::g_textColor = ImVec4(al["colors"]["textColor"][0], al["colors"]["textColor"][1], 
                                                    al["colors"]["textColor"][2], al["colors"]["textColor"][3]);
                }
                if (al["colors"].contains("suffixColor") && al["colors"]["suffixColor"].size() == 4) {
                    ArrayList::g_suffixColor = ImVec4(al["colors"]["suffixColor"][0], al["colors"]["suffixColor"][1], 
                                                      al["colors"]["suffixColor"][2], al["colors"]["suffixColor"][3]);
                }
            }

            if (al.contains("position") && ArrayList::g_hud) {
                ArrayList::g_hud->pos.x = al["position"]["x"];
                ArrayList::g_hud->pos.y = al["position"]["y"];
                ArrayList::g_hud->hasConfigPos = true;
            }
            if (al.contains("hudScale") && ArrayList::g_hud) {
                ArrayList::g_hud->scale = al["hudScale"];
            }
        }
        if (visuals.contains("MotionBlur")) {
            if (visuals["MotionBlur"].contains("enabled")) {
                MotionBlur::g_motionBlurEnabled = visuals["MotionBlur"]["enabled"];
            }
        }
        if (visuals.contains("Keystrokes")) {
            if (visuals["Keystrokes"].contains("enabled")) {
                Keystrokes::g_showKeystrokes = visuals["Keystrokes"]["enabled"];
            }
            if (visuals["Keystrokes"].contains("scale")) Keystrokes::g_keystrokesUIScale = visuals["Keystrokes"]["scale"];
            if (visuals["Keystrokes"].contains("blurEffect")) Keystrokes::g_keystrokesBlurEffect = visuals["Keystrokes"]["blurEffect"];
            if (visuals["Keystrokes"].contains("rounding")) Keystrokes::g_keystrokesRounding = visuals["Keystrokes"]["rounding"];
            if (visuals["Keystrokes"].contains("showBg")) Keystrokes::g_keystrokesShowBg = visuals["Keystrokes"]["showBg"];
            if (visuals["Keystrokes"].contains("rectShadow")) Keystrokes::g_keystrokesRectShadow = visuals["Keystrokes"]["rectShadow"];
            if (visuals["Keystrokes"].contains("border")) Keystrokes::g_keystrokesBorder = visuals["Keystrokes"]["border"];
            if (visuals["Keystrokes"].contains("glow")) Keystrokes::g_keystrokesGlow = visuals["Keystrokes"]["glow"];
            if (visuals["Keystrokes"].contains("glowEnabled")) Keystrokes::g_keystrokesGlowEnabled = visuals["Keystrokes"]["glowEnabled"];
            if (visuals["Keystrokes"].contains("glowSpeed")) Keystrokes::g_keystrokesGlowSpeed = visuals["Keystrokes"]["glowSpeed"];
            if (visuals["Keystrokes"].contains("keySpacing")) Keystrokes::g_keystrokesKeySpacing = visuals["Keystrokes"]["keySpacing"];
            if (visuals["Keystrokes"].contains("edSpeed")) Keystrokes::g_keystrokesEdSpeed = visuals["Keystrokes"]["edSpeed"];
            if (visuals["Keystrokes"].contains("textShadow")) Keystrokes::g_keystrokesTextShadow = visuals["Keystrokes"]["textShadow"];
            if (visuals["Keystrokes"].contains("showMouseButtons")) Keystrokes::g_keystrokesShowMouseButtons = visuals["Keystrokes"]["showMouseButtons"];
            if (visuals["Keystrokes"].contains("showSpacebar")) Keystrokes::g_keystrokesShowSpacebar = visuals["Keystrokes"]["showSpacebar"];
            if (visuals["Keystrokes"].contains("fontName") && visuals["Keystrokes"]["fontName"].is_string()) {
                Keystrokes::g_fontName = visuals["Keystrokes"]["fontName"];
            }

            if (visuals["Keystrokes"].contains("position") && Keystrokes::g_keystrokesHud) {
                Keystrokes::g_keystrokesHud->pos.x = visuals["Keystrokes"]["position"]["x"];
                Keystrokes::g_keystrokesHud->pos.y = visuals["Keystrokes"]["position"]["y"];
                Keystrokes::g_keystrokesHud->hasConfigPos = true;
            }
            if (visuals["Keystrokes"].contains("hudScale") && Keystrokes::g_keystrokesHud) {
                Keystrokes::g_keystrokesHud->scale = visuals["Keystrokes"]["hudScale"];
            }
            // Load Keystrokes colors
            if (visuals["Keystrokes"].contains("colors")) {
                auto& colors = visuals["Keystrokes"]["colors"];
                if (colors.contains("bgColor") && colors["bgColor"].size() == 4) {
                    Keystrokes::g_keystrokesBgColor = ImVec4(colors["bgColor"][0], colors["bgColor"][1], 
                                                               colors["bgColor"][2], colors["bgColor"][3]);
                }
                if (colors.contains("enabledColor") && colors["enabledColor"].size() == 4) {
                    Keystrokes::g_keystrokesEnabledColor = ImVec4(colors["enabledColor"][0], colors["enabledColor"][1], 
                                                                    colors["enabledColor"][2], colors["enabledColor"][3]);
                }
                if (colors.contains("textColor") && colors["textColor"].size() == 4) {
                    Keystrokes::g_keystrokesTextColor = ImVec4(colors["textColor"][0], colors["textColor"][1], 
                                                                 colors["textColor"][2], colors["textColor"][3]);
                }
                if (colors.contains("textEnabledColor") && colors["textEnabledColor"].size() == 4) {
                    Keystrokes::g_keystrokesTextEnabledColor = ImVec4(colors["textEnabledColor"][0], colors["textEnabledColor"][1], 
                                                                        colors["textEnabledColor"][2], colors["textEnabledColor"][3]);
                }
                if (colors.contains("borderColor") && colors["borderColor"].size() == 4) {
                    Keystrokes::g_keystrokesBorderColor = ImVec4(colors["borderColor"][0], colors["borderColor"][1], 
                                                                        colors["borderColor"][2], colors["borderColor"][3]);
                }
                if (colors.contains("glowColor") && colors["glowColor"].size() == 4) {
                    Keystrokes::g_keystrokesGlowColor = ImVec4(colors["glowColor"][0], colors["glowColor"][1], 
                                                                        colors["glowColor"][2], colors["glowColor"][3]);
                }
                if (colors.contains("glowEnabledColor") && colors["glowEnabledColor"].size() == 4) {
                    Keystrokes::g_keystrokesGlowEnabledColor = ImVec4(colors["glowEnabledColor"][0], colors["glowEnabledColor"][1], 
                                                                        colors["glowEnabledColor"][2], colors["glowEnabledColor"][3]);
                }
                if (colors.contains("rectShadowColor") && colors["rectShadowColor"].size() == 4) {
                    Keystrokes::g_keystrokesRectShadowColor = ImVec4(colors["rectShadowColor"][0], colors["rectShadowColor"][1], 
                                                                        colors["rectShadowColor"][2], colors["rectShadowColor"][3]);
                }
                if (colors.contains("textShadowColor") && colors["textShadowColor"].size() == 4) {
                    Keystrokes::g_keystrokesTextShadowColor = ImVec4(colors["textShadowColor"][0], colors["textShadowColor"][1], 
                                                                        colors["textShadowColor"][2], colors["textShadowColor"][3]);
                }
                if (colors.contains("enabledShadowColor") && colors["enabledShadowColor"].size() == 4) {
                    Keystrokes::g_keystrokesEnabledShadowColor = ImVec4(colors["enabledShadowColor"][0], colors["enabledShadowColor"][1], 
                                                                        colors["enabledShadowColor"][2], colors["enabledShadowColor"][3]);
                }
                if (colors.contains("disabledShadowColor") && colors["disabledShadowColor"].size() == 4) {
                    Keystrokes::g_keystrokesDisabledShadowColor = ImVec4(colors["disabledShadowColor"][0], colors["disabledShadowColor"][1], 
                                                                        colors["disabledShadowColor"][2], colors["disabledShadowColor"][3]);
                }
            }
        }
        if (visuals.contains("CPSCounter")) {
            if (visuals["CPSCounter"].contains("enabled")) {
                CPSCounter::g_showCpsCounter = visuals["CPSCounter"]["enabled"];
            }
            if (visuals.contains("CPSCounter") && visuals["CPSCounter"].contains("position")) {
                if (CPSCounter::g_cpsHud) {
                    CPSCounter::g_cpsHud->pos.x = visuals["CPSCounter"]["position"]["x"];
                    CPSCounter::g_cpsHud->pos.y = visuals["CPSCounter"]["position"]["y"];
                    CPSCounter::g_cpsHud->hasConfigPos = true;
                }
            }
            if (visuals["CPSCounter"].contains("hudScale") && CPSCounter::g_cpsHud) {
                CPSCounter::g_cpsHud->scale = visuals["CPSCounter"]["hudScale"];
            }
            if (visuals["CPSCounter"].contains("fontName") && visuals["CPSCounter"]["fontName"].is_string()) {
                CPSCounter::g_fontName = visuals["CPSCounter"]["fontName"];
            }

            // Load CPSCounter colors
            if (visuals["CPSCounter"].contains("colors")) {
                auto& colors = visuals["CPSCounter"]["colors"];
                if (colors.contains("textColor") && colors["textColor"].size() == 4) {
                    CPSCounter::g_cpsTextColor = ImVec4(colors["textColor"][0], colors["textColor"][1], 
                                                         colors["textColor"][2], colors["textColor"][3]);
                }
                if (colors.contains("shadowColor") && colors["shadowColor"].size() == 4) {
                    CPSCounter::g_cpsCounterShadowColor = ImVec4(colors["shadowColor"][0], colors["shadowColor"][1], 
                                                                   colors["shadowColor"][2], colors["shadowColor"][3]);
                }
            }
        }
        
        if (visuals.contains("FPSOverlay")) {
            auto& fps = visuals["FPSOverlay"];
            if (fps.contains("enabled")) FPSOverlay::g_showFpsOverlay = fps["enabled"];
            if (fps.contains("scale")) FPSOverlay::g_fpsTextScale = fps["scale"];
            if (fps.contains("showBackground")) FPSOverlay::g_showBackground = fps["showBackground"];
            if (fps.contains("bgOpacity")) FPSOverlay::g_bgOpacity = fps["bgOpacity"];
            if (fps.contains("fontName") && fps["fontName"].is_string()) {
                FPSOverlay::g_fontName = fps["fontName"];
            }

            if (fps.contains("colors")) {
                auto& c = fps["colors"];
                if (c.contains("textColor")) {
                    FPSOverlay::g_fpsTextColor = ImVec4(c["textColor"][0], c["textColor"][1], c["textColor"][2], c["textColor"][3]);
                }
                if (c.contains("accentColor")) {
                    FPSOverlay::g_accentColor = ImVec4(c["accentColor"][0], c["accentColor"][1], c["accentColor"][2], c["accentColor"][3]);
                }
            }

            if (fps.contains("position") && FPSOverlay::g_fpsHud) {
                FPSOverlay::g_fpsHud->pos.x = fps["position"]["x"];
                FPSOverlay::g_fpsHud->pos.y = fps["position"]["y"];
                FPSOverlay::g_fpsHud->hasConfigPos = true;
            }
            if (fps.contains("hudScale") && FPSOverlay::g_fpsHud) {
                FPSOverlay::g_fpsHud->scale = fps["hudScale"];
            }
        }
        // PingCounter
        if (visuals.contains("PingCounter")) {
            auto& pc = visuals["PingCounter"];
            if (pc.contains("enabled")) PingCounter::g_showPingCounter = pc["enabled"];
            if (pc.contains("textScale")) PingCounter::g_pingTextScale = pc["textScale"];
            if (pc.contains("showBackground")) PingCounter::g_showBackground = pc["showBackground"];
            if (pc.contains("bgOpacity")) PingCounter::g_bgOpacity = pc["bgOpacity"];
            if (pc.contains("textShadow")) PingCounter::g_pingTextShadow = pc["textShadow"];
            if (pc.contains("updateInterval")) PingCounter::g_pingUpdateInterval = pc["updateInterval"];
            if (pc.contains("fontName") && pc["fontName"].is_string()) {
                PingCounter::g_fontName = pc["fontName"];
            }
            if (pc.contains("colors")) {
                auto& c = pc["colors"];
                if (c.contains("textColor") && c["textColor"].size() == 4)
                    PingCounter::g_pingTextColor = ImVec4(c["textColor"][0], c["textColor"][1], c["textColor"][2], c["textColor"][3]);
                if (c.contains("shadowColor") && c["shadowColor"].size() == 4)
                    PingCounter::g_pingCounterShadowColor = ImVec4(c["shadowColor"][0], c["shadowColor"][1], c["shadowColor"][2], c["shadowColor"][3]);
            }
            if (pc.contains("position") && PingCounter::g_pingHud) {
                PingCounter::g_pingHud->pos.x = pc["position"]["x"];
                PingCounter::g_pingHud->pos.y = pc["position"]["y"];
                PingCounter::g_pingHud->hasConfigPos = true;
            }
            if (pc.contains("hudScale") && PingCounter::g_pingHud) {
                PingCounter::g_pingHud->scale = pc["hudScale"];
            }
        }
        // PlayerInfo
        if (visuals.contains("PlayerInfo")) {
            auto& pi = visuals["PlayerInfo"];
            if (pi.contains("enabled")) PlayerInfo::g_showPlayerInfo = pi["enabled"];
            if (pi.contains("showHatLayer")) PlayerInfo::g_showHatLayer = pi["showHatLayer"];
            if (pi.contains("headSize")) PlayerInfo::g_headSize = pi["headSize"];
            if (pi.contains("textScale")) PlayerInfo::g_textScale = pi["textScale"];
            if (pi.contains("headRounded")) PlayerInfo::g_headRounded = pi["headRounded"];
            if (pi.contains("headRadius")) PlayerInfo::g_headRadius = pi["headRadius"];
            if (pi.contains("showBackground")) PlayerInfo::g_showBackground = pi["showBackground"];
            if (pi.contains("bgOpacity")) PlayerInfo::g_bgOpacity = pi["bgOpacity"];
            if (pi.contains("bgRadius")) PlayerInfo::g_bgRadius = pi["bgRadius"];
            if (pi.contains("showBorder")) PlayerInfo::g_showBorder = pi["showBorder"];
            if (pi.contains("colors")) {
                auto& c = pi["colors"];
                if (c.contains("borderColor") && c["borderColor"].size() == 4)
                    PlayerInfo::g_borderColor = ImVec4(c["borderColor"][0], c["borderColor"][1], c["borderColor"][2], c["borderColor"][3]);
                if (c.contains("nameColor") && c["nameColor"].size() == 4)
                    PlayerInfo::g_nameColor = ImVec4(c["nameColor"][0], c["nameColor"][1], c["nameColor"][2], c["nameColor"][3]);
            }
            if (pi.contains("position") && PlayerInfo::g_playerInfoHud) {
                PlayerInfo::g_playerInfoHud->pos.x = pi["position"]["x"];
                PlayerInfo::g_playerInfoHud->pos.y = pi["position"]["y"];
                PlayerInfo::g_playerInfoHud->hasConfigPos = true;
            }
            if (pi.contains("hudScale") && PlayerInfo::g_playerInfoHud) {
                PlayerInfo::g_playerInfoHud->scale = pi["hudScale"];
            }
        }
        if (visuals.contains("ClickGUI")) {
            auto& cg = visuals["ClickGUI"];
            if (cg.contains("enabled")) {
                ClickGUI::g_enabled = cg["enabled"];
                g_showMenu = ClickGUI::g_enabled;
                ::GUI::g_showMenu = g_showMenu;
            }
            if (cg.contains("bindKey")) ClickGUI::g_bindKey = cg["bindKey"];
            if (cg.contains("guiStyle")) ClickGUI::g_guiStyle = cg["guiStyle"];
            if (cg.contains("showParticles")) ClickGUI::g_showParticles = cg["showParticles"];
            if (cg.contains("showRiseBackground")) ClickGUI::g_showRiseBackground = cg["showRiseBackground"];
            if (cg.contains("bgOpacity")) ClickGUI::g_bgOpacity = cg["bgOpacity"];
            if (cg.contains("bgStyle")) ClickGUI::g_bgStyle = cg["bgStyle"];
            if (cg.contains("blurRadius")) ClickGUI::g_blurRadius = cg["blurRadius"];
            if (cg.contains("blurOpacity")) ClickGUI::g_blurOpacity = cg["blurOpacity"];
            if (cg.contains("theme")) GUI::ApplyThemePreset(cg["theme"]);
        }
    }

    // Misc modules
    if (config.contains("Misc")) {
        auto& misc = config["Misc"];
        if (misc.contains("UnlockFPS")) {
            if (misc["UnlockFPS"].contains("enabled")) {
                UnlockFPS::g_unlockFpsEnabled = misc["UnlockFPS"]["enabled"];
            }
            if (misc["UnlockFPS"].contains("fpsLimit")) {
                UnlockFPS::g_fpsLimit = misc["UnlockFPS"]["fpsLimit"];
            }
            if (misc["UnlockFPS"].contains("lowLatency")) {
                UnlockFPS::g_lowLatency = misc["UnlockFPS"]["lowLatency"];
            }
        }
        if (misc.contains("AntiAFK")) {
            auto& afk = misc["AntiAFK"];
            if (afk.contains("enabled")) AntiAFK::g_enabled = afk["enabled"];
            if (afk.contains("intervalSecs")) AntiAFK::g_intervalSecs = afk["intervalSecs"];
            if (afk.contains("pressDurationMs")) AntiAFK::g_pressDurationMs = afk["pressDurationMs"];
            if (afk.contains("randomizeKeys")) AntiAFK::g_randomizeKeys = afk["randomizeKeys"];
            if (afk.contains("jump")) AntiAFK::g_jump = afk["jump"];
        }
        if (misc.contains("Screenshot")) {
            auto& ss = misc["Screenshot"];
            if (ss.contains("enabled")) Screenshot::g_enabled = ss["enabled"];
            if (ss.contains("hotkey")) Screenshot::g_hotkey = ss["hotkey"];
            if (ss.contains("showHud")) Screenshot::g_showHud = ss["showHud"];
            if (ss.contains("notifyOnCapture")) Screenshot::g_notifyOnCapture = ss["notifyOnCapture"];
        }
    }

    Terminal::AddOutput("Configuration applied successfully.");
}

void ConfigManager::ReloadModulesAfterConfig() {
    // Re-enable or disable modules based on their saved state
    if (AutoSprint::g_autoSprintEnabled) {
        AutoSprint::Enable();
    } else {
        AutoSprint::Disable();
    }

    if (FullBright::g_fullBrightEnabled) {
        FullBright::Enable();
    } else {
        FullBright::Disable();
    }

    // Update the startup notification to reflect the actual bound key
    {
        std::string keyName = VkToNotifName(ClickGUI::g_bindKey);
        sprintf_s(g_notifMessage, "DLL loaded successfully.\nPress %s to open GUI.", keyName.c_str());
    }

    Terminal::AddOutput("Modules reloaded after config load.");
}

// ---------------------------------------------------------------------------
// Persistence helpers
// ---------------------------------------------------------------------------

void ConfigManager::SaveCurrentFile() {
    // Writes the name of the currently active config to current.txt
    try {
        if (configDir.empty() || currentConfig.empty()) return;
        std::filesystem::path p = std::filesystem::path(configDir) / "current.txt";
        std::ofstream f(p, std::ios::out | std::ios::trunc);
        if (f.is_open()) f << currentConfig;
    } catch (...) {}
}

std::string ConfigManager::LoadCurrentFile() {
    // Reads the last config name from current.txt, returns empty on failure
    try {
        if (configDir.empty()) return "";
        std::filesystem::path p = std::filesystem::path(configDir) / "current.txt";
        std::ifstream f(p);
        if (!f.is_open()) return "";
        std::string name;
        std::getline(f, name);
        // Trim whitespace
        while (!name.empty() && (name.back() == '\r' || name.back() == '\n' || name.back() == ' '))
            name.pop_back();
        return name;
    } catch (...) { return ""; }
}

bool ConfigManager::AutoSave() {
    // Silently saves the current settings back into the currently loaded config.
    // Returns false if no config is active or the save fails.
    if (currentConfig.empty()) return false;
    try {
        if (configDir.empty()) return false;
        nlohmann::json config = CollectCurrentConfig();
        std::filesystem::path filepath = std::filesystem::path(configDir) / (currentConfig + ".json");
        std::ofstream file(filepath, std::ios::out | std::ios::trunc);
        if (file.is_open()) {
            file << config.dump(4);
            return true;
        }
    } catch (...) {}
    return false;
}
