# Server Native Addon Template

A complete, standalone development template for creating native C/C++ addons for **Legacy Server**.

Native addons are dynamically loaded shared libraries (`.dll` on Windows, `.so` on Linux) that extend the server through a versioned, ABI-stable C API (`AddonAPI`). Addons can expose native routines to Lua, listen to server events, dispatch custom events, and invoke existing server-side Lua functions via dynamic call builders.

---

## 1. Project Structure

```
Legacy-Server-Addon-Template/
├── CMakeLists.txt              # Standalone CMake build configuration
├── README.md                   # Project documentation & quickstart
├── LICENSE                     # MIT license
├── .gitignore                  # Git ignore rules for build artifacts
│
├── include/
│   └── Addon.h                 # Public Addon SDK & C ABI specification
│
├── src/
│   └── Main.cpp                # Addon entry points & example implementation
│
├── examples/
│   └── example.lua             # Example Lua script demonstrating addon interop
│
└── docs/
    ├── API.md                  # Comprehensive Addon SDK API reference
    ├── BUILDING.md             # Detailed build & deployment instructions
    └── DEVELOPMENT.md          # Step-by-step developer guide for new addons
```

---

## 2. Requirements

- **CMake**: Version 3.15 or higher
- **C++ Compiler**: C++17 compatible
  - **Windows**: Microsoft Visual C++ (MSVC 2019 / 2022 via Visual Studio or Build Tools)
  - **Linux**: GCC 9+ or Clang 10+
- **Legacy Server**: Addon loader enabled (`addons/` directory)

---

## 3. Quickstart: Building & Installation

### Windows (PowerShell / Command Prompt):
```powershell
cmake -S . -B build
cmake --build build --config Release --parallel
```
Output: `build/Release/ExampleAddon.dll`

### Linux (Terminal):
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```
Output: `build/ExampleAddon.so`

### Deployment:
Copy the compiled binary (`ExampleAddon.dll` or `ExampleAddon.so`) into your Legacy Server's `addons/` directory:
```
Legacy-Server/
├── server.exe
├── setup.json
└── addons/
    └── ExampleAddon.dll
```
Upon server startup, the addon loader will automatically discover and initialize the addon.

---

## 4. Architecture & Lifecycle

### Addon Entry Points
Every addon exports two standard functions:

1. **`AddonMain`**:
   ```c
   ADDON_EXPORT bool AddonMain(const AddonAPI* api, void* addon);
   ```
   Invoked immediately after the dynamic library is loaded. Receives the `AddonAPI` function table and an opaque `addon` instance handle. Returning `true` keeps the addon loaded; returning `false` cleanly unloads the library without crashing the server.

2. **`AddonUnload`**:
   ```c
   ADDON_EXPORT void AddonUnload(void);
   ```
   Invoked during server shutdown before the library is unloaded from memory. Used to unregister functions, event handlers, and free resources.

---

## 5. Main Features Demonstrated

### 1. Exposing Native Functions to Lua
Addons register native functions using `api->RegisterFunction`. The server's internal Sol2 engine automatically manages type marshalling between Lua and C:

```cpp
static int HelloFunction(const Value* arguments, size_t argumentCount, Value* results, size_t maxResults, void* userData)
{
    const char* name = (argumentCount > 0 && arguments[0].type == ADDON_VALUE_STRING)
        ? arguments[0].stringValue
        : "World";

    char greeting[256];
    snprintf(greeting, sizeof(greeting), "Hello, %s!", name);

    if (results && maxResults > 0)
    {
        results[0] = ValueString(greeting);
        return 1; // 1 return value
    }
    return 0;
}

// In AddonMain:
api->RegisterFunction(addon, "hello", HelloFunction, nullptr);
```

In Lua:
```lua
local msg = hello("Developer")
print(msg) -- "Hello, Developer!"
```

---

### 2. Calling Existing Lua Functions (`sendClientMessage`)
The addon demonstrates dynamic invocation of `sendClientMessage`, a built-in server Lua function.

> [!NOTE]
> `sendClientMessage` is already registered by the server. The addon invokes it dynamically via `CallContext` without re-registering or reimplementing it.

```cpp
CallContext* call = api->BeginCall(addon, "sendClientMessage");

// Push arguments according to the server's Lua signature: (playerId, color, message)
api->PushInteger(call, playerId);
api->PushInteger(call, 0x00FF00FF); // Green RGBA
api->PushString(call, "Welcome to the server! (From Native Addon)");

if (api->ExecuteCall(call))
{
    // Function executed successfully
    if (api->GetResultCount(call) > 0)
    {
        bool sent = api->GetResultBoolean(call, 0);
    }
}
else if (api->HasError(call))
{
    api->Log(addon, ADDON_LOG_ERROR, api->GetError(call));
}

api->EndCall(call);
```

---

### 3. Subscribing to Server Events
Addons can subscribe to server lifecycle and gameplay events (e.g. `onPlayerConnect`, `onPlayerDisconnect`, `onPlayerSpawn`):

```cpp
static void OnPlayerConnectHandler(const char* eventName, const Value* arguments, size_t argumentCount, void* userData)
{
    if (argumentCount > 0 && (arguments[0].type == ADDON_VALUE_INTEGER || arguments[0].type == ADDON_VALUE_NUMBER))
    {
        int64_t playerId = arguments[0].integerValue;
        // Handle player connection...
    }
}

// In AddonMain:
api->RegisterEvent(addon, "onPlayerConnect", OnPlayerConnectHandler, nullptr);
```

---

### 4. Triggering Custom Addon Events
Addons can dispatch custom events into the server's Lua event bus (`LuaEvents`):

```cpp
Value eventArgs[1];
eventArgs[0] = ValueString("ExampleAddon is ready.");
api->TriggerEvent(addon, "ExampleAddon.OnReady", eventArgs, 1);
```

In Lua:
```lua
addEventHandler("ExampleAddon.OnReady", function(message)
    print("Received custom addon event: " .. message)
end)
```

---

## 6. ABI & Safety Guarantees

- **No C++ Standard Library in ABI**: The public API uses C-compatible structs (`Value`, `AddonAPI`) and opaque pointers (`CallContext`).
- **No Sol2 / Lua Dependencies**: Addons do not link against Lua or Sol2. The server handles all runtime interactions internally.
- **Reentrancy**: `CallContext` allocations are isolated per-call, making nested calls within callbacks fully thread-safe on the main Lua thread.
- **Protected Execution**: Lua errors during function execution are captured and returned via `api->GetError(call)` rather than crashing the server.

---

## 7. Troubleshooting & Common Mistakes

| Issue | Cause | Solution |
| :--- | :--- | :--- |
| Addon fails to load (`AddonMain` returns `false`) | API version mismatch | Ensure `Addon.h` version matches the server's `ADDON_API_VERSION`. |
| `ExecuteCall` returns `false` | Function name not found or invalid arguments | Verify the Lua function name and argument order against `docs/API.md`. Use `api->GetError(call)` to inspect the Lua runtime error message. |
| Memory leak on dynamic calls | Missing `EndCall` | Always invoke `api->EndCall(call)` after finishing execution and reading results. |
| Crash during server shutdown | Missing `AddonUnload` cleanup | Ensure `UnregisterFunction` and `UnregisterEvent` are called for all active handles in `AddonUnload`. |

---

## 8. Documentation Index

- [**API Reference**](docs/API.md): Full technical specification of all types, enums, functions, and callbacks.
- [**Building Guide**](docs/BUILDING.md): Detailed compilation instructions for Windows and Linux.
- [**Development Guide**](docs/DEVELOPMENT.md): Guide for creating and customizing new native addons.

---

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
