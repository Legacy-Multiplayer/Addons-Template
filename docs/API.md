# Legacy Server Addon SDK — API Reference

This document provides the complete technical specification for the Legacy Server Native Addon SDK (`include/Addon.h`).

---

## 1. Overview & Architecture

Legacy Server native addons are dynamic shared libraries (`.dll` on Windows, `.so` on Linux) loaded directly into the server process. Addons interact with the server through a versioned, C-compatible function table (`AddonAPI`) and lightweight, ABI-safe data structures (`Value`, `CallContext`).

### Key Principles:
- **ABI Stability**: No C++ standard library structures (`std::string`, `std::vector`) or internal engine headers (Sol2, Lua) cross the ABI boundary.
- **Server Owns Runtime**: The server manages the Lua 5.4.1 state, Sol2 binding layer, and event dispatch pipeline.
- **Dynamic Invocations**: The addon can call any existing Lua function using the dynamic `CallContext` builder pattern without rigid argument buffers.

---

## 2. Types & Data Structures

### `LogLevel`
Severity levels for addon log messages sent through `api->Log`.

| Identifier | Value | Description |
| :--- | :--- | :--- |
| `ADDON_LOG_INFO` | `0` | Informational messages. |
| `ADDON_LOG_WARN` | `1` | Non-fatal warnings. |
| `ADDON_LOG_ERROR` | `2` | Error conditions. |
| `ADDON_LOG_DEBUG` | `3` | Verbose diagnostic messages. |

---

### `ValueType`
Enumerates the runtime types represented by a `Value` struct or returned by `CallContext`.

| Identifier | Value | Lua Equivalent | Notes |
| :--- | :--- | :--- | :--- |
| `ADDON_VALUE_NIL` | `0` | `nil` | Represents absence of a value. |
| `ADDON_VALUE_BOOLEAN` | `1` | `boolean` | `true` or `false`. |
| `ADDON_VALUE_INTEGER` | `2` | `integer` | 64-bit signed integer (`int64_t`). |
| `ADDON_VALUE_NUMBER` | `3` | `number` | 64-bit IEEE 754 floating-point (`double`). |
| `ADDON_VALUE_STRING` | `4` | `string` | Null-terminated UTF-8 string pointer (`const char*`). |
| `ADDON_VALUE_USERDATA` | `5` | `userdata` | Opaque memory/handle pointer (`void*`). |
| `ADDON_VALUE_TABLE` | `6` | `table` | Reserved for nested table representations. |
| `ADDON_VALUE_FUNCTION` | `7` | `function` | Reserved for first-class function references. |

---

### `Value`
An ABI-safe tagged union holding primitive data types passed to or from Lua.

```c
typedef struct Value {
    ValueType type;
    union {
        bool booleanValue;
        int64_t integerValue;
        double numberValue;
        const char* stringValue;
        void* pointerValue;
    };
} Value;
```

#### Helper Constructors:
- `Value ValueNil(void)`: Constructs a nil Value.
- `Value ValueBoolean(bool booleanValue)`: Constructs a boolean Value.
- `Value ValueInteger(int64_t integerValue)`: Constructs a 64-bit integer Value.
- `Value ValueNumber(double numberValue)`: Constructs a double-precision float Value.
- `Value ValueString(const char* stringValue)`: Constructs a string Value.
- `Value ValuePointer(void* pointerValue)`: Constructs a userdata pointer Value.

---

### `CallContext`
An opaque struct representing an active dynamic function call context.

```c
typedef struct CallContext CallContext;
```

- **Lifetime**: Allocated by `api->BeginCall(...)` and must be released via `api->EndCall(...)`.
- **Reentrancy**: Each `CallContext` instance is completely isolated. Nested calls (e.g. inside a callback) are fully supported.
- **Memory Safety**: String pointers returned from `GetResultString` remain valid throughout the lifetime of the `CallContext`.

---

## 3. Callback Signatures

### `FunctionCallback`
Signature for native C functions callable directly from Lua scripts.

