#include "StdAfx.h"
#include "WindowHelpers.h"
#include <zUtilO/BCMenu.h>

#pragma comment(lib, "dwmapi.lib")


void WindowHelpers::CenterOnScreen(HWND hWnd)
{
    // from https://docs.microsoft.com/en-us/windows/win32/dlgbox/using-dialog-boxes
    RECT rc;
    RECT rcDlg;
    RECT rcOwner;

    GetWindowRect(GetDesktopWindow(), &rcOwner);
    GetWindowRect(hWnd, &rcDlg);
    CopyRect(&rc, &rcOwner);

    OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
    OffsetRect(&rc, -rc.left, -rc.top);
    OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom);

    SetWindowPos(hWnd, HWND_TOP, rcOwner.left + ( rc.right / 2 ), rcOwner.top + ( rc.bottom / 2 ), 0, 0, SWP_NOSIZE);
}


namespace CenterOnParentHelpers
{
    using MonitorAndIndexCounter = std::tuple<HMONITOR, int>;

    BOOL CALLBACK GetMonitorByHandle(HMONITOR hMonitor, HDC /*hdcMonitor*/, LPRECT /*lprcMonitor*/, LPARAM dwData)
    {
        auto& monitor_and_index = *(MonitorAndIndexCounter*)dwData;

        // once the monitor has been found, stop the enumeration
        if( std::get<0>(monitor_and_index) == hMonitor )
            return FALSE;

        // otherwise increment the monitor index and continue enumerating
        ++std::get<1>(monitor_and_index);
        return TRUE;
    }

    int GetMonitorIndex(HMONITOR hMonitor)
    {
        MonitorAndIndexCounter monitor_and_index(hMonitor, 0);

        // success is FALSE because the GetMonitorByHandle callback will have returned FALSE
        if( EnumDisplayMonitors(nullptr, nullptr, GetMonitorByHandle, (LPARAM)&monitor_and_index) == FALSE )
            return std::get<1>(monitor_and_index);

        return -1;
    }
}

void WindowHelpers::CenterOnParent(HWND hWnd, int width, int height)
{
    // get the parent window
    HWND hParentWnd = AfxGetMainWnd()->GetSafeHwnd();
    CRect parent_wnd_rect;
    GetWindowRect(hParentWnd, parent_wnd_rect);

    // compensate for the "shadow" area
    parent_wnd_rect.left += 8;
    parent_wnd_rect.top += 8;
    parent_wnd_rect.right -= 8;
    parent_wnd_rect.bottom -= 8;

    RECT extended_frame_bounds;
    MONITORINFO monitor_info;
    bool use_monitor_info = false;

    if( DwmGetWindowAttribute(hParentWnd, DWMWA_EXTENDED_FRAME_BOUNDS,
                              &extended_frame_bounds, sizeof(extended_frame_bounds)) == S_OK )
    {
        // get information about the monitor so as to adjust based on the DPI
        HMONITOR hMonitor = MonitorFromWindow(hParentWnd, MONITOR_DEFAULTTONEAREST);

        monitor_info.cbSize = sizeof(monitor_info);
    
        if( GetMonitorInfo(hMonitor, &monitor_info) != 0 )
        {
            use_monitor_info = true;

            DISPLAY_DEVICE display_device;
            display_device.cb = sizeof(display_device);

            if( EnumDisplayDevices(nullptr, CenterOnParentHelpers::GetMonitorIndex(hMonitor), &display_device, 0) != 0 )
            {
                DEVMODE dev_mode;
                dev_mode.dmSize = sizeof(dev_mode);

                if( EnumDisplaySettings(display_device.DeviceName, ENUM_CURRENT_SETTINGS, &dev_mode) != 0 )
                {
                    LONG monitor_height = monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top;

                    if( monitor_height != (LONG)dev_mode.dmPelsHeight )
                    {
                        double scaling_factor = monitor_height / (double)dev_mode.dmPelsHeight;

                        parent_wnd_rect.left = (LONG)( extended_frame_bounds.left * scaling_factor );
                        parent_wnd_rect.top = (LONG)( extended_frame_bounds.top * scaling_factor );
                        parent_wnd_rect.right = (LONG)( extended_frame_bounds.right * scaling_factor );
                        parent_wnd_rect.bottom = (LONG)( extended_frame_bounds.bottom * scaling_factor );
                    }
                }
            }
        }
    }

    // center the dialog on the parent (as best as possible)
    auto calculate_position = [&](int size, auto parent_min, auto parent_max, auto screen_min, auto screen_max)
    {
        int position = ( parent_max + parent_min - size ) / 2;

        if( ( position + size ) > screen_max )
            position = screen_max - size;

        return std::max((int)screen_min, position);
    };

    auto& screen_rect = use_monitor_info ? monitor_info.rcWork : parent_wnd_rect;
    
    int x = calculate_position(width, parent_wnd_rect.left, parent_wnd_rect.right, screen_rect.left, screen_rect.right);
    int y = calculate_position(height, parent_wnd_rect.top, parent_wnd_rect.bottom, screen_rect.top, screen_rect.bottom);

    SetWindowPos(hWnd, HWND_TOP, x, y, width, height, 0);
}


