--[[
    Legacy Server - Example Addon Lua Script
    Demonstrates interaction with the native ExampleAddon.
]]

-- ---------------------------------------------------------------------
-- 1. Reading Global Constants Exported by Native Addon
-- ---------------------------------------------------------------------

if EXAMPLE_ADDON_VERSION then
    print(string.format("[Lua] ExampleAddon Version: %s (Loaded: %s)", tostring(EXAMPLE_ADDON_VERSION), tostring(EXAMPLE_ADDON_LOADED)))
end

-- ---------------------------------------------------------------------
-- 2. Listening to Custom Addon Events
-- ---------------------------------------------------------------------

-- The native addon triggers "ExampleAddon.OnReady" upon successful initialization in AddonMain
addEventHandler("ExampleAddon.OnReady", function(statusMessage)
    print(string.format("[Lua] Received event from Native Addon: %s", tostring(statusMessage)))
end)

-- ---------------------------------------------------------------------
-- 3. Calling Functions Registered by the Native Addon
-- ---------------------------------------------------------------------

-- The native addon exports `hello([name]) -> string` into the global Lua environment
local greeting = hello("Legacy Developer")
print(string.format("[Lua] Addon function returned: %s", greeting))

-- ---------------------------------------------------------------------
-- 4. Server Event Handling
-- ---------------------------------------------------------------------

-- When a player connects, both this Lua script and the native addon receive the event
addEventHandler("onPlayerConnect", function(playerId)
    print(string.format("[Lua] Player %d connected (Handled in Lua script)", playerId))
end)
