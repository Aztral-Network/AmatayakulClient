/*
Under an4rch Development Public Source License 1.0
*/

#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <nlohmann/json.hpp>
#include "Modules/Visuals/ClickGUI/ClickGUI.hpp"
#include "GUI/GUI.hpp"

class ConfigManager {
public:
    // Configuration management
    static bool SaveConfig(const std::string& name);
    static bool LoadConfig(const std::string& name);
    static bool DeleteConfig(const std::string& name);
    static std::vector<std::string> ListConfigs();
    static bool OpenConfigDirectory();

    // Initialize config directory (creates Default, restores last config)
    static void Initialize();

    // Get config directory
    static const std::string& GetConfigDir() { return configDir; }

    // Get / set the name of the currently active config
    static const std::string& GetCurrentConfig() { return currentConfig; }
    static void SetCurrentConfig(const std::string& name) { currentConfig = name; }

    // Auto-save the currently loaded config (call after any setting change)
    static bool AutoSave();

    // Reload modules after config load (re-enable hooks, reinitialize, etc)
    static void ReloadModulesAfterConfig();

private:
    static std::string configDir;
    static std::string currentConfig;

    // Persist / restore which config was last used
    static void SaveCurrentFile();
    static std::string LoadCurrentFile();

    // Helper functions
    static nlohmann::json CollectCurrentConfig();
    static void ApplyConfig(const nlohmann::json& config);
};
