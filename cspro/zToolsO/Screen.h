#pragma once

#include <zToolsO/zToolsO.h>


class CLASS_DECL_ZTOOLSO Screen
{
private:
    Screen();
    static const Screen& Instance();

public:
#ifdef WIN_DESKTOP

    // --- full size
    static const CSize& GetFullSize() { return Instance().m_fullSize; }
    static LONG GetFullWidth()        { return Instance().m_fullSize.cx; }
    static LONG GetFullHeight()       { return Instance().m_fullSize.cy; }

    // --- display center point (assuming the taskbar is on the bottom or right)
    static const POINT& GetMaxDisplayCenterPoint() { return Instance().m_maxDisplayCenterPoint; }

    // --- bottom point that will not cover the taskbar (assuming the taskbar is on the bottom)
    static const int GetMaxViewableHeight() { return Instance().m_maxViewableHeight; }

#endif

    // --- display size (on Windows, these values are 90% of the screen size
    //                   and account for the taskbar width/height)
    static const CSize& GetMaxDisplaySize() { return Instance().m_maxDisplaySize; }
    static LONG GetMaxDisplayWidth()        { return Instance().m_maxDisplaySize.cx; }
    static LONG GetMaxDisplayHeight()       { return Instance().m_maxDisplaySize.cy; }

    static CSize GetScaledDisplaySize(double scale)
    {
        ASSERT(scale > 0 && scale <= 1);
        CSize size = GetMaxDisplaySize();
        size.cx = static_cast<LONG>(scale * size.cx);
        size.cy = static_cast<LONG>(scale * size.cy);
        return size;
    }

    // parses the dimension text, which can be specified as one of: "500", "500px", or "80%";
    // if the text cannot be parsed, or is less than 1 or greater than max_display_size,
    // the value for max_display_size is returned
    static LONG ParseDimensionText(const std::wstring& dimension_text, LONG max_display_size, std::function<void()> on_error_callback = { });

private:
#ifdef WIN_DESKTOP
    CSize m_fullSize;
    POINT m_maxDisplayCenterPoint;
    int m_maxViewableHeight;
#endif
    CSize m_maxDisplaySize;
};
