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
// Opaque Handles
// =====================================================================

/**
 * @brief Opaque handle representing a dynamic function call context.
 * 
 * Allocated via BeginCall and freed via EndCall. Holds dynamic arguments,
 * execution state, error information, and returned values without exposing
 * internal Sol2/Lua engine structures.
 */
typedef struct CallContext CallContext;

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

/**
 * @brief Function callback signature callable from Lua scripts.
 * 
 * @param arguments Array of input arguments passed from Lua.
 * @param argumentCount Number of arguments in arguments array.
 * @param results Output buffer where the callback writes its return values.
 * @param maxResults Maximum capacity of the results buffer (typically 16).
 * @param userData User data pointer passed during registration.
 * @return int Number of return values written to results (0 to maxResults).
 */
typedef int (*FunctionCallback)(
    const Value* arguments,
    size_t argumentCount,
    Value* results,
    size_t maxResults,
    void* userData
);

/**
 * @brief Event callback signature for server and custom events.
 * 
 * @param eventName Name of the event being dispatched.
 * @param arguments Array of event arguments.
 * @param argumentCount Number of arguments in arguments array.
 * @param userData User data pointer passed during registration.
 */
typedef void (*EventCallback)(
    const char* eventName,
    const Value* arguments,
    size_t argumentCount,
    void* userData
);

// =====================================================================
// Addon API Function Table
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
} AddonAPI;

// =====================================================================
// Addon Entry Point Types
// =====================================================================

/**
 * @brief Required entry point for native addons.
 * 
 * Invoked immediately after the dynamic library is loaded.
 * 
 * @param api Pointer to the AddonAPI function table.
 * @param addon Opaque handle representing this addon instance.
 * @return true if initialization succeeded and the addon should remain active.
 * @return false if initialization failed; the server will cleanly unload the library.
 */
typedef bool (*AddonMainFunction)(const AddonAPI* api, void* addon);

/**
 * @brief Optional exit entry point for native addons.
 * 
 * Invoked during server shutdown before the dynamic library is unloaded.
 */
typedef void (*AddonUnloadFunction)(void);

#ifdef __cplusplus
}
#endif

#endif // ADDON_H
