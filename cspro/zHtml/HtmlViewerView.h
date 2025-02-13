#pragma once

#include <zHtml/zHtml.h>
#include <zHtml/HtmlViewCtrl.h>


class ZHTML_API HtmlViewerView : public CView
{
    DECLARE_DYNCREATE(HtmlViewerView)

public:
    HtmlViewerView();

    HtmlViewCtrl& GetHtmlViewCtrl() { return m_htmlViewCtrl; }

protected:
    DECLARE_MESSAGE_MAP()

    int OnCreate(LPCREATESTRUCT lpCreateStruct);
    void OnSize(UINT nType, int cx, int cy);

    void OnDraw(CDC* pDC) override;

private:
    HtmlViewCtrl m_htmlViewCtrl;
};
