#include "StdAfx.h"
#include "Definitions.h"


// form defaults
// --------------------------------------------------

const PortableColor FormDefaults::FormBackgoundColor = PortableColor::FromRGB(240, 240, 240); // the result of GetSysColor(COLOR_3DFACE)



// ToString functions
// --------------------------------------------------

const TCHAR* ToString(RosterOrientation roster_orientation)
{
    return ( roster_orientation == RosterOrientation::Horizontal ) ? ROSTER_ORIENT_HORZ :
                                                                     ROSTER_ORIENT_VERT;
}
