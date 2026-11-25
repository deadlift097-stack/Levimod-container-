#pragma once

#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

#include "container/SubMod.h"

namespace modcontainer {

// Owns the bundled sub-mods, registers one Mod Menu module for each, and keeps
// start/stop in step with the menu toggles. All callbacks are serialised by one
// mutex because the Mod Menu may invoke them from its own thread.
class SubModRegistry {
  public:
    void add(std::unique_ptr<ISubMod> mod);

    // Call from enable(): registers the modules and starts those enabled in config.
    void registerAll(SubModHost &host);

    // Call from disable(): stops running sub-mods and unregisters their modules.
    void shutdown(SubModHost &host);

    [[nodiscard]] std::size_t size() const { return mEntries.size(); }

  private:
    struct Entry {
        std::unique_ptr<ISubMod> mod;
        std::string moduleId;
        bool registered = false;
        bool running = false;
    };

    void startLocked(SubModHost &host, Entry &entry);
    void stopLocked(SubModHost &host, Entry &entry);
    void onToggle(SubModHost &host, std::size_t index, bool enabled);
    void onConfigChanged(SubModHost &host, std::size_t index, std::string_view key,
                         std::string_view value);

    std::vector<Entry> mEntries;
    std::mutex mMutex;
};

} // namespace modcontainer
