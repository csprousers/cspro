#pragma once

    #ifdef WIN32
    #pragma message( "WIN32              : ON")
    #else
    #pragma message( "WIN32              : OFF")
    #endif

    #ifdef GENERATE_BINARY
    #pragma message( "GENERATE_BINARY    : ON")
    #else
    #pragma message( "GENERATE_BINARY    : OFF")
    #endif

    #ifdef USE_BINARY
    #pragma message( "USE_BINARY         : ON")
    #else
    #pragma message( "USE_BINARY         : OFF")
    #endif
