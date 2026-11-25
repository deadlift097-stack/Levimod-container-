#pragma once

#include <mutex>
#include <optional>

#include "mod/Config.h"
#include "mod/GameWatcher.h"
#include "mod/MenuBridge.h"

#include <pl/Mod.hpp>

namespace mod_container {

class ContainerMod {
public:
    static ContainerMod& instance();

    ContainerMod();

    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return mSelf; }

    bool load();
    bool enable();
    bool disable();
    bool unload();

    void persistConfig();

private:
    ll::mod::NativeMod& mSelf;
    ModConfig mConfig;
    std::optional<pl::config::ConfigFile<ModConfig>> mConfigFile;
    std::mutex mConfigMutex;
    MenuBridge mMenuBridge;
    GameWatcher mGameWatcher;
};

} // namespace mod_container