#include "mod/MenuBridge.h"
#include "mod/Config.h"
#include "mod/SubModRegistry.h"

#include <cerrno>
#include <cstdlib>
#include <string>
#include <string_view>

#include <pl/ModMenu.hpp>

namespace mod_container {

namespace {

constexpr const char* kMainModuleId = "mod_container.hub";
constexpr const char* kOpenButtonId = "mod_container.hud_toggle";

bool parseDoubleSafe(std::string_view text, double& out) noexcept {
    if (text.empty() || text.size() > 64) {
        return false;
    }
    std::string buf(text);
    errno = 0;
    char* end = nullptr;
    const double val = std::strtod(buf.c_str(), &end);
    if (errno != 0 || end == buf.c_str() || *end != '\0') {
        return false;
    }
    out = val;
    return true;
}

} // namespace

bool MenuBridge::registerAll(ll::mod::NativeMod& host, ModConfig& config,
                             std::function<void()> onPersistRequested) {
    unregisterAll();

    const std::string modId = host.getId();

    const bool mainOk =
        pl::modmenu::ModuleBuilder(kMainModuleId, "Mod Container")
            .modId(modId)
            .description("Manage native C++ sub-mods and container settings.")
            .defaultEnabled(config.enabled)
            .onToggle([&config, &host, onPersistRequested](std::string_view, bool enabled) {
                config.enabled = enabled;
                host.getLogger().info("Container toggled: {}", enabled ? "ON" : "OFF");
                if (onPersistRequested) {
                    onPersistRequested();
                }
            })
            .onConfigChanged([&config, &host, onPersistRequested](std::string_view,
                                                                  std::string_view key,
                                                                  std::string_view value) {
                if (key == "menuOpacity") {
                    double parsed = config.menuOpacity;
                    if (parseDoubleSafe(value, parsed)) {
                        if (parsed < 0.25) parsed = 0.25;
                        if (parsed > 1.0) parsed = 1.0;
                        config.menuOpacity = parsed;
                        host.getLogger().info("Updated menuOpacity -> {:.2f}", parsed);
                        if (onPersistRequested) {
                            onPersistRequested();
                        }
                    }
                }
            })
            .config("menuOpacity", "Menu Opacity", pl::modmenu::ConfigType::SliderFloat,
                    std::to_string(config.menuOpacity), "0.25", "1.0")
            .registerModule();

    if (mainOk) {
        mRegisteredModules.emplace_back(kMainModuleId);
    } else {
        host.getLogger().warn("Mod Menu rejected main module {}", kMainModuleId);
    }

    for (const auto& subMod : SubModRegistry::instance().all()) {
        const std::string subModuleId = std::string("mod_container.") + subMod->id();
        const bool subOk =
            pl::modmenu::ModuleBuilder(subModuleId, subMod->displayName())
                .modId(modId)
                .description(subMod->description())
                .defaultEnabled(subMod->isEnabled())
                .onToggle([subMod, &host, onPersistRequested](std::string_view, bool enabled) {
                    subMod->setEnabled(enabled);
                    if (enabled) {
                        subMod->onEnable(host);
                    } else {
                        subMod->onDisable(host);
                    }
                    host.getLogger().info("Sub-mod '{}' -> {}", subMod->id(),
                                          enabled ? "enabled" : "disabled");
                    if (onPersistRequested) {
                        onPersistRequested();
                    }
                })
                .registerModule();

        if (subOk) {
            mRegisteredModules.push_back(subModuleId);
        } else {
            host.getLogger().warn("Mod Menu rejected sub-mod module {}", subModuleId);
        }
    }

    if (config.registerHudButton) {
        mButtonRegistered =
            pl::modmenu::ButtonBuilder(kOpenButtonId, "MODS")
                .modId(modId)
                .moduleId(kMainModuleId)
                .label("MODS")
                .androidKeyCode(0)
                .behavior(pl::modmenu::ButtonBehavior::Toggle)
                .onEvent([](std::string_view, pl::modmenu::ButtonEvent event, float value) {
                    if (event == pl::modmenu::ButtonEvent::StateChanged) {
                        if (value > 0.5f) {
                            pl::modmenu::requestOpenMenu();
                        } else {
                            pl::modmenu::requestCloseMenu();
                        }
                    }
                })
                .registerButton();

        if (!mButtonRegistered) {
            // Fail-open: Mod Menu entry remains usable even if HUD button is rejected.
            host.getLogger().warn("HUD button {} registration failed; continuing without it",
                                  kOpenButtonId);
        }
    }

    return mainOk;
}

void MenuBridge::unregisterAll() {
    if (mButtonRegistered) {
        pl::modmenu::unregisterButton(kOpenButtonId);
        mButtonRegistered = false;
    }
    for (auto it = mRegisteredModules.rbegin(); it != mRegisteredModules.rend(); ++it) {
        pl::modmenu::unregisterModule(*it);
    }
    mRegisteredModules.clear();
}

} // namespace mod_container