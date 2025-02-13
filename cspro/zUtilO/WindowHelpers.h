#pragma once

#include <zUtilO/zUtilO.h>


namespace WindowHelpers
{
    // centers the window on the current screen
    CLASS_DECL_ZUTILO void CenterOnScreen(HWND hWnd);

    // centers the window on the parent window (regardless of what monitor the parent window is on)
    CLASS_DECL_ZUTILO void CenterOnParent(HWND hWnd, int width, int height);

    // disables the window's close button
    CLASS_DECL_ZUTILO void DisableClose(HWND hWnd);

    // brings the window to the forefront
    CLASS_DECL_ZUTILO void BringToForefront(HWND hWnd);

    // sets a dialog's system icon
    CLASS_DECL_ZUTILO void SetDialogSystemIcon(CDialog& dlg, HICON hIcon);

    // removes a dialog's system icon
    CLASS_DECL_ZUTILO void RemoveDialogSystemIcon(CDialog& dlg);

    // adds the "About..." menu item to a dialog's system menu
    CLASS_DECL_ZUTILO void AddDialogAboutMenuItem(CDialog& dlg, int about_box_text_resource_id, int about_box_menu_resource_id);

    // replaces a menu item with a popup menu
    CLASS_DECL_ZUTILO void ReplaceMenuItemWithPopupMenu(CMenu* pPopupMenu, unsigned menu_item_resource_id, HINSTANCE hInstance, unsigned popup_menu_resource_id);

    // calls the update handlers for the menu items
    CLASS_DECL_ZUTILO void DoUpdateForMenuItems(CMenu* pPopupMenu, CCmdTarget* pTarget, BOOL disable_if_no_handler = FALSE);



    // shows a modeless dialog, assuming ownership of the dialog memory,
    // and then makes sure the dialog is destroyed on destruction
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
