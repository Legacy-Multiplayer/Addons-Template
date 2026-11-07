# C++ addon API

Use this repository's `main` branch. The essential Addons repository uses `master`.

The public C++ entry point is `#include "CAddon.h"`. The underlying C tables remain in `Addon.h` for low-level integrations.

## Lifecycle and registration

| Member | Behavior |
| --- | --- |
| `bool OnLoad()` | Override to register features. Return true on success. False or an exception rolls back owned resources. |
| `void OnUnload()` | Optional override; called before the framework releases bindings. Also called after partial initialization. |
| `REGISTER_ADDON(ClassName)` | Generate the instance, version/layout exports, and load/unload exports. Use once per DLL. |
| `Function(name, callable)` | Expose a typed function to Lua. |
| `Event(name, callable)` | Subscribe to an event using a typed prefix of its arguments; returns an event handle. |
| `Once(name, callable)` | Subscribe once, including protection against recursive delivery. |
| `Off(handle)` | Remove one of this addon's event subscriptions; returns success. Captures are released after a currently executing callback returns. |
| `Builtin(name)` | Essential-addon facility exposing an existing host `__core_` closure directly, preserving Lua tables and callbacks. Missing host functions fail initialization. |
| `RawFunction(name, callback, userData)` | Low-level `Value` callback for special result shapes. The framework owns registration, not external userData. |
| `OnCleanup(callback)` | Register a main-thread cleanup callback; callbacks run in reverse order. |
| `API()` / `Handle()` | Access the native host table / opaque owner for advanced operations. |

`Function` accepts named functions and ordinary typed lambdas. Supported values: bool, signed/unsigned integers within Lua's int64 range, float/double, string, and optional values. Missing/nil optional arguments become `std::nullopt`. Extra function arguments, fractional IDs, non-finite numbers, and overflow are errors. Void returns no Lua values; `std::tuple` returns multiple values. Strings returned by typed functions are copied into binding-owned storage until the host converts them.

```cpp
Function("describe", [](int id, std::optional<std::string> prefix)
{
    return std::make_tuple(id, prefix.value_or("Entity"));
});
```

Use `std::string` for retained text. Low-level `const char*` argument views are valid only during the call.

## Calls, logging, files and configuration

| Member | Behavior |
| --- | --- |
| `Call(name, args...)` | Invoke a Lua/host function; throw on failure; discard results. |
| `CallResult<T>(name, args...)` | Invoke and return its first result using checked conversion. Use owned types, especially `std::string`. |
| `Log.Info(format, args...)` | Log at info level with the addon's name supplied by the host. |
| `Log.Warning(format, args...)` / `Log.Error(...)` | Warning/error variants. Replace each `{}` with the next argument. |
| `Files.Exists(path)` | Check a resource-relative file path. |
| `Files.ReadText(path)` | Read UTF-8 text, up to 4 MiB; throw on failure. |
| `Files.WriteText(path, text)` | Write and flush text, up to 4 MiB; throw on failure. Not an atomic transaction. |
| `Config.Load(path)` | Load an INI-style key/value file transactionally. False if absent; malformed data throws. |
| `Config.Get<T>(key, fallback)` | Read a typed value; absent uses fallback, invalid stored data throws. |
| `Config.Set(key, value)` | Set one value in memory. Keys allow letters, numbers, dot, underscore and hyphen. |
| `Config.Save(path)` | Write the current configuration using the resource file API. |

Paths resolve within server resources, with the host's existing path restrictions. They are not relative to a developer's computer. Text functions reject embedded NUL data; binary files belong to the native filesystem/asset APIs. Configuration uses `key = "value"`, one entry per line, with `#` or `;` comments on separate lines. Bool values are `true`/`false`.

```cpp
Config.Load("example.ini");
const int limit = Config.Get<int>("limit", 10);
Config.Set("limit", limit);
Config.Save("example.ini");
```

## Timers and background work

| Member | Behavior |
| --- | --- |
| `Every(intervalMs, callback)` | Repeating main-thread timer; returns a handle. |
| `After(delayMs, callback)` | One-shot main-thread timer; returns a handle. |
| `CancelTimer(handle)` | Cancel this addon's timer; returns success. |
| `Async(worker, completion)` | Queue bounded worker work and return a handle; completion runs on the main thread. |
| `CAddonCancellation::IsCancelled()` | Worker checks whether its owner is shutting down. |

Intervals must be positive. Timer callbacks never overlap. Missed intervals are coalesced instead of firing a burst. Native addon timers are processed by the host loop, including while gameplay is paused. A throwing timer is cancelled and logged.

```cpp
After(1000, [this]()
{
    Log.Info("One second passed");
});
Async([](CAddonCancellation cancellation)
{
    // Pure bounded computation or independently owned I/O only.
    return cancellation.IsCancelled() ? std::string() : std::string("ready");
}, [this](std::string result)
{
    Log.Info("Worker result: {}", result);
});
```

Do not access Lua, `API()`, entity collections, `Files`, `Config`, or `Log` from the worker. Copy inputs before scheduling; revalidate entities in completion. Workers must finish or cooperate with cancellation: unloading waits for them before freeing the DLL. A worker exception logs an error and skips completion. Unload cancels timers and pending completions, waits for in-flight work, then disposes captured objects on the main thread. The host caps pending native jobs at 32, uses two worker threads, and caps native timers at 4096. Rejection throws from the C++ helper. There is no per-job force termination.

## Checked entity access

`Players`, `NPCs`, `Actors`, `Vehicles`, and `Objects` provide:

- `Find(id)` returns a checked entity reference.
- `reference.IsValid()` checks existence and its generation.
- `reference.Id()` returns the ID or throws if the reference is stale.
- `ForEach(callback)` snapshots references, then skips those deleted or replaced during iteration.

`Players` contains humans only; NPCs and actors have separate collections. `Objects` enumerates global objects; player-private objects still use their explicit native APIs. Entity helpers are main-thread only. An ID reused after disconnect or deletion does not validate an earlier reference. OnLoad runs before world pools exist, so create/query entities from `onMainScriptInit` or later events.

```cpp
Players.ForEach([this](const CAddonEntity& player)
{
    API()->player->SetMoney(player.Id(), 1000);
});
```

## Native C interface

Subsystem tables: actor, player, vehicle, object, pickup, gangzone, label, menu, messaging, netstats, filesystem, world, timers, and audio. `audio->Play` / `audio->Stop` use registered audio assets, not URLs. `player->IsNPC` is available when using a shared player ID explicitly. Lua uses `getPlayerIPAddress` and `isPlayerAFK`; removed aliases and URL audio functions are not restored.

Native owned services are `CreateNativeTimer`, `CancelNativeTimer`, `QueueWork`, `IsWorkCancelled`, and `CancelOwnedTasks`. A successful registration transfers userData cleanup to the host via its dispose callback. Failure returns zero and leaves ownership with the caller. Raw C plugins must export `AddonMain`, `GetAddonApiVersion`, and `GetAddonSDKSignature`; optional `AddonUnload` releases plugin-owned resources. The C++ framework generates all four.
