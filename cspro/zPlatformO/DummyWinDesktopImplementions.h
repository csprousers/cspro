#pragma once

#include <zToolsO/CSProException.h>

class CBrush;
class CGdiObject;
using HGDIOBJ = void*;

#ifndef WIN32
constexpr int BF_RECT = 0;
constexpr int BLACK_BRUSH = 0;
constexpr int BLACK_PEN = 0;
constexpr int EDGE_ETCHED = 0;
constexpr int EDGE_BUMP = 0;
using HFONT = void*;
#endif // !WIN32


class CFont
{
public:
    HFONT CreateFontIndirect(const LOGFONT*) { return ReturnProgrammingError(nullptr); }
};


class CDC
{
public:
    BOOL DrawEdge(CRect&, UINT, UINT) { return ReturnProgrammingError(FALSE); }
    BOOL DrawEdge(CRect*, UINT, UINT) { return ReturnProgrammingError(FALSE); }

    void FrameRect(const CRect&, CBrush*) { ASSERT(false); }
    void FrameRect(const CRect*, CBrush*) { ASSERT(false); }

    CBrush* GetCurrentBrush() const { return ReturnProgrammingError(nullptr); }

    CSize GetTextExtent(LPCTSTR, int) const   { return ReturnProgrammingError(CSize()); }
    CSize GetTextExtent(const CString&) const { return ReturnProgrammingError(CSize()); }

    BOOL LineTo(CCSProPoint) { return ReturnProgrammingError(FALSE); }

    CPoint MoveTo(CCSProPoint) { return ReturnProgrammingError(CPoint()); }

    BOOL RestoreDC(int) { return ReturnProgrammingError(FALSE); }

    int SaveDC() { return ReturnProgrammingError(0); }

    HGDIOBJ SelectObject(CFont&) { return ReturnProgrammingError(nullptr); }
    HGDIOBJ SelectObject(HGDIOBJ) { return ReturnProgrammingError(nullptr); }

    CGdiObject* SelectStockObject(int) { return ReturnProgrammingError(nullptr); }

    COLORREF SetTextColor(COLORREF) { return ReturnProgrammingError(0); }

    BOOL TextOut(int, int, LPCTSTR, int) { return ReturnProgrammingError(FALSE); }
};
