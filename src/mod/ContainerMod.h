#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "container/SubModRegistry.h"
#include "mod/Config.h"

#include <pl/Mod.hpp>

namespace modcontainer {

class ContainerMod final : public SubModHost {
  public:
    static ContainerMod &instance();

    ContainerMod();

    [[nodiscard]] ll::mod::NativeMod &getSelf() const { return mSelf; }

    bool load();
    bool enable();
    bool disable();
    bool unload();

    // SubModHost
    ModConfig &config() override { return mConfigFile->value(); }
    void saveConfig() override;
    pl::log::Logger &logger() override { return mSelf.getLogger(); }
    [[nodiscard]] std::string moduleId(std::string_view subModId) const override;

  private:
    void registerBuiltins();
    void loadExternalPlugins();

    ll::mod::NativeMod &mSelf;
    std::optional<pl::config::ConfigFile<ModConfig>> mConfigFile;
    SubModRegistry mRegistry;
    bool mBuiltinsRegistered = false;
};

} // namespace modcontainer
