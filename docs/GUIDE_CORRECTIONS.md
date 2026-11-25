# Where the earlier guide disagreed with the real SDK

This project was written against the SDK headers in `LiteLDev/preloader-android` and the
official template, which are the compiled contract. These points from
`LEVIPACK_MOD_CONTAINER_GUIDE.txt` did not match them:

| Earlier guide | Real SDK / template |
|---|---|
| `#include <pl/modmenu/ModuleBuilder.hpp>` and friends, `pl/mod/ModManager.hpp`, `pl/core/*` | One header: `<pl/ModMenu.hpp>`; also `<pl/Mod.hpp>`, `<pl/Config.hpp>`, `<pl/Logger.hpp>` |
| `ModuleBuilder("id").name(...).addToggle(...).addSlider(...).registerModule()` | `ModuleBuilder(id, displayName).description(...).defaultEnabled(...).onToggle(...).config(key, title, ConfigType, default, min, max).registerModule()` |
| `ButtonBuilder(id).label().position().size().onClick()` | `ButtonBuilder(id, displayName).label().behavior().onEvent(...)`; position is controlled by the launcher |
| `pl::core::getModDataDir("mod_container")` | `getSelf().getDataDir()` / `getConfigDir()` on `ll::mod::NativeMod` |
| Hand-written JSON writer; `load()` was a stub | `pl::config::ConfigFile<T>` (typed, merged, schema written) |
| Manual `cmake` with `libpreloader.a`, NDK `android-21`, `.\build.ps1` | Template CMake fetches the SDK, NDK 28 / `android-28`, `c++_shared`, 16 KB page size, `scripts/package.ps1` |
| `config_schema` inside `manifest.json` | Config ships as `config/config.json` + `config/config.schema.json` |
| `.levipack` = zip of `.so` + manifest at the root | Template packs a folder named `MOD_ID/` containing manifest, `.so` and `config/` |
| `PL_REGISTER_MOD(ContainerMod, ContainerMod::instance())` with no `NativeMod` handle | Constructor must take `*ll::mod::NativeMod::current()` |
| `manifest.minecraft_versions: []` presented as "supports everything" | Empty means every version is allowed; hooking code still needs per-version testing |

What was kept from the guide: the "mod container + mod menu + settings + MODS button"
design, the honest Fabric limitation, and the sub-mod workflow.
