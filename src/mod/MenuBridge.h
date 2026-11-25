#pragma once

#include <string>
#include <vector>

#include <pl/Mod.hpp>

namespace mod_container {

struct ModConfig;

class MenuBridge {
public:
    // Must be called synchronously inside load()/enable() so preloader attributes
    // the Mod Menu modules to this mod's ownerModId.
    bool registerAll(ll::mod::NativeMod& host, ModConfig& config,
                     std::function<void()> onPersistRequested);
    void unregisterAll();

private:
    std::vector<std::string> mRegisteredModules;
    bool mButtonRegistered = false;
};

} // namespace mod_container