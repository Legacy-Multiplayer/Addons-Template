/*
    Legacy Server - Native Addon Template
    Main Entry Point and Example Addon Implementation

    Demonstrates:
    - Addon ABI initialization and version negotiation
    - Exposing native C/C++ functions to Lua (with argument/return handling)
    - Subscribing to existing server events (e.g., onPlayerConnect)
    - Dispatching custom addon events to the server's Lua event bus
    - Invoking existing server Lua functions (e.g., sendClientMessage) via CallContext
    - Safe handle tracking and clean resource cleanup during unload
*/

#include "Addon.h"
#include <cstdio>
#include <cstring>
#include <cstdint>

// =====================================================================
// Global Addon State
// =====================================================================

static const AddonAPI* g_API = nullptr;
static void* g_Addon = nullptr;

static uint64_t g_HelloFuncHandle = 0;
static uint64_t g_PlayerConnectEventHandle = 0;

// =====================================================================
// Invoking Existing Server Lua Functions
// =====================================================================

/**
 * @brief Demonstrates calling the server's existing Lua function: sendClientMessage
 * 
 * Server Lua signature:
 *   sendClientMessage(playerId: integer, color: integer, message: string) -> boolean
 * 
 * @param playerId Target player ID (0..MAX_PLAYERS-1)
 * @param color Hexadecimal RGBA/ARGB color code (e.g., 0x00FF00FF for green, 0xFFFFFFFF for white)
 * @param message The chat message string to send to the client
 * @return true if the function executed successfully and message was dispatched
 */
static bool CallSendClientMessage(int64_t playerId, int64_t color, const char* message)
{
    if (!g_API || !g_Addon || !message)
    {
        return false;
    }

    // 1. Begin dynamic call context for the existing server Lua function
    CallContext* call = g_API->BeginCall(g_Addon, "sendClientMessage");
    if (!call)
    {
        return false;
    }

    // 2. Push arguments in the exact order declared by the server's Lua registration:
    //    Argument 0: playerId (integer)
    //    Argument 1: color (integer)
    //    Argument 2: message (string)
    g_API->PushInteger(call, playerId);
    g_API->PushInteger(call, color);
    g_API->PushString(call, message);

    // 3. Execute the call protected by the server's Sol2/Lua engine
    bool result = false;
    if (g_API->ExecuteCall(call))
    {
        // 4. Retrieve return values if needed (sendClientMessage returns boolean)
        if (g_API->GetResultCount(call) > 0)
        {
            result = g_API->GetResultBoolean(call, 0);
        }
        else
        {
            result = true;
        }
    }
    else if (g_API->HasError(call))
    {
        char errBuffer[256];
        snprintf(errBuffer, sizeof(errBuffer), "Error invoking sendClientMessage: %s", g_API->GetError(call));
        g_API->Log(g_Addon, ADDON_LOG_WARN, errBuffer);
    }

    // 5. Explicitly end and free the call context
    g_API->EndCall(call);
    return result;
}

// =====================================================================
// Example Native Function Callback (Exposed to Lua)
// =====================================================================

/**
 * @brief Native function callable from Lua: hello([name]) -> greeting
 * 
 * Example Lua usage:
 *   local msg = hello("World")
 *   print(msg) -- "Hello, World! (From Native Addon)"
 */
static int HelloFunction(
    const Value* arguments,
    size_t argumentCount,
    Value* results,
    size_t maxResults,
    void* userData
) {
    const char* targetName = "Developer";

    if (argumentCount > 0 && arguments[0].type == ADDON_VALUE_STRING && arguments[0].stringValue)
    {
        targetName = arguments[0].stringValue;
    }

    // Construct greeting message
    static thread_local char s_greetingBuffer[256];
    snprintf(s_greetingBuffer, sizeof(s_greetingBuffer), "Hello, %s! (From Native Addon)", targetName);

    if (g_API && g_Addon)
    {
        g_API->Log(g_Addon, ADDON_LOG_INFO, s_greetingBuffer);
    }

    // Return greeting string to Lua
    if (results && maxResults > 0)
    {
        results[0] = ValueString(s_greetingBuffer);
        return 1; // 1 return value
    }

    return 0;
}

