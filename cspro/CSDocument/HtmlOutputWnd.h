#pragma once

#include <zHtml/HtmlViewerView.h>


class HtmlOutputWnd : public CDockablePane
{
public:
    HtmlOutputWnd();

    HtmlViewCtrl& GetHtmlViewCtrl() { return m_htmlViewerView->GetHtmlViewCtrl(); }

protected:
	DECLARE_MESSAGE_MAP()

	int OnCreate(LPCREATESTRUCT lpCreateStruct);
	void OnSize(UINT nType, int cx, int cy);

private:
    HtmlViewerView* m_htmlViewerView;
};
