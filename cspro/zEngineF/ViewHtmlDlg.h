#pragma once

#include <zHtml/HtmlViewDlg.h>
#include <zUtilO/Viewers.h>

namespace ActionInvoker { class ListenerHolder; }


class ViewHtmlDlg : public HtmlViewDlg
{
public:
    ViewHtmlDlg(const Viewer& viewer, CWnd* pParent = nullptr);
    ~ViewHtmlDlg();

protected:
    DECLARE_MESSAGE_MAP()

    LRESULT OnCloseDialog(WPARAM wParam, LPARAM lParam);

private:
    void SetUpActionInvoker();

private:
    const Viewer& m_viewer;
    std::unique_ptr<ActionInvoker::ListenerHolder> m_actionInvokerListenerHolder;
};
