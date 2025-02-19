#pragma once

#include <zUtilO/zUtilO.h>


namespace WindowHelpers
{
    // Centers the window on the current screen.
    CLASS_DECL_ZUTILO void CenterOnScreen(HWND hWnd);

    // Centers the window on the parent window (regardless of what monitor the parent window is on).
    CLASS_DECL_ZUTILO void CenterOnParent(HWND hWnd, int width, int height);

    // Disables the window's close button.
    CLASS_DECL_ZUTILO void DisableClose(HWND hWnd);

    // Brings the window to the forefront.
    CLASS_DECL_ZUTILO void BringToForefront(HWND hWnd);

    // Calls EnableWindow on all controls on the dialog.
    CLASS_DECL_ZUTILO void EnableWindow(CDialog& dlg, BOOL bEnable);

    // Sets a dialog's system icon.
    CLASS_DECL_ZUTILO void SetDialogSystemIcon(CDialog& dlg, HICON hIcon);

    // Removes a dialog's system icon.
    CLASS_DECL_ZUTILO void RemoveDialogSystemIcon(CDialog& dlg);

    // Adds the "About..." menu item to a dialog's system menu.
    CLASS_DECL_ZUTILO void AddDialogAboutMenuItem(CDialog& dlg, int about_box_text_resource_id, int about_box_menu_resource_id);

    // Replaces a menu item with a popup menu.
    CLASS_DECL_ZUTILO void ReplaceMenuItemWithPopupMenu(CMenu* pPopupMenu, unsigned menu_item_resource_id, HINSTANCE hInstance, unsigned popup_menu_resource_id);

    // Calls the update handlers for the menu items.
    CLASS_DECL_ZUTILO void DoUpdateForMenuItems(CMenu* pPopupMenu, CCmdTarget* pTarget, BOOL disable_if_no_handler = FALSE);

    // Returns the position of a menu item, searching by command, returning -1 if not found.
    CLASS_DECL_ZUTILO int GetMenuItemPositionByCommand(CMenu& menu, unsigned command);



    // Shows a modeless dialog, assuming ownership of the dialog memory,
    // and then makes sure the dialog is destroyed on destruction.
    class ModelessDialogHolder
    {
    public:
        ModelessDialogHolder(CDialog* dlg)
            :   m_dlg(dlg)
        {
            ASSERT(m_dlg != nullptr);
            m_dlg->ShowWindow(SW_SHOW);
        }

        ModelessDialogHolder(const ModelessDialogHolder&) = delete;

        ModelessDialogHolder(ModelessDialogHolder&& rhs) noexcept
            :   m_dlg(rhs.m_dlg)
        {
            rhs.m_dlg = nullptr;
        }

        ~ModelessDialogHolder()
        {
            if( m_dlg != nullptr )
            {
                m_dlg->DestroyWindow();
                delete m_dlg;
            }
        }

        HWND GetHwnd() const { return m_dlg->m_hWnd; }

    private:
        CDialog* m_dlg;
    };
}
