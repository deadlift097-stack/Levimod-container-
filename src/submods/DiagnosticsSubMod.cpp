#include "submods/DiagnosticsSubMod.h"

#include <dlfcn.h>

namespace mod_container {

void DiagnosticsSubMod::onLoad(ll::mod::NativeMod& host) {
    host.getLogger().info("[{}] Loaded (dataDir={})", id(), host.getDataDir().string());
}

void DiagnosticsSubMod::onEnable(ll::mod::NativeMod& host) {
    host.getLogger().info("[{}] Enabled", id());
}

void DiagnosticsSubMod::onGameAttached(ll::mod::NativeMod& host, void* gameHandle) {
    (void)gameHandle;
    Dl_info info{};
    if (dladdr(dlsym(RTLD_DEFAULT, "ANativeActivity_onCreate"), &info) != 0 && info.dli_fbase) {
        host.getLogger().info("[{}] Game attach verified (sample base={})", id(), info.dli_fbase);
    } else {
        host.getLogger().info("[{}] Game library settled (ready for verified signature hooks)",
                              id());
    }
}

void DiagnosticsSubMod::onDisable(ll::mod::NativeMod& host) {
    host.getLogger().info("[{}] Disabled", id());
}

void DiagnosticsSubMod::onUnload(ll::mod::NativeMod& host) {
    host.getLogger().info("[{}] Unloaded", id());
}

} // namespace mod_container