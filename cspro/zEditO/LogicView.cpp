#include "stdafx.h"
#include "LogicView.h"
#include "LogicViewTextSourceModifier.h"
#include <zScintilla/src/Position.h>
#include <zToolsO/NumberToString.h>
#include <zToolsO/WinSettings.h>


extern const UINT _ScintillaMsgFindReplace;


IMPLEMENT_DYNCREATE(LogicView, CScintillaView)


BEGIN_MESSAGE_MAP(LogicView, CScintillaView)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_CONTEXTMENU()
    ON_COMMAND(ID_HELP, OnHelp)
    ON_REGISTERED_MESSAGE(_ScintillaMsgFindReplace, OnFindReplaceCmd)
END_MESSAGE_MAP()


LogicView::LogicView()
    :   m_logicCtrl(nullptr)
{
}


LogicView::~LogicView() noexcept
{
}


void LogicView::RegisterTextSourceModifier(std::shared_ptr<TextSourceEditable> text_source)
{
    ASSERT(m_textSourceModifier == nullptr);
    m_textSourceModifier = std::make_unique<TextSourceModifier>(*this, std::move(text_source));
}


int LogicView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    // bypass CScintillaView's OnCreate as we will create the control here and set various properties
    if( CView::OnCreate(lpCreateStruct) == -1 )
        return -1;

    // don't create the window if already created
    if( m_pEdit != nullptr && m_pEdit->GetSafeHwnd() != nullptr )
        return 0;

    m_pEdit = LogicView::CreateScintillaControl();
    m_logicCtrl = assert_cast<LogicCtrl*>(m_pEdit.get());

    if( !m_logicCtrl->Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP, CRect(), this, 0) )
        return -1;

    // setup the control to show logic
    m_logicCtrl->InitLogicControl(true, true);

    // disable the Scintilla default editing menu
    m_logicCtrl->UsePopUp(Scintilla::PopUp::Never);

    const int zoom_level = WinSettings::Read<DWORD>(WinSettings::Type::LogicEditorZoomLevel, 1);
    m_logicCtrl->SetZoom(zoom_level);
    m_logicCtrl->SetMouseDwellTime(1000);

    m_logicCtrl->SetScrollWidth(1);
    m_logicCtrl->SetScrollWidthTracking(TRUE);

    return 0;
}


std::unique_ptr<Scintilla::CScintillaCtrl> LogicView::CreateScintillaControl()
{
    return std::make_unique<LogicCtrl>();
}


