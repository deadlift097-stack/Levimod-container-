#include "container/SubModRegistry.h"

#include <exception>
#include <utility>

namespace modcontainer {

void SubModRegistry::add(std::unique_ptr<ISubMod> mod) {
    Entry entry;
    entry.mod = std::move(mod);
    mEntries.push_back(std::move(entry));
}

void SubModRegistry::startLocked(SubModHost &host, Entry &entry) {
    if (entry.running)
        return;
    try {
        entry.mod->start(host);
        entry.running = true;
    } catch (const std::exception &ex) {
        host.logger().error("Sub-mod {} failed to start: {}", entry.mod->id(), ex.what());
    }
}

void SubModRegistry::stopLocked(SubModHost &host, Entry &entry) {
    if (!entry.running)
        return;
    try {
        entry.mod->stop(host);
    } catch (const std::exception &ex) {
        host.logger().error("Sub-mod {} failed to stop: {}", entry.mod->id(), ex.what());
    }
    entry.running = false;
}

void SubModRegistry::registerAll(SubModHost &host) {
    std::lock_guard<std::mutex> lock(mMutex);

    for (std::size_t i = 0; i < mEntries.size(); ++i) {
        auto &entry = mEntries[i];
        entry.moduleId = host.moduleId(entry.mod->id());
        const bool enabled = entry.mod->enabledInConfig(host.config());

        // Do NOT call .modId(...): inside a lifecycle callback the preloader fills in
        // the owning mod id itself and rejects a mismatching one.
        pl::modmenu::ModuleBuilder builder(entry.moduleId, entry.mod->displayName());
        builder.description(entry.mod->description())
            .defaultEnabled(enabled)
            .onToggle([this, &host, i](std::string_view, bool on) { onToggle(host, i, on); })
            .onConfigChanged([this, &host, i](std::string_view, std::string_view key,
                                              std::string_view value) {
                onConfigChanged(host, i, key, value);
            });
        entry.mod->describe(host.config(), builder);

        entry.registered = builder.registerModule();
        if (!entry.registered) {
            host.logger().error("Mod Menu rejected module {} (see the Preloader log)",
                                entry.moduleId);
        }

        if (enabled)
            startLocked(host, entry);
    }
}

void SubModRegistry::shutdown(SubModHost &host) {
    std::lock_guard<std::mutex> lock(mMutex);
    for (auto &entry : mEntries) {
        stopLocked(host, entry);
        if (entry.registered) {
            pl::modmenu::unregisterModule(entry.moduleId);
            entry.registered = false;
        }
    }
}

void SubModRegistry::onToggle(SubModHost &host, std::size_t index, bool enabled) {
    std::lock_guard<std::mutex> lock(mMutex);
    if (index >= mEntries.size())
        return;
    auto &entry = mEntries[index];

    entry.mod->setEnabledInConfig(host.config(), enabled);
    if (enabled)
        startLocked(host, entry);
    else
        stopLocked(host, entry);
    host.saveConfig();
}

void SubModRegistry::onConfigChanged(SubModHost &host, std::size_t index, std::string_view key,
                                     std::string_view value) {
    std::lock_guard<std::mutex> lock(mMutex);
    if (index >= mEntries.size())
        return;
    try {
        mEntries[index].mod->configChanged(host, key, value);
    } catch (const std::exception &ex) {
        host.logger().error("Sub-mod {} rejected config {}: {}", mEntries[index].mod->id(),
                            std::string(key), ex.what());
    }
}

} // namespace modcontainer