void WindowHelpers::DisableClose(HWND hWnd)
{
    // from https://devblogs.microsoft.com/oldnewthing/?p=13803
    EnableMenuItem(GetSystemMenu(hWnd, FALSE), SC_CLOSE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
}


void WindowHelpers::BringToForefront(HWND hWnd)
{
    // from https://stackoverflow.com/questions/916259/win32-bring-a-window-to-top/34414846
    SendMessage(hWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
    SetForegroundWindow(hWnd);
    SetActiveWindow(hWnd);
    SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
}


void WindowHelpers::SetDialogSystemIcon(CDialog& dlg, HICON hIcon)
{
    dlg.SetIcon(hIcon, TRUE);
    dlg.SetIcon(hIcon, FALSE);
}


void WindowHelpers::RemoveDialogSystemIcon(CDialog& dlg)
{
    int extended_window_style = GetWindowLong(dlg.m_hWnd, GWL_EXSTYLE);
    SetWindowLong(dlg.m_hWnd, GWL_EXSTYLE, extended_window_style | WS_EX_DLGMODALFRAME);
}


void WindowHelpers::AddDialogAboutMenuItem(CDialog& dlg, int about_box_text_resource_id, int about_box_menu_resource_id)
{
    // the resource ID must be in the system command range
    ASSERT(( about_box_menu_resource_id & 0xFFF0 ) == about_box_menu_resource_id);
    ASSERT(about_box_menu_resource_id < 0xF000);

    CString about_menu_text;
    about_menu_text.LoadString(about_box_text_resource_id);

    if( !about_menu_text.IsEmpty() )
    {
        CMenu* system_menu = dlg.GetSystemMenu(FALSE);

        if( system_menu != nullptr )
        {
            system_menu->AppendMenu(MF_SEPARATOR);
            system_menu->AppendMenu(MF_STRING, about_box_menu_resource_id, about_menu_text);
        }
    }
}


void WindowHelpers::ReplaceMenuItemWithPopupMenu(CMenu* pPopupMenu, unsigned menu_item_resource_id, HINSTANCE hInstance, unsigned popup_menu_resource_id)
{
    // this functionality does not work properly with BCMenu menus
    ASSERT(dynamic_cast<BCMenu*>(pPopupMenu) == nullptr);

    ASSERT(pPopupMenu->GetMenuState(menu_item_resource_id, MF_BYCOMMAND) != static_cast<unsigned>(-1));

    CString menu_item_text;
    pPopupMenu->GetMenuString(menu_item_resource_id, menu_item_text, MF_BYCOMMAND);

    // load the menu from the resource file
    HMENU hMenu = LoadMenu(hInstance, MAKEINTRESOURCE(popup_menu_resource_id));
    ASSERT(hMenu != nullptr);
        
    CMenu* resource_menu = CMenu::FromHandle(hMenu);
    ASSERT(resource_menu->GetMenuItemCount() == 1);

#ifdef _DEBUG
    // make sure the popup menu is the correct one
    CString resource_menu_item_text;
    resource_menu->GetMenuString(0, resource_menu_item_text, MF_BYPOSITION);
    ASSERT(resource_menu_item_text == menu_item_text);
#endif

    // replace the menu item with the resource menu's submenu
    CMenu* resource_submenu = resource_menu->GetSubMenu(0);

    pPopupMenu->ModifyMenu(menu_item_resource_id, MF_BYCOMMAND | MF_POPUP | MF_STRING, reinterpret_cast<UINT>(resource_submenu->Detach()), menu_item_text);
}


void WindowHelpers::DoUpdateForMenuItems(CMenu* pPopupMenu, CCmdTarget* pTarget, BOOL disable_if_no_handler/* = FALSE*/)
{
    ASSERT(pPopupMenu != nullptr);

    // from https://forums.codeguru.com/showthread.php?369301-UPDATE_COMMAND_UI-does-not-work-for-popup-menu&p=1295853#post1295853
    CCmdUI state;
    state.m_pMenu = pPopupMenu;
 
    state.m_nIndexMax = state.m_pMenu->GetMenuItemCount();
 
    for( state.m_nIndex = 0; state.m_nIndex < state.m_nIndexMax; ++state.m_nIndex )
    {
        state.m_nID = state.m_pMenu->GetMenuItemID(state.m_nIndex);
        state.DoUpdate(pTarget, disable_if_no_handler);
    }
}
