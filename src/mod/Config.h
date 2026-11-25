#pragma once

#include <string>
#include <string_view>

#include <pl/Config.hpp>

namespace modcontainer {

// Per-sub-mod settings live in their own struct so every bundled mod owns one
// slice of config.json. Add a new struct + a member in ModConfig for each new
// sub-mod (see docs/ADDING_A_SUBMOD.md).

struct HelloConfig {
    bool enabled = true;
    std::string message = "Hello from Mod Container!";
};

struct CrosshairConfig {
    bool enabled = false;
    float size = 4.0f; // square edge length, in overlay pixels
};

struct ModConfig {
    int version = 1; // bump when the layout changes; ConfigFile rewrites on mismatch
    bool enabled = true;
    bool showHudButton = true;          // on-screen "MODS" button (applied at startup)
    bool showFloatingMenuButton = true; // launcher's floating menu button (applied at startup)
    HelloConfig hello;
    CrosshairConfig crosshair;
};

nlohmann::json makeDefaultConfigJson();
nlohmann::json makeConfigSchemaJson();

} // namespace modcontainer

template <> struct pl::config::Schema<modcontainer::HelloConfig> {
    static constexpr std::string_view title = "Hello sub-mod";
    static constexpr std::string_view description = {};

    static constexpr FieldSchema field(std::string_view name) {
        if (name == "enabled")
            return {.title = "Enabled", .description = "Starts the sub-mod when the game starts."};
        if (name == "message")
            return {.title = "Message", .description = "Line written to the Android log when started."};
        return {};
    }
};

template <> struct pl::config::Schema<modcontainer::CrosshairConfig> {
    static constexpr std::string_view title = "Crosshair sub-mod";
    static constexpr std::string_view description = {};

    static constexpr FieldSchema field(std::string_view name) {
        if (name == "enabled")
            return {.title = "Enabled", .description = "Draws a small square at the screen centre."};
        if (name == "size")
            return {.title = "Size", .description = "Edge length in overlay pixels.",
                    .minimum = 1.0, .maximum = 64.0};
        return {};
    }
};

template <> struct pl::config::Schema<modcontainer::ModConfig> {
    static constexpr std::string_view title = "Mod Container Config";
    static constexpr std::string_view description = {};

    static constexpr FieldSchema field(std::string_view name) {
        if (name == "version")
            return {.title = "Version", .readOnly = true};
        if (name == "enabled")
            return {.title = "Enabled", .description = "Master switch for the whole container."};
        if (name == "showHudButton")
            return {.title = "Show MODS button",
                    .description = "Registers an on-screen MODS button that opens the Mod Menu. Takes effect on next launch."};
        if (name == "showFloatingMenuButton")
            return {.title = "Show floating menu button",
                    .description = "Shows the launcher's floating menu button. Takes effect on next launch."};
        return {};
    }
};
