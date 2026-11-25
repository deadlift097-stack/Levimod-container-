#include "ModContainerPlugin.h"
static int start(const McHost *) { return 0; }
static void stop() {}
static const McPlugin kPlugin = {99u, "bad_abi", "Bad ABI", "wrong ABI version", nullptr, 0, start, stop, nullptr};
extern "C" MC_PLUGIN_EXPORT const McPlugin *ModContainerGetPlugin(void) { return &kPlugin; }
