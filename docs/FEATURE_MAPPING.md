# Fabric Java → LeviLauncher Native C++ Feature Mapping

| Fabric Concept | Native Bedrock (`pl::` / BedrockTools) Equivalent | Feasibility |
|---|---|---|
| `ModInitializer` / `onInitialize()` | `PL_REGISTER_MOD` + `load()` / `enable()` / `disable()` / `unload()` | High |
| Config file (Cloth/JSON) | `pl::config::ConfigFile<ModConfig>` + `pl::config::Schema<ModConfig>` | High |
| `@Mixin` (`@Inject`, `@Redirect`) | `pl::memory::resolveSignature` + `pl::memory::hook` (`UNIQUE` patterns only) | Medium (requires RE per version) |
| Constant/branch tweak | `pl::memory::readBytes` + verify original + `pl::memory::writeBytes` | Medium (requires RE) |
| Virtual method override/call | `pl::memory::resolveVtableFunction` / RTTI vtable walk | Medium |
| Keybinds / Touch input | `pl::input::registerKeyCallback` / `registerTouchCallback` (no unregister API; gate inside callback) | High |
| Settings menu / HUD button | `pl::modmenu::ModuleBuilder` / `ButtonInfo` / `submitDrawCommands` | High |
| Client tick / Frame hook | Hook `LocalPlayer::normalTick` / `eglSwapBuffers` | Medium–High |
| Custom GUI screens | `MinecraftUIRenderContext` finish/drawText hooks or embedded Android `View` | Low–Medium |
| Custom dimensions / deep worldgen | No direct native SDK equivalent | Very Low / Not Recommended |