#pragma once

#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <pl/Logger.hpp>

#include "container/SubMod.h"

namespace modcontainer {

struct PluginLoadResult {
    std::vector<std::unique_ptr<ISubMod>> plugins;
    size_t rejected = 0;
};

// Loads every lib*.so found directly in `dirs` that exports ModContainerGetPlugin and passes
// validation. `reservedIds` holds ids already taken (built-in sub-mods); accepted ids are
// added to it so duplicates across folders are rejected too. Libraries stay loaded for the
// whole process (never dlclose'd) because callbacks may still be in flight at shutdown.
PluginLoadResult loadPlugins(const std::vector<std::filesystem::path> &dirs,
                             std::set<std::string> &reservedIds, pl::log::Logger &logger);

// Exposed for tests.
bool isValidPluginId(const std::string &id);

} // namespace modcontainer
