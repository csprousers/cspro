#include "stdafx.h"
#include "CSProScintillaFindReplaceDlg.h"
#include "resource_win32.h"


BEGIN_MESSAGE_MAP(CSProScintillaFindReplaceDlg, Scintilla::CScintillaFindReplaceDlg)
    ON_WM_CLOSE()
    ON_BN_CLICKED(IDC_REPLACE_IN_SELECTION, OnReplaceInSelection)
    ON_CONTROL_RANGE(CBN_SELCHANGE, IDC_FIND_COMBO, IDC_REPLACE_COMBO, OnComboSelChange)
    ON_CONTROL_RANGE(CBN_EDITCHANGE, IDC_FIND_COMBO, IDC_REPLACE_COMBO, OnComboEditChange)
END_MESSAGE_MAP()


std::optional<CPoint> CSProScintillaFindReplaceDlg::m_lastWindowPosition;


namespace
{
    // CFindReplaceDialog uses these IDs, 0x480 and 0x481, to get the find/replace text
    static_assert(IDC_FIND_EDIT == 0x480 && IDC_REPLACE_EDIT == 0x481);

    constexpr bool IsFindComboBoxId(const UINT nID)
    {
        ASSERT(nID == IDC_FIND_COMBO || nID == IDC_REPLACE_COMBO);
        return ( nID == IDC_FIND_COMBO );
    }

    constexpr const TCHAR* GetRegistryKey()
    {
        return _T("Settings");
    }

    constexpr const TCHAR* GetRegistryValueName(UINT nID)
    {
        return IsFindComboBoxId(nID) ? _T("LogicEditorFindString") :
                                       _T("LogicEditorReplaceString");
    }
}


