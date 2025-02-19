#pragma once

#include <zRuntimeO/zRuntimeO.h>
#include <zToolsO/PointerClasses.h>

class RuntimeHost;


class ZRUNTIMEO_API Runtime
{
public:
    virtual ~Runtime() { }

    // sets the current runtime host;
    // the runtime host is guaranteed to be set before any other calls to the Runtime
    void SetRuntimeHost(cs::non_null_shared_or_raw_ptr<RuntimeHost> runtime_host) { m_runtimeHost = std::move(runtime_host); }

    // returns a description of the runtime to be displayed by the runtime host
    virtual SharableString GetDescription() noexcept = 0;

    // returns the unique URL that identifies this runtime;
    // multiple instances of the same runtime should yield unique URLs
    virtual SharableString GetUrl() noexcept = 0;

    // returns true if the runtime can be suspended immediately (i.e., no background tasks are running);
    // the base implementation calls IsCloseable
    virtual bool IsSuspendable() noexcept;

    // called from the runtime host thread when the runtime is being suspended;
    // the base implementation does nothing
    virtual void OnSuspend() noexcept;

    // returns true if the runtime can be closed immediately (i.e., no background tasks are running)
    virtual bool IsCloseable() noexcept = 0;

    // called from the runtime host thread when the user attempts to close the runtime from the interface;
    // when the program can be closed, call RuntimeHost::CloseRuntime, which is what the base implementation does
    virtual void OnClose() noexcept;

    // starts the runtime; the runtime host must be called prior to starting the runtime;
    // exceptions can be thrown and displayed by the runtime host
    virtual void Start() = 0;

    // called when the runtime is activated, either after starting or when resuming (if a runtime was stacked on top of this runtime);
    // the base implementation navigates to GetUrl()
    virtual void OnActivate() noexcept;

    // called from the runtime host, in a non-UI thread, when a message is received;
    // the message is split into its action and data;
    // return the results of processing (as JSON), or SharableString() if undefined;
    // call up to a parent class to handle other actions;
    // exceptions can be thrown and will be displayed by the runtime host
    virtual SharableString OnMessage(std::string_view action_sv, const JsonNode& json_node);

protected:
    cs::shared_or_raw_ptr<RuntimeHost> m_runtimeHost;
};
