#pragma once

#include <DataManager/CaseDoc.h>
#include <DataManager/CaseHoldingFrame.h>

class CaseView;


class CaseFrame : public CaseHoldingFrame
{
    DECLARE_DYNCREATE(CaseFrame)

protected:
    CaseFrame(); // create from serialization only

public:
    CaseDoc& GetCaseDoc() { return *assert_cast<CaseDoc*>(GetActiveDocument()); }

protected:
    CaseHoldingDoc& GetCaseHoldingDoc() override;
    HtmlView& GetHtmlView() override;

    std::unique_ptr<CaseProvider> CreateCaseProvider() override;

protected:
    DECLARE_MESSAGE_MAP()

    BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) override;

    void OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd);

private:
    HtmlView* m_htmlView;
};
