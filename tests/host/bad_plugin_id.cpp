#include "ModContainerPlugin.h"
static int start(const McHost *) { return 0; }
static void stop() {}
static const McPlugin kPlugin = {MC_PLUGIN_ABI_VERSION, "Bad Id!", "Bad Id", "invalid id", nullptr, 0, start, stop, nullptr};
extern "C" MC_PLUGIN_EXPORT const McPlugin *ModContainerGetPlugin(void) { return &kPlugin; }
