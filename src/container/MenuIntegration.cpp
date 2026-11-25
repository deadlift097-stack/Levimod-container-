#include "container/MenuIntegration.h"

#include <pl/ModMenu.hpp>

namespace modcontainer::menu {

namespace {
constexpr const char *kOpenButtonId = "mod_container_open";
bool gButtonRegistered = false;
} // namespace

void install(const ModConfig &cfg, pl::log::Logger &logger) {
    // Only works inside a lifecycle callback (the preloader needs the owning mod id).
    if (!pl::modmenu::setFloatingMenuButtonVisible(cfg.showFloatingMenuButton))
        logger.warn("Could not change the floating menu button visibility");

    if (!cfg.showHudButton)
        return;

    // HUD buttons are snapshotted by the launcher when its overlay starts, so register
    // this one during enable() and keep it for the whole session.
    gButtonRegistered =
        pl::modmenu::ButtonBuilder(kOpenButtonId, "Open Mod Menu")
            .label("MODS")
            .behavior(pl::modmenu::ButtonBehavior::Click)
            .defaultVisible(true)
            .onEvent([](std::string_view, pl::modmenu::ButtonEvent event, float) {
                if (event == pl::modmenu::ButtonEvent::Click)
                    pl::modmenu::requestOpenMenu();
            })
            .registerButton();

    if (!gButtonRegistered)
        logger.warn("Mod Menu rejected the MODS button");
}

void uninstall() {
    if (gButtonRegistered) {
        pl::modmenu::unregisterButton(kOpenButtonId);
        gButtonRegistered = false;
    }
}

} // namespace modcontainer::menu
