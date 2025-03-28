#pragma once

#include <zCapiO/zCapiO.h>


class CRectExt : public CRect
{
public:
    bool Collapse( const CRect* pRect, const bool bCenter=TRUE );
    bool CenterRect( const CRect* parenRect );
    bool BestPos( const CRect* parentRect, const CRect* fieldRect, const int iPosition, CRect* bestRect );
    CLASS_DECL_ZCAPIO static bool UnIntersect( CRect* pRect, const CRect cFixedRect, CRect maxRect );
    static bool MoveTo( CRect* pRect, int iPos, const CRect cFixedRect );
    static bool FitIn( CRect* pRect, const CRect maxRect );
};
