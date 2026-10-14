# Legacy Server Addon SDK — API Reference

This document provides the complete technical specification for the Legacy Server Native Addon SDK (`include/Addon.h`).

---

## 1. Overview & Architecture

Legacy Server native addons are dynamic shared libraries (`.dll` on Windows, `.so` on Linux) loaded directly into the server process. Addons interact with the server through a versioned (current ABI v5), C-compatible function table (`AddonAPI`) and lightweight, ABI-safe data structures (`Value`, `CallContext`, `Vector3`, `Vector4`).

### Key Principles:
- **Direct Subsystem Performance**: 13 direct server subsystem API tables provide native access to the server core (Players, Vehicles, Objects, Messaging, World, etc.).
- **Dynamic Invocations**: The addon can call any Lua script function dynamically using the `CallContext` builder pattern.
- **Global Lua Exports**: Addons can export typed global constants (`SetGlobalInteger`, `SetGlobalString`, etc.) directly into the Lua state.
- **ABI Stability**: No C++ standard library structures (`std::string`, `std::vector`) cross the ABI boundary.
- **Server Owns Runtime**: The server manages Lua state and lifecycle.

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

### POD Vectors

```c
typedef struct Vector3 {
    float x;
    float y;
    float z;
} Vector3;

typedef struct Vector4 {
    float x;
    float y;
    float z;
    float w;
} Vector4;
```

---

### `CallContext`
An opaque struct representing an active dynamic function call context.

```c
typedef struct CallContext CallContext;
```

- **Lifetime**: Allocated by `api->BeginCall(...)` and must be released via `api->EndCall(...)`.
- **Reentrancy**: Each `CallContext` instance is completely isolated.
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

## 5. AddonAPI Core Methods

### Addon Metadata & Logging
- `const char* (*GetName)(void* addon);` — Returns the addon's name.
- `const char* (*GetPath)(void* addon);` — Returns the absolute path of the loaded addon library.
- `void (*Log)(void* addon, LogLevel level, const char* message);` — Logs a message to the server console and log file.

### Function Registration
- `uint64_t (*RegisterFunction)(void* addon, const char* functionName, FunctionCallback callback, void* userData);` — Registers a native function into Lua.
- `bool (*UnregisterFunction)(void* addon, uint64_t handle);` — Unregisters a native function.

### Event System
- `uint64_t (*RegisterEvent)(void* addon, const char* eventName, EventCallback callback, void* userData);` — Subscribes to server/custom events.
- `bool (*UnregisterEvent)(void* addon, uint64_t handle);` — Unsubscribes from an event.
- `bool (*TriggerEvent)(void* addon, const char* eventName, const Value* arguments, size_t argumentCount);` — Dispatches an event to the Lua event bus.

### Global Lua Constants
- `bool (*SetGlobalInteger)(void* addon, const char* name, int64_t value);`
- `bool (*SetGlobalNumber)(void* addon, const char* name, double value);`
- `bool (*SetGlobalString)(void* addon, const char* name, const char* value);`
- `bool (*SetGlobalBoolean)(void* addon, const char* name, bool value);`

### Dynamic Function Calling (`CallContext`)
- `CallContext* (*BeginCall)(void* addon, const char* functionName);`
- `void (*EndCall)(CallContext* context);`
- `bool (*PushNil)(CallContext* context);`
- `bool (*PushBoolean)(CallContext* context, bool value);`
- `bool (*PushInteger)(CallContext* context, int64_t value);`
- `bool (*PushNumber)(CallContext* context, double value);`
- `bool (*PushString)(CallContext* context, const char* value);`
- `bool (*PushValue)(CallContext* context, const Value* value);`
- `bool (*ExecuteCall)(CallContext* context);`
- `size_t (*GetResultCount)(const CallContext* context);`
- `ValueType (*GetResultType)(const CallContext* context, size_t index);`
- `bool (*GetResultBoolean)(const CallContext* context, size_t index);`
- `int64_t (*GetResultInteger)(const CallContext* context, size_t index);`
- `double (*GetResultNumber)(const CallContext* context, size_t index);`
- `const char* (*GetResultString)(const CallContext* context, size_t index);`
- `bool (*GetResultValue)(const CallContext* context, size_t index, Value* outValue);`
- `bool (*HasError)(const CallContext* context);`
- `const char* (*GetError)(const CallContext* context);`

