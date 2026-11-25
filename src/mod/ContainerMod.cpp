#include "mod/ContainerMod.h"

#include <filesystem>
#include <memory>

#include "container/MenuIntegration.h"
#include "submods/CrosshairSubMod.h"
#include "submods/HelloSubMod.h"

namespace modcontainer {

ContainerMod &ContainerMod::instance() {
    static ContainerMod instance;
    return instance;
}

ContainerMod::ContainerMod() : mSelf(*ll::mod::NativeMod::current()) {}

std::string ContainerMod::moduleId(std::string_view subModId) const {
    return mSelf.getId() + "_" + std::string(subModId);
}

void ContainerMod::saveConfig() {
    if (mConfigFile && !mConfigFile->save())
        mSelf.getLogger().warn("Failed to save config");
}

// The ONE place that lists the bundled sub-mods. Add new ones here.
void ContainerMod::registerBuiltins() {
    if (mBuiltinsRegistered)
        return;
    mRegistry.add(std::make_unique<HelloSubMod>());
    mRegistry.add(std::make_unique<CrosshairSubMod>());
    mBuiltinsRegistered = true;
}

bool ContainerMod::load() {
    auto &self = getSelf();
    self.getLogger().debug("Loading...");

    std::error_code ec;
    std::filesystem::create_directories(self.getDataDir(), ec);
    if (ec) {
        self.getLogger().error("Failed to create data directory {}: {}", self.getDataDir().string(),
                               ec.message());
        return false;
    }
    std::filesystem::create_directories(self.getConfigDir(), ec);
    if (ec) {
        self.getLogger().error("Failed to create config directory {}: {}",
                               self.getConfigDir().string(), ec.message());
        return false;
    }

    mConfigFile.emplace();
    if (!mConfigFile->load()) {
        self.getLogger().warn("Failed to load typed config");
        return false;
    }

    registerBuiltins();
    self.getLogger().info("Loaded {} with {} sub-mod(s) from {}", self.getName(), mRegistry.size(),
                          self.getModDir().string());
    return true;
}

bool ContainerMod::enable() {
    auto &self = getSelf();
    self.getLogger().debug("Enabling...");

    if (!config().enabled) {
        self.getLogger().info("Mod Container is disabled by config");
        return true;
    }

    // Register menu pieces from the lifecycle callback, synchronously (the preloader
    // attributes them to this mod only while it is calling us).
    menu::install(config(), self.getLogger());
    mRegistry.registerAll(*this);
    return true;
}

bool ContainerMod::disable() {
    getSelf().getLogger().debug("Disabling...");
    mRegistry.shutdown(*this);
    menu::uninstall();
    saveConfig();
    return true;
}

bool ContainerMod::unload() {
    getSelf().getLogger().debug("Unloading...");
    mConfigFile.reset();
    return true;
}

} // namespace modcontainer
