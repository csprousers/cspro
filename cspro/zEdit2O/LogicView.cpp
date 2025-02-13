#include "stdafx.h"
#include "LogicView.h"
#include <zScintilla/src/Position.h>
#include <zToolsO/NumberToString.h>
#include <zToolsO/WinSettings.h>


IMPLEMENT_DYNCREATE(CLogicView, CScintillaView)

extern const UINT _ScintillaMsgFindReplace ;

CLogicView::CLogicView()
{
    m_pEdit = NULL;
}

CLogicView::~CLogicView()
{
}

BEGIN_MESSAGE_MAP(CLogicView, CScintillaView)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_COMMAND(ID_HELP, OnHelp)
    ON_REGISTERED_MESSAGE(_ScintillaMsgFindReplace, &CLogicView::OnFindReplaceCmd)
END_MESSAGE_MAP()


int CLogicView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if( CView::OnCreate(lpCreateStruct) == -1 )
        return -1;

    // don't create the window if already created
    if( m_pEdit != nullptr && m_pEdit->GetSafeHwnd() != nullptr )
        return 0;
    
    m_pEdit = std::make_unique<CLogicCtrl>();
    CLogicCtrl* logic_ctrl = GetLogicCtrl();
    
    if( !logic_ctrl->Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP, CRect(0, 0, 0, 0), this, 100) )
        return -1;

    // setup the control to show logic
    logic_ctrl->InitLogicControl(true, true);

    // disable the Scintilla default editing menu
    logic_ctrl->UsePopUp(Scintilla::PopUp::Never);

    int zoom_level = WinSettings::Read<DWORD>(WinSettings::Type::LogicEditorZoomLevel, 1);
    logic_ctrl->SetZoom(zoom_level);
    logic_ctrl->SetMouseDwellTime(1000);

    logic_ctrl->SetScrollWidthTracking(true);

    return 0;
}


void CLogicView::OnSize(UINT nType, int cx, int cy) {

    CScintillaView::OnSize(nType, cx, cy);

    CRect rc, rcEdit;
    GetClientRect(rc);
    GetLogicCtrl()->GetWindowRect(rcEdit);
    ScreenToClient(rcEdit);

    GetLogicCtrl()->SetWindowPos(NULL, 0, 0, rc.Width() - rcEdit.left, rc.Height(), SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
}

void CLogicView::OnDraw(CDC* /*pDC*/)
{

}

void CLogicView::OnPrepareDC(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
#ifdef _ENABLE_PRINTING
    //Validate our parameters
    ASSERT_VALID(this);
    ASSERT_VALID(pDC);
#pragma warning(suppress: 26496)
    AFXASSUME(pInfo != nullptr);

    if (pInfo->m_nCurPage <= pInfo->GetMaxPage())
    {
#pragma warning(suppress: 26472)
        if ((pInfo->m_nCurPage > static_cast<UINT>(m_PageStart.size())) && !PaginateTo(pDC, pInfo))
        {
            //can't paginate to that page, thus cannot print it.
            pInfo->m_bContinuePrinting = FALSE;
        }
        ASSERT_VALID(this);
    }
    else
    {
        //Reached the max page to print
        pInfo->m_bContinuePrinting = FALSE;
    }
#endif
}

BOOL CLogicView::PreTranslateMessage(MSG* pMsg)
{
    // TODO: Add your specialized code here and/or call the base class
    CLogicCtrl* pLogicCtrl = GetLogicCtrl();
    if (pLogicCtrl && pMsg->hwnd == pLogicCtrl->GetSafeHwnd()) {
        if (pMsg->message == WM_KEYDOWN) {
            // Control+ALT is same as AltGr on French keyboards which is used for other characters like "{" (AltGr+4)) so only consider CTRL without ALT
            if( GetKeyState(VK_CONTROL) < 0 && GetKeyState(VK_MENU) >= 0 )
            {
                if( pMsg->wParam == VK_SPACE )
                {
                    AfxGetMainWnd()->SendMessage(ZEDIT2O_LOGIC_AUTO_COMPLETE, 0, (LPARAM)pLogicCtrl);
                    return TRUE;
                }

                else if( pMsg->wParam == _T('4') ) // $ is on the 4 key on a U.S. keyboard
                {
                    AfxGetMainWnd()->SendMessage(ZEDIT2O_LOGIC_INSERT_PROC_NAME, 0, (LPARAM)pLogicCtrl);
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
                    case _T('A'): nID = 16; break;
                    case _T('V'): nID = 14; break;
                    case _T('C'): nID = 13; break;
                    case _T('X'): nID = 12; break;
                    case _T('Y'): nID = 11; break;
                    case _T('Z'): nID = 10; break;
                }

                if( nID != 0 )
                {
                    pLogicCtrl->PostMessage(WM_COMMAND, nID, 0);
                    return TRUE;
                }
            }
        }

    }

    return CView::PreTranslateMessage(pMsg);
}


void CLogicView::OnZoom(_Inout_ Scintilla::NotificationData* /*pSCNotification*/)
{
    WinSettings::Write<DWORD>(WinSettings::Type::LogicEditorZoomLevel, GetLogicCtrl()->GetZoom());

    UpdateMarginWidth();
}


void CLogicView::OnDwellStart(_Inout_ Scintilla::NotificationData* pSCNotification)
{
    // only display the tooltip if the Scintilla window has focus
    const CWnd* pFocusWnd = GetFocus();

    if( pFocusWnd != nullptr )
    {
        CLogicCtrl* pLogicCtrl = GetLogicCtrl();

        if( pFocusWnd->GetSafeHwnd() == pLogicCtrl->GetSafeHwnd() )
            pLogicCtrl->ShowTooltip(pSCNotification->position, false);
    }
}

void CLogicView::OnDwellEnd(_Inout_ Scintilla::NotificationData* /*pSCNotification*/)
{
    // cancel any outstanding tooltip
    if( GetLogicCtrl()->CallTipActive() )
        GetLogicCtrl()->CallTipCancel();
}


BOOL CLogicView::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
    const NMHDR* pNMHdr = reinterpret_cast<NMHDR*>(lParam);
    BOOL bRet = CScintillaView::OnNotify(wParam, lParam, pResult);
    if (pNMHdr && pNMHdr->hwndFrom == GetLogicCtrl()->GetSafeHwnd()) {
        switch (pNMHdr->code)
        {
        case SCN_UPDATEUI:
            GetLogicCtrl()->SendMessage(UWM::Edit::UpdateStatusPaneCaretPos, 1); // force update of the current control
            UpdateMarginWidth();
            break;
        case SCN_AUTOCSELECTION:
        {
            SCNotification* pSCNotification  = reinterpret_cast<SCNotification*>(lParam);
            if(pSCNotification->listCompletionMethod == SC_AC_NEWLINE)
                GetLogicCtrl()->SetAutoComplete(true);
            else
                GetLogicCtrl()->SetAutoComplete(false);
        }
            break;
        case SCN_AUTOCCANCELLED:
            GetLogicCtrl()->SetAutoComplete(false);
            break;

        default:
            break;
        }
    }
    return bRet;
}


