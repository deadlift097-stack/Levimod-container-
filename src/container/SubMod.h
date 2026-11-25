#pragma once

#include <string>
#include <string_view>

#include <pl/Logger.hpp>
#include <pl/ModMenu.hpp>

#include "mod/Config.h"

namespace modcontainer {

// What the container offers to every sub-mod. Implemented by ContainerMod.
class SubModHost {
  public:
    virtual ~SubModHost() = default;

    // Shared config. Only touch it from start/stop/configChanged callbacks and
    // call saveConfig() afterwards; the container serialises those callbacks.
    virtual ModConfig &config() = 0;
    virtual void saveConfig() = 0;
    virtual pl::log::Logger &logger() = 0;

    // Mod Menu module id for a sub-mod id (needed for submitDrawCommands etc.).
    [[nodiscard]] virtual std::string moduleId(std::string_view subModId) const = 0;
};

// One bundled native feature. Each sub-mod becomes one Mod Menu module.
//
// Rules:
//  - no game hooks in the constructor; hook/patch work belongs in start()
//  - start()/stop() must be idempotent and exception-free
//  - keep id() stable after release: it is the Mod Menu module id suffix
class ISubMod {
  public:
    virtual ~ISubMod() = default;

    [[nodiscard]] virtual std::string id() const = 0;
    [[nodiscard]] virtual std::string displayName() const = 0;
    [[nodiscard]] virtual std::string description() const = 0;

    // Mirror of this sub-mod's `enabled` flag in ModConfig.
    [[nodiscard]] virtual bool enabledInConfig(const ModConfig &cfg) const = 0;
    virtual void setEnabledInConfig(ModConfig &cfg, bool enabled) const = 0;

    // Add Mod Menu entries (sliders, text fields...) to the module.
    virtual void describe(const ModConfig &cfg, pl::modmenu::ModuleBuilder &builder) const {
        (void)cfg;
        (void)builder;
    }

    virtual void start(SubModHost &host) = 0;
    virtual void stop(SubModHost &host) = 0;

    // A Mod Menu entry declared in describe() changed; `value` is always a string.
    virtual void configChanged(SubModHost &host, std::string_view key, std::string_view value) {
        (void)host;
        (void)key;
        (void)value;
    }
};

} // namespace modcontainer
