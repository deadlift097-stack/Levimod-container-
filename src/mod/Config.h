#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <pl/Config.hpp>

namespace mod_container {

struct SubModConfigEntry {
    std::string id;
    std::string displayName;
    bool enabled = true;
};

struct ModConfig {
    int version = 1;
    bool enabled = true;
    bool registerHudButton = true;
    bool enableGameWatcher = false;
    int settleDelayMs = 5000;
    double menuOpacity = 0.9;
    std::string welcomeMessage = "Mod Container ready";
    std::vector<SubModConfigEntry> subMods = {
        {"core.diagnostics", "Runtime Diagnostics", true},
    };
};

nlohmann::json makeDefaultConfigJson();
nlohmann::json makeConfigSchemaJson();

} // namespace mod_container

template <> struct pl::config::Schema<mod_container::SubModConfigEntry> {
    static constexpr std::string_view title = "Sub-Mod Entry";
    static constexpr std::string_view description = "Configuration for an embedded native sub-mod.";

    static constexpr FieldSchema field(std::string_view name) {
        if (name == "id") {
            return {.title = "ID", .readOnly = true};
        }
        if (name == "displayName") {
            return {.title = "Display Name", .readOnly = true};
        }
        if (name == "enabled") {
            return {.title = "Enabled", .description = "Enable or disable this sub-mod."};
        }
        return {};
    }
};

template <> struct pl::config::Schema<mod_container::ModConfig> {
    static constexpr std::string_view title = "Mod Container Config";
    static constexpr std::string_view description =
        "Central settings for Mod Container and its registered native sub-mods.";

    static constexpr FieldSchema field(std::string_view name) {
        if (name == "version") {
            return {.title = "Config Version", .readOnly = true};
        }
        if (name == "enabled") {
            return {.title = "Enabled", .description = "Master switch for Mod Container."};
        }
        if (name == "registerHudButton") {
            return {.title = "HUD Button",
                    .description = "Register a floating MODS button in the LeviLauncher HUD."};
        }
        if (name == "enableGameWatcher") {
            return {.title = "Attach Game Watcher",
                    .description = "Poll with RTLD_NOLOAD and notify sub-mods after settle delay."};
        }
        if (name == "settleDelayMs") {
            return {.title = "Settle Delay (ms)",
                    .description = "Wait time after game library mapping before hook attach.",
                    .minimum = 0.0,
                    .maximum = 15000.0};
        }
        if (name == "menuOpacity") {
            return {.title = "Menu Opacity",
                    .description = "Stored opacity preference for container panels.",
                    .minimum = 0.25,
                    .maximum = 1.0};
        }
        if (name == "welcomeMessage") {
            return {.title = "Startup Log Message",
                    .description = "Message logged when the container enables."};
        }
        if (name == "subMods") {
            return {.title = "Sub-Mods",
                    .description = "Per-sub-mod enabled states persisted to disk."};
        }
        return {};
    }
};