#pragma once

#ifndef WIN32
#include <zPlatformO/PortableWindowsDefines.h>
#endif

class CCSProSize
{
// tagSIZE inheritance facade
public:
    LONG        cx;
    LONG        cy;

// constructors
public:
    CCSProSize(){}
    CCSProSize(LONG cx, LONG cy){this->cx = cx; this->cy = cy;}

};
