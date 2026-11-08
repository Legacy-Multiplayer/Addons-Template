addEventHandler("onMainScriptInit", function()
    local options = echoOptions({
        name = "Example",
        scores = { 10, 20, 30 },
        enabled = true,
    })

    print(options.name, options.scores[2])

    countOptionsLater(options, function(result)
        print("Options counted: " .. result.count)
    end)
end)
