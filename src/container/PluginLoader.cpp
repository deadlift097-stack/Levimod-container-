#include "container/PluginLoader.h"

#include <dlfcn.h>

#include <algorithm>
#include <cctype>

#include "container/ExternalSubMod.h"

namespace modcontainer {

namespace {

constexpr size_t kMaxSettings = 32;

bool isValidKey(const char *key) {
    if (!key || !*key)
        return false;
    for (const char *c = key; *c; ++c)
        if (!std::isalnum(static_cast<unsigned char>(*c)) && *c != '_')
            return false;
    return true;
}

// Returns an empty string when the descriptor is acceptable, otherwise the reason.
std::string validate(const McPlugin *plugin) {
    if (plugin->abi_version != MC_PLUGIN_ABI_VERSION)
        return "ABI version " + std::to_string(plugin->abi_version) + " (expected " +
               std::to_string(MC_PLUGIN_ABI_VERSION) + ")";
    if (!plugin->id || !isValidPluginId(plugin->id))
        return "invalid or missing id (use [a-z0-9_], 1-32 chars)";
    if (!plugin->display_name || !*plugin->display_name)
        return "missing display_name";
    if (!plugin->start || !plugin->stop)
        return "start/stop callbacks are required";
    if (plugin->setting_count > kMaxSettings)
        return "too many settings (max 32)";
    if (plugin->setting_count > 0 && !plugin->settings)
        return "setting_count set but settings is null";
    for (size_t i = 0; i < plugin->setting_count; ++i) {
        const McSetting &s = plugin->settings[i];
        if (!isValidKey(s.key))
            return "setting " + std::to_string(i) + " has an invalid key";
        if (!s.title)
            return "setting '" + std::string(s.key) + "' has no title";
        if (s.type < MC_SETTING_TOGGLE || s.type > MC_SETTING_TEXT)
            return "setting '" + std::string(s.key) + "' has an unknown type";
    }
    return {};
}

} // namespace

bool isValidPluginId(const std::string &id) {
    if (id.empty() || id.size() > 32)
        return false;
    return std::all_of(id.begin(), id.end(), [](unsigned char c) {
        return std::islower(c) || std::isdigit(c) || c == '_';
    });
}

PluginLoadResult loadPlugins(const std::vector<std::filesystem::path> &dirs,
                             std::set<std::string> &reservedIds, pl::log::Logger &logger) {
    PluginLoadResult result;

    for (const auto &dir : dirs) {
        std::error_code ec;
        if (!std::filesystem::is_directory(dir, ec))
            continue;

        std::vector<std::filesystem::path> files;
        for (const auto &entry : std::filesystem::directory_iterator(dir, ec)) {
            if (entry.is_regular_file(ec) && entry.path().extension() == ".so")
                files.push_back(entry.path());
        }
        std::sort(files.begin(), files.end());

        for (const auto &file : files) {
            void *handle = dlopen(file.c_str(), RTLD_NOW | RTLD_LOCAL);
            if (!handle) {
                const char *err = dlerror();
                logger.warn("Plugin {} could not be loaded: {}", file.filename().string(),
                            err ? err : "unknown error");
                ++result.rejected;
                continue;
            }

            auto entry = reinterpret_cast<McPluginEntryFn>(dlsym(handle, MC_PLUGIN_ENTRY_SYMBOL));
            if (!entry) {
                // Not a container plugin (maybe a helper library); leave it alone.
                logger.debug("{} has no {}; skipped", file.filename().string(),
                             MC_PLUGIN_ENTRY_SYMBOL);
                dlclose(handle);
                continue;
            }

            const McPlugin *plugin = entry();
            if (!plugin) {
                logger.warn("Plugin {} returned no descriptor", file.filename().string());
                ++result.rejected;
                dlclose(handle);
                continue;
            }

            if (const auto reason = validate(plugin); !reason.empty()) {
                logger.warn("Plugin {} rejected: {}", file.filename().string(), reason);
                ++result.rejected;
                dlclose(handle);
                continue;
            }

            if (!reservedIds.insert(plugin->id).second) {
                logger.warn("Plugin {} rejected: id '{}' is already in use", file.filename().string(),
                            plugin->id);
                ++result.rejected;
                dlclose(handle);
                continue;
            }

            logger.info("Loaded plugin '{}' ({})", plugin->id, file.filename().string());
            result.plugins.push_back(std::make_unique<ExternalSubMod>(plugin, file));
        }
    }
    return result;
}

} // namespace modcontainer
