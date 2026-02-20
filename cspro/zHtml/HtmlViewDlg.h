#pragma once

#include <zHtml/zHtml.h>
#include <zHtml/HtmlViewCtrl.h>
#include <zUtilO/Viewers.h>


class ZHTML_API HtmlViewDlg : public CDialog
{
public:
    HtmlViewDlg(CWnd* pParent = nullptr);

    void SetViewerOptions(const ViewerOptions& viewer_options);

    void SetInitialHtml(SharableString html);
    void SetInitialUrl(SharableString url);

    INT_PTR DoModal() override;

    INT_PTR DoModalOnUIThread();

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
    afx_msg void OnSize(UINT nType, int cx, int cy);

protected:
    HtmlViewCtrl m_htmlViewCtrl;
    ViewerOptions m_viewerOptions;

private:
    CSize m_initialWindowSize;
    CWnd* m_closeButton;
    int m_closeButtonBorder;
    int m_htmlViewCtrlBorder;
    std::optional<std::tuple<bool, SharableString>> m_initialContents;
};
