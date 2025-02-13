#pragma once

#ifdef WIN_DESKTOP

#include <zHtml/zHtml.h>
#include <zHtml/HtmlViewCtrl.h>
#include <zHtml/NavigationAddress.h>

struct HtmlDlgDisplayOptions;
class SharedHtmlLocalFileServer;
namespace ActionInvoker { class ListenerHolder; }


// the base class for showing CSPro-style HTML-based dialogs; one implementation,
// CSHtmlDlg, is used for showing our own UI elements; another implementation,
// HtmlDialogFunctionDlg, is used for the logic function

class ZHTML_API HtmlDlgBase : public CDialog
{
    friend class HtmlDlgBaseActionInvokerListener;

public:
    HtmlDlgBase(CWnd* pParent = nullptr);
    ~HtmlDlgBase();

    void SetActionInvokerAccessTokenOverride(std::wstring access_token) { m_actionInvokerAccessTokenOverride = std::make_unique<std::wstring>(std::move(access_token)); }

    INT_PTR DoModal() override;

    INT_PTR DoModalOnUIThread();

    const std::optional<std::wstring>& GetResultsText() const { return m_resultsText; }

protected:
    virtual NavigationAddress GetNavigationAddress() = 0;
    virtual std::wstring GetInputData() = 0;

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam) override;

    LRESULT OnCloseDialog(WPARAM wParam, LPARAM lParam);

    HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
    void OnSize(UINT nType, int cx, int cy);
    void OnTimer(UINT nIDEvent);

    LRESULT OnProcessDisplayOptions(WPARAM wParam, LPARAM lParam);

    bool UpdateSize(int width, int height);
    LRESULT OnExecuteSizeUpdate(WPARAM wParam, LPARAM lParam);

private:
    void SetUpActionInvoker();

    HtmlDlgDisplayOptions ParseDisplayOptions(const JsonNode<wchar_t>& json_node);

protected:
    bool m_resizable;

private:
    CStatic m_simulatedTitleBar;
    HtmlViewCtrl m_htmlViewCtrl;

    std::unique_ptr<SharedHtmlLocalFileServer> m_fileServer;

    std::unique_ptr<std::wstring> m_actionInvokerAccessTokenOverride;
    std::unique_ptr<ActionInvoker::ListenerHolder> m_actionInvokerListenerHolder;

    std::optional<CSize> m_requestedDisplaySize;
    std::optional<CSize> m_fixedDisplaySize;

    std::optional<UINT_PTR> m_forceUpdateSizeTimerId;

    struct BorderDetails
    {
        int border_thickness;
        std::shared_ptr<CBrush> border_brush;
        int simulated_title_bar_height;
        std::shared_ptr<CBrush> simulated_title_bar_brush;
    };

    std::optional<BorderDetails> m_borderDetails;

    std::optional<std::wstring> m_resultsText;
};

#endif