```c
typedef int (*FunctionCallback)(
    const Value* arguments,
    size_t argumentCount,
    Value* results,
    size_t maxResults,
    void* userData
);
```

- **`arguments`**: Array of input arguments passed from the Lua caller.
- **`argumentCount`**: Number of elements in `arguments`.
- **`results`**: Buffer where the callback writes its return values.
- **`maxResults`**: Capacity of the `results` buffer (typically 16).
- **`userData`**: Context pointer passed during `RegisterFunction`.
- **Returns**: Number of return values written to `results` ($0..\text{maxResults}$).

---

### `EventCallback`
Signature for event handler functions listening to server or custom events.

```c
typedef void (*EventCallback)(
    const char* eventName,
    const Value* arguments,
    size_t argumentCount,
    void* userData
);
```

- **`eventName`**: Name of the event being dispatched (e.g., `"onPlayerConnect"`).
- **`arguments`**: Array of event parameters.
- **`argumentCount`**: Number of event parameters.
- **`userData`**: Context pointer passed during `RegisterEvent`.

---

## 4. Addon Entry Points

Every native addon must export `AddonMain`. `AddonUnload` is optional but strongly recommended.

### `AddonMain`
```c
ADDON_EXPORT bool AddonMain(const AddonAPI* api, void* addon);
```
- **Invoked**: Immediately upon loading the `.dll` or `.so`.
- **Parameters**:
  - `api`: Pointer to the server's function table.
  - `addon`: Opaque handle representing this addon instance.
- **Returns**: `true` to keep the addon loaded; `false` to cleanly unload without crashing.

### `AddonUnload`
```c
ADDON_EXPORT void AddonUnload(void);
```
- **Invoked**: During server shutdown or manual addon unloading before the shared library handle is freed.
- **Responsibilities**: Unregister active function/event handles and free addon-owned heap resources.

---

## 5. AddonAPI Function Reference

### Addon Metadata & Logging

#### `GetName`
```c
const char* (*GetName)(void* addon);
```
Returns the base name of the addon (e.g., `"ExampleAddon"`).

#### `GetPath`
```c
const char* (*GetPath)(void* addon);
```
Returns the absolute filesystem path of the loaded addon binary.

#### `Log`
```c
void (*Log)(void* addon, LogLevel level, const char* message);
```
Writes a formatted message to the server log and console with automatic `[Addon: <Name>]` prefixing.

---

### Function Registration

#### `RegisterFunction`
```c
uint64_t (*RegisterFunction)(
    void* addon,
    const char* functionName,
    FunctionCallback callback,
    void* userData
);
```
Registers a native C function into the server's Lua global namespace.
- **Returns**: A non-zero registration handle on success, or `0` on failure.

#### `UnregisterFunction`
```c
bool (*UnregisterFunction)(void* addon, uint64_t handle);
```
Removes a previously registered native function from the Lua state.

---

### Event Registration & Dispatching

#### `RegisterEvent`
```c
uint64_t (*RegisterEvent)(
    void* addon,
    const char* eventName,
    EventCallback callback,
    void* userData
);
```
Subscribes to an existing server event (e.g., `"onPlayerConnect"`, `"onPlayerDisconnect"`, `"onPlayerSpawn"`) or a custom addon event.
- **Returns**: A non-zero event registration handle on success, or `0` on failure.

#### `UnregisterEvent`
```c
bool (*UnregisterEvent)(void* addon, uint64_t handle);
```
Unsubscribes the event handler associated with `handle`.

#### `TriggerEvent`
```c
bool (*TriggerEvent)(
    void* addon,
    const char* eventName,
    const Value* arguments,
    size_t argumentCount
);
```
Dispatches an event into the server's Lua event bus (`LuaEvents`). Any Lua event handlers registered via `addEventHandler(eventName, ...)` will be invoked synchronously.

---

### Dynamic Function Calling (`CallContext`)

The `CallContext` builder pattern allows native addons to invoke any server Lua function with arbitrary arguments and return values.

