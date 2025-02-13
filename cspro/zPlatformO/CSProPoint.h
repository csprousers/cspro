#pragma once

#ifndef WIN32
#include <zPlatformO/PortableWindowsDefines.h>
#endif

class CCSProPoint
{
    // tagPOINT inheritance facade
public:
    LONG  x;
    LONG  y;

// constructors
public:
    CCSProPoint(){}
    CCSProPoint(LONG x, LONG y){this->x = x;this->y = y;}

};
