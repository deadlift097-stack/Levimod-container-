#pragma once

#include <string>
#include <string_view>

#include <pl/Config.hpp>

namespace my_mod {

struct ModConfig {
    int version = 1;
    bool enabled = true;
    std::string message = "Hello from my first Android mod!";
};

nlohmann::json makeDefaultConfigJson();
nlohmann::json makeConfigSchemaJson();

} // namespace my_mod

template <> struct pl::config::Schema<my_mod::ModConfig> {
    static constexpr std::string_view title = "My Android Mod Config";
    static constexpr std::string_view description = {};

    static constexpr FieldSchema field(std::string_view name) {
        if (name == "version")
            return {.title = "Version", .readOnly = true};
        if (name == "enabled")
            return {.title = "Enabled", .description = "Turns mod behavior on or off."};
        if (name == "message")
            return {.title = "Message", .description = "Message written when the mod is enabled."};
        return {};
    }
};