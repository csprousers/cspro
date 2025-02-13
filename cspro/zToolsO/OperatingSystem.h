#pragma once


constexpr bool OnAndroid()
{
#ifdef ANDROID
    return true;
#else
    return false;
#endif
}


constexpr bool OnWasm()
{
#ifdef WASM
    return true;
#else
    return false;
#endif
}


constexpr bool OnAndroidOrWasm()
{
    return ( OnAndroid() || OnWasm() );
}


constexpr bool OnWindows()
{
#ifdef WIN32
    return true;
#else
    return false;
#endif
}


constexpr bool OnWindowsDesktop()
{
#ifdef WIN_DESKTOP
    return true;
#else
    return false;
#endif
}
