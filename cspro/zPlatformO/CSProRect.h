#pragma once

#ifndef WIN32
#include <zPlatformO/PortableWindowsDefines.h>
#endif
#include <zPlatformO/CSProPoint.h>
#include <zPlatformO/CSProSize.h>

class CCSProRect
{
    // tagRect inheritance facade
public:
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;

public:

    CCSProRect() { }
    CCSProRect(LONG left, LONG top, LONG right, LONG bottom) { this->left = left; this->top = top; this->right = right; this->bottom = bottom; }

    CCSProRect(CCSProPoint pt, CCSProSize size) {this->left = pt.x; this->top = pt.y; this->right = pt.x + size.cx; this->bottom = pt.y + size.cy; }

    void SetRect(LONG left, LONG top, LONG right, LONG bottom) { this->left = left; this->top = top; this->right = right; this->bottom = bottom; }
    void SetRectEmpty() { this->left = this->top = this->right = this->bottom = 0; }

    BOOL IsRectEmpty() const { return ( right - left ) <= 0 || ( bottom - top ) <= 0; }
    CCSProPoint& BottomRight() const { return *( (CCSProPoint*)&right ); }
    CCSProPoint& TopLeft() const { return *( (CCSProPoint*)&left ); }

    BOOL PtInRect(CCSProPoint point)
    {
        return point.x >= left &&
            point.x < right &&
            point.y >= top &&
            point.y < bottom;
    }

    CCSProSize Size() const
    {
        return CCSProSize(right - left, bottom - top);
    }

    int Width() const
    {
	    return right - left;
    }

    int Height() const
    {
	    return bottom - top;
    }

    // unimplemented (and unused in the portable environment)
    void InflateRect(int, int) { ASSERT(false); }

    void OffsetRect(int, int)           { ASSERT(false); }
    void OffsetRect(const CCSProPoint&) { ASSERT(false); }
};
