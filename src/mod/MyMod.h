#pragma once

#include <optional>

#include "mod/Config.h"
#include "mod/HookWatcher.h"

#include <pl/Mod.hpp>

namespace my_mod {

class MyMod {
  public:
    static MyMod &instance();

    MyMod();

    [[nodiscard]] ll::mod::NativeMod &getSelf() const { return mSelf; }

    bool load();
    bool enable();
    bool disable();
    bool unload();

  private:
    ll::mod::NativeMod &mSelf;
    ModConfig mConfig;
    std::optional<pl::config::ConfigFile<ModConfig>> mConfigFile;
    HookWatcher mWatcher;
};

} // namespace my_mod