void CLogicView::OnHelp()
{
    // if the logic control has focus, override F1 because it sometimes
    // will just active the reference window
    CWnd* pWnd = GetActiveWindow()->GetFocus();

    if( pWnd->IsKindOf(RUNTIME_CLASS(CLogicCtrl)) )
    {
        if( !OnHandleHelp() )
            AfxGetMainWnd()->SendMessage(ZEDIT2O_LOGIC_REFERENCE, ZEDIT2O_LOGIC_REFERENCE_HELP, reinterpret_cast<LPARAM>(pWnd));
    }

    else
    {
        CScintillaView::OnHelp();
    }
}


void CLogicView::UpdateMarginWidth(bool force_margin_width_update/* = false*/)
{
    // when there are few lines, use a margin that will account for at least 10 lines so
    // that the view don't have a jarring update upon the frequent task of adding more 9 lines
    int line_count = std::max(GetLogicCtrl()->GetLineCount(), 10);
    int line_count_number_digits = IntToStringLength(line_count);

    if( force_margin_width_update || m_lineCountNumberDigitsAtLastUpdateMarginWidth != line_count_number_digits )
    {
        m_lineCountNumberDigitsAtLastUpdateMarginWidth = line_count_number_digits;
        GetLogicCtrl()->SetMarginWidthN(0, GetLogicCtrl()->TextWidth(STYLE_LINENUMBER, FormatText(_T("_%d"), line_count)));
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


void CLogicView::OnUpdateUI(_Inout_ Scintilla::NotificationData* pSCNotification)
{
    CScintillaView::OnUpdateUI(pSCNotification);

    // Has the caret changed position?
    CLogicCtrl* pLogicCtrl = GetLogicCtrl();
    Sci_Position caret_pos = pLogicCtrl->GetCurrentPos();

    if( last_caret_pos != caret_pos )
    {
        last_caret_pos = caret_pos;

        Sci_Position brace_pos1 = Sci::invalidPosition;

        // Is there a brace to the left or right?
        if( caret_pos > 0 && IsBrace(pLogicCtrl->GetCharAt(caret_pos - 1)) )
            brace_pos1 = caret_pos - 1;

        else if( IsBrace(pLogicCtrl->GetCharAt(caret_pos)) )
            brace_pos1 = caret_pos;

        if( brace_pos1 != Sci::invalidPosition )
        {
            // Find the matching brace
            Sci_Position brace_pos2 = pLogicCtrl->BraceMatch(brace_pos1, 0);

            if( brace_pos2 == Sci::invalidPosition )
                pLogicCtrl->BraceBadLight(brace_pos1);

            else
                pLogicCtrl->BraceHighlight(brace_pos1, brace_pos2);
        }

        // Turn off brace matching
        else
        {
            pLogicCtrl->BraceHighlight(Sci::invalidPosition, Sci::invalidPosition);
        }
    }
}

LRESULT CLogicView::OnFindReplaceCmd(WPARAM /*wParam*/, LPARAM lParam)
{
    if (m_pEdit && m_pEdit->GetSafeHwnd()) {
         m_pEdit->SendMessage(_ScintillaMsgFindReplace, 0, lParam);
    }

    return 0L;
}
