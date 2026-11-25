# Plugins: adding mods to the container

A **plugin** is an extra native mod (one `.so`) that the container loads and lists in its
Mod Menu. You do not edit the container to add one.

Two ways to add a plugin:

| Way | How | Good for |
|---|---|---|
| **Bundle it** | Put a folder in `plugins/<name>/` and rebuild; it is packed into the `.levipack` under `submods/` | Your own mods, shipped together |
| **Drop it in** | Copy `lib<name>.so` into `<data dir>/submods/` on the device | Trying a prebuilt plugin without rebuilding |

Both folders are scanned at startup: `<mod dir>/submods/` and `<data dir>/submods/`. The
second one is created automatically on first run. Plugins appear in the Mod Menu **OFF** and
you switch them on there; the choice and their settings are saved in `config.json`.

> Plugins are native code from a `.so`. Only add ones you built or trust, and only for the
> same ABI as the container (arm64-v8a on modern phones).

## Write a plugin (bundled)

1. Copy `plugins/hello_plugin/` to `plugins/my_feature/` and rename the source file.
2. In `plugins/my_feature/CMakeLists.txt` use your target name:
   `add_container_plugin(my_feature SOURCES my_feature.cpp)`
3. In the source, include `ModContainerPlugin.h` (from `sdk/`) and export the entry point:

   ```cpp
   extern "C" MC_PLUGIN_EXPORT const McPlugin *ModContainerGetPlugin(void) { return &kPlugin; }
   ```
4. Fill in the `McPlugin` descriptor: stable `id` (`[a-z0-9_]`, 1-32 chars), `display_name`,
   `description`, up to 32 `McSetting`s, and the callbacks `start`, `stop`, optional
   `setting_changed`.
5. Build (`./scripts/package.ps1 -Abi arm64-v8a` or push to GitHub). Every folder under
   `plugins/` with a `CMakeLists.txt` is built and packaged automatically.

## What a plugin can do

Through the `McHost` passed to `start()`:

- `log`: write to the Android log under the container's name.
- `get_setting` / `set_setting`: read and persist its own settings. Values are text:
  toggles are `"true"`/`"false"`, sliders are numbers.
- `surface_size` / `draw_rects`: draw filled rectangles on the HUD overlay.

Anything more (hooks, patches, signatures, input) needs the Preloader SDK, which a plugin can
include and use like any mod. That is not covered by the stable plugin ABI and depends on
the game version.

## Rules

- Never let a C++ exception escape a callback.
- Call host functions (other than `log`) only from inside `start`/`stop`/`setting_changed`.
- `start()` returns `0` on success; any other value is logged as a failure.
- Undo everything in `stop()`: hooks, overlay, threads.
- Keep `id` stable after release; it is the key for the saved state.

## Rejected plugins

The container logs a warning and skips a plugin when: the library cannot be loaded, the ABI
version differs from `MC_PLUGIN_ABI_VERSION`, the id is invalid or already taken (including by
a built-in sub-mod), required callbacks are missing, or a setting is malformed. A `.so`
without the entry point is ignored silently (so helper libraries can live in the folder).
Check with `adb logcat | grep "Mod Container"`.

## Turning it off

Set `loadPlugins` to `false` in the container's config (needs a restart) to ignore the
folders entirely.

## Tests

`tests/host/` loads real `.so` plugins (the example plus deliberately broken ones) through the
loader, registry and Mod Menu callbacks with the launcher parts stubbed:

```
cmake -S . -B build-host -G Ninja -DLEVI_HOST_TESTS=ON
cmake --build build-host && ctest --test-dir build-host --output-on-failure
```

This proves the logic on a desktop. Loading `.so` files from the app's folders on a real
Android device is not covered and is still untested.
