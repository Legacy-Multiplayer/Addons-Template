# Addon Developer Guide

This guide walks through creating custom native addons using this template.

---

## 1. Setting Up Your Own Addon

### Step 1: Copy and Rename the Project
1. Copy `Legacy-Server-Addon-Template` to a new folder (e.g., `MyCustomAddon`).
2. Open `CMakeLists.txt` and update the project name:
   ```cmake
   project(MyCustomAddon VERSION 1.0.0 LANGUAGES CXX)
   ```
   and the target output name:
   ```cmake
   set_target_properties(${PROJECT_NAME} PROPERTIES
       PREFIX ""
       OUTPUT_NAME "MyCustomAddon"
   )
   ```

### Step 2: Configure Addon Entry Points
In `src/Main.cpp`, keep the standard exported entry points:
- `ADDON_EXPORT bool AddonMain(const AddonAPI* api, void* addon)`
- `ADDON_EXPORT void AddonUnload(void)`

---

## 2. Calling Direct Server Subsystem APIs

Legacy Server exposes 15 direct subsystem tables on `AddonAPI`:
`actor`, `player`, `vehicle`, `object`, `pickup`, `gangzone`, `label`, `menu`, `messaging`, `netstats`, `textdraw`, `filesystem`, `variables`, `world`, `timers`.

These are direct function pointers into the server core, offering maximum performance without scripting overhead.

### Example: Player and Messaging Subsystems
```cpp
void HealAndNotifyPlayer(int playerId)
{
    if (g_API && g_API->player && g_API->messaging)
    {
        if (g_API->player->IsConnected(playerId))
        {
            // Set player health to 100
            g_API->player->SetHealth(playerId, 100.0f);

            // Send green client message
            g_API->messaging->SendClientMessage(playerId, 0x00FF00FF, "You have been healed!");
        }
    }
}
```

---

## 3. Exposing Native Functions to Lua

Addons can expose high-performance native C/C++ routines directly to server Lua scripts.

### 1. Implement Callback
```cpp
static int MyMathFunction(
    const Value* arguments,
    size_t argumentCount,
    Value* results,
    size_t maxResults,
    void* userData
) {
    if (argumentCount < 2)
    {
        return 0; // Return 0 values to Lua (evaluates to nil in script)
    }

    // Convert input arguments
    double a = (arguments[0].type == ADDON_VALUE_INTEGER) ? (double)arguments[0].integerValue : arguments[0].numberValue;
    double b = (arguments[1].type == ADDON_VALUE_INTEGER) ? (double)arguments[1].integerValue : arguments[1].numberValue;

    // Write return values into results buffer
    if (results && maxResults > 0)
    {
        results[0] = ValueNumber(a * b);
        return 1; // 1 return value
    }

    return 0;
}
```

### 2. Register in `AddonMain`
```cpp
static uint64_t g_MathFuncHandle = 0;

ADDON_EXPORT bool AddonMain(const AddonAPI* api, void* addon)
{
    g_MathFuncHandle = api->RegisterFunction(addon, "myMultiply", MyMathFunction, nullptr);
    return true;
}
```

### 3. Call from Lua Script
```lua
local product = myMultiply(6, 7)
print("Product: " .. product) -- 42
```

---

## 4. Subscribing to Server & Custom Events

Addons can listen to events dispatched by the server or by Lua scripts.

### 1. Implement Event Callback
```cpp
static void OnPlayerDisconnectHandler(
    const char* eventName,
    const Value* arguments,
    size_t argumentCount,
    void* userData
) {
    if (argumentCount > 1)
    {
        int64_t playerId = arguments[0].integerValue;
        int64_t reason = arguments[1].integerValue;
        // Handle disconnection...
    }
}
```

### 2. Register in `AddonMain`
```cpp
static uint64_t g_DisconnectEventHandle = 0;

ADDON_EXPORT bool AddonMain(const AddonAPI* api, void* addon)
{
    g_DisconnectEventHandle = api->RegisterEvent(addon, "onPlayerDisconnect", OnPlayerDisconnectHandler, nullptr);
    return true;
}
```

---

## 5. Exporting Global Variables & Constants to Lua

Addons can export global variables into the server's Lua runtime during `AddonMain`:

```cpp
api->SetGlobalString(addon, "MY_ADDON_VERSION", "2.0.0");
api->SetGlobalInteger(addon, "MY_ADDON_MAX_SLOTS", 64);
api->SetGlobalBoolean(addon, "MY_ADDON_ENABLED", true);
```

In Lua:
```lua
print(MY_ADDON_VERSION)   -- "2.0.0"
print(MY_ADDON_MAX_SLOTS) -- 64
```

---

## 6. Invoking Dynamic Lua Functions (`CallContext`)

For functions defined dynamically in Lua scripts or third-party gamemodes:

```cpp
CallContext* call = api->BeginCall(addon, "customGamemodeFunction");
if (call)
{
    api->PushInteger(call, playerId);
    api->PushString(call, "bonus_code");

    if (api->ExecuteCall(call))
    {
        if (api->GetResultCount(call) > 0)
        {
            bool granted = api->GetResultBoolean(call, 0);
        }
    }
    else if (api->HasError(call))
    {
        api->Log(addon, ADDON_LOG_WARN, api->GetError(call));
    }

    // Always release CallContext
    api->EndCall(call);
}
```

---

## 7. Resource Management & Clean Unload

Addons must clean up registered handles and heap allocations inside `AddonUnload`:

```cpp
ADDON_EXPORT void AddonUnload(void)
{
    if (g_API && g_Addon)
    {
        if (g_MathFuncHandle)
        {
            g_API->UnregisterFunction(g_Addon, g_MathFuncHandle);
            g_MathFuncHandle = 0;
        }

        if (g_DisconnectEventHandle)
        {
            g_API->UnregisterEvent(g_Addon, g_DisconnectEventHandle);
            g_DisconnectEventHandle = 0;
        }
    }
}
```

---

## 8. Architecture & ABI Guidelines

1. **Self-Contained ABI**: Do not link your addon directly against `server.exe` or private server static libraries. The server exposes everything through `Addon.h`.
2. **No STL in ABI**: Use primitive C types, `Value`, and `Vector3`/`Vector4` structures across the boundary.
3. **Thread Affinity**: All callbacks and function invocations run synchronously on the server's main thread.
4. **Error Resilience**: `CallContext` and `FunctionCallback` executions are protected — runtime errors are reported through `api->GetError(call)` rather than crashing the server.