void LogicView::OnSize(UINT nType, int cx, int cy) {

    CScintillaView::OnSize(nType, cx, cy);

    CRect rc, rcEdit;
    GetClientRect(rc);
    m_logicCtrl->GetWindowRect(rcEdit);
    ScreenToClient(rcEdit);

    m_logicCtrl->SetWindowPos(NULL, 0, 0, rc.Width() - rcEdit.left, rc.Height(), SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
}

void LogicView::OnDraw(CDC* /*pDC*/)
{
    // CScintillaView's OnDraw asserts, so it is overridden here
}


HMENU LogicView::GetContextMenu()
{
    CMenu menu;
    return menu.LoadMenu(IDR_EDIT) ? menu.Detach() : nullptr;
}


void LogicView::OnContextMenu(CWnd* const pWnd, CPoint point)
{
    ASSERT(pWnd == m_logicCtrl);

    HMENU menu = GetContextMenu();
    HMENU submenu = ( menu != nullptr ) ? GetSubMenu(menu, 0) : nullptr;

    if( submenu == nullptr )
    {
        ASSERT(false);
        return;
    }

    // when triggered by the keyboard, not the mouse,
    // show the menu at the current caret
    if( point.x == -1 && point.y == -1 )
    {
        point = m_logicCtrl->GetCaretPoint();
        m_logicCtrl->ClientToScreen(&point);
    }

    TrackPopupMenu(
        submenu,
        TPM_LEFTALIGN | TPM_RIGHTBUTTON,
        point.x, point.y,
        0, // nReserved
        AfxGetMainWnd()->m_hWnd, // using the frame will properly update (enable/disable) all of the commands
        nullptr // prcRect
    );
}


BOOL LogicView::PreTranslateMessage(MSG* pMsg)
{
    // TODO: Add your specialized code here and/or call the base class
    if (m_logicCtrl != nullptr && pMsg->hwnd == m_logicCtrl->GetSafeHwnd()) {
        if (pMsg->message == WM_KEYDOWN) {
            // Control+ALT is same as AltGr on French keyboards which is used for other characters like "{" (AltGr+4)) so only consider CTRL without ALT
            if( GetKeyState(VK_CONTROL) < 0 && GetKeyState(VK_MENU) >= 0 )
            {
                if( pMsg->wParam == VK_SPACE )
                {
                    AfxGetMainWnd()->SendMessage(ZEDITO_LOGIC_AUTO_COMPLETE, 0, (LPARAM)m_logicCtrl);
                    return TRUE;
                }

                else if( pMsg->wParam == '4' ) // $ is on the 4 key on a U.S. keyboard
                {
                    AfxGetMainWnd()->SendMessage(ZEDITO_LOGIC_INSERT_PROC_NAME, 0, (LPARAM)m_logicCtrl);
                    return TRUE;
                }

                int nID = 0;
                //these codes are from the IDs in scintilla control
                /*  idcmdUndo=10,
                    idcmdRedo=11,
                    idcmdCut=12,
                    idcmdCopy=13,
                    idcmdPaste=14,
                    idcmdDelete=15,
                    idcmdSelectAll=16*/
                //without this pretranslate processing some accelerators get
                //handled by parent frames.

                switch (pMsg->wParam)
                {
                    case 'A': nID = 16; break;
                    case 'V': nID = 14; break;
                    case 'C': nID = 13; break;
                    case 'X': nID = 12; break;
                    case 'Y': nID = 11; break;
                    case 'Z': nID = 10; break;
                }

                if( nID != 0 )
                {
                    m_logicCtrl->PostMessage(WM_COMMAND, nID, 0);
                    return TRUE;
                }
            }
        }

    }

    return __super::PreTranslateMessage(pMsg);
}


void LogicView::OnSavePointReached(Scintilla::NotificationData* /*pSCNotification*/)
{
    if( m_textSourceModifier != nullptr )
        m_textSourceModifier->SetModified(false);
}


void LogicView::OnModified(Scintilla::NotificationData* const pSCNotification)
{
    constexpr Scintilla::ModificationFlags TextModifications =
        Scintilla::ModificationFlags::InsertText |
        Scintilla::ModificationFlags::DeleteText;

    if( ( m_textSourceModifier != nullptr ) &&
        ( ( pSCNotification->modificationType & TextModifications ) != Scintilla::ModificationFlags::None ) )
    {
        m_textSourceModifier->SetModified(true);
    }
}


void LogicView::OnZoom(Scintilla::NotificationData* /*pSCNotification*/)
{
    WinSettings::Write<DWORD>(WinSettings::Type::LogicEditorZoomLevel, m_logicCtrl->GetZoom());

    UpdateMarginWidth();
}


void LogicView::OnDwellStart(Scintilla::NotificationData* pSCNotification)
{
    // only display the tooltip if the Scintilla window has focus
    const CWnd* pFocusWnd = GetFocus();

    if( pFocusWnd != nullptr )
    {
        if( pFocusWnd->GetSafeHwnd() == m_logicCtrl->GetSafeHwnd() )
            m_logicCtrl->ShowTooltip(pSCNotification->position, false);
    }
}


void LogicView::OnDwellEnd(Scintilla::NotificationData* /*pSCNotification*/)
{
    // cancel any outstanding tooltip
    if( m_logicCtrl->CallTipActive() )
        m_logicCtrl->CallTipCancel();
}


void LogicView::OnUpdateUI(Scintilla::NotificationData* /*pSCNotification*/)
{
    // increase or decrease the margin based on the number of lines
    UpdateMarginWidth();

    // update brace maching
    UpdateBraceMatchingUI();

    // update the current caret position
    m_logicCtrl->SendMessage(UWM::Edit::UpdateStatusPaneCaretPos, 1);
}


void LogicView::OnAutoCSelection(Scintilla::NotificationData* const pSCNotification)
{
    m_logicCtrl->SetAutoCompleteActive(
        ( pSCNotification->listCompletionMethod == Scintilla::CompletionMethods::Newline )
    );
}


void LogicView::OnAutoCCancelled(Scintilla::NotificationData* /*pSCNotification*/)
{
    m_logicCtrl->SetAutoCompleteActive(false);
}


void LogicView::OnHelp()
{
    // if the logic control has focus, override F1 because it sometimes
    // will just active the reference window
    CWnd* pWnd = GetActiveWindow()->GetFocus();

    if( pWnd->IsKindOf(RUNTIME_CLASS(LogicCtrl)) )
    {
        if( !OnHandleHelp() )
            AfxGetMainWnd()->SendMessage(ZEDITO_LOGIC_REFERENCE, ZEDITO_LOGIC_REFERENCE_HELP, reinterpret_cast<LPARAM>(pWnd));
    }

    else
    {
        __super::OnHelp();
    }
}


void LogicView::UpdateMarginWidth(bool force_margin_width_update/* = false*/)
{
    // when there are few lines, use a margin that will account for at least 10 lines so
    // that the view don't have a jarring update upon the frequent task of adding more 9 lines
    const int line_count = std::max(static_cast<int>(m_logicCtrl->GetLineCount()), 10);
    const int line_count_number_digits = IntToStringLength(line_count);

    if( force_margin_width_update || m_lineCountNumberDigitsAtLastUpdateMarginWidth != line_count_number_digits )
    {
        m_lineCountNumberDigitsAtLastUpdateMarginWidth = line_count_number_digits;
        m_logicCtrl->SetMarginWidthN(0, m_logicCtrl->TextWidth(STYLE_LINENUMBER, FormatText("_%d", line_count).c_str()));
    }
}


namespace
{
    // brace matching code based on https://github.com/jacobslusser/ScintillaNET/wiki/Brace-Matching
    Sci_Position last_caret_pos = 0;

    constexpr bool IsBrace(int ch)
    {
        switch( ch )
        {
            case '(':
            case ')':
            case '[':
            case ']':
                return true;
        }

        return false;
    }
}


void LogicView::UpdateBraceMatchingUI()
{
    // Has the caret changed position?
    Sci_Position caret_pos = m_logicCtrl->GetCurrentPos();

    if( last_caret_pos != caret_pos )
    {
        last_caret_pos = caret_pos;

        Sci_Position brace_pos1 = Sci::invalidPosition;

        // Is there a brace to the left or right?
        if( caret_pos > 0 && IsBrace(m_logicCtrl->GetCharAt(caret_pos - 1)) )
            brace_pos1 = caret_pos - 1;

        else if( IsBrace(m_logicCtrl->GetCharAt(caret_pos)) )
            brace_pos1 = caret_pos;

        if( brace_pos1 != Sci::invalidPosition )
        {
            // Find the matching brace
            Sci_Position brace_pos2 = m_logicCtrl->BraceMatch(brace_pos1, 0);

            if( brace_pos2 == Sci::invalidPosition )
                m_logicCtrl->BraceBadLight(brace_pos1);

            else
                m_logicCtrl->BraceHighlight(brace_pos1, brace_pos2);
        }

        // Turn off brace matching
        else
        {
            m_logicCtrl->BraceHighlight(Sci::invalidPosition, Sci::invalidPosition);
        }
    }
}


LRESULT LogicView::OnFindReplaceCmd(WPARAM /*wParam*/, LPARAM lParam)
{
    if (m_pEdit && m_pEdit->GetSafeHwnd()) {
         m_pEdit->SendMessage(_ScintillaMsgFindReplace, 0, lParam);
    }

    return 0;
}