// =====================================================================
// Example Server Event Callback (Listening to Server Events)
// =====================================================================

/**
 * @brief Event callback listening for the server's onPlayerConnect event.
 * 
 * Dispatched by the server when a client connects:
 *   eventName: "onPlayerConnect"
 *   arguments[0]: playerId (integer)
 */
static void OnPlayerConnectHandler(
    const char* eventName,
    const Value* arguments,
    size_t argumentCount,
    void* userData
) {
    if (!g_API || !g_Addon)
    {
        return;
    }

    if (argumentCount > 0 && (arguments[0].type == ADDON_VALUE_INTEGER || arguments[0].type == ADDON_VALUE_NUMBER))
    {
        int64_t playerId = (arguments[0].type == ADDON_VALUE_INTEGER)
            ? arguments[0].integerValue
            : static_cast<int64_t>(arguments[0].numberValue);

        char logMsg[128];
        snprintf(logMsg, sizeof(logMsg), "Player ID %lld connected to the server.", static_cast<long long>(playerId));
        g_API->Log(g_Addon, ADDON_LOG_INFO, logMsg);

        // Send a welcome message directly to the connecting player using sendClientMessage
        CallSendClientMessage(playerId, 0x00FF00FF, "Welcome to the server! (Message sent from Native Addon)");
    }
}

// =====================================================================
// Addon Lifecycle Entry Points
// =====================================================================

/**
 * @brief Main entry point for native addons.
 * 
 * Invoked by the Legacy Server addon loader immediately after loading the library.
 * 
 * @param api Pointer to the AddonAPI function table.
 * @param addon Opaque handle representing this addon instance.
 * @return true if initialization succeeded and the addon should remain active.
 * @return false if initialization failed; the server will cleanly unload the library.
 */
ADDON_EXPORT bool AddonMain(const AddonAPI* api, void* addon)
{
    if (!api || api->apiVersion != ADDON_API_VERSION)
    {
        return false;
    }

    g_API = api;
    g_Addon = addon;

    g_API->Log(g_Addon, ADDON_LOG_INFO, "Initializing ExampleAddon...");

    // 1. Register native function into the server's Lua runtime
    g_HelloFuncHandle = g_API->RegisterFunction(g_Addon, "hello", HelloFunction, nullptr);
    if (!g_HelloFuncHandle)
    {
        g_API->Log(g_Addon, ADDON_LOG_ERROR, "Failed to register native function 'hello'.");
        return false;
    }

    // 2. Register event listener for existing server event: onPlayerConnect
    g_PlayerConnectEventHandle = g_API->RegisterEvent(g_Addon, "onPlayerConnect", OnPlayerConnectHandler, nullptr);
    if (!g_PlayerConnectEventHandle)
    {
        g_API->Log(g_Addon, ADDON_LOG_WARN, "Failed to register event listener for 'onPlayerConnect'.");
    }

    // 3. Trigger a custom addon event through the server's event bus
    Value readyArgs[1];
    readyArgs[0] = ValueString("ExampleAddon is ready.");
    g_API->TriggerEvent(g_Addon, "ExampleAddon.OnReady", readyArgs, 1);

    g_API->Log(g_Addon, ADDON_LOG_INFO, "ExampleAddon initialized successfully!");
    return true;
}

/**
 * @brief Optional unload entry point for native addons.
 * 
 * Invoked by the server prior to dynamic library unloading during server shutdown.
 */
ADDON_EXPORT void AddonUnload(void)
{
    if (g_API && g_Addon)
    {
        g_API->Log(g_Addon, ADDON_LOG_INFO, "Unloading ExampleAddon...");

        // Unregister native function
        if (g_HelloFuncHandle)
        {
            g_API->UnregisterFunction(g_Addon, g_HelloFuncHandle);
            g_HelloFuncHandle = 0;
        }

        // Unregister event listener
        if (g_PlayerConnectEventHandle)
        {
            g_API->UnregisterEvent(g_Addon, g_PlayerConnectEventHandle);
            g_PlayerConnectEventHandle = 0;
        }

        g_API->Log(g_Addon, ADDON_LOG_INFO, "ExampleAddon unloaded cleanly.");
    }

    g_API = nullptr;
    g_Addon = nullptr;
}
