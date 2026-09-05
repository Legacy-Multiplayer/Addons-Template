# Server Native Addon Template

A complete, standalone development template for creating native C/C++ addons for **Legacy Server**.

Native addons are dynamically loaded shared libraries (`.dll` on Windows, `.so` on Linux) that extend the server through a versioned, ABI-stable C API (`AddonAPI`). Addons can call direct server subsystems (players, vehicles, objects, messaging, world, etc.), expose native routines to Lua, listen to server events, dispatch custom events, and invoke dynamic Lua functions.

---

## 1. Project Structure

```
Legacy-Server-Addon-Template/
├── CMakeLists.txt              # Standalone CMake build configuration
├── README.md                   # Project documentation & quickstart
├── LICENSE                     # MIT license
│
├── include/
│   └── Addon.h                 # Public Addon SDK & C ABI specification (15 subsystems)
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

### Windows (PowerShell / Command Prompt - 32-bit x86):
```powershell
cmake -S . -B build -A Win32 -DCMAKE_BUILD_TYPE=Release
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

### 1. Direct Server Subsystem APIs (15 Subsystems)
Addons have direct zero-overhead access to 15 core server subsystems:
`actor`, `player`, `vehicle`, `object`, `pickup`, `gangzone`, `label`, `menu`, `messaging`, `netstats`, `textdraw`, `filesystem`, `variables`, `world`, `timers`.

```cpp
if (api->player && api->player->IsConnected(playerId))
{
    const char* name = api->player->GetName(playerId);
    api->messaging->SendClientMessage(playerId, 0x00FF00FF, "Welcome!");
}
```

---

### 2. Exposing Native Functions to Lua
Addons register native functions using `api->RegisterFunction`:

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

### 3. Setting Global Lua Constants & Variables
Addons can export typed globals into Lua during initialization:

```cpp
api->SetGlobalString(addon, "EXAMPLE_ADDON_VERSION", "1.0.0");
api->SetGlobalInteger(addon, "EXAMPLE_ADDON_LOADED", 1);
```

---

### 4. Dynamic Function Invocation (`CallContext`)
For functions defined dynamically in Lua scripts:

```cpp
CallContext* call = api->BeginCall(addon, "onCustomLuaEvent");
if (call)
{
    api->PushInteger(call, 42);
    api->ExecuteCall(call);
    api->EndCall(call);
}
```

---

### 5. Subscribing to Server & Custom Events
Addons can subscribe to server lifecycle and gameplay events (e.g. `onPlayerConnect`):

```cpp
static void OnPlayerConnectHandler(const char* eventName, const Value* arguments, size_t argumentCount, void* userData)
{
    if (argumentCount > 0)
    {
        int playerId = (int)arguments[0].integerValue;
        // Handle player connection...
    }
}

// In AddonMain:
api->RegisterEvent(addon, "onPlayerConnect", OnPlayerConnectHandler, nullptr);
```

---

## 6. ABI & Safety Guarantees

- **No C++ Standard Library in ABI**: The public API uses C-compatible structs (`Value`, `Vector3`, `AddonAPI`) and opaque pointers (`CallContext`).
- **No Sol2 / Lua Direct Linking**: Addons do not link against Lua or Sol2. The server handles all runtime interactions internally.
- **Protected Execution**: Lua errors during dynamic function execution are captured and returned via `api->GetError(call)` rather than crashing the server.

---

## 7. Documentation Index

- [**API Reference**](docs/API.md): Full technical specification of all 15 subsystems, types, functions, and callbacks.
- [**Building Guide**](docs/BUILDING.md): Detailed compilation instructions for Windows and Linux.
- [**Development Guide**](docs/DEVELOPMENT.md): Guide for creating and customizing new native addons.

---

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
