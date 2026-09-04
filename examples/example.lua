--[[
    Legacy Server - Example Addon Lua Script
    Demonstrates interaction with the native ExampleAddon.

    IMPORTANT NOTE ON sendClientMessage:
    `sendClientMessage(playerId, color, message)` is an EXISTING server-side
    Lua function registered by Legacy Server's core messaging system.
    The native addon invokes `sendClientMessage` directly using the dynamic
    `CallContext` API — the addon does NOT register or reimplement it.
]]

-- ---------------------------------------------------------------------
-- 1. Listening to Custom Addon Events
-- ---------------------------------------------------------------------

-- The native addon triggers "ExampleAddon.OnReady" upon successful initialization in AddonMain
addEventHandler("ExampleAddon.OnReady", function(statusMessage)
    print(string.format("[Lua] Received event from Native Addon: %s", tostring(statusMessage)))
end)

-- ---------------------------------------------------------------------
-- 2. Calling Functions Registered by the Native Addon
-- ---------------------------------------------------------------------

-- The native addon exports `hello([name]) -> string` into the global Lua environment
local greeting = hello("Legacy Developer")
print(string.format("[Lua] Addon function returned: %s", greeting))

-- ---------------------------------------------------------------------
-- 3. Demonstrating Existing Server Lua Function: sendClientMessage
-- ---------------------------------------------------------------------

-- Example function demonstrating standard Lua usage of the server's built-in sendClientMessage
function WelcomePlayer(playerId)
    -- Signature: sendClientMessage(playerId: int, color: int, message: string) -> bool
    -- Color: 0x00FF00FF (Green in RGBA)
    local success = sendClientMessage(playerId, 0x00FF00FF, "Welcome to Legacy Multiplayer!")
    if success then
        print(string.format("[Lua] Welcome message sent to player ID %d", playerId))
    else
        print(string.format("[Lua] Failed to send message to player ID %d (player may be disconnected)", playerId))
    end
end

-- ---------------------------------------------------------------------
-- 4. Server Event Handling
-- ---------------------------------------------------------------------

-- When a player connects, both this Lua script and the native addon's OnPlayerConnectHandler receive the event
addEventHandler("onPlayerConnect", function(playerId)
    print(string.format("[Lua] Player %d connected (Handled in Lua script)", playerId))
    WelcomePlayer(playerId)
end)
