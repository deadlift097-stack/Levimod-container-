#include "mod/ContainerMod.h"
#include "mod/SubModRegistry.h"

#include <filesystem>

namespace mod_container {

ContainerMod& ContainerMod::instance() {
    static ContainerMod inst;
    return inst;
}

ContainerMod::ContainerMod() : mSelf(*ll::mod::NativeMod::current()) {}

bool ContainerMod::load() {
    auto& self = getSelf();
    self.getLogger().debug("Loading...");

    std::error_code ec;
    std::filesystem::create_directories(self.getDataDir(), ec);
    if (ec) {
        self.getLogger().error("Failed to create data directory {}: {}",
                               self.getDataDir().string(), ec.message());
        return false;
    }

    std::filesystem::create_directories(self.getConfigDir(), ec);
    if (ec) {
        self.getLogger().error("Failed to create config directory {}: {}",
                               self.getConfigDir().string(), ec.message());
        return false;
    }

    SubModRegistry::instance().registerBuiltins();

    {
        std::lock_guard<std::mutex> lock(mConfigMutex);
        mConfigFile.emplace();
        if (!mConfigFile->load()) {
            self.getLogger().warn("Failed to load typed config; using defaults");
        } else {
            mConfig = mConfigFile->value();
        }
        SubModRegistry::instance().syncFromConfig(mConfig);
        if (mConfigFile) {
            mConfigFile->value() = mConfig;
            mConfigFile->save();
        }
    }

    SubModRegistry::instance().dispatchLoad(self);

    self.getLogger().info("Loaded {} ({} sub-mod(s)) from {}", self.getName(),
                          SubModRegistry::instance().all().size(), self.getModDir().string());
    return true;
}

bool ContainerMod::enable() {
    auto& self = getSelf();
    self.getLogger().debug("Enabling...");

    if (!mConfig.enabled) {
        self.getLogger().info("{} is disabled by config", self.getName());
        return true;
    }

    self.getLogger().info("Config message: {}", mConfig.welcomeMessage);

    mMenuBridge.registerAll(self, mConfig, [this]() { persistConfig(); });
    SubModRegistry::instance().dispatchEnable(self);

    if (mConfig.enableGameWatcher) {
        self.getLogger().info("Starting RTLD_NOLOAD game watcher (settleDelayMs={})",
                              mConfig.settleDelayMs);
        mGameWatcher.start(mConfig.settleDelayMs, [this](void* gameHandle) {
            getSelf().getLogger().info("libminecraftpe.so settled; notifying sub-mods");
            SubModRegistry::instance().dispatchGameAttached(getSelf(), gameHandle);
        });
    }

    return true;
}

bool ContainerMod::disable() {
    auto& self = getSelf();
    self.getLogger().debug("Disabling...");

    mGameWatcher.stop();
    SubModRegistry::instance().dispatchDisable(self);
    mMenuBridge.unregisterAll();
    persistConfig();

    return true;
}

bool ContainerMod::unload() {
    auto& self = getSelf();
    self.getLogger().debug("Unloading...");

    mGameWatcher.stop();
    SubModRegistry::instance().dispatchUnload(self);
    SubModRegistry::instance().clear();
    mConfigFile.reset();

    return true;
}

void ContainerMod::persistConfig() {
    std::lock_guard<std::mutex> lock(mConfigMutex);
    if (!mConfigFile) {
        return;
    }
    SubModRegistry::instance().syncToConfig(mConfig);
    mConfigFile->value() = mConfig;
    if (!mConfigFile->save()) {
        getSelf().getLogger().warn("Failed to persist config.json");
    }
}

} // namespace mod_container