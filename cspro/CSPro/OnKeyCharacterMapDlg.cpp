#include "StdAfx.h"
#include "OnKeyCharacterMapDlg.h"
#include <zEngineO/OnKeyMapping.h>


IMPLEMENT_DYNAMIC(OnKeyCharacterMapDlg, CDialogEx)


OnKeyCharacterMapDlg::OnKeyCharacterMapDlg(CWnd* pParent/* = nullptr*/)
    :   CDialogEx(IDD_ONKEY_CHAR_MAP, pParent)
{
}


BOOL OnKeyCharacterMapDlg::PreTranslateMessage(MSG* pMsg)
{
    auto set_char_and_number = [&](auto char_resource_id, auto number_resource_id, auto number)
    {
        GetDlgItem(char_resource_id)->SetWindowText(FormatText(_T("%c"), (TCHAR)pMsg->wParam));
        GetDlgItem(number_resource_id)->SetWindowText(IntToString(number));
    };

    if( pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN )
    {
        bool bCtrl = GetAsyncKeyState(VK_CONTROL) < 0;
        bool bAlt = GetAsyncKeyState(VK_MENU) < 0;
        bool bShift = GetAsyncKeyState(VK_SHIFT) < 0;
        int iChar = pMsg->wParam;

        if( bCtrl )     iChar += OnKeyMapping::Ctrl;
        if( bAlt )      iChar += OnKeyMapping::Alt;
        if( bShift )    iChar += OnKeyMapping::Shift;

        ((CButton*)GetDlgItem(IDC_ONKEY_CONTROL))->SetCheck(bCtrl);
        ((CButton*)GetDlgItem(IDC_ONKEY_ALT))->SetCheck(bAlt);
        ((CButton*)GetDlgItem(IDC_ONKEY_SHIFT))->SetCheck(bShift);

        set_char_and_number(IDC_ONKEY_CHAR, IDC_ONKEY_CODE, iChar);

        GetDlgItem(IDC_ONKEY_UNICODE_CHAR)->SetWindowText(_T(""));
        GetDlgItem(IDC_ONKEY_UNICODE_CODE)->SetWindowText(_T(""));

        // just Alt by itself
        if( iChar == 4018 )
            return TRUE;

        // these keystrokes could be used to close the dialog box
        if( pMsg->wParam == VK_ESCAPE || pMsg->wParam == VK_SPACE || pMsg->wParam == VK_RETURN ||
            pMsg->wParam == VK_TAB || ( pMsg->wParam >= VK_F2 && pMsg->wParam <= VK_F24 ) )
        {
            return TRUE;
        }
    }

    else if( pMsg->message == WM_CHAR )
    {
        set_char_and_number(IDC_ONKEY_UNICODE_CHAR, IDC_ONKEY_UNICODE_CODE, pMsg->wParam);
        return TRUE;
    }

    return CDialogEx::PreTranslateMessage(pMsg);
}
