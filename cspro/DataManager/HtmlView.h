#pragma once

#include <zHtml/HtmlViewerView.h>


class HtmlView : public HtmlViewerView
{
    DECLARE_DYNCREATE(HtmlView)

protected:
    HtmlView(); // create from serialization only

public:
    CaseHoldingDoc& GetCaseHoldingDoc() { return *assert_cast<CaseHoldingDoc*>(GetDocument()); }

protected:
    void OnInitialUpdate() override;
};
