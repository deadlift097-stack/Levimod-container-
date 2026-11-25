#include "submods/HelloSubMod.h"

namespace modcontainer {

void HelloSubMod::describe(const ModConfig &cfg, pl::modmenu::ModuleBuilder &builder) const {
    builder.config("message", "Message", pl::modmenu::ConfigType::Text, cfg.hello.message);
}

void HelloSubMod::start(SubModHost &host) {
    host.logger().info("[hello] {}", host.config().hello.message);
}

void HelloSubMod::stop(SubModHost &host) { host.logger().info("[hello] stopped"); }

void HelloSubMod::configChanged(SubModHost &host, std::string_view key, std::string_view value) {
    if (key != "message")
        return;
    host.config().hello.message = std::string(value);
    host.saveConfig();
    host.logger().info("[hello] message is now: {}", host.config().hello.message);
}

} // namespace modcontainer
