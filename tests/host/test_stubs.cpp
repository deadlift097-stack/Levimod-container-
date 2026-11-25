// Host-side replacements for the pieces the launcher provides at runtime.
#include "test_stubs.h"

#include <cstdarg>
#include <cstdio>
#include <string>

#include <pl/ModMenu.hpp>

namespace teststubs {
std::vector<std::string> logLines;
std::map<std::string, pl::modmenu::ModuleInfo> modules;
std::map<std::string, std::vector<pl::modmenu::DrawCommand>> overlays;
float surfaceW = 1920.0f;
float surfaceH = 1080.0f;
} // namespace teststubs

extern "C" int __android_log_print(int prio, const char *tag, const char *fmt, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(buffer, sizeof buffer, fmt, args);
    va_end(args);
    // Prefix with the priority (5 = warn) so tests can tell debug lines from warnings.
    teststubs::logLines.push_back("p" + std::to_string(prio) + " " + tag + ": " + buffer);
    return 0;
}

namespace pl::modmenu {
bool registerModule(const ModuleInfo &info) {
    teststubs::modules[info.moduleId] = info;
    return true;
}
void unregisterModule(std::string_view moduleId) { teststubs::modules.erase(std::string(moduleId)); }
void submitDrawCommands(std::string_view moduleId, std::span<const DrawCommand> commands) {
    teststubs::overlays[std::string(moduleId)] =
        std::vector<DrawCommand>(commands.begin(), commands.end());
}
HudSurfaceSize getHudSurfaceSize() { return {teststubs::surfaceW, teststubs::surfaceH}; }
} // namespace pl::modmenu
