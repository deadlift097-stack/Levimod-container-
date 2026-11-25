#pragma once

#include "container/SubMod.h"

namespace modcontainer {

// Smallest useful sub-mod: logs a configurable message when started. Use it as the
// copy-paste starting point for new sub-mods.
class HelloSubMod final : public ISubMod {
  public:
    [[nodiscard]] std::string id() const override { return "hello"; }
    [[nodiscard]] std::string displayName() const override { return "Hello"; }
    [[nodiscard]] std::string description() const override {
        return "Writes a configurable message to the Android log.";
    }

    [[nodiscard]] bool enabledInConfig(const ModConfig &cfg) const override {
        return cfg.hello.enabled;
    }
    void setEnabledInConfig(ModConfig &cfg, bool enabled) const override {
        cfg.hello.enabled = enabled;
    }

    void describe(const ModConfig &cfg, pl::modmenu::ModuleBuilder &builder) const override;
    void start(SubModHost &host) override;
    void stop(SubModHost &host) override;
    void configChanged(SubModHost &host, std::string_view key, std::string_view value) override;
};

} // namespace modcontainer
