#include "StdAfx.h"
#include "CaseFrame.h"


IMPLEMENT_DYNCREATE(CaseFrame, CaseHoldingFrame)

BEGIN_MESSAGE_MAP(CaseFrame, CaseHoldingFrame)
    ON_WM_MDIACTIVATE()
END_MESSAGE_MAP()


CaseFrame::CaseFrame()
    :   m_htmlView(nullptr)
{
}


CaseHoldingDoc& CaseFrame::GetCaseHoldingDoc()
{
    return GetCaseDoc();
}


HtmlView& CaseFrame::GetHtmlView()
{
    ASSERT(m_htmlView != nullptr);
    return *m_htmlView;
}


std::unique_ptr<CaseProvider> CaseFrame::CreateCaseProvider()
{
    const CaseDoc& case_doc = GetCaseDoc();
    return std::make_unique<SingleCaseProvider>(case_doc.GetSharedCurrentCase());
}


BOOL CaseFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* const pContext)
{
    ASSERT(pContext->m_pNewViewClass == RUNTIME_CLASS(HtmlView));

    if( !__super::OnCreateClient(lpcs, pContext) )
        return FALSE;

    m_htmlView = assert_cast<HtmlView*>(GetWindow(GW_CHILD));

    return TRUE;
}


void CaseFrame::OnMDIActivate(const BOOL bActivate, CWnd* const pActivateWnd, CWnd* const pDeactivateWnd)
{
    __super::OnMDIActivate(bActivate, pActivateWnd, pDeactivateWnd);

    CMainFrame* const main_frame = assert_cast<CMainFrame*>(AfxGetMainWnd());
    CaseDoc& case_doc = GetCaseDoc();

    // if closing the last window, clear all status text
    if( pActivateWnd == nullptr )
    {
        main_frame->ClearStatusBarPaneText();
    }

    // otherwise update the data source info and case key
    else
    {
        main_frame->SetStatusBarPaneText(ID_STATUS_PANE_DATA_SOURCE_INFO, TC::ToWide(case_doc.GetDictionary().GetName()).c_str());
        main_frame->SetStatusBarPaneText(ID_STATUS_PANE_CASE_COUNT, nullptr);

        const std::wstring text = L"Key: " + TC::ToWide(case_doc.GetCurrentCase()->GetSingleLineKey());
        main_frame->SetStatusBarPaneText(ID_STATUS_PANE_CASE_KEY, text.c_str());

        main_frame->SetStatusBarPaneText(ID_STATUS_PANE_CASE_POSITION, nullptr);
    }
}
