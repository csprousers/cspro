#pragma once

#include <zHtml/HtmlBrowserView.h>

namespace ActionInvoker { class ListenerHolder; }


class HtmlViewerWnd : public CDockablePane
{
public:
    HtmlViewerWnd();

    HtmlBrowserView& GetHtmlBrowser() { return *m_htmlBrowserView; }
    HtmlViewCtrl& GetHtmlViewCtrl()   { return m_htmlBrowserView->GetHtmlViewCtrl(); }

protected:
	DECLARE_MESSAGE_MAP()

	int OnCreate(LPCREATESTRUCT lpCreateStruct);
	void OnSize(UINT nType, int cx, int cy);

private:
    void SetUpActionInvoker();

private:
    HtmlBrowserView* m_htmlBrowserView;
    std::unique_ptr<ActionInvoker::ListenerHolder> m_actionInvokerListenerHolder;
};
