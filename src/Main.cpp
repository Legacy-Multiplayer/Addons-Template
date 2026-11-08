#include "SDK/Plugin.h"

using namespace SDK;

class CExampleAddon final : public CPlugin
{
    bool OnLoad() override
    {
        Function("hello", [](std::optional<std::string> name)
        {
            return "Hello, " + name.value_or("Developer") + "!";
        });

        Function("echoOptions", [](CTable options)
        {
            return options;
        });

        Function("countOptionsLater", [this](CTable options, CCallback callback)
        {
            Async([options](CCancellation cancellation)
            {
                return cancellation.IsCancelled() ? size_t{0} : options.Size();
            }, [callback](size_t count)
            {
                CTable result;
                result.Set("count", count);
                callback.Invoke(result);
            });

            return true;
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

REGISTER_PLUGIN(CExampleAddon)
