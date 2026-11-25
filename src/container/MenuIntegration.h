#pragma once

#include <pl/Logger.hpp>

#include "mod/Config.h"

namespace modcontainer::menu {

// Call from enable(): the preloader only accepts these while a lifecycle callback runs.
void install(const ModConfig &cfg, pl::log::Logger &logger);

// Call from disable().
void uninstall();

} // namespace modcontainer::menu
