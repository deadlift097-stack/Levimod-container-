#include "ModContainerPlugin.h"
static int start(const McHost *) { return 0; }
static void stop() {}
// Same id as a built-in sub-mod ("hello"), must be rejected.
static const McPlugin kPlugin = {MC_PLUGIN_ABI_VERSION, "hello", "Dup", "id clash", nullptr, 0, start, stop, nullptr};
extern "C" MC_PLUGIN_EXPORT const McPlugin *ModContainerGetPlugin(void) { return &kPlugin; }
