# Mod Lifecycle & Crash Safety Rules

Distilled from `BedrockTools` / `preloader-android` field testing:

1. **Do NOT hook `dlopen`:** `enable()` runs before `libminecraftpe.so` is mapped. Hooking `dlopen` inside the Java launcher process causes recursion and startup crashes. Use `HookWatcher` (`dl_iterate_phdr` poll + 5s settle delay).
2. **Verify every signature offline:** Only hook patterns that resolve as `UNIQUE` (`1` match) across every supported `libminecraftpe.so` build via `tools/check_mod_signatures.py`.
3. **Verify before patching/hooking:** Check `dladdr(target)` belongs to `libminecraftpe.so`, and verify expected bytes before `pl::memory::writeBytes`.
4. **Fail open:** Always call the original function in detours unless intentionally cancelling a verified event. Never let C++ exceptions cross hook or lifecycle boundaries (`-fno-unwind-tables`).
5. **16 KB page alignment:** Keep `-Wl,-z,max-page-size=16384` in `CMakeLists.txt` for modern Android compatibility.
6. **Keep `MOD_ID` stable:** LeviLauncher derives mod directories, config storage, and Mod Menu ownership from the package folder name (`MOD_ID`).