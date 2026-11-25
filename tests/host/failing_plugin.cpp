#include "ModContainerPlugin.h"
static int start(const McHost *) { return 7; } // start() fails
static void stop() {}
static const McPlugin kPlugin = {MC_PLUGIN_ABI_VERSION, "failing", "Failing", "start returns 7", nullptr, 0, start, stop, nullptr};
extern "C" MC_PLUGIN_EXPORT const McPlugin *ModContainerGetPlugin(void) { return &kPlugin; }
