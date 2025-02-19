#pragma once

#include <zRuntimeO/zRuntimeO.h>

class Runtime;


class ZRUNTIMEO_API RuntimeHost
{
public:
    virtual ~RuntimeHost() { }

    // indicates that the runtime host should try to close the current runtime
    virtual void CloseRuntimeAsync() noexcept = 0;

    // navigates to the specified URL
    virtual void NavigateToAsync(const SharableString& url) noexcept = 0;

    // posts the JSON message to the web view
    virtual void PostWebMessageAsync(const SharableString& message) noexcept = 0;

    // formats the action and JSON data and posts the message using PostWebMessageAsync
    void PostActionMessageAsync(std::string_view action_sv) noexcept;
    void PostActionMessageAsync(std::string_view action_sv, std::string_view data_sv) noexcept;

    // creates a non-null runtime for an application;
    // if a runtime cannot be created, ErrorDisplayRuntime is returned showing the error
    virtual std::unique_ptr<Runtime> CreateRuntimeForApplication(const std::string& file_path) noexcept = 0;

    // starts the non-null runtime;
    // if the current runtime cannot be suspended, the runtime is held for starting following
    // the suspension or termination of the current runtime;
    // if there is an error starting the runtime, the error is displayed using ErrorDisplayRuntime
    virtual void StartRuntimeAsync(std::shared_ptr<Runtime> runtime) noexcept = 0;
};
