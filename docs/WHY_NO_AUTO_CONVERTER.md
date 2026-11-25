# Why Automatic Fabric `.jar` → `.levipack` Conversion Is Impossible

1. **Different Runtimes:** Fabric mods compile to JVM bytecode and depend on Fabric Loader. LeviLauncher `preload-native` mods are Android NDK C++ shared libraries (`.so`) running inside Minecraft Bedrock's native C++ engine (`libminecraftpe.so`).
2. **Mixins Cannot Be Translated:** Fabric `@Inject`, `@Redirect`, and `@ModifyVariable` annotations patch Java classes at runtime. Bedrock is a stripped C++ binary without Java classes; native mods must hook verified ARM64 function signatures via `pl::memory::hook`.
3. **API Mismatch:** Complex client rendering, custom GUIs, custom dimensions, and deep worldgen hooks do not map 1:1 to Bedrock native APIs.
4. **Per-Version Maintenance:** Native hooks, member offsets, and vtable slots change across Bedrock releases. A `.levipack` native mod cannot reliably target "all Minecraft versions" without verifying each target `libminecraftpe.so`.