#### `BeginCall`
```c
CallContext* (*BeginCall)(void* addon, const char* functionName);
```
Allocates and initializes a new dynamic call context targeting `functionName`. Supports global names (e.g. `"sendClientMessage"`) and nested table paths (e.g. `"math.floor"`).

#### `EndCall`
```c
void (*EndCall)(CallContext* context);
```
Frees the memory associated with the call context. Must always be called after execution and reading results.

#### Argument Push Functions:
- `bool (*PushNil)(CallContext* context);`
- `bool (*PushBoolean)(CallContext* context, bool value);`
- `bool (*PushInteger)(CallContext* context, int64_t value);`
- `bool (*PushNumber)(CallContext* context, double value);`
- `bool (*PushString)(CallContext* context, const char* value);`
- `bool (*PushValue)(CallContext* context, const Value* value);`

Appends an argument to the call's dynamic parameter list. Returns `true` on success.

#### `ExecuteCall`
```c
bool (*ExecuteCall)(CallContext* context);
```
Executes the function inside the server's Sol2 protected call environment.
- **Returns**: `true` if execution succeeded; `false` if the target was not found, was not callable, or threw a Lua runtime exception.

#### Result Query Functions:
- `size_t (*GetResultCount)(const CallContext* context);` — Returns the number of values returned by Lua ($0..M$).
- `ValueType (*GetResultType)(const CallContext* context, size_t index);` — Returns the type of the result at `index`.
- `bool (*GetResultBoolean)(const CallContext* context, size_t index);` — Retrieves boolean result.
- `int64_t (*GetResultInteger)(const CallContext* context, size_t index);` — Retrieves integer result.
- `double (*GetResultNumber)(const CallContext* context, size_t index);` — Retrieves floating-point number result.
- `const char* (*GetResultString)(const CallContext* context, size_t index);` — Retrieves string result.
- `bool (*GetResultValue)(const CallContext* context, size_t index, Value* outValue);` — Copies result into a `Value` struct.

#### Error Handling Functions:
- `bool (*HasError)(const CallContext* context);` — Returns `true` if an error occurred.
- `const char* (*GetError)(const CallContext* context);` — Returns the descriptive error message or Lua stack trace.

---

## 6. Calling Existing Lua Functions (`sendClientMessage`)

Legacy Server already registers core game functions in its Lua runtime. Native addons invoke these functions through `CallContext`.

> [!IMPORTANT]
> **Do not re-register built-in server functions.** Built-in functions such as `sendClientMessage` are already provided by the server. Your addon simply calls them.

### Real Server Lua Signature:
```lua
sendClientMessage(playerId: integer, color: integer, message: string) -> boolean
```

- **`playerId`**: Target player ID ($0..\text{MAX\_PLAYERS}-1$).
- **`color`**: Hexadecimal color code in RGBA/ARGB format (e.g. `0x00FF00FF` for green, `0xFFFFFFFF` for white).
- **`message`**: UTF-8 string to display in the client chat window.
- **Returns**: `true` if dispatched to the connected client; `false` if player is not connected or network is unavailable.

### Complete Native Invocation Example:
```cpp
bool SendMessageToPlayer(const AddonAPI* api, void* addon, int64_t playerId, int64_t color, const char* text)
{
    if (!api || !addon || !text) return false;

    // 1. Begin call context for "sendClientMessage"
    CallContext* call = api->BeginCall(addon, "sendClientMessage");
    if (!call) return false;

    // 2. Push arguments in exact order: (playerId, color, message)
    api->PushInteger(call, playerId);
    api->PushInteger(call, color);
    api->PushString(call, text);

    // 3. Execute protected call
    bool sent = false;
    if (api->ExecuteCall(call))
    {
        if (api->GetResultCount(call) > 0)
        {
            sent = api->GetResultBoolean(call, 0);
        }
    }
    else if (api->HasError(call))
    {
        api->Log(addon, ADDON_LOG_ERROR, api->GetError(call));
    }

    // 4. Clean up call context
    api->EndCall(call);
    return sent;
}
```
