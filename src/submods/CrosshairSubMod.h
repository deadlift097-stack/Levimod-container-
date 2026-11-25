#pragma once

#include "container/SubMod.h"

namespace modcontainer {

// Hook-free example that uses the HUD overlay: a small white square at the screen
// centre, drawn with the portable RectFilled primitive. Needs no game signatures, so it
// keeps working across Minecraft versions.
class CrosshairSubMod final : public ISubMod {
  public:
    [[nodiscard]] std::string id() const override { return "crosshair"; }
    [[nodiscard]] std::string displayName() const override { return "Crosshair"; }
    [[nodiscard]] std::string description() const override {
        return "Draws a small square at the centre of the screen.";
    }

    [[nodiscard]] bool enabledInConfig(const ModConfig &cfg) const override {
        return cfg.crosshair.enabled;
    }
    void setEnabledInConfig(ModConfig &cfg, bool enabled) const override {
        cfg.crosshair.enabled = enabled;
    }

    void describe(const ModConfig &cfg, pl::modmenu::ModuleBuilder &builder) const override;
    void start(SubModHost &host) override;
    void stop(SubModHost &host) override;
    void configChanged(SubModHost &host, std::string_view key, std::string_view value) override;

  private:
    void draw(SubModHost &host) const;
};

} // namespace modcontainer
