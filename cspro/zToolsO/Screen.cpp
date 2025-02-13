#include "StdAfx.h"
#include "Screen.h"
#include <zPlatformO/PlatformInterface.h>


namespace
{
    const double MaxDisplaySize = 0.90;
}


Screen::Screen()
{
#ifdef WIN_DESKTOP
    CRect rect;
    CWnd::GetDesktopWindow()->GetWindowRect(rect);

    m_fullSize = { rect.Width(),
                   rect.Height() };

    // get the max display area, accounting for the taskbar
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);

    // rect is now the display area if the function above succeeded
    m_maxDisplaySize = { (LONG)( rect.Width() * MaxDisplaySize ),
                         (LONG)( rect.Height() * MaxDisplaySize ) };

    auto taskbar_width = m_fullSize.cx - rect.Width();
    auto taskbar_height = m_fullSize.cy - rect.Height();

    m_maxDisplayCenterPoint.x = ( m_fullSize.cx - taskbar_width ) / 2;
    m_maxDisplayCenterPoint.y = ( m_fullSize.cy - taskbar_height ) / 2;

    m_maxViewableHeight = m_fullSize.cy - taskbar_height;

#else
    std::tie(m_maxDisplaySize.cx, m_maxDisplaySize.cy) = PlatformInterface::GetInstance()->GetApplicationInterface()->GetMaxDisplaySize();

#endif
}

const Screen& Screen::Instance()
{
    static Screen instance;
    return instance;
}


LONG Screen::ParseDimensionText(const std::wstring& dimension_text, LONG max_display_size, std::function<void()> on_error_callback/* = { }*/)
{
    // if the text is not simply a number, the "px" or "%" suffixes are processed
    constexpr wstring_view PixelText = _T("px");
    constexpr wstring_view PercentText = _T("%");

    LONG dimension_value = 0;

    try
    {
        size_t post_value_pos;
        double value = std::stod(dimension_text, &post_value_pos);        

        wstring_view type_sv = SO::Trim(wstring_view(dimension_text).substr(post_value_pos));

        if( type_sv.empty() || type_sv == PixelText )
        {
            dimension_value = static_cast<LONG>(value);
        }

        else if( SO::EqualsNoCase(type_sv, PercentText) )
        {
            dimension_value = static_cast<LONG>(value / 100 * max_display_size);
        }
    }
    catch(...) { }

    if( dimension_value >= 1 && dimension_value <= max_display_size )
        return dimension_value;

    if( on_error_callback )
        on_error_callback();

    return max_display_size;
}
