# Step-by-Step Porting Workflow

1. **Assess the Fabric mod:** Run `python3 tools/assess_fabric_mod.py your_mod.jar` to extract Mixins and Java subsystem dependencies.
2. **Separate essential vs optional features:** Drop features with no viable Bedrock path (e.g., Java-only shaders or custom dimensions).
3. **Locate Bedrock equivalents:** Extract `libminecraftpe.so` from your target APK and locate functions via string xrefs, RTTI vtables, or BedrockTools signatures.
4. **Verify uniqueness:** Add your `constexpr std::string_view` patterns and run `python3 tools/check_mod_signatures.py --source src/mod/... --binary libminecraftpe.so`.
5. **Implement in C++:** Configure `src/mod/Config.h`, implement lifecycle in `src/mod/MyMod.cpp`, and attach hooks asynchronously through `HookWatcher`.
6. **Build & Validate:**
   - `./scripts/package.sh arm64-v8a` (or `./scripts/package.ps1 -Abi arm64-v8a`)
   - `python3 tools/validate_levipack.py build-arm64-v8a/<mod>.levipack`
7. **Test on device:** Import `.levipack` into LeviLauncher and verify with `adb logcat`.