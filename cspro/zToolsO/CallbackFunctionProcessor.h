#pragma once


namespace CallbackFunctionProcessor
{
    // --------------------------------------------------------------------------
    // KeepProcessing
    //
    // the KeepProcessing method allows the callback function to:
    // - return a boolean value that, if true, indicates that processing should continue;
    // - return void, which means that processing should always continue
    // --------------------------------------------------------------------------

    template<typename CF, typename ...Args>
    bool KeepProcessing(const CF& callback_function, Args&& ...args)
    {
        if constexpr(std::is_same_v<decltype(callback_function(args...)), bool>)
        {
            return callback_function(std::forward<Args&&>(args)...);
        }

        else
        {
            callback_function(std::forward<Args&&>(args)...);
            return true;
        }
    }
}
