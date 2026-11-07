# Developing an addon

Use **main** for this template and **master** for the separate essential addons.

1. Rename `ExampleAddon` in `CMakeLists.txt`.
2. Derive a class from `CAddon` in `src/Main.cpp`.
3. Implement `OnLoad()` and register typed functions/events there.
4. Add `REGISTER_ADDON(YourClass)` once.
5. Build for the server's 32-bit architecture and install the binary in `addons/`.

The starter has a Lua `hello(name)` function, a connection event, and a once-only script-ready event. Additional source files can contain ordinary helper classes; list them as further arguments to `legacy_addon`.

## Ownership

Return false or throw from OnLoad to reject initialization. Registration failures automatically throw; a half-registered addon will not remain active. OnUnload must tolerate partial initialization. Use OnCleanup for external connections and resources you create. Cleanup runs once per load in reverse registration order, even if a prior cleanup callback throws.

Functions, events, timers, and background completions cannot remain callable after unload. Saved Lua closures report an unloaded-function error. The host cancels and joins asynchronous work before freeing its DLL. No worker may run Lua or mutate world state.

Native function results support up to the host's 16-value result limit. Typed tuple strings have independent storage. RawFunction callbacks must keep returned strings valid until the host converts them immediately after return; never return a pointer into a temporary local string.

## Entity lifetimes

Do not retain raw server pointers. Keep a checked `CAddonEntity`, then call IsValid before applying a change. This checks a lifetime generation, not just whether the numeric slot is occupied. Use Players for humans, NPCs for server-controlled players, and Actors for actors.

OnLoad runs before server world initialization. Register features there; defer entity work to onMainScriptInit or later callbacks. Server shutdown calls script-exit handlers while addon bindings and pools still exist, then unloads addons, then destroys pools and Lua.

## Updating the SDK

Replace all four SDK headers together and rebuild. Version remains 1, but this update is deliberately not binary compatible with previous addon builds. The layout fingerprint prevents accidental calls through mismatched tables. Do not use an old signature or add compatibility forwarding.

The template and essential addons share identical SDK headers and the same CMake helper. For complex existing Lua functions with table/callback values, use the existing host closure through Builtin; do not create a lossy scalar forwarding bridge.

## Validation

Build your DLL, test successful and failed initialization, unload/reload, missing/wrong Lua arguments, timers cancelling themselves, entity ID reuse, and background work during shutdown. A build does not establish in-game behavior. Validate Windows and Linux with their native toolchains before publishing binaries for both platforms.

## Formatting and entry declaration

Use `REGISTER_ADDON(YourClass)` once in a source file. There is no alias for the previous entry macro. It exports the same native loader functions, so this rename does not alter the host ABI or protocol.

Keep a blank line between function definitions and between independent logical steps inside functions. Keep related assignments together, spaces around operators, `if (`/`for (` spacing, and one statement per line. The included `.clang-format` maintains the shared SDK style. Do not concatenate method bodies or raw callback declarations to save vertical space.
