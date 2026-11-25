# Limitations

A `.levipack` **cannot**:

- load, run or display Java Fabric mods (`.jar`),
- translate Java bytecode into native ARM64 code,
- bridge Minecraft: Java Edition and Minecraft: Bedrock Edition,
- support "all Minecraft versions" with one binary.

Fabric mods are bytecode inside the JVM of Java Edition. `.levipack` mods are native
shared libraries (`.so`) loaded by the Preloader into Bedrock on Android. They are
different engines with different runtimes and architectures; no translator sits between
them. Any tool claiming a one-click Fabric to `.levipack` conversion should be treated with
heavy suspicion.

The legitimate route is to read a Fabric mod's source, reimplement the behaviour in C++
with the Preloader SDK, and register it as a sub-mod here.

Version support: `minecraft_versions: []` means "every version", but anything that hooks
game code must be re-checked per Bedrock release because signatures and layouts change.
Hook-free features (config, menu, HUD overlay, logging) are the most portable.
