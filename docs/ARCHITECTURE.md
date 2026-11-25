# Architecture

```
LeviLauncher ──loads──▶ libmod_container.so
                          │ PL_REGISTER_MOD
                          ▼
                    ContainerMod  (implements SubModHost)
                     load()    config dirs, ConfigFile<ModConfig>, add sub-mods
                     enable()  menu::install(), SubModRegistry::registerAll()
                     disable() SubModRegistry::shutdown(), menu::uninstall(), save
                     unload()  release config
                          │
              ┌───────────┴────────────┐
              ▼                        ▼
        SubModRegistry            menu::install
   one Mod Menu module per       floating-button visibility +
   ISubMod, toggle ↔ start/stop  on-screen MODS button
```

## Plugins

Besides compiled-in sub-mods, `loadExternalPlugins()` (in `load()`) scans `<mod dir>/submods`
and `<data dir>/submods` for `.so` files. `PluginLoader` `dlopen`s each, calls
`ModContainerGetPlugin`, validates the C descriptor (`sdk/ModContainerPlugin.h`) and wraps it
in `ExternalSubMod`, which implements `ISubMod`. From then on a plugin is handled exactly
like a built-in sub-mod by `SubModRegistry`; its enabled flag and settings live in
`ModConfig::plugins`. Libraries are never `dlclose`d. See [PLUGINS.md](PLUGINS.md).

## Flow

1. `load()` creates `data/` and `config/`, loads `config/config.json` (created from the
   packaged defaults on first run), and instantiates the sub-mods.
2. `enable()` registers everything with the Mod Menu **synchronously**. The preloader
   attributes modules to this mod only while it is calling us, so never register from
   your own thread and never pass `.modId(...)`.
3. Each sub-mod becomes a module `mod_container_<id>`. Toggling it calls `start()` /
   `stop()` and writes `<sub-mod>.enabled` back to `config.json`.
4. Menu settings arrive as **strings** in `configChanged()`; sub-mods parse them without
   exceptions and persist them.
5. `disable()` stops running sub-mods and unregisters their modules.

All Mod Menu callbacks are serialised by one mutex in `SubModRegistry`, because the menu
may call them from its own thread.

## Config

`ModConfig` (`src/mod/Config.h`) is a plain aggregate with an integer `version`.
`pl::config::ConfigFile` serialises it by reflection, merges new default fields into old
files, and writes `config.schema.json` next to it. At build time `levi_config_generator`
writes the same two files into the package, so LeviLauncher can show a settings page
before the game ever runs.

## Assumptions (not verified on a device)

- The launcher keeps its own per-module enabled state; this project also mirrors it into
  `config.json` so both agree after a restart.
- `ButtonBehavior::Click` with `androidKeyCode = 0` delivers `ButtonEvent::Click` for the
  MODS button. HUD buttons are snapshotted when the overlay starts, so it may not appear on
  every launcher build.
- `RectFilled` with `color = 0xFFFFFFFF` is opaque white; surface size can be 0 during
  `enable()`, in which case the crosshair asks you to toggle it once.
- The preloader/launcher allows `dlopen` of `.so` files from the mod and data folders (the
  preloader does the same for the container itself). Not verified on a device.
- `setFloatingMenuButtonVisible` only works inside a lifecycle callback, so
  `showFloatingMenuButton` and `showHudButton` apply at next launch, not live.

## SDK version

`CMakeLists.txt` pins `preloader-android` to commit `cc08b40`, the one bundled by
LeviLauncher-Android at the time of writing. Older tags (<= 0.2.3) do not have the floating
button, `requestOpenMenu`, `getHudSurfaceSize` or `ConfigType::Text`, so the build fails
against them. If your launcher is older than that commit, the mod may fail to load with an
undefined-symbol error; update LeviLauncher.
