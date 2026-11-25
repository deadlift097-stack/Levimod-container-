#include "mod/MyMod.h"

#include <filesystem>

#include <pl/Mod.hpp>

namespace my_mod {

MyMod &MyMod::instance() {
    static MyMod instance;
    return instance;
}

MyMod::MyMod() : mSelf(*ll::mod::NativeMod::current()) {}

bool MyMod::load() {
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
    mConfig = mConfigFile->value();

    self.getLogger().info("Loaded {} from {}", self.getName(), self.getModDir().string());
    return true;
}

bool MyMod::enable() {
    auto &self = getSelf();
    self.getLogger().debug("Enabling...");
    if (!mConfig.enabled) {
        self.getLogger().info("{} is disabled by config", self.getName());
        return true;
    }

    self.getLogger().info("Config message: {}", mConfig.message);

    // Example: if your rewritten Fabric mod hooks libminecraftpe.so functions,
    // use HookWatcher so hooks only resolve after libminecraftpe.so maps & settles:
    //
    // mWatcher.start([this]() {
    //     // 1. Verify signatures with pl::memory::resolveSignature(..., "libminecraftpe.so")
    //     // 2. Check target is inside game image (dladdr)
    //     // 3. Install via pl::memory::hook(...)
    // });

    return true;
}

bool MyMod::disable() {
    getSelf().getLogger().debug("Disabling...");
    mWatcher.stop();
    // Unhook and clean up enable-time state here.
    return true;
}

bool MyMod::unload() {
    getSelf().getLogger().debug("Unloading...");
    mWatcher.stop();
    mConfigFile.reset();
    return true;
}

} // namespace my_mod