#pragma once


class WindowsDesktopMessage
{
public:
    // WPARAM and LPARAM values can come in:
    // - values (which will be cast using static_cast)
    // - pointers (which will be cast using reinterpret_cast)

    // Send: sends a message to the Windows main window
    template<typename WT = WPARAM, typename LT = LPARAM>
    static LONG Send(UINT message, WT wparam_value = 0, LT lparam_value = 0)
    {
        return Worker<LONG, WT, LT>(message, wparam_value, lparam_value);
    }

    // Post: posts a message to the Windows main window
    template<typename WT = WPARAM, typename LT = LPARAM>
    static BOOL Post(UINT message, WT wparam_value = 0, LT lparam_value = 0)
    {
        return Worker<BOOL, WT, LT>(message, wparam_value, lparam_value);
    }

private:
    template<typename RT, typename WT, typename LT>
    static RT Worker(UINT message, WT wparam_value, LT lparam_value)
    {
#ifdef WIN_DESKTOP
        CWnd* main_wnd = ( AfxGetApp() != nullptr ) ? AfxGetApp()->GetMainWnd() : nullptr;

        if( IsWindow(main_wnd->GetSafeHwnd()) )
        {
            WPARAM wparam;
            LPARAM lparam;

            if constexpr(std::is_pointer_v<WT>)
            {
                wparam = reinterpret_cast<WPARAM>(wparam_value);
            }

            else
            {
                wparam = static_cast<WPARAM>(wparam_value);
            }

            if constexpr(std::is_pointer_v<LT>)
            {
                lparam = reinterpret_cast<LPARAM>(lparam_value);
            }

            else
            {
                lparam = static_cast<LPARAM>(lparam_value);
            }

            if constexpr(std::is_same_v<RT, LONG>)
            {
                static_assert(sizeof(LONG) == sizeof(LRESULT));
                return main_wnd->SendMessage(message, wparam, lparam);
            }

            else
            {
                return main_wnd->PostMessage(message, wparam, lparam);
            }
        }
    #endif

        return 0;
    }
};
