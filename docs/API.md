# C++ addon API

Use this repository's `main` branch. The essential Addons repository uses `master`.

The public C++ entry point is `#include "SDK/Plugin.h"`. The underlying C tables remain in `SDK/API.h` for low-level integrations.

C++ helper names below belong to `namespace SDK`. Qualify them, or use `using namespace SDK;` in a plugin source file. Native C ABI descriptors remain global.

## Lifecycle and registration

| Member | Behavior |
| --- | --- |
| `bool OnLoad()` | Override to register features. Return true on success. False or an exception rolls back owned resources. |
| `void OnUnload()` | Optional override; called before the framework releases bindings. Also called after partial initialization. |
| `REGISTER_PLUGIN(ClassName)` | Generate the instance, version/layout exports, and load/unload exports. Use once per DLL. |
| `Function(name, callable)` | Expose a typed function to Lua. |
| `Event(name, callable)` | Subscribe to an event using a typed prefix of its arguments; returns an event handle. |
| `Once(name, callable)` | Subscribe once, including protection against recursive delivery. |
| `Off(handle)` | Remove one of this addon's event subscriptions; returns success. Captures are released after a currently executing callback returns. |
| `Builtin(name)` | Essential-addon facility exposing an existing host `__core_` closure directly, preserving Lua tables and callbacks. Missing host functions fail initialization. |
| `RawFunction(name, callback, userData)` | Low-level `Value` callback for special result shapes. The framework owns registration, not external userData. |
| `OnCleanup(callback)` | Register a main-thread cleanup callback; callbacks run in reverse order. |
| `API()` / `Handle()` | Access the native host table / opaque owner for advanced operations. |

`Function` accepts named functions and ordinary typed lambdas. Supported values: bool, signed/unsigned integers within Lua's int64 range, float/double, length-delimited string, `CValue`, `CTable`, `CCallback`, and optional values. Missing/nil optional arguments become `std::nullopt`. Extra function arguments, fractional IDs, non-finite numbers, and overflow are errors. Void returns no Lua values; `std::tuple` returns multiple values. Strings and tables returned by typed functions are copied into binding-owned storage until the host converts them. Strings preserve embedded NUL bytes when using `std::string`; `const char*` remains a zero-terminated view.

```cpp
Function("describe", [](int id, std::optional<std::string> prefix)
{
    return std::make_tuple(id, prefix.value_or("Entity"));
});
```

Use `std::string` for retained text. Low-level `const char*` argument views are valid only during the call.

## Tables and retained Lua callbacks

Use `CTable` directly in `Function` arguments and results. No Lua module, JSON codec, Lua headers or separately linked Lua library is required. Integer keys and string keys are supported; tables may be nested. `CTable` and `CValue` own their data independently of Lua, so a copied table can be used by a worker. Nested snapshots are immutable; reading a nested table returns a table copy.

| Member | Behavior |
| --- | --- |
| `table.Set("name", value)` / `table.Set(int64_t{1}, value)` | Set a dictionary field or an integer key. Values accept nil (`CValue{}`), bool, int64, finite number, string and another table. |
| `table.Get<T>("name")` / `table.Get<T>(int64_t{1})` | Read into an owned type such as `std::string` with checked conversion; a missing key or wrong type throws. |
| `table.Get<T>("name", fallback)` | Use fallback only when absent; wrong types still throw. |
| `table.Contains(key)` / `table.Size()` | Test for a key or count stored entries, not Lua's sequence length. |
| `table.GetEntries()` | Read all integer/string key-value entries. |
| `CTable::Array({1, 2, "three"})` | Create a table with consecutive integer keys starting at 1. |
| `value.As<T>()` / `value.IsNil()` | Read an owned `CValue` or test nil. |
| `callback.IsValid()` | Main-thread check that its addon, script and Lua runtime still exist. |
| `callback.Invoke(args...)` | Invoke on the main thread with scalar/table arguments; returns false for a stale handle or Lua error. Lua errors are logged. Return values from the Lua callback are discarded. |
| `callback.Reset()` | Release this C++ shared handle; its last copy releases the native Lua reference. |

