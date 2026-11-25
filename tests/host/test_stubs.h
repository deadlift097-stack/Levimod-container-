#pragma once
#include <map>
#include <string>
#include <vector>

#include <pl/ModMenu.hpp>

namespace teststubs {
extern std::vector<std::string> logLines;
extern std::map<std::string, pl::modmenu::ModuleInfo> modules;
extern std::map<std::string, std::vector<pl::modmenu::DrawCommand>> overlays;
extern float surfaceW, surfaceH;
} // namespace teststubs
