#pragma once
// RectExt.h: interface for the CRectExt class.
//
//////////////////////////////////////////////////////////////////////

#include <zUtilF/zUtilF.h>

#define CRECTEX_NONE               0

// Position near current field
#define CRECTEX_FIELD              1

// Horizontal parameters
#define CRECTEX_BOTTOM             2
#define CRECTEX_TOP                4
#define CRECTEX_HCENTER            8

// Vertical parameters
#define CRECTEX_VCENTER           16
#define CRECTEX_LEFT              32
#define CRECTEX_RIGHT             64


class CLASS_DECL_ZUTILF CRectExt : public CRect
{
public:
    bool Collapse( const CRect* pRect, const bool bCenter=TRUE );
    bool CenterRect( const CRect* parenRect );
    bool BestPos( const CRect* parentRect, const CRect* fieldRect, const int iPosition, CRect* bestRect );
    static bool UnIntersect( CRect* pRect, const CRect cFixedRect, CRect maxRect );
    static bool MoveTo( CRect* pRect, int iPos, const CRect cFixedRect );
    static bool FitIn( CRect* pRect, const CRect maxRect );
    CRectExt();
    virtual ~CRectExt();
};
