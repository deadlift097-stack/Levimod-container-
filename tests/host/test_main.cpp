// Host-side test of the plugin pipeline: PluginLoader -> ExternalSubMod -> SubModRegistry ->
// Mod Menu callbacks -> config persistence. Uses real .so files built from plugins/ and from
// the deliberately broken plugins in this folder. The Mod Menu itself is stubbed.
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <set>

#include "container/PluginLoader.h"
#include "container/SubModRegistry.h"
#include "test_stubs.h"

using namespace modcontainer;

namespace {

int gFailures = 0;

#define CHECK(cond)                                                                           \
    do {                                                                                      \
        if (!(cond)) {                                                                        \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << "  " #cond "\n";           \
            ++gFailures;                                                                      \
        }                                                                                     \
    } while (0)

class TestHost final : public SubModHost {
  public:
    ModConfig cfg;
    int saves = 0;
    pl::log::Logger log{"TestHost"};

    ModConfig &config() override { return cfg; }
    void saveConfig() override { ++saves; }
    pl::log::Logger &logger() override { return log; }
    std::string moduleId(std::string_view id) const override { return "test_" + std::string(id); }
};

bool logged(const std::string &needle) {
    return std::any_of(teststubs::logLines.begin(), teststubs::logLines.end(),
                       [&](const std::string &line) { return line.find(needle) != std::string::npos; });
}

const PluginState *stateOf(const ModConfig &cfg, const std::string &id) {
    for (const auto &s : cfg.plugins)
        if (s.id == id)
            return &s;
    return nullptr;
}

std::string settingOf(const PluginState &state, const std::string &key) {
    for (const auto &s : state.settings)
        if (s.key == key)
            return s.value;
    return "<missing>";
}

} // namespace