BOOL CSProScintillaFindReplaceDlg::OnInitDialog()
{
    if( !__super::OnInitDialog() )
        return FALSE;

    // handle the enable/check values of the Replace in Selection checkbox and button
    CButton* replace_in_selection_button = static_cast<CButton*>(GetDlgItem(IDC_REPLACE_IN_SELECTION));

    if( replace_in_selection_button != nullptr )
    {
        if( !m_allowReplaceInSelection )
        {
            replace_in_selection_button->EnableWindow(FALSE);            
        }

        else
        {
            replace_in_selection_button->SetCheck(m_replaceInSelection);            
        }
    }


    // restore the find/replace strings from the registry and hide the associated edit controls
    RestoreComboStringsAndHideEditControl(IDC_FIND_COMBO);
    RestoreComboStringsAndHideEditControl(IDC_REPLACE_COMBO);


    // properly position the dialog, restoring its old position when possible
    CRect rect;
    GetWindowRect(&rect);

    HMONITOR monitor = MonitorFromWindow(AfxGetMainWnd()->GetSafeHwnd(), MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO monitor_info { sizeof(MONITORINFO) };
    GetMonitorInfo(monitor, &monitor_info);
    CRect monitor_rect = monitor_info.rcMonitor;

    if( m_lastWindowPosition.has_value() &&
        monitor_rect.PtInRect(*m_lastWindowPosition) &&
        monitor_rect.PtInRect(*m_lastWindowPosition + rect.Size()) )
    {
        // the saved last position of the dialog is non null and is inside the screen of the main window's monitor
        rect.MoveToXY(*m_lastWindowPosition);
    }

    else
    {
        // this is the first time showing the dialog, or the saved position is no longer valid;
        // use the default position of top right corner of window
        CRect main_window_rect;
        AfxGetMainWnd()->GetWindowRect(main_window_rect);

        main_window_rect.IntersectRect(main_window_rect, monitor_rect);

        rect.MoveToXY(CPoint(main_window_rect.right - rect.Width() - 30,
                             main_window_rect.top + 30));
    }

    MoveWindow(&rect);

    return TRUE;
}


BOOL CSProScintillaFindReplaceDlg::PreTranslateMessage(MSG* pMsg)
{
    if( pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_F3 )
    {
        CWnd* ok_button = GetDlgItem(IDOK);

        if( ok_button->GetSafeHwnd() != nullptr )
        {
            ok_button->SendMessage(BM_CLICK, 0, 0);
            return TRUE;
        }
    }

    return __super::PreTranslateMessage(pMsg);
}


void CSProScintillaFindReplaceDlg::OnCancel()
{
    OnClose();
}


void CSProScintillaFindReplaceDlg::OnClose() 
{
    // save the window position
    CRect window_rect;
    GetWindowRect(&window_rect);
    m_lastWindowPosition = window_rect.TopLeft();

    // save the find/replace strings
    SaveComboStrings(IDC_FIND_COMBO);
    SaveComboStrings(IDC_REPLACE_COMBO);

    DestroyWindow();
}


std::tuple<CComboBox*, CWnd*> CSProScintillaFindReplaceDlg::GetComboBoxAndRelatedEditControl(const UINT nID)
{
    const UINT edit_control_id = IsFindComboBoxId(nID) ? IDC_FIND_EDIT :
                                                         IDC_REPLACE_EDIT;

    return std::make_tuple(static_cast<CComboBox*>(GetDlgItem(nID)),
                           GetDlgItem(edit_control_id));
}


void CSProScintillaFindReplaceDlg::RestoreComboStringsAndHideEditControl(const UINT nID)
{
    auto [combo_box, edit_ctrl] = GetComboBoxAndRelatedEditControl(nID);
    ASSERT(( combo_box == nullptr ) == ( edit_ctrl == nullptr ));

    if( combo_box == nullptr )
    {
        ASSERT(nID == IDC_REPLACE_COMBO);
        return;
    }

    // restore the strings from the registry
    const CString registry_value = AfxGetApp()->GetProfileString(GetRegistryKey(), GetRegistryValueName(nID));
    
    SO::ForeachLine(registry_value, false,
        [&](const std::wstring& find_or_replace_text)
        {
            // only add unique strings
            if( combo_box->FindStringExact(-1, find_or_replace_text.c_str()) == CB_ERR )
                combo_box->AddString(find_or_replace_text.c_str());

            return true;
        });

    // update the initial state of the combo box to show the find/replace word (stored in the edit control)
    CString current_find_text;
    edit_ctrl->GetWindowText(current_find_text);
    combo_box->SetWindowText(current_find_text);
 
    // hide the edit control, which is only used for CFindReplaceDialog's internals
    edit_ctrl->ShowWindow(SW_HIDE);
}


void CSProScintillaFindReplaceDlg::SaveComboStrings(const UINT nID)
{
    CComboBox* combo_box = static_cast<CComboBox*>(GetDlgItem(nID));

    if( combo_box == nullptr )
    {
        ASSERT(nID == IDC_REPLACE_COMBO);
        return;
    }

    std::wstring registry_value;
    CString find_or_replace_text;
    const int strings_to_save = std::min(MaxComboStringsToSave, combo_box->GetCount());

    for( int index = 0; index < strings_to_save; ++index )
    {
        combo_box->GetLBText(index, find_or_replace_text);

        // only remember non-blank text
        if( !SO::IsWhitespace(find_or_replace_text) )
            SO::AppendWithSeparator(registry_value, find_or_replace_text, '\n');
    }

    AfxGetApp()->WriteProfileString(GetRegistryKey(), GetRegistryValueName(nID), registry_value.c_str());
}


void CSProScintillaFindReplaceDlg::AddStringToCombo(const UINT nID, const TCHAR* const text)
{
    CComboBox* combo_box = static_cast<CComboBox*>(GetDlgItem(nID));
    ASSERT(combo_box != nullptr);

    // if the text already exists, delete it
    const int current_position = combo_box->FindString(0, text);

    if( current_position == 0 )
        return;

    if( current_position > 0 )
        combo_box->DeleteString(current_position);

    // add the text in the first position
    combo_box->InsertString(0, text);
    combo_box->SetCurSel(0);
}


void CSProScintillaFindReplaceDlg::OnReplaceInSelection()
{
    CButton* replace_in_selection_button = static_cast<CButton*>(GetDlgItem(IDC_REPLACE_IN_SELECTION));
    ASSERT(replace_in_selection_button != nullptr);

    m_replaceInSelection = ( replace_in_selection_button->GetCheck() == 1 );
}


void CSProScintillaFindReplaceDlg::OnComboSelChange(const UINT nID)
{
    auto [combo_box, edit_ctrl] = GetComboBoxAndRelatedEditControl(nID);
    const int current_selection = combo_box->GetCurSel();

    if( current_selection < 0 )
        return;

    CString find_or_replace_text;
    combo_box->GetLBText(current_selection, find_or_replace_text);

    edit_ctrl->SetWindowText(find_or_replace_text);
}


void CSProScintillaFindReplaceDlg::OnComboEditChange(const UINT nID)
{
    auto [combo_box, edit_ctrl] = GetComboBoxAndRelatedEditControl(nID);

    CString find_or_replace_text;
    combo_box->GetWindowText(find_or_replace_text);

    edit_ctrl->SetWindowText(find_or_replace_text);
}
