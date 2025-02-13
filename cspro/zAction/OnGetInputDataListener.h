#pragma once

#include <zAction/Listener.h>


namespace ActionInvoker
{
    // --------------------------------------------------------------------------
    // OnGetInputDataListener
    // a listener, not associated with any caller, that serves input data
    // --------------------------------------------------------------------------

    class OnGetInputDataListener : public Listener
    {
    public:
        OnGetInputDataListener(std::function<std::wstring()> on_get_input_data_callback);

        // Listener overrides
        std::optional<std::wstring> OnGetInputData(Caller& caller, bool match_caller) override;

    private:
        std::function<std::wstring()> m_onGetInputDataCallback;
    };
}


inline ActionInvoker::OnGetInputDataListener::OnGetInputDataListener(std::function<std::wstring()> on_get_input_data_callback)
    :   m_onGetInputDataCallback(std::move(on_get_input_data_callback))
{
    ASSERT(m_onGetInputDataCallback);
}


inline std::optional<std::wstring> ActionInvoker::OnGetInputDataListener::OnGetInputData(Caller& /*caller*/, bool match_caller)
{
    if( !match_caller )
        return m_onGetInputDataCallback();

    return std::nullopt;
}