int main() {
    const std::filesystem::path pluginDir = PLUGIN_TEST_DIR;
    TestHost host;

    // ---- id validation ------------------------------------------------------------------
    CHECK(isValidPluginId("hello_plugin"));
    CHECK(isValidPluginId("a1"));
    CHECK(!isValidPluginId(""));
    CHECK(!isValidPluginId("Upper"));
    CHECK(!isValidPluginId("has space"));
    CHECK(!isValidPluginId(std::string(33, 'a')));

    // ---- loading ------------------------------------------------------------------------
    std::set<std::string> reserved = {"hello", "crosshair"}; // built-in ids
    auto result = loadPlugins({pluginDir, pluginDir / "does_not_exist"}, reserved, host.logger());
    CHECK(result.plugins.size() == 2);   // hello_plugin + failing
    CHECK(result.rejected == 3);         // bad abi, bad id, id clash with built-in
    CHECK(reserved.count("hello_plugin") == 1);
    CHECK(logged("rejected: ABI version 99"));
    CHECK(logged("invalid or missing id"));
    CHECK(logged("id 'hello' is already in use"));
    // Entry-less helper libs are skipped without a warning/error (p5 = warn, p6 = error).
    CHECK(std::none_of(teststubs::logLines.begin(), teststubs::logLines.end(), [](const std::string &l) {
        return l.find("libhelper_lib.so") != std::string::npos &&
               (l.rfind("p5 ", 0) == 0 || l.rfind("p6 ", 0) == 0);
    }));

    SubModRegistry registry;
    for (auto &p : result.plugins)
        registry.add(std::move(p));
    CHECK(registry.size() == 2);

    // ---- registration: plugins appear in the menu, OFF by default -----------------------
    registry.registerAll(host);
    CHECK(teststubs::modules.count("test_hello_plugin") == 1);
    CHECK(teststubs::modules.count("test_failing") == 1);
    const auto &hello = teststubs::modules.at("test_hello_plugin");
    CHECK(!hello.defaultEnabled);
    CHECK(hello.displayName == "Hello Plugin");
    CHECK(hello.configs.size() == 3);
    if (hello.configs.size() == 3) {
        CHECK(hello.configs[0].key == "message" && hello.configs[0].type == pl::modmenu::ConfigType::Text);
        CHECK(hello.configs[0].defaultValue == "Hello from a plugin!");
        CHECK(hello.configs[1].type == pl::modmenu::ConfigType::Toggle);
        CHECK(hello.configs[1].defaultValue == "false");
        CHECK(hello.configs[2].type == pl::modmenu::ConfigType::SliderInt);
        CHECK(hello.configs[2].minValue == "4" && hello.configs[2].maxValue == "64");
    }
    CHECK(!logged("Hello from a plugin!")); // not started yet

    // ---- toggle on: start() runs, state persisted ---------------------------------------
    const int savesBefore = host.saves;
    hello.onToggle("test_hello_plugin", true);
    CHECK(logged("[hello_plugin] Hello from a plugin! (plugin)"));
    CHECK(host.saves > savesBefore);
    const auto *state = stateOf(host.cfg, "hello_plugin");
    CHECK(state && state->enabled);

    // ---- settings: persisted + forwarded to the plugin ----------------------------------
    hello.onConfigChanged("test_hello_plugin", "show_marker", "true");
    {
        const auto &overlay = teststubs::overlays["test_hello_plugin"];
        CHECK(overlay.size() == 1);
        if (!overlay.empty()) {
            CHECK(overlay[0].type == pl::modmenu::DrawCommandType::RectFilled);
            CHECK(overlay[0].w == 12.0f); // slider default
            CHECK(overlay[0].color == 0xFFFFFFFFu);
        }
    }
    hello.onConfigChanged("test_hello_plugin", "marker_size", "30");
    {
        const auto &overlay = teststubs::overlays["test_hello_plugin"];
        CHECK(overlay.size() == 1 && overlay[0].w == 30.0f);
    }
    hello.onConfigChanged("test_hello_plugin", "no_such_key", "x"); // ignored, no crash
    state = stateOf(host.cfg, "hello_plugin");
    CHECK(state && settingOf(*state, "show_marker") == "true");
    CHECK(state && settingOf(*state, "marker_size") == "30");
    CHECK(state && settingOf(*state, "no_such_key") == "<missing>");

    // ---- config file round trip with plugin state ---------------------------------------
    {
        const auto path = std::filesystem::temp_directory_path() / "levi_host_test_config.json";
        const auto schema = std::filesystem::temp_directory_path() / "levi_host_test_schema.json";
        std::filesystem::remove(path);
        pl::config::ConfigFile<ModConfig> file(ModConfig{}, path, schema);
        file.value() = host.cfg;
        CHECK(file.save());
        pl::config::ConfigFile<ModConfig> reloaded(ModConfig{}, path, schema);
        CHECK(reloaded.load());
        const auto *loaded = stateOf(reloaded.value(), "hello_plugin");
        CHECK(loaded && loaded->enabled);
        CHECK(loaded && settingOf(*loaded, "marker_size") == "30");
        CHECK(reloaded.value().loadPlugins);
        std::filesystem::remove(path);
        std::filesystem::remove(schema);
    }

    // ---- a plugin whose start() fails must not crash or stay "running" ------------------
    const auto &failing = teststubs::modules.at("test_failing");
    failing.onToggle("test_failing", true);
    CHECK(logged("Sub-mod failing failed to start: plugin start() returned 7"));

    // ---- toggle off: stop() runs, overlay cleared ---------------------------------------
    hello.onToggle("test_hello_plugin", false);
    CHECK(logged("[hello_plugin] hello_plugin stopped"));
    CHECK(teststubs::overlays["test_hello_plugin"].empty());
    state = stateOf(host.cfg, "hello_plugin");
    CHECK(state && !state->enabled);

    // ---- shutdown unregisters everything -------------------------------------------------
    hello.onToggle("test_hello_plugin", true); // leave it running to test shutdown stop
    registry.shutdown(host);
    CHECK(teststubs::modules.empty());

    if (gFailures == 0)
        std::cout << "All plugin pipeline checks passed.\n";
    else
        std::cout << gFailures << " check(s) failed.\n";
    return gFailures == 0 ? 0 : 1;
}
