#include "StdAfx.h"
#include "PropertiesDlgPage.h"


namespace
{
    struct ShiftProperties
    {
        CWnd* pWnd;
        bool calculating_locations;
        LONG x_min;
        LONG y_min;
    };

    BOOL CALLBACK ShiftPropertiesCallback(HWND hWnd, LPARAM lParam)
    {
        ShiftProperties& shift_properties = *reinterpret_cast<ShiftProperties*>(lParam);

        RECT rect;
        GetWindowRect(hWnd, &rect);
        shift_properties.pWnd->ScreenToClient(&rect);

        if( shift_properties.calculating_locations )
        {
            shift_properties.x_min = std::min(shift_properties.x_min, rect.left);
            shift_properties.y_min = std::min(shift_properties.y_min, rect.top);
        }

        else
        {
            SetWindowPos(hWnd, nullptr, rect.left - shift_properties.x_min, rect.top - shift_properties.y_min, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }

        return TRUE;
    }
}


void PropertiesDlgPage::ShiftNonPropertyPageDialogChildWindows(CWnd* pWnd)
{
    // first calculate the minimum x/y
    ShiftProperties shift_properties
    {
        pWnd,
        true,
        std::numeric_limits<LONG>::max(),
        std::numeric_limits<LONG>::max(),
    };

    EnumChildWindows(pWnd->m_hWnd, ShiftPropertiesCallback, reinterpret_cast<LPARAM>(&shift_properties));

    // then move the windows
    shift_properties.calculating_locations = false;
    EnumChildWindows(pWnd->m_hWnd, ShiftPropertiesCallback, reinterpret_cast<LPARAM>(&shift_properties));
}
