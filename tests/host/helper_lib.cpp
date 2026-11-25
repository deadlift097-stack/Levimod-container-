// A .so without the plugin entry point: must be skipped silently, not counted as rejected.
extern "C" int helper_value() { return 1; }
