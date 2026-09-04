/*
    Legacy Server - Native Addon Template
    Main Entry Point and Example Addon Implementation

    Demonstrates:
    - Addon ABI initialization and version negotiation
    - Direct server subsystem APIs (PlayerAPI, MessagingAPI, WorldAPI, etc.)
    - Exposing native C/C++ functions to Lua (with argument and return handling)
    - Subscribing to server events (e.g., onPlayerConnect)
    - Dispatching custom addon events to the server's Lua event bus
    - Invoking dynamic Lua functions via CallContext
    - Setting global Lua constants and variables
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
// 1. Direct Server Subsystem API Example
// =====================================================================

/**
 * @brief Demonstrates calling direct server subsystem APIs (PlayerAPI and MessagingAPI).
 * 
 * Legacy Server exposes 15 direct subsystem API tables on AddonAPI:
 * - actor, player, vehicle, object, pickup, gangzone, label, menu,
 *   messaging, netstats, textdraw, filesystem, variables, world, timers.
 * 
 * These interfaces provide zero-overhead direct function calls into the server core.
 */
static void SendWelcomeMessage(int playerId)
{
    if (!g_API)
    {
        return;
    }

    // Direct access to PlayerAPI
    const char* playerName = "Player";
    if (g_API->player && g_API->player->IsConnected(playerId))
    {
        playerName = g_API->player->GetName(playerId);
    }

    // Construct greeting message
    char welcomeMsg[256];
    snprintf(welcomeMsg, sizeof(welcomeMsg), "Welcome to the server, %s! (Sent via direct MessagingAPI)", playerName);

    // Direct access to MessagingAPI (Color: 0x00FF00FF RGBA green)
    if (g_API->messaging)
    {
        g_API->messaging->SendClientMessage(playerId, 0x00FF00FF, welcomeMsg);
    }
}

// =====================================================================
// 2. Dynamic Lua Function Invocation Example (CallContext)
// =====================================================================

/**
 * @brief Demonstrates calling any dynamic or script-defined Lua function using CallContext.
 * 
 * Use CallContext when you need to call a function defined in a Lua script or gamemode
 * that is not part of the direct C subsystem tables.
 * 
 * @param functionName Global Lua function name to invoke (e.g. "OnCustomScriptAction")
 * @param param Integer parameter to pass
 * @return true if function executed without runtime errors
 */
static bool CallCustomLuaFunction(const char* functionName, int64_t param)
{
    if (!g_API || !g_Addon || !functionName)
    {
        return false;
    }

    // 1. Begin dynamic call context
    CallContext* call = g_API->BeginCall(g_Addon, functionName);
    if (!call)
    {
        return false;
    }

    // 2. Push arguments in exact order expected by the Lua function
    g_API->PushInteger(call, param);

    // 3. Execute the call protected by the server's Lua engine
    bool success = false;
    if (g_API->ExecuteCall(call))
    {
        // 4. Retrieve return values if any
        if (g_API->GetResultCount(call) > 0)
        {
            success = g_API->GetResultBoolean(call, 0);
        }
        else
        {
            success = true;
        }
    }
    else if (g_API->HasError(call))
    {
        char errBuffer[256];
        snprintf(errBuffer, sizeof(errBuffer), "Error calling Lua function '%s': %s", functionName, g_API->GetError(call));
        g_API->Log(g_Addon, ADDON_LOG_WARN, errBuffer);
    }

    // 5. Always end and release the call context
    g_API->EndCall(call);
    return success;
}

// =====================================================================
// 3. Example Native Function Callback (Exposed to Lua)
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
    (void)userData;
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
// 4. Example Server Event Callback (Listening to Server Events)
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
    (void)eventName;
    (void)userData;

    if (!g_API || !g_Addon)
    {
        return;
    }

    if (argumentCount > 0 && (arguments[0].type == ADDON_VALUE_INTEGER || arguments[0].type == ADDON_VALUE_NUMBER))
    {
        int playerId = (arguments[0].type == ADDON_VALUE_INTEGER)
            ? static_cast<int>(arguments[0].integerValue)
            : static_cast<int>(arguments[0].numberValue);

        char logMsg[128];
        snprintf(logMsg, sizeof(logMsg), "Player ID %d connected to the server.", playerId);
        g_API->Log(g_Addon, ADDON_LOG_INFO, logMsg);

        // Send a welcome message directly via subsystem API
        SendWelcomeMessage(playerId);
    }
}

// =====================================================================
// 5. Addon Lifecycle Entry Points
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

    // 1. Export global constants / variables into the server's Lua runtime
    g_API->SetGlobalString(g_Addon, "EXAMPLE_ADDON_VERSION", "1.0.0");
    g_API->SetGlobalInteger(g_Addon, "EXAMPLE_ADDON_LOADED", 1);
    g_API->SetGlobalBoolean(g_Addon, "EXAMPLE_ADDON_ACTIVE", true);

    // 2. Register native function into the server's Lua runtime
    g_HelloFuncHandle = g_API->RegisterFunction(g_Addon, "hello", HelloFunction, nullptr);
    if (!g_HelloFuncHandle)
    {
        g_API->Log(g_Addon, ADDON_LOG_ERROR, "Failed to register native function 'hello'.");
        return false;
    }

    // 3. Register event listener for existing server event: onPlayerConnect
    g_PlayerConnectEventHandle = g_API->RegisterEvent(g_Addon, "onPlayerConnect", OnPlayerConnectHandler, nullptr);
    if (!g_PlayerConnectEventHandle)
    {
        g_API->Log(g_Addon, ADDON_LOG_WARN, "Failed to register event listener for 'onPlayerConnect'.");
    }

    // 4. Trigger a custom addon event through the server's event bus
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
