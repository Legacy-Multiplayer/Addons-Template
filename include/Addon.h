#ifndef ADDON_H
#define ADDON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#if defined(_WIN32) || defined(WIN32)
    #define ADDON_EXPORT extern "C" __declspec(dllexport)
#else
    #define ADDON_EXPORT extern "C" __attribute__((visibility("default")))
#endif

#define ADDON_API_VERSION 3

#ifdef __cplusplus
extern "C" {
#endif

// =====================================================================
// Opaque Handles & POD Structures
// =====================================================================

typedef struct CallContext CallContext;

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

// =====================================================================
// Log Levels
// =====================================================================

typedef enum LogLevel {
    ADDON_LOG_INFO = 0,
    ADDON_LOG_WARN = 1,
    ADDON_LOG_ERROR = 2,
    ADDON_LOG_DEBUG = 3
} LogLevel;

// =====================================================================
// Value Types
// =====================================================================

typedef enum ValueType {
    ADDON_VALUE_NIL = 0,
    ADDON_VALUE_BOOLEAN = 1,
    ADDON_VALUE_INTEGER = 2,
    ADDON_VALUE_NUMBER = 3,
    ADDON_VALUE_STRING = 4,
    ADDON_VALUE_USERDATA = 5,
    ADDON_VALUE_TABLE = 6,
    ADDON_VALUE_FUNCTION = 7
} ValueType;

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

// =====================================================================
// Value Helper Constructors
// =====================================================================

static inline Value ValueNil(void) {
    Value value;
    value.type = ADDON_VALUE_NIL;
    value.integerValue = 0;
    return value;
}

static inline Value ValueBoolean(bool booleanValue) {
    Value value;
    value.type = ADDON_VALUE_BOOLEAN;
    value.booleanValue = booleanValue;
    return value;
}

static inline Value ValueInteger(int64_t integerValue) {
    Value value;
    value.type = ADDON_VALUE_INTEGER;
    value.integerValue = integerValue;
    return value;
}

static inline Value ValueNumber(double numberValue) {
    Value value;
    value.type = ADDON_VALUE_NUMBER;
    value.numberValue = numberValue;
    return value;
}

static inline Value ValueString(const char* stringValue) {
    Value value;
    value.type = ADDON_VALUE_STRING;
    value.stringValue = stringValue;
    return value;
}

static inline Value ValuePointer(void* pointerValue) {
    Value value;
    value.type = ADDON_VALUE_USERDATA;
    value.pointerValue = pointerValue;
    return value;
}

// =====================================================================
// Function & Event Callback Signatures
// =====================================================================

typedef int (*FunctionCallback)(
    const Value* arguments,
    size_t argumentCount,
    Value* results,
    size_t maxResults,
    void* userData
);

typedef void (*EventCallback)(
    const char* eventName,
    const Value* arguments,
    size_t argumentCount,
    void* userData
);

// =====================================================================
// Subsystem Server APIs (Real Server Interfaces)
// =====================================================================

typedef struct ActorAPI {
    int (*Create)(int modelId, float x, float y, float z, float rotation);
    bool (*Destroy)(int actorId);
    bool (*IsValid)(int actorId);
    bool (*SetPosition)(int actorId, float x, float y, float z);
    bool (*GetPosition)(int actorId, float* outX, float* outY, float* outZ);
    bool (*SetFacingAngle)(int actorId, float angle);
    bool (*GetFacingAngle)(int actorId, float* outAngle);
    bool (*SetHealth)(int actorId, float health);
    bool (*GetHealth)(int actorId, float* outHealth);
    bool (*SetInvulnerable)(int actorId, bool invulnerable);
    bool (*IsInvulnerable)(int actorId);
    bool (*SetVirtualWorld)(int actorId, int virtualWorld);
    int (*GetVirtualWorld)(int actorId);
    bool (*ApplyAnimation)(int actorId, const char* animLib, const char* animName, float speed, bool loop, bool lockX, bool lockY, bool freeze, int time);
    bool (*ClearAnimations)(int actorId);
} ActorAPI;

typedef struct PlayerAPI {
    bool (*IsConnected)(int playerId);
    const char* (*GetName)(int playerId);
    const char* (*GetIp)(int playerId);
    bool (*GetPosition)(int playerId, float* outX, float* outY, float* outZ);
    bool (*GetVelocity)(int playerId, float* outX, float* outY, float* outZ);
    bool (*GetCameraPosition)(int playerId, float* outX, float* outY, float* outZ);
    bool (*GetCameraFrontVector)(int playerId, float* outX, float* outY, float* outZ);
    bool (*GetWorldBounds)(int playerId, float* outXMax, float* outYMax, float* outXMin, float* outYMin);
    bool (*GetKeys)(int playerId, int* outKeys, int* outUpDown, int* outLeftRight);
    bool (*GetTime)(int playerId, int* outHour, int* outMinute);
    bool (*GetWeaponData)(int playerId, int slot, int* outWeapon, int* outAmmo);
    bool (*GetHealth)(int playerId, float* outHealth);
    bool (*GetArmour)(int playerId, float* outArmour);
    bool (*GetFacingAngle)(int playerId, float* outAngle);
    int (*GetPing)(int playerId);
    float (*GetCameraZoom)(int playerId);
    float (*GetCameraAspectRatio)(int playerId);
    int (*GetTeam)(int playerId);
    int (*GetSkin)(int playerId);
    uint32_t (*GetColor)(int playerId);
    int (*GetInterior)(int playerId);
    int (*GetVirtualWorld)(int playerId);
    int (*GetState)(int playerId);
    int (*GetVehicleId)(int playerId);
    int (*GetScore)(int playerId);
    int (*GetMoney)(int playerId);
    int (*GetWeapon)(int playerId);
    int (*GetAmmo)(int playerId);
    int (*GetWantedLevel)(int playerId);
    int (*GetSpecialAction)(int playerId);
    int (*GetDrunkLevel)(int playerId);
    int (*GetTargetPlayer)(int playerId);
    int (*GetTargetActor)(int playerId);
    int (*GetCameraMode)(int playerId);
    int (*GetVehicleSeat)(int playerId);
    int (*GetFightingStyle)(int playerId);
    int (*GetFightingMove)(int playerId);
    int (*GetWeaponState)(int playerId);
    int (*GetSurfingVehicleId)(int playerId);
    bool (*IsInCheckpoint)(int playerId);
    bool (*IsInRaceCheckpoint)(int playerId);
    bool (*IsInVehicle)(int playerId, int vehicleId);
    bool (*IsInAnyVehicle)(int playerId);
    bool (*IsTyping)(int playerId);
    bool (*IsNPC)(int playerId);
    bool (*IsAdmin)(int playerId);
    int (*GetCount)(void);
    int (*GetPoolSize)(void);
    int (*GetIdFromName)(const char* name);
    const char* (*GetVersion)(int playerId);
    const char* (*GetGPCI)(int playerId);
    bool (*IsInRangeOfPoint)(int playerId, float range, float x, float y, float z);
    float (*GetDistanceFromPoint)(int playerId, float x, float y, float z);
    bool (*IsPickupStreamedIn)(int pickupId, int playerId);
    bool (*IsActorStreamedIn)(int actorId, int playerId);

    bool (*SetSpawnInfo)(int playerId, int team, int skin, float x, float y, float z, float rotation, int weapon1, int ammo1, int weapon2, int ammo2, int weapon3, int ammo3);
    bool (*Spawn)(int playerId);
    bool (*ForceClassSelection)(int playerId);
    bool (*SetPosition)(int playerId, float x, float y, float z);
    bool (*SetPositionFindZ)(int playerId, float x, float y, float z);
    bool (*SetHealth)(int playerId, float health);
    bool (*SetArmour)(int playerId, float armour);
    bool (*SetFacingAngle)(int playerId, float angle);
    bool (*SetInterior)(int playerId, int interiorId);
    bool (*SetVelocity)(int playerId, float x, float y, float z);
    bool (*SetSkin)(int playerId, int skinId);
    bool (*SetTeam)(int playerId, int team);
    bool (*SetColor)(int playerId, uint32_t color);
    bool (*SetVirtualWorld)(int playerId, int worldId);
    bool (*SetTime)(int playerId, int hour, int minute);
    bool (*SetScore)(int playerId, int score);
    bool (*GiveMoney)(int playerId, int money);
    bool (*SetMoney)(int playerId, int money);
    bool (*ResetMoney)(int playerId);
    int (*SetName)(int playerId, const char* name);
    bool (*SetDrunkLevel)(int playerId, int level);
    bool (*SetWantedLevel)(int playerId, int level);
    bool (*PutInVehicle)(int playerId, int vehicleId, int seatId);
    bool (*RemoveFromVehicle)(int playerId);
    bool (*ToggleControllable)(int playerId, bool toggle);
    bool (*PlaySound)(int playerId, int soundId, float x, float y, float z);
    bool (*ApplyAnimation)(int playerId, const char* animLib, const char* animName, float speed, bool loop, bool lockX, bool lockY, bool lockF, int time);
    bool (*ClearAnimations)(int playerId);
    bool (*SetSpecialAction)(int playerId, int actionId);
    bool (*SetCheckpoint)(int playerId, float x, float y, float z, float size);
    bool (*DisableCheckpoint)(int playerId);
    bool (*SetRaceCheckpoint)(int playerId, int type, float x, float y, float z, float nextX, float nextY, float nextZ, float size);
    bool (*DisableRaceCheckpoint)(int playerId);
    bool (*SetWorldBounds)(int playerId, float xMax, float yMax, float xMin, float yMin);
    bool (*ClearWorldBounds)(int playerId);
    bool (*SetMarkerForPlayer)(int playerId, int showPlayerId, uint32_t color);
    bool (*ShowNameTagForPlayer)(int playerId, int showPlayerId, bool show);
    bool (*SetMapIcon)(int playerId, int iconId, float x, float y, float z, int markerType, uint32_t color, int style);
    bool (*RemoveMapIcon)(int playerId, int iconId);
    bool (*AllowTeleport)(int playerId, bool allow);
    bool (*SetCameraPos)(int playerId, float x, float y, float z, float rx, float ry, float rz);
    bool (*SetCameraLookAt)(int playerId, float x, float y, float z, int cut);
    bool (*SetCameraBehindPlayer)(int playerId);
    bool (*ToggleSpectating)(int playerId, bool toggle);
    bool (*SpectatePlayer)(int playerId, int targetPlayerId, int mode);
    bool (*SpectateVehicle)(int playerId, int vehicleId, int mode);
    bool (*SetAmmo)(int playerId, int weaponSlot, int ammo);
    bool (*GiveWeapon)(int playerId, int weaponId, int ammo);
    bool (*ResetWeapons)(int playerId);
    bool (*SetSkillLevel)(int playerId, int skill, int level);
    int (*SetArmedWeapon)(int playerId, int weaponId);
    bool (*SetFightingStyle)(int playerId, int style, int moves);
    bool (*SetMaxHealth)(int playerId, float maxHealth);
    bool (*InterpolateCameraPos)(int playerId, float fromX, float fromY, float fromZ, float toX, float toY, float toZ, int time, int cut);
    bool (*InterpolateCameraLookAt)(int playerId, float fromX, float fromY, float fromZ, float toX, float toY, float toZ, int time, int cut);
    bool (*SetBlurLevel)(int playerId, int blurLevel);
    int (*SetGameSpeed)(int playerId, float speed);
    bool (*SendClientCheck)(int playerId, int type, uint32_t address, uint16_t offset, uint16_t count);
    bool (*ToggleChatbox)(int playerId, bool show);
    bool (*ToggleWidescreen)(int playerId, bool on);
    bool (*SetShopName)(int playerId, const char* shopName);
    bool (*PlayAudioStream)(int playerId, const char* url, float posX, float posY, float posZ, float distance, bool usePos);
    bool (*StopAudioStream)(int playerId);
    bool (*PlayCrimeReport)(int playerId, int suspectId, int crimeId);
    bool (*DisableRemoteVehicleCollisions)(int playerId, bool disable);
    bool (*SetChatBubble)(int playerId, const char* text, uint32_t color, float drawDistance, int expireTime);
    bool (*SetWeather)(int playerId, int weatherId);
    bool (*ToggleClock)(int playerId, bool toggle);
    bool (*SetAdmin)(int playerId, bool admin);
    bool (*CreatePlayerPickup)(int pickupId, int playerId, int model, int type, float x, float y, float z);
    bool (*DestroyPlayerPickup)(int pickupId, int playerId);

    bool (*SetPVarInt)(int playerId, const char* name, int value);
    bool (*SetPVarString)(int playerId, const char* name, const char* value);
    bool (*SetPVarFloat)(int playerId, const char* name, float value);
    int (*GetPVarInt)(int playerId, const char* name);
    const char* (*GetPVarString)(int playerId, const char* name);
    float (*GetPVarFloat)(int playerId, const char* name);
    bool (*DeletePVar)(int playerId, const char* name);
    int (*GetPVarType)(int playerId, const char* name);
    const char* (*GetPVarNameAtIndex)(int playerId, int index);
    int (*GetPVarsUpperIndex)(int playerId);
} PlayerAPI;

typedef struct VehicleAPI {
    int (*Create)(int modelId, float x, float y, float z, float rotation, int color1, int color2, int respawnDelay, bool addSiren);
    int (*AddStatic)(int modelId, float x, float y, float z, float rotation, int color1, int color2);
    int (*AddStaticEx)(int modelId, float x, float y, float z, float rotation, int color1, int color2, int respawnDelay, bool addSiren);
    bool (*Destroy)(int vehicleId);
    bool (*IsValid)(int vehicleId);
    int (*GetModel)(int vehicleId);
    int (*GetInterior)(int vehicleId);
    bool (*LinkToInterior)(int vehicleId, int interior);
    bool (*GetPosition)(int vehicleId, float* outX, float* outY, float* outZ);
    bool (*SetPosition)(int vehicleId, float x, float y, float z);
    bool (*GetZAngle)(int vehicleId, float* outAngle);
    bool (*SetZAngle)(int vehicleId, float angle);
    bool (*GetHealth)(int vehicleId, float* outHealth);
    bool (*SetHealth)(int vehicleId, float health);
    bool (*SetToRespawn)(int vehicleId);
    bool (*GetColor)(int vehicleId, int* outColor1, int* outColor2);
    bool (*ChangeColor)(int vehicleId, int color1, int color2);
    bool (*ChangePaintjob)(int vehicleId, int paintjob);
    int (*GetPaintjob)(int vehicleId);
    bool (*SetNumberPlate)(int vehicleId, const char* plate);
    const char* (*GetNumberPlate)(int vehicleId);
    bool (*AttachTrailer)(int trailerId, int vehicleId);
    bool (*DetachTrailer)(int vehicleId);
    bool (*IsTrailerAttached)(int vehicleId);
    int (*GetTrailer)(int vehicleId);
    bool (*SetVirtualWorld)(int vehicleId, int world);
    int (*GetVirtualWorld)(int vehicleId);
    bool (*GetVelocity)(int vehicleId, float* outX, float* outY, float* outZ);
    bool (*SetVelocity)(int vehicleId, float x, float y, float z);
    bool (*SetAngularVelocity)(int vehicleId, float x, float y, float z);
    bool (*IsOnItsSide)(int vehicleId);
    bool (*IsUpsideDown)(int vehicleId);
    int (*GetSirenState)(int vehicleId);
    bool (*IsWrecked)(int vehicleId);
    bool (*IsSunked)(int vehicleId);
    int (*GetRespawnDelay)(int vehicleId);
    bool (*SetRespawnDelay)(int vehicleId, int delay);
    bool (*GetDamageStatus)(int vehicleId, int* outPanels, int* outDoors, uint8_t* outLights, uint8_t* outTires);
    bool (*UpdateDamageStatus)(int vehicleId, int panels, int doors, uint8_t lights, uint8_t tires);
    bool (*SetParamsForPlayer)(int vehicleId, int playerId, int objective, int locked);
    bool (*ManualEngineAndLights)(void);
    bool (*Repair)(int vehicleId);
    bool (*Explode)(int vehicleId);
    bool (*SetParamsCarDoors)(int vehicleId, int driver, int passenger, int backLeft, int backRight);
    bool (*GetParamsCarDoors)(int vehicleId, int* outDriver, int* outPassenger, int* outBackLeft, int* outBackRight);
    bool (*SetParamsCarWindows)(int vehicleId, int driver, int passenger, int backLeft, int backRight);
    bool (*GetParamsCarWindows)(int vehicleId, int* outDriver, int* outPassenger, int* outBackLeft, int* outBackRight);
    bool (*ToggleTaxiLight)(int vehicleId, bool toggle);
    bool (*SetEngineState)(int vehicleId, bool engineState);
    bool (*SetLightState)(int vehicleId, bool lightState);
    bool (*SetFeature)(int vehicleId, bool feature);
    bool (*SetVisibility)(int vehicleId, bool visible);
    bool (*SetHoodState)(int vehicleId, bool state);
    bool (*SetTrunkState)(int vehicleId, bool state);
    bool (*SetDoorState)(int vehicleId, int doorId, bool state);
    bool (*GetSpawnInfo)(int vehicleId, float* outX, float* outY, float* outZ, float* outRot, int* outCol1, int* outCol2);
    bool (*SetSpawnInfo)(int vehicleId, int modelId, float x, float y, float z, float rotation, int color1, int color2, int respawnDelay, int interior);
    bool (*GetSpawnPos)(int vehicleId, float* outX, float* outY, float* outZ);
    bool (*SetSpawnPos)(int vehicleId, float x, float y, float z);
    int (*GetComponentInSlot)(int vehicleId, int slot);
    int (*GetComponentType)(int componentId);
    bool (*AddComponent)(int vehicleId, int componentId);
    bool (*RemoveComponent)(int vehicleId, int componentId);
    bool (*GetModelInfo)(int modelId, int infoType, float* outX, float* outY, float* outZ);
    int (*GetParamsSirenState)(int vehicleId);
    bool (*GetRotationQuat)(int vehicleId, float* outW, float* outX, float* outY, float* outZ);
    float (*GetDistanceFromPoint)(int vehicleId, float x, float y, float z);
    int (*GetPoolSize)(void);
    int (*GetModelCount)(int modelId);
    int (*GetModelsUsed)(void);
} VehicleAPI;

typedef struct ObjectAPI {
    int (*Create)(int modelId, float x, float y, float z, float rx, float ry, float rz, float drawDistance);
    bool (*Destroy)(int objectId);
    bool (*IsValid)(int objectId);
    int (*GetModel)(int objectId);
    bool (*GetPos)(int objectId, float* outX, float* outY, float* outZ);
    bool (*SetPos)(int objectId, float x, float y, float z);
    bool (*GetRot)(int objectId, float* outRX, float* outRY, float* outRZ);
    bool (*SetRot)(int objectId, float rx, float ry, float rz);
    float (*Move)(int objectId, float x, float y, float z, float speed);
    bool (*Stop)(int objectId);
    bool (*IsMoving)(int objectId);
    bool (*SetScale)(int objectId, float scale);
    bool (*AttachToPlayer)(int objectId, int playerId, float offsetX, float offsetY, float offsetZ, float rx, float ry, float rz);

    int (*CreatePlayer)(int playerId, int modelId, float x, float y, float z, float rx, float ry, float rz, float drawDistance);
    bool (*DestroyPlayer)(int playerId, int objectId);
    bool (*IsValidPlayer)(int playerId, int objectId);
    int (*GetPlayerModel)(int playerId, int objectId);
    bool (*GetPlayerPos)(int playerId, int objectId, float* outX, float* outY, float* outZ);
    bool (*SetPlayerPos)(int playerId, int objectId, float x, float y, float z);
    bool (*GetPlayerRot)(int playerId, int objectId, float* outRX, float* outRY, float* outRZ);
    bool (*SetPlayerRot)(int playerId, int objectId, float rx, float ry, float rz);
    float (*MovePlayer)(int playerId, int objectId, float x, float y, float z, float speed);
    bool (*StopPlayer)(int playerId, int objectId);
    bool (*IsPlayerMoving)(int playerId, int objectId);
    bool (*AttachPlayerToPlayer)(int playerId, int objectId, int attachToPlayerId, float offsetX, float offsetY, float offsetZ, float rx, float ry, float rz);
} ObjectAPI;

typedef struct PickupAPI {
    int (*Create)(int model, int type, float x, float y, float z, int virtualWorld);
    bool (*AddStatic)(int model, int type, float x, float y, float z, int virtualWorld);
    bool (*Destroy)(int pickupId);
    bool (*DestroyAll)(void);
    bool (*IsValid)(int pickupId);
    bool (*IsStatic)(int pickupId);
    bool (*GetPosition)(int pickupId, float* outX, float* outY, float* outZ);
    int (*GetModel)(int pickupId);
    int (*GetType)(int pickupId);
    int (*GetCount)(void);
    int (*GetPoolSize)(void);
    int (*GetVirtualWorld)(int pickupId);
} PickupAPI;

typedef struct GangZoneAPI {
    int (*Create)(float minX, float minY, float maxX, float maxY);
    bool (*Destroy)(int zoneId);
    bool (*ShowForPlayer)(int playerId, int zoneId, uint32_t color);
    bool (*ShowForAll)(int zoneId, uint32_t color);
    bool (*HideForPlayer)(int playerId, int zoneId);
    bool (*HideForAll)(int zoneId);
    bool (*FlashForPlayer)(int playerId, int zoneId, uint32_t color);
    bool (*FlashForAll)(int zoneId, uint32_t color);
    bool (*StopFlashForPlayer)(int playerId, int zoneId);
    bool (*StopFlashForAll)(int zoneId);
    bool (*IsValid)(int zoneId);
} GangZoneAPI;

typedef struct LabelAPI {
    int (*Create3DText)(const char* text, uint32_t color, float x, float y, float z, float drawDistance, int virtualWorld, bool testLOS);
    bool (*Delete3DText)(int labelId);
    bool (*Attach3DTextToPlayer)(int labelId, int playerId, float offsetX, float offsetY, float offsetZ);
    bool (*Attach3DTextToVehicle)(int labelId, int vehicleId, float offsetX, float offsetY, float offsetZ);
    bool (*Update3DText)(int labelId, uint32_t color, const char* text);
    int (*CreatePlayer3DText)(int playerId, const char* text, uint32_t color, float x, float y, float z, float drawDistance, int attachedPlayerId, int attachedVehicleId, bool testLOS);
    bool (*DeletePlayer3DText)(int playerId, int labelId);
    bool (*UpdatePlayer3DText)(int playerId, int labelId, uint32_t color, const char* text);
    bool (*IsValid3DText)(int labelId);
    bool (*IsValidPlayer3DText)(int playerId, int labelId);
} LabelAPI;

typedef struct MenuAPI {
    int (*Create)(const char* title, int columns, float x, float y, float col1Width, float col2Width);
    bool (*Destroy)(int menuId);
    int (*AddItem)(int menuId, int column, const char* text);
    bool (*SetColumnHeader)(int menuId, int column, const char* header);
    bool (*ShowForPlayer)(int menuId, int playerId);
    bool (*HideForPlayer)(int menuId, int playerId);
    bool (*IsValid)(int menuId);
    bool (*Disable)(int menuId);
    bool (*DisableRow)(int menuId, int row);
    int (*GetPlayerMenu)(int playerId);
} MenuAPI;

typedef struct MessagingAPI {
    bool (*SendClientMessage)(int playerId, uint32_t color, const char* message);
    bool (*SendClientMessageToAll)(uint32_t color, const char* message);
    bool (*SendPlayerMessageToPlayer)(int playerId, int senderId, const char* message);
    bool (*SendPlayerMessageToAll)(int senderId, const char* message);
    bool (*SendDeathMessage)(int killerId, int victimId, int reason);
    bool (*SendDeathMessageToPlayer)(int playerId, int killerId, int victimId, int reason);
    bool (*GameTextForAll)(const char* text, int time, int style);
    bool (*GameTextForPlayer)(int playerId, const char* text, int time, int style);
} MessagingAPI;

typedef struct NetStatsAPI {
    const char* (*GetNetworkStats)(void);
    const char* (*GetPlayerNetworkStats)(int playerId);
    int (*GetBytesReceived)(int playerId);
    int (*GetBytesSent)(int playerId);
    int (*GetMessagesReceived)(int playerId);
    int (*GetMessagesSent)(int playerId);
    int (*GetMessagesRecvPerSecond)(int playerId);
    float (*GetPacketLossPercent)(int playerId);
    int (*GetConnectionStatus)(int playerId);
    int (*GetConnectedTime)(int playerId);
    const char* (*GetIpPort)(int playerId);
} NetStatsAPI;

typedef struct TextDrawAPI {
    int (*Create)(float x, float y, const char* text);
    bool (*Destroy)(int id);
    bool (*IsValid)(int id);
    bool (*LetterSize)(int id, float x, float y);
    bool (*TextSize)(int id, float x, float y);
    bool (*Alignment)(int id, int alignment);
    bool (*Color)(int id, uint32_t color);
    bool (*UseBox)(int id, bool use);
    bool (*BoxColor)(int id, uint32_t color);
    bool (*SetShadow)(int id, int size);
    bool (*SetOutline)(int id, int size);
    bool (*BackgroundColor)(int id, uint32_t color);
    bool (*Font)(int id, int font);
    bool (*SetProportional)(int id, bool set);
    bool (*ShowForPlayer)(int playerId, int id);
    bool (*HideForPlayer)(int playerId, int id);
    bool (*ShowForAll)(int id);
    bool (*HideForAll)(int id);
    bool (*SetString)(int id, const char* text);

    int (*CreatePlayer)(int playerId, float x, float y, const char* text);
    bool (*DestroyPlayer)(int playerId, int id);
    bool (*ShowPlayer)(int playerId, int id);
    bool (*HidePlayer)(int playerId, int id);
    bool (*SetStringPlayer)(int playerId, int id, const char* text);
    bool (*LetterSizePlayer)(int playerId, int id, float x, float y);
    bool (*TextSizePlayer)(int playerId, int id, float x, float y);
    bool (*AlignmentPlayer)(int playerId, int id, int alignment);
    bool (*ColorPlayer)(int playerId, int id, uint32_t color);
    bool (*BoxColorPlayer)(int playerId, int id, uint32_t color);
    bool (*BackgroundColorPlayer)(int playerId, int id, uint32_t color);
    bool (*UseBoxPlayer)(int playerId, int id, bool use);
    bool (*SetShadowPlayer)(int playerId, int id, int size);
    bool (*FontPlayer)(int playerId, int id, int font);
    bool (*SetOutlinePlayer)(int playerId, int id, int size);
    bool (*SetProportionalPlayer)(int playerId, int id, bool set);
} TextDrawAPI;

typedef struct FileSystemAPI {
    int (*Open)(const char* path, const char* mode);
    bool (*Close)(int handle);
    const char* (*Read)(int handle, size_t bytes);
    int (*Write)(int handle, const char* data);
    int (*Seek)(int handle, int offset, int origin);
    int (*Tell)(int handle);
    bool (*Flush)(int handle);
    bool (*Eof)(int handle);
    int (*Size)(int handle);
    bool (*Exists)(const char* path);
    bool (*Delete)(const char* path);
    bool (*DirExists)(const char* path);
    bool (*DirCreate)(const char* path);
    bool (*DirDelete)(const char* path);
} FileSystemAPI;

typedef struct VariablesAPI {
    bool (*SetInt)(const char* name, int value);
    bool (*SetString)(const char* name, const char* value);
    bool (*SetFloat)(const char* name, float value);
    int (*GetInt)(const char* name);
    const char* (*GetString)(const char* name);
    float (*GetFloat)(const char* name);
    bool (*Delete)(const char* name);
    int (*GetType)(const char* name);
    const char* (*GetNameAtIndex)(int index);
    int (*GetUpperIndex)(void);
} VariablesAPI;

typedef struct WorldAPI {
    bool (*SetWeather)(int weatherId);
    int (*GetWeather)(void);
    bool (*SetWorldTime)(int hour);
    int (*GetWorldTime)(void);
    bool (*SetGravity)(float gravity);
    float (*GetGravity)(void);
    int (*GetTickCount)(void);
    int (*GetServerTickCount)(void);
    int (*GetServerTickRate)(void);
    int (*GetMaxPlayers)(void);
    int (*GetPlayerPoolSize)(void);
    int (*GetVehiclePoolSize)(void);
    int (*GetActorPoolSize)(void);
    bool (*SetGameModeText)(const char* text);
    int (*AddPlayerClass)(int skinId, float x, float y, float z, float a, int weapon1, int ammo1, int weapon2, int ammo2, int weapon3, int ammo3);
    int (*AddPlayerClassEx)(int teamId, int skinId, float x, float y, float z, float a, int weapon1, int ammo1, int weapon2, int ammo2, int weapon3, int ammo3);
    bool (*ShowNameTags)(bool show);
    bool (*ShowPlayerMarkers)(bool show);
    bool (*AllowInteriorWeapons)(bool allow);
    bool (*AllowAdminTeleport)(bool allow);
    bool (*EnableZoneNames)(bool enable);
    bool (*EnableTirePopping)(bool enable);
    bool (*UsePlayerPedAnims)(void);
    bool (*DisableInteriorEnterExits)(void);
    bool (*DisableVehicleMarkers)(void);
    bool (*DisableNameTagLOS)(void);
    bool (*SetNameTagDrawDistance)(float distance);
    bool (*LimitGlobalChatRadius)(float radius);
    bool (*LimitPlayerMarkerRadius)(float radius);
    bool (*SetDeathDropAmount)(int amount);
    bool (*GameModeExit)(void);
    bool (*SetMaxRconLoginAttempt)(int maxAttempt);
    const char* (*GetWeaponName)(int weaponId);
    int (*FindWeaponId)(const char* name);
    const char* (*GetVehicleName)(int modelId);
    int (*FindVehicleModel)(const char* name);
    bool (*CreateExplosion)(float x, float y, float z, int type, float radius);
    bool (*CreateExplosionForPlayer)(int playerId, float x, float y, float z, int type, float radius);
    bool (*SetDisabledWeapons)(const int* weaponIds, size_t count);
    bool (*EnableStuntBonusForAll)(bool enable);
    bool (*EnableStuntBonusForPlayer)(int playerId, bool enable);
    const char* (*GetServerVarAsString)(const char* name);
    int (*GetServerVarAsInt)(const char* name);
    bool (*GetServerVarAsBool)(const char* name);
    bool (*Kick)(int playerId);
    bool (*Ban)(int playerId);
    bool (*BanEx)(int playerId, const char* reason);
    bool (*RemoveBan)(const char* ip);
    bool (*IsBanned)(const char* ip);
    bool (*BlockIpAddress)(const char* ip, int timeMs);
    bool (*UnblockIpAddress)(const char* ip);
} WorldAPI;

typedef struct TimersAPI {
    int (*SetTimer)(int interval, bool repeating, const char* funcName);
    bool (*KillTimer)(int timerId);
} TimersAPI;

// =====================================================================
// Master Addon API Function Table
// =====================================================================

typedef struct AddonAPI {
    uint32_t apiVersion;
    uint32_t structSize;

    // --- Addon Information ---
    const char* (*GetName)(void* addon);
    const char* (*GetPath)(void* addon);

    // --- Logging ---
    void (*Log)(void* addon, LogLevel level, const char* message);

    // --- Function Registration ---
    uint64_t (*RegisterFunction)(void* addon, const char* functionName, FunctionCallback callback, void* userData);
    bool (*UnregisterFunction)(void* addon, uint64_t handle);

    // --- Event Registration & Triggering ---
    uint64_t (*RegisterEvent)(void* addon, const char* eventName, EventCallback callback, void* userData);
    bool (*UnregisterEvent)(void* addon, uint64_t handle);
    bool (*TriggerEvent)(void* addon, const char* eventName, const Value* arguments, size_t argumentCount);

    // --- Dynamic Function Invocation (CallContext Builder) ---
    CallContext* (*BeginCall)(void* addon, const char* functionName);
    void (*EndCall)(CallContext* context);

    bool (*PushNil)(CallContext* context);
    bool (*PushBoolean)(CallContext* context, bool value);
    bool (*PushInteger)(CallContext* context, int64_t value);
    bool (*PushNumber)(CallContext* context, double value);
    bool (*PushString)(CallContext* context, const char* value);
    bool (*PushValue)(CallContext* context, const Value* value);

    bool (*ExecuteCall)(CallContext* context);

    size_t (*GetResultCount)(const CallContext* context);
    ValueType (*GetResultType)(const CallContext* context, size_t index);
    bool (*GetResultBoolean)(const CallContext* context, size_t index);
    int64_t (*GetResultInteger)(const CallContext* context, size_t index);
    double (*GetResultNumber)(const CallContext* context, size_t index);
    const char* (*GetResultString)(const CallContext* context, size_t index);
    bool (*GetResultValue)(const CallContext* context, size_t index, Value* outValue);

    bool (*HasError)(const CallContext* context);
    const char* (*GetError)(const CallContext* context);

    // --- Global Lua Value Registration ---
    bool (*SetGlobalInteger)(void* addon, const char* name, int64_t value);
    bool (*SetGlobalNumber)(void* addon, const char* name, double value);
    bool (*SetGlobalString)(void* addon, const char* name, const char* value);
    bool (*SetGlobalBoolean)(void* addon, const char* name, bool value);

    // --- Real Server Subsystem APIs ---
    const ActorAPI* actor;
    const PlayerAPI* player;
    const VehicleAPI* vehicle;
    const ObjectAPI* object;
    const PickupAPI* pickup;
    const GangZoneAPI* gangzone;
    const LabelAPI* label;
    const MenuAPI* menu;
    const MessagingAPI* messaging;
    const NetStatsAPI* netstats;
    const TextDrawAPI* textdraw;
    const FileSystemAPI* filesystem;
    const VariablesAPI* variables;
    const WorldAPI* world;
    const TimersAPI* timers;
} AddonAPI;

// =====================================================================
// Addon Entry Point Types
// =====================================================================

typedef bool (*AddonMainFunction)(const AddonAPI* api, void* addon);
typedef void (*AddonUnloadFunction)(void);

#ifdef __cplusplus
}
#endif

#endif // ADDON_H