---

## 6. Server Subsystem APIs (13 Subsystems)

These tables match the current `include/Addon.h` ABI. The function names below are the members exposed by each table.

### 1. `ActorAPI` (`api->actor`)

- Create, Destroy, IsValid, SetPosition, GetPosition, SetFacingAngle, GetFacingAngle, SetHealth, GetHealth, SetInvulnerable, IsInvulnerable, SetVirtualWorld, GetVirtualWorld, ApplyAnimation, ClearAnimations.

### 2. `PlayerAPI` (`api->player`)

- IsConnected, GetName, GetIp, GetPosition, GetVelocity, GetCameraPosition, GetCameraFrontVector, GetWorldBounds, GetKeys, GetTime, GetWeaponData, GetHealth, GetArmour, GetFacingAngle, GetPing, GetCameraZoom, GetCameraAspectRatio, GetTeam, GetSkin, GetColor, GetInterior, GetVirtualWorld, GetState, GetVehicleId, GetScore, GetMoney, GetWeapon, GetAmmo, GetWantedLevel, GetSpecialAction, GetDrunkLevel, GetTargetPlayer, GetTargetActor, GetCameraMode, GetVehicleSeat, GetFightingStyle, GetFightingMove, GetWeaponState, GetSurfingVehicleId, IsInCheckpoint, IsInRaceCheckpoint, IsInVehicle, IsInAnyVehicle, IsTyping, IsNPC, IsAdmin, GetCount, GetIdFromName, IsInRangeOfPoint, GetDistanceFromPoint, IsPickupStreamedIn, IsActorStreamedIn, SetSpawnInfo, Spawn, ForceClassSelection, SetPosition, SetPositionFindZ, SetHealth, SetArmour, SetFacingAngle, SetInterior, SetVelocity, SetSkin, SetTeam, SetColor, SetVirtualWorld, SetTime, SetScore, GiveMoney, SetMoney, ResetMoney, SetName, SetDrunkLevel, SetWantedLevel, PutInVehicle, RemoveFromVehicle, ToggleControllable, PlaySound, ApplyAnimation, ClearAnimations, SetSpecialAction, SetCheckpoint, DisableCheckpoint, SetRaceCheckpoint, DisableRaceCheckpoint, SetWorldBounds, ClearWorldBounds, SetMarkerForPlayer, ShowNameTagForPlayer, SetMapIcon, RemoveMapIcon, SetCameraPos, SetCameraLookAt, SetCameraBehindPlayer, ToggleSpectating, SpectatePlayer, SpectateVehicle, SetAmmo, GiveWeapon, ResetWeapons, SetSkillLevel, SetArmedWeapon, SetFightingStyle, SetMaxHealth, InterpolateCameraPos, InterpolateCameraLookAt, SetBlurLevel, SetGameSpeed, ToggleWidescreen, SetShopName, PlayAudioStream, StopAudioStream, PlayCrimeReport, DisableRemoteVehicleCollisions, SetChatBubble, SetWeather, ToggleClock, SetAdmin, CreatePlayerPickup, DestroyPlayerPickup.

### 3. `VehicleAPI` (`api->vehicle`)

