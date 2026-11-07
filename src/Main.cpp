#include "CAddon.h"

class CExampleAddon final : public CAddon
{
    bool OnLoad() override
    {
        Function("hello", [](std::optional<std::string> name)
            {
                return "Hello, " + name.value_or("Developer") + "!";
            });

        Event("onPlayerConnect", [this](int playerId)
            {
                const auto player = Players.Find(playerId);
                if (player.IsValid())
                {
                    Log.Info("Player {} connected", player.Id());
                }
            });

        Once("onMainScriptInit", [this]()
            {
                Log.Info("The game script is ready");
            });
        return true;
    }
};

LEGACY_ADDON(CExampleAddon)
