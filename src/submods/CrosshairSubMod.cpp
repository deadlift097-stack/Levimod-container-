#include "submods/CrosshairSubMod.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <span>
#include <string>

namespace modcontainer {

namespace {

// Menu values arrive as strings; parse without exceptions.
bool parseFloat(std::string_view text, float &out) {
    const std::string copy(text);
    char *end = nullptr;
    const float value = std::strtof(copy.c_str(), &end);
    if (end == copy.c_str() || !std::isfinite(value))
        return false;
    out = value;
    return true;
}

} // namespace

void CrosshairSubMod::describe(const ModConfig &cfg, pl::modmenu::ModuleBuilder &builder) const {
    builder.config("size", "Size", pl::modmenu::ConfigType::SliderFloat,
                   std::to_string(cfg.crosshair.size), "1", "64");
}

void CrosshairSubMod::draw(SubModHost &host) const {
    const auto surface = pl::modmenu::getHudSurfaceSize();
    if (surface.width <= 0.0f || surface.height <= 0.0f) {
        // The HUD may not be up yet when enable() runs. Toggling the module in the Mod
        // Menu starts it again once the surface exists.
        host.logger().warn("[crosshair] HUD surface not ready; toggle the module to redraw");
        return;
    }

    const float size = std::clamp(host.config().crosshair.size, 1.0f, 64.0f);

    pl::modmenu::DrawCommand rect;
    rect.type = pl::modmenu::DrawCommandType::RectFilled;
    rect.x = surface.width * 0.5f - size * 0.5f;
    rect.y = surface.height * 0.5f - size * 0.5f;
    rect.w = size;
    rect.h = size;
    rect.color = 0xFFFFFFFFu; // opaque white; identical in ARGB and ABGR

    pl::modmenu::submitDrawCommands(host.moduleId(id()), std::span<const pl::modmenu::DrawCommand>(&rect, 1));
}

void CrosshairSubMod::start(SubModHost &host) { draw(host); }

void CrosshairSubMod::stop(SubModHost &host) {
    pl::modmenu::submitDrawCommands(host.moduleId(id()), {});
}

void CrosshairSubMod::configChanged(SubModHost &host, std::string_view key, std::string_view value) {
    if (key != "size")
        return;
    float parsed = 0.0f;
    if (!parseFloat(value, parsed))
        return;
    host.config().crosshair.size = std::clamp(parsed, 1.0f, 64.0f);
    host.saveConfig();
    draw(host);
}

} // namespace modcontainer