- Create, AddStatic, AddStaticEx, Destroy, IsValid, GetModel, GetInterior, LinkToInterior, GetPosition, SetPosition, GetZAngle, SetZAngle, GetHealth, SetHealth, SetToRespawn, GetColor, ChangeColor, ChangePaintjob, GetPaintjob, SetNumberPlate, GetNumberPlate, AttachTrailer, DetachTrailer, IsTrailerAttached, GetTrailer, SetVirtualWorld, GetVirtualWorld, GetVelocity, SetVelocity, SetAngularVelocity, IsOnItsSide, IsUpsideDown, GetSirenState, IsWrecked, IsSunked, GetRespawnDelay, SetRespawnDelay, GetDamageStatus, UpdateDamageStatus, SetParamsForPlayer, ManualEngineAndLights, Repair, Explode, SetParamsCarDoors, GetParamsCarDoors, SetParamsCarWindows, GetParamsCarWindows, ToggleTaxiLight, SetEngineState, SetLightState, SetFeature, SetVisibility, SetHoodState, SetTrunkState, SetDoorState, GetSpawnInfo, SetSpawnInfo, GetSpawnPos, SetSpawnPos, GetComponentInSlot, GetComponentType, AddComponent, RemoveComponent, GetModelInfo, GetParamsSirenState, GetRotationQuat, GetDistanceFromPoint, GetModelCount, GetModelsUsed.

### 4. `ObjectAPI` (`api->object`)

- Create, Destroy, IsValid, GetModel, GetPos, SetPos, GetRot, SetRot, Move, Stop, IsMoving, SetScale, AttachToPlayer, CreatePlayer, DestroyPlayer, IsValidPlayer, GetPlayerModel, GetPlayerPos, SetPlayerPos, GetPlayerRot, SetPlayerRot, MovePlayer, StopPlayer, IsPlayerMoving, AttachPlayerToPlayer.

### 5. `PickupAPI` (`api->pickup`)

- Create, AddStatic, Destroy, DestroyAll, IsValid, IsStatic, GetPosition, GetModel, GetType, GetCount, GetVirtualWorld.

### 6. `GangZoneAPI` (`api->gangzone`)

- Create, Destroy, ShowForPlayer, ShowForAll, HideForPlayer, HideForAll, FlashForPlayer, FlashForAll, StopFlashForPlayer, StopFlashForAll, IsValid.

### 7. `LabelAPI` (`api->label`)

- Create3DText, Delete3DText, Attach3DTextToPlayer, Attach3DTextToVehicle, Update3DText, CreatePlayer3DText, DeletePlayer3DText, UpdatePlayer3DText, IsValid3DText, IsValidPlayer3DText.

### 8. `MenuAPI` (`api->menu`)

- Create, Destroy, AddItem, SetColumnHeader, ShowForPlayer, HideForPlayer, IsValid, Disable, DisableRow, GetPlayerMenu.

### 9. `MessagingAPI` (`api->messaging`)

- SendClientMessage, SendClientMessageToAll, SendPlayerMessageToPlayer, SendPlayerMessageToAll, SendDeathMessage, SendDeathMessageToPlayer, GameTextForAll, GameTextForPlayer.

### 10. `NetStatsAPI` (`api->netstats`)

- GetNetworkStats, GetPlayerNetworkStats, GetBytesReceived, GetBytesSent, GetMessagesReceived, GetMessagesSent, GetMessagesRecvPerSecond, GetPacketLossPercent, GetConnectionStatus, GetConnectedTime, GetIpPort.

### 11. `FileSystemAPI` (`api->filesystem`)

- Open, Close, Read, Write, Seek, Tell, Flush, Eof, Size, Exists, Delete, DirExists, DirCreate, DirDelete.

### 12. `WorldAPI` (`api->world`)

- SetWeather, GetWeather, SetWorldTime, GetWorldTime, SetGravity, GetGravity, GetTickCount, GetServerTickCount, GetServerTickRate, SetGameModeText, ShowNameTags, ShowPlayerMarkers, EnableZoneNames, EnableTirePopping, UsePlayerPedAnims, DisableInteriorEnterExits, DisableVehicleMarkers, DisableNameTagLOS, SetNameTagDrawDistance, LimitGlobalChatRadius, LimitPlayerMarkerRadius, SetDeathDropAmount, GameModeExit, GetWeaponName, FindWeaponId, GetVehicleName, FindVehicleModel, CreateExplosion, CreateExplosionForPlayer, SetDisabledWeapons, EnableStuntBonusForAll, EnableStuntBonusForPlayer, Kick, Ban, BanEx, RemoveBan, IsBanned, BlockIpAddress, UnblockIpAddress.

### 13. `TimersAPI` (`api->timers`)

- SetTimer, KillTimer.
