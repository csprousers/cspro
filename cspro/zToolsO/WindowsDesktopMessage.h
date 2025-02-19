#pragma once

#include <zToolsO/zToolsO.h>

struct CacheableObject;
class CWnd;


// --------------------------------------------------------------------------
// WindowsDesktopMessage
//
// A mechanism for sending or posting messages on Windows. These calls
// will be ignored on other platforms.
//
// WPARAM and LPARAM values can come in:
//     - values (which will be cast using static_cast)
//     - pointers (which will be cast using reinterpret_cast)
//
// Additionally, PostObject allows objects to be cached, a message posted,
// and then the object retrieved.
// --------------------------------------------------------------------------

class WindowsDesktopMessage
{
public:
    // Sends a message to the Windows main window.
    template<typename WT = WPARAM, typename LT = LPARAM>
    static LONG Send(UINT message, WT wparam_value = 0, LT lparam_value = 0);

    // Posts a message to the Windows main window.
    template<typename WT = WPARAM, typename LT = LPARAM>
    static BOOL Post(UINT message, WT wparam_value = 0, LT lparam_value = 0);

#ifdef WIN_DESKTOP
    // Posts a message to the Windows main window, or a specified window, along with an object.
    // The object will be cached and a cache key will be posted as the WPARAM value.
    template<typename LT = LPARAM>
    static BOOL PostObject(UINT message, SharableString object, LT lparam_value = 0);

    template<typename LT = LPARAM>
    static BOOL PostObject(CWnd* wnd, UINT message, SharableString object, LT lparam_value = 0);

    template<typename LT = LPARAM>
    static BOOL PostObject(UINT message, std::shared_ptr<CacheableObject> object, LT lparam_value = 0);

    template<typename LT = LPARAM>
    static BOOL PostObject(CWnd* wnd, UINT message, std::shared_ptr<CacheableObject> object, LT lparam_value = 0);

    // Returns a posted object. If the cache key is invalid, then an unset SharableString
    // or a null CacheableObject is returned.
    template<typename T>
    static T GetPostedObject(WPARAM cache_key);
#endif

private:
    static CWnd* GetMainWindow();

    template<typename RT, typename WT, typename LT>
    static RT Worker(CWnd* wnd, UINT message, WT wparam_value, LT lparam_value);

#ifdef WIN_DESKTOP
    using Object = std::variant<std::monostate, SharableString, std::shared_ptr<CacheableObject>>;

    CLASS_DECL_ZTOOLSO static WPARAM CacheObject(Object object);

    template<typename T>
    CLASS_DECL_ZTOOLSO static T GetCachedObject(size_t cache_key);

private:
    struct Data;
    static Data m_data;
#endif
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline CWnd* WindowsDesktopMessage::GetMainWindow()
{
#ifdef WIN_DESKTOP
    if( AfxGetApp() != nullptr )
        return AfxGetApp()->GetMainWnd();
#endif

    return nullptr;
}


template<typename WT/* = WPARAM*/, typename LT/* = LPARAM*/>
LONG WindowsDesktopMessage::Send(const UINT message, WT wparam_value/* = 0*/, LT lparam_value/* = 0*/)
{
    return Worker<LONG, WT, LT>(GetMainWindow(), message, wparam_value, lparam_value);
}


template<typename WT/* = WPARAM*/, typename LT/* = LPARAM*/>
BOOL WindowsDesktopMessage::Post(const UINT message, WT wparam_value/* = 0*/, LT lparam_value/* = 0*/)
{
    return Worker<BOOL, WT, LT>(GetMainWindow(), message, wparam_value, lparam_value);
}


#ifdef WIN_DESKTOP

template<typename RT, typename WT, typename LT>
RT WindowsDesktopMessage::Worker(CWnd* const wnd, UINT message, WT wparam_value, LT lparam_value)
{
    if( IsWindow(wnd->GetSafeHwnd()) )
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
            return wnd->SendMessage(message, wparam, lparam);
        }

        else
        {
            return wnd->PostMessage(message, wparam, lparam);
        }
    }

    return 0;
}


template<typename LT/* = LPARAM*/>
BOOL WindowsDesktopMessage::PostObject(const UINT message, SharableString object, LT lparam_value/* = 0*/)
{
    return Worker<BOOL, WPARAM, LT>(GetMainWindow(), message, CacheObject(std::move(object)), lparam_value);
}


template<typename LT/* = LPARAM*/>
BOOL WindowsDesktopMessage::PostObject(CWnd* const wnd, const UINT message, SharableString object, LT lparam_value/* = 0*/)
{
    return Worker<BOOL, WPARAM, LT>(wnd, message, CacheObject(std::move(object)), lparam_value);
}


template<typename LT/* = LPARAM*/>
BOOL WindowsDesktopMessage::PostObject(const UINT message, std::shared_ptr<CacheableObject> object, LT lparam_value/* = 0*/)
{
    return Worker<BOOL, WPARAM, LT>(GetMainWindow(), message, CacheObject(std::move(object)), lparam_value);
}


template<typename LT/* = LPARAM*/>
BOOL WindowsDesktopMessage::PostObject(CWnd* const wnd, const UINT message, std::shared_ptr<CacheableObject> object, LT lparam_value/* = 0*/)
{
    return Worker<BOOL, WPARAM, LT>(wnd, message, CacheObject(std::move(object)), lparam_value);
}


template<typename T>
static T WindowsDesktopMessage::GetPostedObject(const WPARAM cache_key)
{
    if constexpr(std::is_same_v<T, SharableString>)
    {
        return GetCachedObject<SharableString>(static_cast<size_t>(cache_key));
    }

    else
    {
        return std::dynamic_pointer_cast<T, CacheObject>(GetCachedObject(static_cast<size_t>(cache_key)));
    }
}


#else

template<typename RT, typename WT, typename LT>
RT WindowsDesktopMessage::Worker(CWnd* /*wnd*/, UINT /*message*/, WT /*wparam_value*/, LT /*lparam_value*/)
{
    return 0;
}

#endif
