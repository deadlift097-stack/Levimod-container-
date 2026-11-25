# Adding a sub-mod

Use `src/submods/HelloSubMod.*` as the template.

1. **Config.** In `src/mod/Config.h` add a struct (with `bool enabled`) and a member in
   `ModConfig`. Optionally add a `pl::config::Schema<>` specialisation for titles and
   limits. Bump `ModConfig::version` only if you change existing fields incompatibly.
2. **Class.** Create `src/submods/MyFeatureSubMod.{h,cpp}` deriving from `ISubMod`:
   `id()` (stable!), `displayName()`, `description()`, `enabledInConfig()` /
   `setEnabledInConfig()`, `start()`, `stop()`, and optionally `describe()` and
   `configChanged()`.
3. **Menu entries.** In `describe()` call `builder.config(key, title, ConfigType::..., default, min, max)`.
   Types: `Toggle`, `SliderInt`, `SliderFloat`, `Radio`, `Color`, `Keybind`, `Text`, `Button`.
4. **CMake.** Add the `.cpp` to `add_library(...)` in `CMakeLists.txt`.
5. **Register.** Add `mRegistry.add(std::make_unique<MyFeatureSubMod>());` in
   `ContainerMod::registerBuiltins()`.

Rules of thumb:

- `start()` / `stop()` must be idempotent and must not throw. Undo everything in `stop()`.
- Hooks and patches go in `start()`, never in constructors. The game library may not be
  mapped yet at enable time, so gate game-image access behind a readiness check.
- Keep callbacks short; they run on the menu's thread.
- Rebuild, re-package, and re-import after every change. If an update seems not to apply,
  the old files may have been kept: add them to `overwrite_files` in `manifest.json.in`.