```cpp
Function("saveOptions", [this](CTable options, CCallback callback)
{
    const auto name = options.Get<std::string>("name");

    Async([options](CCancellation cancellation)
    {
        // Only owned data is used here. Never invoke Lua from the worker.
        return cancellation.IsCancelled() ? size_t{0} : options.Size();
    }, [callback, name](size_t count)
    {
        CTable result;
        result.Set("name", name);
        result.Set("fields", count);
        callback.Invoke(true, result);
    });
});
```

```lua
saveOptions({ name = "Example", enabled = true }, function(success, result)
    print(success, result.name, result.fields)
end)
```

Callbacks are explicit function arguments (`CCallback` or `std::optional<CCallback>`); functions inside data tables are rejected. Receiving a callback retains an addon-owned reference automatically. Copies share one retained reference. A callback belongs to the script resource active when it is passed to the addon. Releasing that resource, resetting/destroying Lua, failing addon initialization or unloading its addon invalidates retained handles. Cancellation does not automatically undo external work that already completed. Callbacks are reusable until released; reset a stored callback yourself if it is intended to run once. A callback does not automatically validate a player ID or account session: the caller must do that.

Retain, validity checks and invocation belong on the server thread. Destruction/release from a worker is safe: the host defers the Lua unreference to its main loop. Keep callbacks in Async completion captures, not in workers. Do not retain references to a Lua runtime, table view, borrowed native callback token or raw entity pointer inside a worker.

Limits: 16 nested table levels, 4096 entries per table, 8192 value nodes and 4 MiB of copied string data per conversion arena. Cycles, non-integer/non-string keys, non-finite numbers, Lua threads, full userdata, and functions inside tables are rejected. Table metatables and identity/shared-alias relationships are not copied; nil fields disappear when converted back to Lua. Native result count is at most 16; invoking a retained callback accepts at most 16 arguments. There are at most 8192 live borrowed/retained callback references across addons.

### C ABI values

`ValueStringSized(data, length)` preserves binary strings; `ValueString(text)` measures a zero-terminated string. `ValueTable(entries, count)` borrows a stable array of `TableEntry { key, value }` during the call. Returned raw strings and tables must remain alive until the host has copied them after the callback returns. Use `CResults` or typed `Function` to avoid temporary-pointer bugs.

Incoming `PLUGIN_VALUE_FUNCTION` values contain a borrowed `referenceValue`, valid only for their native callback scope. `RetainLuaCallback(addon, borrowed)` returns a separate owned token; `ReleaseLuaCallback` releases it; `IsLuaCallbackValid` checks it; `InvokeLuaCallback` calls it safely. Handles are addon-specific and never recycled; cross-addon use is rejected. Never store an unretained token after returning from a raw callback.

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
| `CCancellation::IsCancelled()` | Worker checks whether its owner is shutting down. |

Intervals must be positive. Timer callbacks never overlap. Missed intervals are coalesced instead of firing a burst. Native addon timers are processed by the host loop, including while gameplay is paused. A throwing timer is cancelled and logged.

```cpp
After(1000, [this]()
{
    Log.Info("One second passed");
});
Async([](CCancellation cancellation)
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
Players.ForEach([this](const CEntity& player)
{
    API()->player->SetMoney(player.Id(), 1000);
});
```

## Native C interface

Subsystem tables: actor, player, vehicle, object, pickup, gangzone, label, menu, messaging, netstats, filesystem, world, timers, and audio. `audio->Play` / `audio->Stop` use registered audio assets, not URLs. `player->IsNPC` is available when using a shared player ID explicitly. Lua uses `getPlayerIPAddress` and `isPlayerAFK`; removed aliases and URL audio functions are not restored.

Native owned services are `CreateNativeTimer`, `CancelNativeTimer`, `QueueWork`, `IsWorkCancelled`, and `CancelOwnedTasks`. A successful registration transfers userData cleanup to the host via its dispose callback. Failure returns zero and leaves ownership with the caller. Raw C plugins must export `PluginMain`, `GetPluginApiVersion`, and `GetPluginSDKSignature`; optional `PluginUnload` releases plugin-owned resources. The C++ framework generates all four.
